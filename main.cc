#include <stdio.h>
#include <SDL.h>
#include <SDL_image.h>
#include <err.h>
#include <epoxy/gl.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "src/common.h"
#include "src/mesh.h"
#include "src/shader.h"
#include "src/ship_space.h"
#include "src/player.h"
#include "src/physics.h"

#include <vector>       // HISSSSSS, for temp global ent list.


#define APP_NAME    "Engineer's Nightmare"
#define DEFAULT_WIDTH   1024
#define DEFAULT_HEIGHT  768


#define WORLD_TEXTURE_DIMENSION     32
#define MAX_WORLD_TEXTURES          64

#define MOUSE_Y_LIMIT   1.54
#define MOUSE_X_SENSITIVITY -0.001
#define MOUSE_Y_SENSITIVITY -0.001


struct wnd {
    SDL_Window *ptr;
    SDL_GLContext gl_ctx;
    int width;
    int height;
} wnd;


/* where T is std140-conforming, and agrees with the shader. */
template<typename T>
struct shader_params
{
    T val;
    GLuint bo;

    shader_params() : bo(0)
    {
        glGenBuffers(1, &bo);
        glBindBuffer(GL_UNIFORM_BUFFER, bo);
        glBufferData(GL_UNIFORM_BUFFER, sizeof(T), NULL, GL_DYNAMIC_DRAW);
    }

    ~shader_params() {
        glDeleteBuffers(1, &bo);
    }

    void upload() {
        /* bind to nonindexed binding point */
        glBindBuffer(GL_UNIFORM_BUFFER, bo);
        glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(T), &val);
    }

    void bind(GLuint index) {
        /* bind to proper index of indexed binding point, for use */
        glBindBufferBase(GL_UNIFORM_BUFFER, index, bo);
    }
};


struct per_camera_params {
    glm::mat4 view_proj_matrix;
};

struct per_object_params {
    glm::mat4 world_matrix;
};


struct texture_set {
    GLuint texobj;
    int dim;
    int array_size;

    texture_set(int dim, int array_size) : texobj(0), dim(dim), array_size(array_size) {
        glGenTextures(1, &texobj);
        glBindTexture(GL_TEXTURE_2D_ARRAY, texobj);
        glTexStorage3D(GL_TEXTURE_2D_ARRAY,
                       1,   /* no mips! I WANT YOUR EYES TO BLEED -- todo, fix this. */
                       GL_RGBA8, dim, dim, array_size);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    }

    void bind(int texunit)
    {
        glActiveTexture(GL_TEXTURE0 + texunit);
        glBindTexture(GL_TEXTURE_2D_ARRAY, texobj);
    }

    void load(int slot, char const *filename)
    {
        SDL_Surface* surf = IMG_Load( filename );

        if (!surf)
            errx(1, "Failed to load texture %d:%s", slot, filename);
        if (surf->w != dim || surf->h != dim)
            errx(1, "Texture %d:%s is the wrong size: %dx%d but expected %dx%d",
                    slot, filename, surf->w, surf->h, dim, dim);

        /* bring on DSA... for now, we disturb the tex0 binding */
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D_ARRAY, texobj);

        /* just blindly upload as if it's RGBA/UNSIGNED_BYTE. TODO: support weirder things */
        glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0,
                        0, 0, slot,
                        dim, dim, 1,
                        GL_RGBA,
                        GL_UNSIGNED_BYTE,
                        surf->pixels);

        SDL_FreeSurface(surf);
    }
};


void
gl_debug_callback(GLenum source __unused,
                  GLenum type __unused,
                  GLenum id __unused,
                  GLenum severity __unused,
                  GLsizei length __unused,
                  GLchar const *message,
                  void const *userParam __unused)
{
    printf("GL: %s\n", message);
}

sw_mesh *scaffold_sw;
sw_mesh *surfs_sw[6];
sw_mesh *frobnicator_sw;
GLuint simple_shader;
shader_params<per_camera_params> *per_camera;
shader_params<per_object_params> *per_object;
texture_set *world_textures;
ship_space *ship;
player player;
physics *phy;
unsigned char const *keys;
hw_mesh *scaffold_hw;
hw_mesh *surfs_hw[6];
hw_mesh *frobnicator_hw;
hw_mesh *frobnicator_mat_hw;


