#ifndef _WIN32
#include <err.h> /* errx */
#else
#include "src/winerr.h"
#endif

#include <algorithm>
#include <epoxy/gl.h>
#include <functional>
#include <glm/glm.hpp>
#include <SDL.h>
#include <SDL_image.h>
#include <unordered_map>
#include <libconfig.h>
#include <iostream>
#include <memory>
#include "src/libconfig_shim.h"
#include "soloud.h"

#include "src/asset_manager.h"
#include "src/bullet_debug_draw.h"
#include "src/common.h"
#include "src/component/component_system_manager.h"
#include "src/config.h"
#include "src/input.h"
#include "src/mesh.h"
#include "src/physics.h"
#include "src/player.h"
#include "src/projectile/projectile.h"
#include "src/particle.h"
#include "src/render_data.h"
#include "src/scopetimer.h"
#include "src/shader.h"
#include "src/ship_space.h"
#include "src/text.h"
#include "src/textureset.h"
#include "src/tools/tools.h"
#include "src/wiring/wiring.h"
#include "src/wiring/wiring_data.h"
#include "src/utils/debugdraw.h"
#include "src/entity_utils.h"
#include "src/save.h"
#include "src/load.h"

#define IMGUI_DISABLE_OBSOLETE_FUNCTIONS
#include "src/imgui/imgui.h"
#include "src/imgui_impl_sdl_gl3.h"
#include "src/game_state.h"

#define APP_NAME        "Engineer's Nightmare"
#define DEFAULT_WIDTH   1366
#define DEFAULT_HEIGHT  768

#define MOUSE_Y_LIMIT      1.54f
#define MAX_AXIS_PER_EVENT 128

#define MAX_REACH_DISTANCE 5.0f

bool exit_requested = false;

bool draw_hud = true;
bool draw_debug_text = false;
bool draw_debug_chunks = false;
bool draw_debug_axis = false;
bool draw_debug_physics = false;
bool draw_fps = false;

en_settings game_settings;

struct {
    SDL_Window *ptr;
    SDL_GLContext gl_ctx;
    int width;
    int height;
    bool has_focus;
} wnd;

frame_info frame_info;

SoLoud::Soloud * audio;

struct per_camera_params {
    glm::mat4 view_proj_matrix;
    glm::mat4 inv_centered_view_proj_matrix;
    float aspect;
    float time;
};

void GLAPIENTRY
gl_debug_callback(GLenum source __unused,
                  GLenum type __unused,
                  GLenum id __unused,
                  GLenum severity __unused,
                  GLsizei length __unused,
                  GLchar const *message,
                  void const *userParam __unused)
{
    // Skip NVIDIA's buffer placement noise
    if (severity == GL_DEBUG_SEVERITY_NOTIFICATION)
        return;

    printf("GL: %s\n", message);
}

frame_data *frames, *frame;
unsigned frame_index;

GLuint simple_shader, overlay_shader, ui_shader, ui_sprites_shader;
GLuint sky_shader, particle_shader, highlight_shader;
GLuint screen_shader;
GLuint palette_tex;
GLuint sky_vao;
ship_space *ship;
player pl;
physics *phy;
unsigned char const *keys;
unsigned int mouse_buttons[input_mouse_buttons_count];
int mouse_axes[input_mouse_axes_count];
text_renderer *text;
sprite_renderer *ui_sprites;

sprite_metrics unlit_ui_slot_sprite, lit_ui_slot_sprite;

projectile_linear_manager proj_man;
particle_manager *particle_man;

asset_manager asset_man;
component_system_manager component_system_man;

void
request_exit() {
    exit_requested = true;
}

void
warp_mouse_to_center_screen() {
    SDL_WarpMouseInWindow(wnd.ptr, wnd.width / 2, wnd.height / 2);
}

bool
window_has_focus() {
    return wnd.has_focus;
}

std::unique_ptr<game_state> current_game_state(game_state::create_play_state());
std::unique_ptr<game_state> next_game_state;

void set_next_game_state(game_state *s) {
    next_game_state.reset(s);
}

void
set_game_state()
{
    if (next_game_state != nullptr) {
        current_game_state = std::move(next_game_state);
        pl.ui_dirty = true; /* state change always requires a ui rebuild. */
    }
}

