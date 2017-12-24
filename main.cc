#ifndef _WIN32
#include <err.h> /* errx */
#else
#include "src/winerr.h"
#endif

#include <algorithm>
#include <epoxy/gl.h>
#include <functional>
#include <glm/glm.hpp>
#include <stdio.h>
#include <SDL.h>
#include <SDL_image.h>
#include <unordered_map>
#include <array>
#include <libconfig.h>
#include <iostream>
#include <memory>
#include "src/libconfig_shim.h"

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

#define APP_NAME        "Engineer's Nightmare"
#define DEFAULT_WIDTH   1024
#define DEFAULT_HEIGHT  768

#define MOUSE_Y_LIMIT      1.54f
#define MAX_AXIS_PER_EVENT 128

#define MAX_REACH_DISTANCE 5.0f

#define IMGUI_DISABLE_OBSOLETE_FUNCTIONS
#include "src/imgui/imgui.h"
#include "src/imgui_impl_sdl_gl3.h"

#include "src/utils/debugdraw.h"
#include "src/entity_utils.h"

bool exit_requested = false;

bool draw_hud = true;
bool draw_debug_text = false;
bool draw_debug_chunks = false;
bool draw_debug_axis = false;
bool draw_debug_physics = false;
bool draw_fps = false;

auto hfov = glm::radians(90.f);

en_settings game_settings;

struct {
    SDL_Window *ptr;
    SDL_GLContext gl_ctx;
    int width;
    int height;
    bool has_focus;
} wnd;

frame_info frame_info;

struct per_camera_params {
    glm::mat4 view_proj_matrix;
    glm::mat4 inv_centered_view_proj_matrix;
    float aspect;
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
GLuint sky_shader, particle_shader, modelspace_uv_shader, highlight_shader;
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

void load_entities();
void use_action_on_entity(ship_space *ship, c_entity ce);

struct game_state {
    virtual ~game_state() {}

    virtual void handle_input() = 0;
    virtual void update(float dt) = 0;
    virtual void render(frame_data *frame) = 0;
    virtual void rebuild_ui() = 0;