struct entity
{
    /* TODO: replace this completely, it's silly. */
    int x, y, z;
    hw_mesh *mesh;

    entity(int x, int y, int z, hw_mesh *mesh) : x(x), y(y), z(z), mesh(mesh) {}
};


/* all the bads */
std::vector<entity *> entities;


void
init()
{
    printf("%s starting up.\n", APP_NAME);
    printf("OpenGL version: %.1f\n", epoxy_gl_version() / 10.0f);

    if (epoxy_gl_version() < 33) {
        errx(1, "At least OpenGL 3.3 is required\n");
    }

    /* Enable GL debug extension */
    if (!epoxy_has_gl_extension("GL_KHR_debug"))
        errx(1, "No support for GL debugging, life isn't worth it.\n");

    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(gl_debug_callback, NULL);

    /* Check for ARB_texture_storage */
    if (!epoxy_has_gl_extension("GL_ARB_texture_storage"))
        errx(1, "No support for ARB_texture_storage\n");

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);         /* pointers given by other libs may not be aligned */
    glEnable(GL_DEPTH_TEST);

    scaffold_sw = load_mesh("mesh/initial_scaffold.obj");

    surfs_sw[surface_xp] = load_mesh("mesh/x_quad_p.obj");
    surfs_sw[surface_xm] = load_mesh("mesh/x_quad.obj");
    surfs_sw[surface_yp] = load_mesh("mesh/y_quad_p.obj");
    surfs_sw[surface_ym] = load_mesh("mesh/y_quad.obj");
    surfs_sw[surface_zp] = load_mesh("mesh/z_quad_p.obj");
    surfs_sw[surface_zm] = load_mesh("mesh/z_quad.obj");

    for (int i = 0; i < 6; i++)
        surfs_hw[i] = upload_mesh(surfs_sw[i]);

    frobnicator_sw = load_mesh("mesh/frobnicator.obj");
    frobnicator_hw = upload_mesh(frobnicator_sw);
    set_mesh_material(frobnicator_sw, 3);
    frobnicator_mat_hw = upload_mesh(frobnicator_sw);

    simple_shader = load_shader("shaders/simple.vert", "shaders/simple.frag");

    scaffold_hw = upload_mesh(scaffold_sw);         /* needed for overlay */

    glUseProgram(simple_shader);

    per_camera = new shader_params<per_camera_params>;
    per_object = new shader_params<per_object_params>;

    per_object->val.world_matrix = glm::mat4(1);    /* identity */

    per_camera->bind(0);
    per_object->bind(1);

    world_textures = new texture_set(WORLD_TEXTURE_DIMENSION, MAX_WORLD_TEXTURES);
    world_textures->load(0, "textures/white.png");
    world_textures->load(1, "textures/scaffold.png");
    world_textures->load(2, "textures/scaffold.png");   /* todo: replace with something else */
    world_textures->load(3, "textures/frobnicator.png");

    world_textures->bind(0);

    ship = ship_space::mock_ship_space();
    if( ! ship )
        errx(1, "Ship_space::mock_ship_space failed\n");

    player.angle = 0;
    player.elev = 0;
    player.pos = glm::vec3(3,2,2);
    player.last_jump = player.jump = false;
    player.selected_slot = 1;
    player.last_use = player.use = false;
    player.last_reset = player.reset = false;

    phy = new physics(&player);

    glEnable(GL_CULL_FACE);
    glFrontFace(GL_CW);
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


int
normal_to_surface_index(raycast_info const *rc)
{
    if (rc->nx == 1) return 0;
    if (rc->nx == -1) return 1;
    if (rc->ny == 1) return 2;
    if (rc->ny == -1) return 3;
    if (rc->nz == 1) return 4;
    if (rc->nz == -1) return 5;

    return 0;   /* unreachable */
}


void
surface_index_to_normal(int index, int *nx, int *ny, int *nz)
{
    *nx = *ny = *nz = 0;

    switch (index) {
        case 0: *nx = 1; break;
        case 1: *nx = -1; break;
        case 2: *ny = 1; break;
        case 3: *ny = -1; break;
        case 4: *nz = 1; break;
        case 5: *nz = -1; break;
    }
}