void
prepare_chunks()
{
    /* walk all the chunks -- TODO: only walk chunks that might contribute to the view */
    for (int k = ship->mins.z; k <= ship->maxs.z; k++) {
        for (int j = ship->mins.y; j <= ship->maxs.y; j++) {
            for (int i = ship->mins.x; i <= ship->maxs.x; i++) {
                chunk *ch = ship->get_chunk(glm::ivec3(i, j, k));
                if (ch) {
                    ch->prepare_render();
                    ch->prepare_phys(i, j, k);
                }
            }
        }
    }
}

void
teardown_chunks()
{
    for (int k = ship->mins.z; k <= ship->maxs.z; k++) {
        for (int j = ship->mins.y; j <= ship->maxs.y; j++) {
            for (int i = ship->mins.x; i <= ship->maxs.x; i++) {
                chunk *ch = ship->get_chunk(glm::ivec3(i, j, k));
                if (ch) {
                    teardown_physics_setup(&ch->phys_chunk.phys_mesh, &ch->phys_chunk.phys_shape, &ch->phys_chunk.phys_body);
                }
            }
        }
    }
}

GLuint render_displays_fbo{ 0 };

ImGuiContext *default_context;
ImGuiContext *tool_offscreen_context;

DDRenderInterfaceCoreGL *ddRenderIfaceGL;

struct {
    GLuint fbo = 0;
    GLuint depth = 0;
    GLuint color = 0;
} main_pass;

void apply_video_settings() {
    unsigned new_mode{0};
    switch (game_settings.video.mode) {
        case window_mode::windowed:
            new_mode = 0;
            break;
        case window_mode::fullscreen:
            new_mode = SDL_WINDOW_FULLSCREEN_DESKTOP;
            break;
        default:
            assert(false);
    }
    SDL_SetWindowFullscreen(wnd.ptr, new_mode);
}