    static game_state *create_play_state();
    static game_state *create_menu_state();
};


game_state *current_game_state = game_state::create_play_state();

void
set_game_state(game_state *s)
{
    if (current_game_state)
        delete current_game_state;

    current_game_state = s;
    pl.ui_dirty = true; /* state change always requires a ui rebuild. */
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

GLuint render_fbo;

ImGuiContext *default_context;
std::array<ImGuiContext*, 2> offscreen_contexts{};

DDRenderInterfaceCoreGL *ddRenderIfaceGL;

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

    glGenFramebuffers(1, &render_fbo);

    asset_man.load_assets();

    default_context = ImGui::GetCurrentContext();
    for (auto &ctx : offscreen_contexts) {
        ctx = ImGui::CreateContext();
        ImGui::SetCurrentContext(ctx);
        ImGui_ImplSdlGL3_Init(wnd.ptr);
    }
    ImGui::SetCurrentContext(default_context);

    // must be called after asset_man is setup
    mesher_init();

    simple_shader = load_shader("shaders/chunk.vert", "shaders/chunk.frag");
    overlay_shader = load_shader("shaders/overlay.vert", "shaders/unlit.frag");
    ui_shader = load_shader("shaders/ui.vert", "shaders/ui.frag");
    ui_sprites_shader = load_shader("shaders/ui_sprites.vert", "shaders/ui_sprites.frag");
    sky_shader = load_shader("shaders/sky.vert", "shaders/sky.frag");
    particle_shader = load_shader("shaders/particle.vert", "shaders/particle.frag");
    modelspace_uv_shader = load_shader("shaders/simple_modelspace_uv.vert", "shaders/simple.frag");
    highlight_shader = load_shader("shaders/highlight.vert", "shaders/highlight.frag");

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

    pl.angle = 0;
    pl.elev = 0;
    pl.pos = glm::vec3(3.0f,2.0f,3.0f);
    pl.active_tool_slot = 0;
    pl.ui_dirty = true;
    pl.disable_gravity = false;

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

    // Absorb all the init time so we dont try to catch up
    frame_info.tick();

    apply_video_settings();
}


void
resize(int width, int height)
{
    /* TODO: resize offscreen (but screen-sized) surfaces, etc. */
    glViewport(0, 0, width, height);
    wnd.width = width;
    wnd.height = height;
    printf("Resized to %dx%d\n", width, height);
}


void
destroy_entity(c_entity e) {
    auto &physics_man = component_system_man.managers.physics_component_man;

    if (physics_man.exists(e)) {
        auto phys_data = physics_man.get_instance_data(e);
        auto *per = (phys_ent_ref *) (*phys_data.rigid)->getUserPointer();
        delete per;

        teardown_physics_setup(nullptr, nullptr, phys_data.rigid);
    }

    component_system_man.managers.destroy_entity_instance(e);
}


void
remove_ents_from_surface(glm::ivec3 b, int face)
{
    auto &surface_man = component_system_man.managers.surface_attachment_component_man;

    chunk *ch = ship->get_chunk_containing(b);
    for (auto it = ch->entities.begin(); it != ch->entities.end(); /* */) {
        auto ce = *it;

        /* entities may have been inserted in this chunk which don't have
         * placement on a surface. don't corrupt everything if we hit one.
         */
        if (!surface_man.exists(ce)) {
            ++it;
            continue;
        }

        auto surface = surface_man.get_instance_data(ce);
        auto p = *surface.block;
        auto f = *surface.face;

        //todo: fix height here. hardcoded is wrong
        auto height = 1;
        if (p.x == b.x && p.y == b.y && p.z <= b.z && p.z + height > b.z && f == face) {
            pop_entity_off(ce);
        }
        else {
            ++it;
        }
    }
}

std::array<tool*, 8> tools {
    //tool::create_fire_projectile_tool(&pl),
    tool::create_add_block_tool(),
    tool::create_remove_block_tool(),
    tool::create_paint_surface_tool(),
    tool::create_remove_surface_tool(),
    tool::create_add_entity_tool(),
    tool::create_remove_entity_tool(),
    tool::create_add_room_tool(),
    tool::create_wiring_tool(),
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


void draw_wires() {
    auto mesh = asset_man.get_mesh("face_marker");

    for (int k = ship->mins.z; k <= ship->maxs.z; k++) {
        for (int j = ship->mins.y; j <= ship->maxs.y; j++) {
            for (int i = ship->mins.x; i <= ship->maxs.x; i++) {
                chunk *ch = ship->get_chunk(glm::ivec3(i, j, k));
                if (ch) {
                    for (int z = 0; z < CHUNK_SIZE; z++) {
                        for (int y = 0; y < CHUNK_SIZE; y++) {
                            for (int x = 0; x < CHUNK_SIZE; x++) {
                                auto bl = ch->blocks.get(x, y, z);
                                for (int face = 0; face < 6; face++) {
                                    if (bl->has_wire[face]) {
                                        auto params = frame->alloc_aligned<glm::mat4>(1);
                                        *(params.ptr) = mat_block_face(glm::vec3(i * CHUNK_SIZE + x, j * CHUNK_SIZE + y, k * CHUNK_SIZE + z), face);
                                        params.bind(1, frame);

                                        draw_mesh(mesh.hw);
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


void render() {
    glEnable(GL_DEPTH_TEST);
    float depthClearValue = 1.0f;
    glClearBufferfv(GL_DEPTH, 0, &depthClearValue);

    frame = &frames[frame_index++];
    if (frame_index >= NUM_INFLIGHT_FRAMES) {
        frame_index = 0;
    }

    frame->begin();

    pl.dir = glm::vec3(
        cosf(pl.angle) * cosf(pl.elev),
        sinf(pl.angle) * cosf(pl.elev),
        sinf(pl.elev)
        );

    /* pl.pos is center of capsule */
    pl.eye = pl.pos;

    auto vfov = hfov * (float)wnd.height / wnd.width;

    glm::mat4 proj = glm::perspective(vfov, (float)wnd.width / wnd.height, 0.01f, 1000.0f);
    glm::mat4 view = glm::lookAt(pl.eye, pl.eye + pl.dir, glm::vec3(0, 0, 1));
    glm::mat4 centered_view = glm::lookAt(glm::vec3(0), pl.dir, glm::vec3(0, 0, 1));

    auto camera_params = frame->alloc_aligned<per_camera_params>(1);

    auto mvp = proj * view;
    camera_params.ptr->view_proj_matrix = mvp;
    camera_params.ptr->inv_centered_view_proj_matrix = glm::inverse(proj * centered_view);
    camera_params.ptr->aspect = (float)wnd.width / wnd.height;
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

    glUseProgram(simple_shader);
    draw_renderables(frame);
    glUseProgram(simple_shader);
    draw_doors(frame);
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

    current_game_state->render(frame);

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

    if (draw_debug_axis) {
        auto p = pl.eye + glm::normalize(pl.dir * 3.0f);
        auto m = glm::translate(glm::mat4{}, p);
        ddMat4x4 at{
            m[0].x, m[0].y, m[0].z, m[0].w,
            m[1].x, m[1].y, m[1].z, m[1].w,
            m[2].x, m[2].y, m[2].z, m[2].w,
            m[3].x, m[3].y, m[3].z, m[3].w,
        };
        dd::axisTriad(at, 0.01f, 0.1f, 0, false);
    }

    if (draw_debug_physics) {
        phy->dynamicsWorld->debugDrawWorld();
    }

    if (draw_debug_chunks || draw_debug_axis || draw_debug_physics) {
        dd::flush(SDL_GetTicks());
    }
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
        tick_readers(ship);
        tick_gas_producers(ship);
        tick_power_consumers(ship);
        tick_light_components(ship);
        tick_pressure_sensors(ship);
        tick_sensor_comparators(ship);
        tick_proximity_sensors(ship, &pl);
        tick_doors(ship);

        calculate_power_wires(ship);
        propagate_comms_wires(ship);

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

    while (fast_tick_accum.tick()) {

        proj_man.simulate(fast_tick_accum.period);
        particle_man->simulate(fast_tick_accum.period);

        phy->tick(fast_tick_accum.period);
    }

    auto &phys_man = component_system_man.managers.physics_component_man;
    auto &pos_man = component_system_man.managers.relative_position_component_man;
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


struct play_state : game_state {
    c_entity use_entity;

    play_state() {
    }

    void rebuild_ui() override {
        auto &type_man = component_system_man.managers.type_component_man;
        auto &switch_man = component_system_man.managers.switch_component_man;
        auto &surf_man = component_system_man.managers.surface_attachment_component_man;

        float w = 0;
        float h = 0;
        char buf[256];
        char buf2[512];

        {
            /* Tool name down the bottom */
            tool *t = tools[pl.active_tool_slot];

            if (t) {
                t->get_description(buf);
            }
            else {
                strcpy(buf, "(no tool)");
            }
        }

        text->measure(".", &w, &h);
        add_text_with_outline(".", -w/2, -w/2);

        auto bind = game_settings.bindings.bindings.find(action_use_tool);
        auto key = lookup_key((*bind).second.binds.inputs[0]);
        sprintf(buf2, "%s: %s", key, buf);
        text->measure(buf2, &w, &h);
        add_text_with_outline(buf2, -w/2, -360);

        /* Gravity state (temp) */
        w = 0; h = 0;
        bind = game_settings.bindings.bindings.find(action_gravity);
        key = lookup_key((*bind).second.binds.inputs[0]);
        sprintf(buf, "Gravity: %s (%s to toggle)", pl.disable_gravity ? "OFF" : "ON", key);
        text->measure(buf, &w, &h);
        add_text_with_outline(buf, -w/2, -430);

        /* Use key affordance */
        bind = game_settings.bindings.bindings.find(action_use);
        key = lookup_key((*bind).second.binds.inputs[0]);
        char *pre = nullptr;
        if (c_entity::is_valid(use_entity)) {
            auto sa = surf_man.get_instance_data(use_entity);
            if (!*sa.attached) {
                pre = const_cast<char *>("Remove");
            }
            else if (switch_man.exists(use_entity)) {
                pre = const_cast<char *>("Use");
            }

            if (pre) {
                auto name = *type_man.get_instance_data(use_entity).name;
                sprintf(buf2, "%s %s the %s", key, pre, name);
                w = 0;
                h = 0;
                text->measure(buf2, &w, &h);
                add_text_with_outline(buf2, -w / 2, -200);
            }
        }


        /* debug text */
        if (draw_debug_text) {
            /* Atmo status */
            glm::ivec3 eye_block = get_coord_containing(pl.eye);

            topo_info *t = topo_find(ship->get_topo_info(eye_block));
            topo_info *outside = topo_find(&ship->outside_topo_info);
            zone_info *z = ship->get_zone_info(t);
            float pressure = z ? (z->air_amount / t->size) : 0.0f;

            if (t != outside) {
                sprintf(buf2, "[INSIDE %p %d %.1f atmo]", t, t->size, pressure);
            }
            else {
                sprintf(buf2, "[OUTSIDE %p %d %.1f atmo]", t, t->size, pressure);
            }

            w = 0; h = 0;
            text->measure(buf2, &w, &h);
            add_text_with_outline(buf2, -w/2, -100);

            w = 0; h = 0;
            sprintf(buf2, "full: %d fast-unify: %d fast-nosplit: %d false-split: %d",
                    ship->num_full_rebuilds,
                    ship->num_fast_unifys,
                    ship->num_fast_nosplits,
                    ship->num_false_splits);
            text->measure(buf2, &w, &h);
            add_text_with_outline(buf2, -w/2, -150);
        }

        for (unsigned i = 0; i < tools.size(); i++) {
            ui_sprites->add(pl.active_tool_slot == i ? &lit_ui_slot_sprite : &unlit_ui_slot_sprite,
                    (i - tools.size() / 2.0f) * 34, -220);
        }
    }

    void update(float dt) override {
        if (wnd.has_focus && SDL_GetRelativeMouseMode() == SDL_FALSE) {
            SDL_SetRelativeMouseMode(SDL_TRUE);
        }

        if (!wnd.has_focus && SDL_GetRelativeMouseMode() != SDL_FALSE) {
            SDL_SetRelativeMouseMode(SDL_FALSE);
        }

        auto *t = tools[pl.active_tool_slot];

        if (t) {
            t->pre_use(&pl);

            /* tool use */
            if (pl.use_tool) {
                t->use();
                pl.ui_dirty = true;
            }

            if (pl.alt_use_tool) {
                t->alt_use();
                pl.ui_dirty = true;
            }

            if (pl.long_use_tool) {
                t->long_use();
                pl.ui_dirty = true;
            }

            if (pl.long_alt_use_tool) {
                t->long_alt_use();
                pl.ui_dirty = true;
            }

            if (pl.cycle_mode) {
                t->cycle_mode();
                pl.ui_dirty = true;
            }
        }

        /* can only interact with entities which have
         * the switch component
         */
        auto &switch_man = component_system_man.managers.switch_component_man;
        auto &surf_man = component_system_man.managers.surface_attachment_component_man;
        raycast_info_world rc_ent;
        phys_raycast_world(pl.eye, pl.eye + 2.f * pl.dir,
                           phy->rb_controller.get(), phy->dynamicsWorld.get(), &rc_ent);

        auto old_entity = use_entity;
        use_entity = rc_ent.entity;
        if (rc_ent.hit && c_entity::is_valid(rc_ent.entity)) {
            // if entity is not bolted down, remove from world
            // otherwise, interact
            auto sa = surf_man.get_instance_data(rc_ent.entity);
            if (!*sa.attached && pl.use) {
                destroy_entity(rc_ent.entity);
                use_entity.id = 0;
            } else if (switch_man.exists(rc_ent.entity)) {
                if (pl.use && c_entity::is_valid(rc_ent.entity)) {
                    use_action_on_entity(ship, rc_ent.entity);
                }
            }
        }
        if (old_entity != use_entity) {
            pl.ui_dirty = true;
        }
    }

    void render(frame_data *frame) override {
        auto *t = tools[pl.active_tool_slot];

        if (t) {
            t->pre_use(&pl);
            t->preview(frame);
        }
    }

    void set_slot(unsigned slot) {
        /* note: all the number keys are bound, but we may not have 10 toolbelt slots.
         * just drop bogus slot requests on the floor.
         */
        if (slot < tools.size() && slot != pl.active_tool_slot) {
            auto *old_tool = tools[pl.active_tool_slot];
            if (old_tool) {
                old_tool->unselect();
            }

            tools[slot]->select();

            pl.active_tool_slot = slot;
            pl.ui_dirty = true;
        }
    }

    void cycle_slot(int d) {
        unsigned num_tools = unsigned(tools.size());
        unsigned int cur_slot = pl.active_tool_slot;
        cur_slot = (cur_slot + num_tools + d) % num_tools;

        set_slot(cur_slot);
    }

    void handle_input() override {
        /* look */
        auto look_x     = get_input(action_look_x)->value;
        auto look_y     = get_input(action_look_y)->value;

        /* movement */
        auto moveX      = get_input(action_right)->active - get_input(action_left)->active;
        auto moveY      = get_input(action_forward)->active - get_input(action_back)->active;

        /* crouch */
        auto crouch     = get_input(action_crouch)->active;
        auto crouch_end = get_input(action_crouch)->just_inactive;

        /* momentary */
        auto jump       = get_input(action_jump)->just_active;
        auto reset      = get_input(action_reset)->just_active;
        auto use        = get_input(action_use)->just_active;
        auto cycle_mode = get_input(action_cycle_mode)->just_active;
        auto slot0      = get_input(action_slot1)->just_active;
        auto slot1      = get_input(action_slot2)->just_active;
        auto slot2      = get_input(action_slot3)->just_active;
        auto slot3      = get_input(action_slot4)->just_active;
        auto slot4      = get_input(action_slot5)->just_active;
        auto slot5      = get_input(action_slot6)->just_active;
        auto slot6      = get_input(action_slot7)->just_active;
        auto slot7      = get_input(action_slot8)->just_active;
        auto slot8      = get_input(action_slot9)->just_active;
        auto slot9      = get_input(action_slot0)->just_active;
        auto gravity    = get_input(action_gravity)->just_active;
        auto next_tool  = get_input(action_tool_next)->just_active;
        auto prev_tool  = get_input(action_tool_prev)->just_active;

        auto input_use_tool     = get_input(action_use_tool);
        auto use_tool           = input_use_tool->just_pressed;
        auto long_use_tool      = input_use_tool->held;
        auto input_alt_use_tool = get_input(action_alt_use_tool);
        auto alt_use_tool       = input_alt_use_tool->just_pressed;
        auto long_alt_use_tool  = input_alt_use_tool->held;

        /* persistent */

        float mouse_invert = game_settings.input.mouse_invert;

        pl.angle += game_settings.input.mouse_x_sensitivity * look_x;
        pl.elev += game_settings.input.mouse_y_sensitivity * mouse_invert * look_y;

        if (pl.elev < -MOUSE_Y_LIMIT)
            pl.elev = -MOUSE_Y_LIMIT;
        if (pl.elev > MOUSE_Y_LIMIT)
            pl.elev = MOUSE_Y_LIMIT;

        pl.move = glm::vec2((float) moveX, (float) moveY);

        pl.jump              = jump;
        pl.crouch            = crouch;
        pl.reset             = reset;
        pl.crouch_end        = crouch_end;
        pl.use               = use;
        pl.cycle_mode        = cycle_mode;
        pl.gravity           = gravity;
        pl.use_tool          = use_tool;
        pl.alt_use_tool      = alt_use_tool;
        pl.long_use_tool     = long_use_tool;
        pl.long_alt_use_tool = long_alt_use_tool;

        // blech. Tool gets used below, then fire projectile gets hit here
        if (pl.fire_projectile) {
            auto below_eye = glm::vec3(pl.eye.x, pl.eye.y, pl.eye.z - 0.1);
            proj_man.spawn(below_eye, pl.dir);
            pl.fire_projectile = false;
        }

        if (next_tool) {
            cycle_slot(1);
        }
        if (prev_tool) {
            cycle_slot(-1);
        }

        if (slot1) set_slot(1);
        if (slot2) set_slot(2);
        if (slot3) set_slot(3);
        if (slot4) set_slot(4);
        if (slot5) set_slot(5);
        if (slot6) set_slot(6);
        if (slot7) set_slot(7);
        if (slot8) set_slot(8);
        if (slot9) set_slot(9);
        if (slot0) set_slot(0);

        /* limit to unit vector */
        float len = glm::length(pl.move);
        if (len > 0.0f)
            pl.move = pl.move / len;

        if (get_input(action_menu)->just_active) {
            set_game_state(create_menu_state());
        }
    }
};


struct menu_state : game_state {
    enum class MenuState {
        Main,
        Settings,
        Keybinds,
    } state{MenuState::Main};

    bool settings_dirty = false;

    unsigned menu_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize;

    menu_state() {
        SDL_WarpMouseInWindow(wnd.ptr, wnd.width / 2, wnd.height / 2);
        state = MenuState::Main;
    }

    void handle_input() override {
        if (get_input(action_menu)->just_active) {
            switch (state) {
                case MenuState::Main:
                    set_game_state(create_play_state());
                    break;
                case MenuState::Settings:
                case MenuState::Keybinds:
                    state = MenuState::Main;
                    break;
            }
        }
    }

    void rebuild_ui() override {
    }

    void update(float dt) override {
        if (wnd.has_focus && SDL_GetRelativeMouseMode() == SDL_TRUE) {
            SDL_SetRelativeMouseMode(SDL_FALSE);
        }
    }

    void handle_main_menu() {
        // todo: use a proper state transition system
        if (settings_dirty) {
            save_settings(game_settings);
            apply_video_settings();
            settings_dirty = false;
        }

        ImGui::Begin("", nullptr, menu_flags);
        {
            ImGui::Text("Engineer's Nightmare");
            ImGui::Separator();
            ImGui::Dummy(ImVec2{ 10, 10 });
            if (ImGui::Button("Resume Game")) {
                set_game_state(create_play_state());
            }
            ImGui::Dummy(ImVec2{10, 10});
            if (ImGui::Button("Settings")) {
                state = MenuState::Settings;
            }
            ImGui::Dummy(ImVec2{10, 10});
            if (ImGui::Button("Exit Game")) {
                exit_requested = true;
            }
        }
        ImGui::End();
    }

    void handle_settings_menu() {
        ImGui::Begin("", nullptr, menu_flags);
        {
            ImGui::Text("Settings");
            ImGui::Separator();

            std::array<const char*, 2> modes{
                get_enum_description(window_mode::windowed),
                get_enum_description(window_mode::fullscreen),
            };
            auto mode = game_settings.video.mode;

            settings_dirty |= ImGui::Combo("Window Mode", (int*)&mode, modes.data(), modes.size());
            game_settings.video.mode = mode;

            bool invert = game_settings.input.mouse_invert == -1.0f;
            settings_dirty |= ImGui::Checkbox("Invert Mouse", &invert);
            game_settings.input.mouse_invert = invert ? -1.0f : 1.0f;

            ImGui::Separator();
            ImGui::Checkbox("Draw FPS", &draw_fps);
            ImGui::Separator();

            // debug
            bool draw_debug = draw_debug_text && draw_debug_chunks && draw_debug_axis && draw_debug_physics;
            if (ImGui::Checkbox("Draw Debug All", &draw_debug)) {
                draw_debug_text = draw_debug_chunks = draw_debug_axis = draw_debug_physics = draw_debug;
            }
            ImGui::Checkbox("Draw Debug Text", &draw_debug_text);
            ImGui::Checkbox("Draw Chunk Debug", &draw_debug_chunks);
            ImGui::Checkbox("Draw Axis Debug", &draw_debug_axis);
            ImGui::Checkbox("Draw Physics Debug", &draw_debug_physics);

            ImGui::Dummy(ImVec2{ 10, 10 });
            if (ImGui::Button("Back")) {
                state = MenuState::Main;
            }
        }
        ImGui::End();
    }

    void render(frame_data *frame) override {
        ImGui::SetCurrentContext(default_context);
        ImGui_ImplSdlGL3_NewFrame(wnd.ptr);

        // center on screen
        ImGui::SetNextWindowPos(ImVec2{ wnd.width / 2.0f, wnd.height / 2.0f }, 0, ImVec2{ 0.5f, 0.5f });
        {
            switch (state) {
                case MenuState::Main: {
                    handle_main_menu();
                    break;
                }
                case MenuState::Settings: {
                    handle_settings_menu();
                    break;
                }
                case MenuState::Keybinds: {
//                    handle_keybinds_menu();
                    break;
                }
            }
        }
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        glViewport(0, 0, wnd.width, wnd.height);
        ImGui::Render();
    }
};

game_state *game_state::create_play_state() { return new play_state; }
game_state *game_state::create_menu_state() { return new menu_state; }

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
            memset(mouse_buttons, 0, input_mouse_buttons_count);
            memset(mouse_axes, 0, input_mouse_axes_count);
        }

        ImGui::SetCurrentContext(offscreen_contexts[0]);
        ImGui_ImplSdlGL3_NewFrame(wnd.ptr);

        auto flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoInputs;
        // center on screen
        // todo: modify NewFrame() to allow us to use this properly
        ImGui::SetNextWindowPos(ImVec2{ RENDER_DIM / 2, RENDER_DIM / 4}, 0, ImVec2{ 0.5f, 0.5f });
        {
            ImGui::Begin("First Window", nullptr, flags);
            {
                static float f = 0.0f;
                ImGui::Text("Hello, world!");
                ImGui::SliderFloat("float", &f, 0.0f, 1.0f);
                ImGui::Button("Test Window");
                ImGui::Button("Another Window");
                ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
            }
            ImGui::End();
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, render_fbo);
            glFramebufferTextureLayer(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, 2, 0, 0);
            glViewport(0, 0, RENDER_DIM, RENDER_DIM);
            glClearColor(0, 0.18f, 0.21f, 1);
            glClear(GL_COLOR_BUFFER_BIT);
            ImGui::Render();
        }

        ImGui::SetCurrentContext(offscreen_contexts[1]);
        ImGui_ImplSdlGL3_NewFrame(wnd.ptr);
        ImGui::SetNextWindowPos(ImVec2{ RENDER_DIM / 2, RENDER_DIM / 4 }, 0, ImVec2{ 0.5f, 0.5f });
        {
            ImGui::Begin("Second Window", nullptr, flags);
            {
                static float f = 0.0f;
                ImGui::Text("Goodbye, world!");
                ImGui::SliderFloat("float", &f, 0.0f, 1.0f);
                ImGui::Button("Test Window");
                ImGui::Button("Another Window");
                ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
            }
            ImGui::End();
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, render_fbo);
            glFramebufferTextureLayer(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, 2, 0, 1);
            glViewport(0, 0, RENDER_DIM, RENDER_DIM);
            glClearColor(1, 0.0f, 0.18f, 1);
            glClear(GL_COLOR_BUFFER_BIT);
            ImGui::Render();
        }

        // reset back to default framebuffer
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

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

    return 0;
}