void
update()
{
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClearDepth(1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    player.dir = glm::vec3(
            cosf(player.angle) * cosf(player.elev),
            sinf(player.angle) * cosf(player.elev),
            sinf(player.elev)
            );

    glm::vec3 eyePos = player.pos + glm::vec3(0, 0, EYE_OFFSET_Z);

    glm::mat4 proj = glm::perspective(45.0f, (float)wnd.width / wnd.height, 0.01f, 1000.0f);
    glm::mat4 view = glm::lookAt(eyePos, eyePos + player.dir, glm::vec3(0, 0, 1));
    per_camera->val.view_proj_matrix = proj * view;
    per_camera->upload();

    /* both tool use and overlays need the raycast itself */
    raycast_info rc;
    ship->raycast(eyePos.x, eyePos.y, eyePos.z, player.dir.x, player.dir.y, player.dir.z, &rc);

    /* tool use */
    if (player.use && !player.last_use) {

        if (rc.hit) {

            switch (player.selected_slot) {
            case 1: {
                    printf("Remove block Raycast: %d,%d,%d\n", rc.x, rc.y, rc.z);
                    block *bl = rc.block;

                    /* if there was a block entity here, find and remove it */
                    if (bl->type == block_entity) {
                        for (std::vector<entity *>::iterator it = entities.begin(); it != entities.end();) {
                            entity *e = *it;
                            if (e->x == rc.x && e->y == rc.y && e->z == rc.z) {
                                it = entities.erase(it);
                            }
                            else {
                                it++;
                            }
                        }
                    }

                    /* block removal */
                    bl->type = block_empty;

                    /* strip any orphaned surfaces */
                    for (int index = 0; index < 6; index++) {
                        if (bl->surfs[index]) {

                            int sx, sy, sz;
                            surface_index_to_normal(index, &sx, &sy, &sz);
                            block *other_side = ship->get_block(rc.x + sx, rc.y + sy, rc.z + sz);

                            if (!other_side) {
                                /* expand: but this should always exist. */
                            }
                            else if (other_side->type != block_support) {
                                /* if the other side has no scaffold, then there is nothing left to support this
                                 * surface pair -- remove it */
                                bl->surfs[index] = surface_none;
                                other_side->surfs[index ^ 1] = surface_none;
                                ship->get_chunk_containing(rc.x + sx, rc.y + sy, rc.z + sz)->render_chunk.valid = false;
                            }
                        }
                    }

                    /* dirty the chunk */
                    ship->get_chunk_containing(rc.x, rc.y, rc.z)->render_chunk.valid = false;
                } break;

            case 2: {
                    printf("Add block Raycast: %d,%d,%d\n", rc.x, rc.y, rc.z);

                    block *bl = ship->get_block(rc.x + rc.nx, rc.y + rc.ny, rc.z + rc.nz);

                    /* can only build on the side of an existing scaffold */
                    if (bl && rc.block->type == block_support) {
                        bl->type = block_support;
                        /* dirty the chunk */
                        ship->get_chunk_containing(rc.x + rc.nx, rc.y + rc.ny, rc.z + rc.nz)->render_chunk.valid = false;
                    }
                } break;

            case 3: {
                    printf("Add surface raycast %d,%d,%d\n", rc.x, rc.y, rc.z);
                    block *bl = rc.block;

                    int index = normal_to_surface_index(&rc);
                    block *other_side = ship->get_block(rc.x + rc.nx, rc.y + rc.ny, rc.z + rc.nz);

                    if (!other_side) {
                        /* expand ! */
                    }
                    else if (bl->surfs[index] == surface_none && (bl->type == block_support || other_side->type == block_support)) {

                        bl->surfs[index] = surface_wall;
                        ship->get_chunk_containing(rc.x, rc.y, rc.z)->render_chunk.valid = false;

                        other_side->surfs[index ^ 1] = surface_wall;
                        ship->get_chunk_containing(rc.x + rc.nx, rc.y + rc.ny, rc.z + rc.nz)->render_chunk.valid = false;

                    }

                } break;

            case 4: {
                    printf("Remove surface raycast %d,%d,%d\n", rc.x, rc.y, rc.z);
                    block *bl = rc.block;

                    int index = normal_to_surface_index(&rc);

                    if (bl->surfs[index] != surface_none) {

                        bl->surfs[index] = surface_none;
                        ship->get_chunk_containing(rc.x, rc.y, rc.z)->render_chunk.valid = false;

                        /* cause the other side to exist */
                        block *other_side = ship->get_block(rc.x + rc.nx, rc.y + rc.ny, rc.z + rc.nz);

                        if (!other_side) {
                            /* expand: note: we shouldn't ever actually have to do this... */
                        }
                        else {
                            other_side->surfs[index ^ 1] = surface_none;
                            ship->get_chunk_containing(rc.x + rc.nx, rc.y + rc.ny, rc.z + rc.nz)->render_chunk.valid = false;
                        }

                    }

                } break;

            case 5: {
                    printf("Add frobnicator entity Raycast: %d,%d,%d\n", rc.x, rc.y, rc.z);

                    block *bl = ship->get_block(rc.x + rc.nx, rc.y + rc.ny, rc.z + rc.nz);

                    /* can only build on the side of an existing scaffold */
                    if (bl && rc.block->type == block_support) {
                        bl->type = block_entity;
                        /* dirty the chunk -- TODO: do we really have to do this when changing a cell from
                         * empty -> entity? */
                        ship->get_chunk_containing(rc.x + rc.nx, rc.y + rc.ny, rc.z + rc.nz)->render_chunk.valid = false;
                    }

                    /* TODO: flesh this out a bit */
                    entities.push_back(new entity(rc.x + rc.nx, rc.y + rc.ny, rc.z + rc.nz, frobnicator_mat_hw));
                } break;
            }
        }
    }

    /* walk all the chunks -- TODO: only walk chunks that might contribute to the view */
    for (int k = ship->chunks.zo; k < ship->chunks.zo + ship->chunks.zd; k++) {
        for (int j = ship->chunks.yo; j < ship->chunks.yo + ship->chunks.yd; j++) {
            for (int i = ship->chunks.xo; i < ship->chunks.xo + ship->chunks.xd; i++) {
                ship->get_chunk(i, j, k)->prepare_render(i, j, k);
            }
        }
    }

    for (int k = ship->chunks.zo; k < ship->chunks.zo + ship->chunks.zd; k++) {
        for (int j = ship->chunks.yo; j < ship->chunks.yo + ship->chunks.yd; j++) {
            for (int i = ship->chunks.xo; i < ship->chunks.xo + ship->chunks.xd; i++) {
                /* TODO: prepare all the matrices first, and do ONE upload */
                per_object->val.world_matrix = glm::translate(glm::mat4(1), glm::vec3(
                            (float)i * CHUNK_SIZE, (float)j * CHUNK_SIZE, (float)k * CHUNK_SIZE));
                per_object->upload();
                draw_mesh(ship->get_chunk(i, j, k)->render_chunk.mesh);
            }
        }
    }

    /* walk all the entities -- TODO: push them into the ship! */
    for (int i = 0; i < entities.size(); i++) {
        entity *e = entities[i];

        /* TODO: batch these matrix uploads too! */
        per_object->val.world_matrix = glm::translate(glm::mat4(1), glm::vec3(
            (float)e->x, (float)e->y, (float)e->z));
        per_object->upload();

        draw_mesh(e->mesh);
    }

    /* tool preview */
    switch (player.selected_slot) {
        case 2: {
                    if (rc.hit) {
                        block *bl = ship->get_block(rc.x + rc.nx, rc.y + rc.ny, rc.z + rc.nz);

                        /* can only build on the side of an existing scaffold */
                        if (bl && rc.block->type == block_support) {
                            per_object->val.world_matrix = glm::translate(glm::mat4(1),
                                    glm::vec3(rc.x + rc.nx, rc.y + rc.ny, rc.z + rc.nz));
                            per_object->upload();

                            draw_mesh(scaffold_hw);
                        }
                    }
                } break;
        case 3: {
                    if (rc.hit) {
                        block *bl = ship->get_block(rc.x, rc.y, rc.z);
                        int index = normal_to_surface_index(&rc);
                        block *other_side = ship->get_block(rc.x + rc.nx, rc.y + rc.ny, rc.z + rc.nz);

                        if (bl && bl->surfs[index] == surface_none && (bl->type == block_support || (other_side && other_side->type == block_support))) {
                            per_object->val.world_matrix = glm::translate(glm::mat4(1),
                                    glm::vec3(rc.x, rc.y, rc.z));
                            per_object->upload();

                            draw_mesh(surfs_hw[index]);
                        }
                    }
                } break;
        case 5: {
                    if (rc.hit) {
                        block *bl = ship->get_block(rc.x + rc.nx, rc.y + rc.ny, rc.z + rc.nz);

                        /* frobnicator can only be placed in empty space, on a scaffold */
                        if (bl && rc.block->type == block_support) {
                            per_object->val.world_matrix = glm::translate(glm::mat4(1),
                                    glm::vec3(rc.x + rc.nx, rc.y + rc.ny, rc.z + rc.nz));
                            per_object->upload();

                            draw_mesh(frobnicator_hw);
                        }
                    }
                } break;
    }
}


void
handle_input()
{
    player.move.x = keys[SDL_SCANCODE_D] - keys[SDL_SCANCODE_A];
    player.move.y = keys[SDL_SCANCODE_W] - keys[SDL_SCANCODE_S];

    /* limit to unit vector */
    float len = glm::length(player.move);
    if (len > 0.0f)
        player.move = player.move / len;

    player.last_jump = player.jump;
    player.jump = keys[SDL_SCANCODE_SPACE];

    player.last_use = player.use;
    player.use = keys[SDL_SCANCODE_E];

    player.last_reset = player.reset;
    player.reset = keys[SDL_SCANCODE_R];

    /* TODO: be less ridiculous */
    if (keys[SDL_SCANCODE_1])
        player.selected_slot = 1;
    if (keys[SDL_SCANCODE_2])
        player.selected_slot = 2;
    if (keys[SDL_SCANCODE_3])
        player.selected_slot = 3;
    if (keys[SDL_SCANCODE_4])
        player.selected_slot = 4;
    if (keys[SDL_SCANCODE_5])
        player.selected_slot = 5;
}


void
run()
{
    for (;;) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            switch (e.type) {
            case SDL_QUIT:
                printf("Quit event caught, shutting down.\n");
                return;

            case SDL_WINDOWEVENT:
                /* We MUST support resize events even if we
                 * don't really care about resizing, because a tiling
                 * WM isn't going to give us what we asked for anyway!
                 */
                if (e.window.event == SDL_WINDOWEVENT_RESIZED)
                    resize(e.window.data1, e.window.data2);
                break;

            case SDL_MOUSEMOTION:
                player.angle += MOUSE_X_SENSITIVITY * e.motion.xrel;
                player.elev += MOUSE_Y_SENSITIVITY * e.motion.yrel;

                if (player.elev < -MOUSE_Y_LIMIT)
                    player.elev = -MOUSE_Y_LIMIT;
                if (player.elev > MOUSE_Y_LIMIT)
                    player.elev = MOUSE_Y_LIMIT;
                break;
            }
        }

        /* SDL_PollEvent above has already pumped the input, so current key state is available */
        handle_input();

        /* physics tick */
        phy->tick();

        update();

        SDL_GL_SwapWindow(wnd.ptr);
    }
}

int
main(int argc, char **argv)
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
        errx(1, "Error initializing SDL: %s\n", SDL_GetError());

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    wnd.ptr = SDL_CreateWindow(APP_NAME,
                               SDL_WINDOWPOS_CENTERED,
                               SDL_WINDOWPOS_CENTERED,
                               DEFAULT_WIDTH,
                               DEFAULT_HEIGHT,
                               SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);

    if (!wnd.ptr)
        errx(1, "Failed to create window.\n");

    wnd.gl_ctx = SDL_GL_CreateContext(wnd.ptr);

    SDL_SetRelativeMouseMode(SDL_TRUE);
    keys = SDL_GetKeyboardState(NULL);

    resize(DEFAULT_WIDTH, DEFAULT_HEIGHT);

    init();

    run();

    return 0;
}