void
init()
{
    ddRenderIfaceGL = new DDRenderInterfaceCoreGL();
    dd::initialize(ddRenderIfaceGL);

    proj_man.create_projectile_data(1000);

    printf("%s starting up.\n", APP_NAME);
    printf("OpenGL version: %.1f\n", epoxy_gl_version() / 10.0f);

    if (epoxy_gl_version() < 33) {
        errx(1, "At least OpenGL 3.3 is required\n");
    }

    /* Enable GL debug extension */
    if (!epoxy_has_gl_extension("GL_KHR_debug"))
        errx(1, "No support for GL debugging, life isn't worth it.\n");

    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(reinterpret_cast<GLDEBUGPROC>(&gl_debug_callback), nullptr);

    /* Check for ARB_texture_storage */
    if (!epoxy_has_gl_extension("GL_ARB_texture_storage"))
        errx(1, "No support for ARB_texture_storage\n");

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);         /* pointers given by other libs may not be aligned */
    glEnable(GL_DEPTH_TEST);
    glPolygonOffset(-0.1f, -0.1f);

    printf("Loading entities\n");
    load_entities();

    particle_man = new particle_manager();
    particle_man->create_particle_data(1000);

    glGenFramebuffers(1, &render_displays_fbo);

    asset_man.load_assets();

    default_context = ImGui::GetCurrentContext();
    ImGui::SetCurrentContext(default_context);

    tool_offscreen_context = ImGui::CreateContext();
    ImGui::SetCurrentContext(tool_offscreen_context);
    ImGui_ImplSdlGL3_Init(wnd.ptr);

    // must be called after asset_man is setup
    mesher_init();

    simple_shader = load_shader("shaders/chunk.vert", "shaders/chunk.frag");
    overlay_shader = load_shader("shaders/overlay.vert", "shaders/overlay.frag");
    ui_shader = load_shader("shaders/ui.vert", "shaders/ui.frag");
    ui_sprites_shader = load_shader("shaders/ui_sprites.vert", "shaders/ui_sprites.frag");
    sky_shader = load_shader("shaders/sky.vert", "shaders/sky.frag");
    particle_shader = load_shader("shaders/particle.vert", "shaders/particle.frag");
    highlight_shader = load_shader("shaders/highlight.vert", "shaders/highlight.frag");
    screen_shader = load_shader("shaders/layered.vert", "shaders/simple.frag");

    glUseProgram(simple_shader);

    ship = ship_space::mock_ship_space();
    if( ! ship )
        errx(1, "Ship_space::mock_ship_space failed\n");

    ship->rebuild_topology();

    printf("Ship is %u chunks, %d..%d %d..%d %d..%d\n",
            (unsigned) ship->chunks.size(),
            ship->mins.x, ship->maxs.x,
            ship->mins.y, ship->maxs.y,
            ship->mins.z, ship->maxs.z);

    ship->validate();

    game_settings = load_settings(en_config_base);
    en_settings user_settings = load_settings(en_config_user);
    game_settings.merge_with(user_settings);

    frames = new frame_data[NUM_INFLIGHT_FRAMES];
    frame_index = 0;

    pl.rot = glm::quat(1, 0, 0, 0);
    pl.pos = glm::vec3(2.5f,1.5f,1.5f);
    pl.active_tool_slot = 0;
    pl.ui_dirty = true;

    phy = new physics(&pl);
    phy->dynamicsWorld->setDebugDrawer(new BulletDebugDraw());

    glEnable(GL_CULL_FACE);
    glFrontFace(GL_CCW);

    text = new text_renderer("fonts/pixelmix.ttf", 16);

    ui_sprites = new sprite_renderer();
    unlit_ui_slot_sprite = ui_sprites->load("textures/ui-slot.png");
    lit_ui_slot_sprite = ui_sprites->load("textures/ui-slot-lit.png");

    printf("World vertex size: %zu bytes\n", sizeof(vertex));

    /* prepare the chunks -- this populates the physics data */
    prepare_chunks();

    /* raw palette tex -- bind and leave it bound */
    /* TODO: generalize texture_set so it can do this nicely. */
    auto image = IMG_Load("textures/palette.png");
    glGenTextures(1, &palette_tex);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_1D, palette_tex);
    glTexStorage1D(GL_TEXTURE_1D, 1, GL_RGBA8, 256);
    glTexSubImage1D(GL_TEXTURE_1D, 0, 0, 256, GL_RGBA, GL_UNSIGNED_BYTE, image->pixels);
    glActiveTexture(GL_TEXTURE0);
    SDL_FreeSurface(image);

    glGenVertexArrays(1, &sky_vao);

    apply_video_settings();

    audio = new SoLoud::Soloud();
    audio->init();
    audio->setGlobalVolume(game_settings.audio.global_volume);

    // Absorb all the init time so we dont try to catch up
    frame_info.tick();

    save(ship, "ship.out");
    auto new_ship = new ship_space();
    load(new_ship, "ship.out");
    teardown_chunks();
    delete ship;
    ship = new_ship;
    ship->rebuild_topology();
}

void
resize(int width, int height)
{
    wnd.width = width;
    wnd.height = height;

    /* TODO: DSA */
    glActiveTexture(GL_TEXTURE3);

    if (!main_pass.fbo) {
        glGenFramebuffers(1, &main_pass.fbo);
    }

    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, main_pass.fbo);

    if (main_pass.depth) {
        glDeleteTextures(1, &main_pass.depth);
    }

    glGenTextures(1, &main_pass.depth);
    glBindTexture(GL_TEXTURE_2D, main_pass.depth);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_DEPTH_COMPONENT24, width, height);
    glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, main_pass.depth, 0);

    if (main_pass.color) {
        glDeleteTextures(1, &main_pass.color);
    }

    glGenTextures(1, &main_pass.color);
    glBindTexture(GL_TEXTURE_2D, main_pass.color);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, width, height);
    glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, main_pass.color, 0);

    glActiveTexture(GL_TEXTURE0);

    printf("Resized to %dx%d\n", width, height);
}

void
update_display_contents()
{
    auto &display_man = component_system_man.managers.display_component_man;
    auto &power_man = component_system_man.managers.power_component_man;
    auto &surf = component_system_man.managers.surface_attachment_component_man;

    for (auto i = 0u; i < display_man.buffer.num; i++) {

        /* TODO: allocate render texture layers based on relevance, not instance pool index. */
        /* It's possible that we have too many displays for our pool; if we hit that point,
            bail now rather than clobbering the final entry (which is used for tool UIs).*/
        if (i == asset_man.render_textures->array_size - 1)
            return;

        auto ce = display_man.instance_pool.entity[i];

        auto power = power_man.get_instance_data(ce);
        auto sa = surf.get_instance_data(ce);

        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, render_displays_fbo);
        glFramebufferTextureLayer(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, asset_man.render_textures->texobj, 0, i);
        glViewport(0, 0, RENDER_DIM, RENDER_DIM);

        if (!*power.powered || !*sa.attached) {
            float color[] = { 0.02f, 0.02f, 0.02f, 1 };
            glClearBufferfv(GL_COLOR, 0, color);
            continue;
        }

        float color[] = { 0, 0.18f, 0.21f, 1 };
        glClearBufferfv(GL_COLOR, 0, color);

        auto &ctx = display_man.instance_pool.imgui_context[i];

        if (!ctx) {
            ctx = ImGui::CreateContext();
            ImGui::SetCurrentContext(ctx);
            ImGui_ImplSdlGL3_Init(wnd.ptr);
        }
        else {
            ImGui::SetCurrentContext(ctx);
        }
        ImGui_ImplSdlGL3_NewFrameOffscreen(RENDER_DIM, RENDER_DIM);

        auto flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoInputs;
        // center on screen

        ImGui::SetNextWindowPos(ImVec2{ RENDER_DIM / 2, RENDER_DIM / 4 }, 0, ImVec2{ 0.5f, 0.5f });
        ImGui::Begin("First Window", nullptr, flags);
        ImGui::SetWindowFontScale(5);
        ImGui::Text("Hello from Display %d!", i);
        ImGui::Text("(Entity %d)", ce.id);
        ImGui::SetWindowFontScale(1);
        ImGui::End();
        ImGui::Render();
    }
}

void
remove_ents_from_surface(glm::ivec3 b, int face)
{
    auto &surface_man = component_system_man.managers.surface_attachment_component_man;

    for (auto i = 0u; i < surface_man.buffer.num; i++) {
        auto ce = surface_man.instance_pool.entity[i];

        auto p = surface_man.instance_pool.block[i];
        auto f = surface_man.instance_pool.face[i];
        auto attached = surface_man.instance_pool.attached[i];

        // TODO: consider multiple attachment points?
        if (attached && p == b && f == face) {
            pop_entity_off(ce);
        }
    }
}

std::array<tool*, 9> tools {
    //tool::create_fire_projectile_tool(&pl),
    tool::create_add_shaped_block_tool(),
    tool::create_remove_block_tool(),
    tool::create_paint_surface_tool(),
    tool::create_remove_surface_tool(),
    tool::create_add_entity_tool(),
    tool::create_remove_entity_tool(),
    tool::create_cut_wall_tool(),
    tool::create_wiring_tool(),
    tool::create_customize_entity_tool(),
};

void
add_text_with_outline(char const *s, float x, float y, float r = 1, float g = 1, float b = 1)
{
    text->add(s, x - 2, y, 0, 0, 0);
    text->add(s, x + 2, y, 0, 0, 0);
    text->add(s, x, y - 2, 0, 0, 0);
    text->add(s, x, y + 2, 0, 0, 0);
    text->add(s, x, y, r, g, b);
}


struct time_accumulator
{
    float period;
    float max_period;
    float accum;

    time_accumulator(float period, float max_period) :
        period(period), max_period(max_period), accum(0.0f) {}

    void add(float dt) {
        accum = std::min(accum + dt, max_period);
    }

    bool tick()
    {
        if (accum >= period) {
            accum -= period;
            return true;
        }

        return false;
    }
};

unsigned shuffle_adj_bits_for_face(unsigned bits, unsigned face) {
    // crazy bit shuffle to get from adj faces to the local face's 4 adjacency bits
    switch (face) {
    case surface_zm:
        return bits;
    case surface_zp:
        return (bits & 0x0c) | ((bits & 1) << 1) | ((bits & 2) >> 1);
    case surface_xm:
        return bits >> 2;
    case surface_xp:
        return ((bits & 0x30) >> 2) | ((bits & 4) >> 1) | ((bits & 8) >> 3);
    case surface_ym:
        return ((bits & 1) << 1) | ((bits & 2) >> 1) | ((bits & 0x30) >> 2);
    case surface_yp:
        return (bits & 3) | ((bits & 0x30) >> 2);
    default:
        return 0;   // unreachable
    }
}


void draw_wires() {
    const mesh_data* outside_meshes[] = {
        &asset_man.get_mesh("wire_1"),
        &asset_man.get_mesh("wire_2"),
        &asset_man.get_mesh("wire_4"),
        &asset_man.get_mesh("wire_8"),
        &asset_man.get_mesh("wire_end"),
    };
    const mesh_data* inside_meshes[] = {
        &asset_man.get_mesh("wire_1i"),
        &asset_man.get_mesh("wire_2i"),
        &asset_man.get_mesh("wire_4i"),
        &asset_man.get_mesh("wire_8i"),
        &asset_man.get_mesh("wire_endi"),
    };

    for (int k = ship->mins.z; k <= ship->maxs.z; k++) {
        for (int j = ship->mins.y; j <= ship->maxs.y; j++) {
            for (int i = ship->mins.x; i <= ship->maxs.x; i++) {
                chunk *ch = ship->get_chunk(glm::ivec3(i, j, k));
                if (ch) {
                    for (int z = 0; z < CHUNK_SIZE; z++) {
                        for (int y = 0; y < CHUNK_SIZE; y++) {
                            for (int x = 0; x < CHUNK_SIZE; x++) {
                                auto bl = ch->blocks.get(x, y, z);

                                auto meshes = bl->type == block_frame ? inside_meshes : outside_meshes;

                                for (int face = 0; face < 6; face++) {
                                    if (bl->has_wire[face]) {
                                        auto params = frame->alloc_aligned<glm::mat4>(1);
                                        *(params.ptr) = mat_block_face(glm::vec3(i * CHUNK_SIZE + x, j * CHUNK_SIZE + y, k * CHUNK_SIZE + z), face);
                                        params.bind(1, frame);

                                        auto bits = shuffle_adj_bits_for_face(bl->wire_bits[face], face);

                                        if (bits & 1)
                                            draw_mesh(meshes[0]->hw);
                                        if (bits & 2)
                                            draw_mesh(meshes[1]->hw);
                                        if (bits & 4)
                                            draw_mesh(meshes[2]->hw);
                                        if (bits & 8)
                                            draw_mesh(meshes[3]->hw);

                                        if (!(bits & (bits - 1))) {
                                            draw_mesh(meshes[4]->hw);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

glm::vec3 fp_item_offset{ 0.115f, 0.2f, -0.12f };
float fp_item_scale{ 0.175f };
glm::quat fp_item_rot{ -1.571f, -0.143f, 2.429f, 1.286f };

glm::mat4 get_fp_item_matrix() {
    auto m = glm::mat4_cast(glm::normalize(pl.rot));
    auto right = glm::vec3(m[0]);
    auto up = glm::vec3(m[1]);
    return glm::scale(mat_position(pl.eye + pl.dir * fp_item_offset.y + right * fp_item_offset.x + up * fp_item_offset.z), glm::vec3(fp_item_scale)) * m * glm::mat4_cast(glm::normalize(fp_item_rot));
}


void render() {
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, main_pass.fbo);

    glEnable(GL_DEPTH_TEST);
    float depthClearValue = 1.0f;
    glClearBufferfv(GL_DEPTH, 0, &depthClearValue);

    frame = &frames[frame_index++];
    if (frame_index >= NUM_INFLIGHT_FRAMES) {
        frame_index = 0;
    }

    frame->begin();

    auto m = glm::mat4_cast(glm::normalize(pl.rot));
    pl.dir = -glm::vec3(m[2]);
    m = glm::transpose(m);

    /* pl.pos is center of capsule */
    pl.eye = pl.pos;

    auto vfov = glm::radians(game_settings.video.fov);

    glm::mat4 proj = glm::perspective(vfov, (float)wnd.width / wnd.height, 0.01f, 1000.0f);
    glm::mat4 view = m * glm::translate(glm::vec3(-pl.eye));
    glm::mat4 centered_view = m;

    auto camera_params = frame->alloc_aligned<per_camera_params>(1);

    auto mvp = proj * view;
    camera_params.ptr->view_proj_matrix = mvp;
    camera_params.ptr->inv_centered_view_proj_matrix = glm::inverse(proj * centered_view);
    camera_params.ptr->aspect = (float)wnd.width / wnd.height;
    camera_params.ptr->time = (float)frame_info.elapsed;
    camera_params.bind(0, frame);

    prepare_chunks();

    glUseProgram(simple_shader);

    for (int k = ship->mins.z; k <= ship->maxs.z; k++) {
        for (int j = ship->mins.y; j <= ship->maxs.y; j++) {
            for (int i = ship->mins.x; i <= ship->maxs.x; i++) {
                /* TODO: prepare all the matrices first, and do ONE upload */
                chunk *ch = ship->get_chunk(glm::ivec3(i, j, k));
                if (ch) {
                    auto chunk_matrix = frame->alloc_aligned<glm::mat4>(1);
                    auto p = glm::vec3(CHUNK_SIZE * glm::ivec3(i, j, k));
                    *chunk_matrix.ptr = mat_position(p);
                    chunk_matrix.bind(1, frame);
                    draw_mesh(ch->render_chunk.mesh);

                    if (draw_debug_chunks) {
                        ddVec3 dv{p.x + CHUNK_SIZE / 2, p.y + CHUNK_SIZE / 2, p.z + CHUNK_SIZE / 2};
                        dd::box(dv, dd::colors::DodgerBlue, CHUNK_SIZE, CHUNK_SIZE, CHUNK_SIZE, 0, false);
                    }
                }
            }
        }
    }

    build_absolute_transforms();
    glUseProgram(simple_shader);
    draw_renderables(frame);
    glUseProgram(simple_shader);
    draw_wires();

    /* draw the sky */
    glUseProgram(sky_shader);
    asset_man.bind_skybox("starry-night", 0);
    glDepthMask(GL_FALSE);
    glDepthFunc(GL_LEQUAL);
    glBindVertexArray(sky_vao);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glDepthFunc(GL_LESS);

    /* Draw particles with depth test on but writes off */
    glUseProgram(particle_shader);
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    draw_particles(particle_man, frame);
    glDisable(GL_BLEND);

    /* Reenable depth write */
    glDepthMask(GL_TRUE);

    glUseProgram(simple_shader);

    if (draw_debug_axis) {
        auto p = pl.eye + glm::normalize(pl.dir) * 0.1f;
        auto m = glm::translate(glm::mat4{}, p);
        ddMat4x4 at{
            m[0].x, m[0].y, m[0].z, m[0].w,
            m[1].x, m[1].y, m[1].z, m[1].w,
            m[2].x, m[2].y, m[2].z, m[2].w,
            m[3].x, m[3].y, m[3].z, m[3].w,
        };
        dd::axisTriad(at, 0.001f, 0.01f, 0, true);
    }

    if (draw_debug_physics) {
        phy->dynamicsWorld->debugDrawWorld();
    }

    if (draw_debug_chunks || draw_debug_axis || draw_debug_physics) {
        dd::flush(SDL_GetTicks());
    }

    auto *t = tools[pl.active_tool_slot];

    if (t) {
        t->pre_use(&pl);
        t->preview(frame);
    }

    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, main_pass.fbo);
    glBlitFramebuffer(0, 0, wnd.width, wnd.height, 0, 0, wnd.width, wnd.height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);

    glViewport(0, 0, wnd.width, wnd.height);

    ImGui::SetCurrentContext(default_context);
    ImGui_ImplSdlGL3_NewFrame(wnd.ptr);
    current_game_state->render(frame);
    ImGui::Render();

    if (draw_hud) {
        /* draw the ui */
        glDisable(GL_DEPTH_TEST);

        glUseProgram(ui_shader);
        text->draw();
        glUseProgram(ui_sprites_shader);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        ui_sprites->draw();
        glDisable(GL_BLEND);

        glEnable(GL_DEPTH_TEST);
    }

    frame->end();
}


time_accumulator main_tick_accum(1/15.0f, 1.f);  /* 15Hz tick for game logic */
time_accumulator fast_tick_accum(1/60.0f, 1.f);  /* 60Hz tick for motion */

void
update()
{
    frame_info.tick();
    auto dt = (float)frame_info.dt;     /* narrowing */

    main_tick_accum.add(dt);
    fast_tick_accum.add(dt);

    set_game_state();

    auto want_relative_mouse_mode = SDL_bool(window_has_focus() && !current_game_state->is_ui_state());
    if (SDL_GetRelativeMouseMode() != want_relative_mouse_mode) {
        SDL_SetRelativeMouseMode(want_relative_mouse_mode);
        warp_mouse_to_center_screen();
    }

    /* this absolutely must run every frame */
    current_game_state->update(dt);

    /* things that can run at a pretty slow rate */
    while (main_tick_accum.tick()) {

        /* remove any air that someone managed to get into the outside */
        {
            topo_info *t = topo_find(&ship->outside_topo_info);
            zone_info *z = ship->get_zone_info(t);
            if (z) {
                /* try as hard as you like, you cannot fill space with your air system */
                z->air_amount = 0;
            }
        }

        /* allow the entities to tick */
        tick_gas_producers(ship);
        tick_power_consumers(ship);
        tick_light_components(ship);
        tick_pressure_sensors(ship);
        tick_sensor_comparators(ship);
        tick_proximity_sensors(ship, &pl);
        tick_doors(ship);
        tick_power_sensors(ship);

        calculate_power_wires(ship);
        propagate_comms_wires(ship);

        update_display_contents();

        if (pl.ui_dirty || draw_debug_text || draw_fps) {
            text->reset();
            ui_sprites->reset();

            current_game_state->rebuild_ui();

            if (draw_fps) {
                char buf[3][256];
                float w[3] = { 0, 0, 0 }, h = 0;

                sprintf(buf[0], "%.2f", frame_info.dt * 1000);
                sprintf(buf[1], "%.2f", 1.f / frame_info.dt);
                sprintf(buf[2], "%.2f", frame_info.fps);

                text->measure(buf[0], &w[0], &h);
                text->measure(buf[1], &w[1], &h);
                text->measure(buf[2], &w[2], &h);

                add_text_with_outline(buf[0], -DEFAULT_WIDTH / 2 + (100 - w[0]), DEFAULT_HEIGHT / 2 + 100);
                add_text_with_outline(buf[1], -DEFAULT_WIDTH / 2 + (100 - w[1]), DEFAULT_HEIGHT / 2 + 82);
                add_text_with_outline(buf[2], -DEFAULT_WIDTH / 2 + (100 - w[2]), DEFAULT_HEIGHT / 2 + 64);
            }

            text->upload();

            ui_sprites->upload();
            pl.ui_dirty = false;
        }
    }

    /* character controller tick: we'd LIKE to run this off the fast_tick_accum, but it has all kinds of
     * every-frame assumptions baked in (player impulse state, etc) */
    phy->tick_controller(dt);

    auto *t = tools[pl.active_tool_slot];
    if (t) {
        t->update();
    }

    while (fast_tick_accum.tick()) {
        proj_man.simulate(fast_tick_accum.period);
        particle_man->simulate(fast_tick_accum.period);

        tick_rotator_components(ship, fast_tick_accum.period);

        phy->tick(fast_tick_accum.period);
    }

    auto &phys_man = component_system_man.managers.physics_component_man;
    auto &pos_man = component_system_man.managers.position_component_man;
    for (auto i = 0u; i < phys_man.buffer.num; i++) {
        auto ce = phys_man.instance_pool.entity[i];
        assert (pos_man.exists(ce));

        auto inst = pos_man.get_instance_data(ce);
        *inst.mat = bt_to_mat4(phys_man.instance_pool.rigid[i]->getWorldTransform());
    }
}

action const* get_input(en_action a) {
    return &game_settings.bindings.bindings[a];
}


void
handle_input()
{
    if (wnd.has_focus) {
        set_inputs(keys, mouse_buttons, mouse_axes, game_settings.bindings.bindings);
        current_game_state->handle_input();
    }
}

void
run()
{
    for (;;) {
        auto sdl_buttons = SDL_GetRelativeMouseState(nullptr, nullptr);
        mouse_buttons[EN_MOUSE_BUTTON(input_mouse_left)]      = sdl_buttons & EN_SDL_BUTTON(input_mouse_left);
        mouse_buttons[EN_MOUSE_BUTTON(input_mouse_middle)]    = sdl_buttons & EN_SDL_BUTTON(input_mouse_middle);
        mouse_buttons[EN_MOUSE_BUTTON(input_mouse_right)]     = sdl_buttons & EN_SDL_BUTTON(input_mouse_right);
        mouse_buttons[EN_MOUSE_BUTTON(input_mouse_thumb1)]    = sdl_buttons & EN_SDL_BUTTON(input_mouse_thumb1);
        mouse_buttons[EN_MOUSE_BUTTON(input_mouse_thumb2)]    = sdl_buttons & EN_SDL_BUTTON(input_mouse_thumb2);
        mouse_buttons[EN_MOUSE_BUTTON(input_mouse_wheeldown)] = 0;
        mouse_buttons[EN_MOUSE_BUTTON(input_mouse_wheelup)]   = 0;

        mouse_axes[EN_MOUSE_AXIS(input_mouse_x)] = 0;
        mouse_axes[EN_MOUSE_AXIS(input_mouse_y)] = 0;

        // only handle input in default imgui context
        ImGui::SetCurrentContext(default_context);

        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            ImGui_ImplSdlGL3_ProcessEvent(&e);
            switch (e.type) {
            case SDL_QUIT:
                printf("Quit event caught, shutting down.\n");
                return;

            case SDL_WINDOWEVENT:
                /* We MUST support resize events even if we
                 * don't really care about resizing, because a tiling
                 * WM isn't going to give us what we asked for anyway!
                 */
                switch (e.window.event) {
                case SDL_WINDOWEVENT_RESIZED:
                    resize(e.window.data1, e.window.data2);
                    break;

                case SDL_WINDOWEVENT_FOCUS_LOST:
                    wnd.has_focus = false;
                    break;

                case SDL_WINDOWEVENT_FOCUS_GAINED:
                    wnd.has_focus = true;
                    break;
                }
                break;

            case SDL_MOUSEMOTION:
            {
                auto x = e.motion.xrel;
                auto y = e.motion.yrel;

                x = clamp(x, -MAX_AXIS_PER_EVENT, MAX_AXIS_PER_EVENT);
                y = clamp(y, -MAX_AXIS_PER_EVENT, MAX_AXIS_PER_EVENT);

                mouse_axes[EN_MOUSE_AXIS(input_mouse_x)] += x;
                mouse_axes[EN_MOUSE_AXIS(input_mouse_y)] += y;
                break;
            }

            case SDL_MOUSEWHEEL:
                if (e.wheel.y != 0) {
                    e.wheel.y > 0
                        ? mouse_buttons[EN_MOUSE_BUTTON(input_mouse_wheelup)] = true
                        : mouse_buttons[EN_MOUSE_BUTTON(input_mouse_wheeldown)] = true;
                }
                break;
            }
        }

        // todo: investigate having imgui impl use our input rather than grabbing SDL
        // if imgui wants mouse capture but we don't clear these
        // then we get the mouse-up button capture on the next frame
        // which causes side-effects
        if (ImGui::GetIO().WantCaptureMouse) {
            std::fill(mouse_buttons, mouse_buttons + input_mouse_buttons_count, 0);
            std::fill(mouse_axes, mouse_axes + input_mouse_axes_count, 0);
        }

        auto *t = tools[pl.active_tool_slot];
        if (t) {
            t->do_offscreen_render();
        }

        /* SDL_PollEvent above has already pumped the input, so current key state is available */
        handle_input();

        update();

        glViewport(0, 0, wnd.width, wnd.height);

        render();

        SDL_GL_SwapWindow(wnd.ptr);

        if (exit_requested) return;
    }
}

int
main(int, char **)
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
        errx(1, "Error initializing SDL: %s\n", SDL_GetError());

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    wnd.ptr = SDL_CreateWindow(APP_NAME,
                               SDL_WINDOWPOS_CENTERED,
                               SDL_WINDOWPOS_CENTERED,
                               DEFAULT_WIDTH,
                               DEFAULT_HEIGHT,
                               SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);

    if (!wnd.ptr)
        errx(1, "Failed to create window.\n");

    wnd.gl_ctx = SDL_GL_CreateContext(wnd.ptr);

    ImGui_ImplSdlGL3_Init(wnd.ptr);

    keys = SDL_GetKeyboardState(nullptr);

    resize(DEFAULT_WIDTH, DEFAULT_HEIGHT);

    init();

    run();

    ImGui_ImplSdlGL3_Shutdown();

    audio->deinit();
    delete audio;

    return 0;
}
