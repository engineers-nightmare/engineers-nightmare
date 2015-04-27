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
GLuint simple_shader, add_overlay_shader, remove_overlay_shader;
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


struct entity
{
    /* TODO: replace this completely, it's silly. */
    int x, y, z;
    hw_mesh *mesh;

    /* TODO: phys */

    entity(int x, int y, int z, hw_mesh *mesh) : x(x), y(y), z(z), mesh(mesh) {}
};


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
    set_mesh_material(frobnicator_sw, 3);
    frobnicator_hw = upload_mesh(frobnicator_sw);

    simple_shader = load_shader("shaders/simple.vert", "shaders/simple.frag");
    add_overlay_shader = load_shader("shaders/add_overlay.vert", "shaders/simple.frag");
    remove_overlay_shader = load_shader("shaders/remove_overlay.vert", "shaders/simple.frag");

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
    world_textures->load(2, "textures/plate.png");
    world_textures->load(3, "textures/frobnicator.png");
    world_textures->load(4, "textures/grate.png");
    world_textures->load(5, "textures/red.png");

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


glm::mat4
mat_position(float x, float y, float z)
{
    return glm::translate(glm::mat4(1), glm::vec3(x, y, z));
}


struct tool
{
    virtual void use(raycast_info *rc) = 0;
    virtual void preview(raycast_info *rc) = 0;
};


struct add_block_tool : public tool
{
    virtual void use(raycast_info *rc)
    {
        block *bl = ship->get_block(rc->px, rc->py, rc->pz);

        /* can only build on the side of an existing scaffold */
        if (bl && rc->block->type == block_support) {
            bl->type = block_support;
            /* dirty the chunk */
            ship->get_chunk_containing(rc->px, rc->py, rc->pz)->render_chunk.valid = false;
        }
    }

    virtual void preview(raycast_info *rc)
    {
        block *bl = ship->get_block(rc->px, rc->py, rc->pz);

        /* can only build on the side of an existing scaffold */
        if (bl && rc->block->type == block_support) {
            per_object->val.world_matrix = mat_position(rc->px, rc->py, rc->pz);
            per_object->upload();

            glUseProgram(add_overlay_shader);
            draw_mesh(scaffold_hw);
            glUseProgram(simple_shader);
        }
    }
};


struct remove_block_tool : public tool
{
    virtual void use(raycast_info *rc)
    {
        block *bl = rc->block;

        /* if there was a block entity here, find and remove it */
        if (bl->type == block_entity) {
            chunk *ch = ship->get_chunk_containing(rc->x, rc->y, rc->z);

            for (std::vector<entity *>::iterator it = ch->entities.begin(); it != ch->entities.end();) {
                entity *e = *it;
                if (e->x == rc->x && e->y == rc->y && e->z == rc->z) {
                    delete e;
                    it = ch->entities.erase(it);
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

                int rx = rc->x + sx;
                int ry = rc->y + sy;
                int rz = rc->z + sz;
                block *other_side = ship->get_block(rx, ry, rz);

                if (!other_side) {
                    /* expand: but this should always exist. */
                }
                else if (other_side->type != block_support) {
                    /* if the other side has no scaffold, then there is nothing left to support this
                     * surface pair -- remove it */
                    bl->surfs[index] = surface_none;
                    other_side->surfs[index ^ 1] = surface_none;
                    ship->get_chunk_containing(rx, ry, rz)->render_chunk.valid = false;
                }
            }
        }

        /* dirty the chunk */
        ship->get_chunk_containing(rc->x, rc->y, rc->z)->render_chunk.valid = false;
    }

    virtual void preview(raycast_info *rc)
    {
        block *bl = rc->block;
        if (bl->type != block_empty) {
            per_object->val.world_matrix = mat_position(rc->x, rc->y, rc->z);
            per_object->upload();

            glUseProgram(remove_overlay_shader);
            glEnable(GL_POLYGON_OFFSET_FILL);
            glPolygonOffset(-0.1, -0.1);
            draw_mesh(scaffold_hw);
            glPolygonOffset(0, 0);
            glDisable(GL_POLYGON_OFFSET_FILL);
            glUseProgram(simple_shader);
        }
    }
};


struct add_surface_tool : public tool
{
    surface_type st;
    add_surface_tool(surface_type st) : st(st) {}

    bool can_place(block *bl, block *other, int index)
    {
        if (!bl) return false;
        if (bl->surfs[index] != surface_none) return false; /* already a surface here */
        return bl->type == block_support || (other && other->type == block_support);
    }

    virtual void use(raycast_info *rc)
    {
        block *bl = rc->block;

        int index = normal_to_surface_index(rc);
        block *other_side = ship->get_block(rc->px, rc->py, rc->pz);

        if (!other_side) {
            /* expand ! */
        }
        else if (can_place(bl, other_side, index)) {

            bl->surfs[index] = this->st;
            ship->get_chunk_containing(rc->x, rc->y, rc->z)->render_chunk.valid = false;

            other_side->surfs[index ^ 1] = surface_wall;
            ship->get_chunk_containing(rc->px, rc->py, rc->pz)->render_chunk.valid = false;

        }
    }

    virtual void preview(raycast_info *rc)
    {
        block *bl = ship->get_block(rc->x, rc->y, rc->z);
        int index = normal_to_surface_index(rc);
        block *other_side = ship->get_block(rc->px, rc->py, rc->pz);

        if (can_place(bl, other_side, index)) {
            per_object->val.world_matrix = mat_position(rc->x, rc->y, rc->z);
            per_object->upload();

            glUseProgram(add_overlay_shader);
            glEnable(GL_POLYGON_OFFSET_FILL);
            glPolygonOffset(-0.1, -0.1);
            draw_mesh(surfs_hw[index]);
            glPolygonOffset(0, 0);
            glDisable(GL_POLYGON_OFFSET_FILL);
            glUseProgram(simple_shader);
        }
    }
};


struct remove_surface_tool : public tool
{
    virtual void use(raycast_info *rc)
    {
        block *bl = rc->block;

        int index = normal_to_surface_index(rc);

        if (bl->surfs[index] != surface_none) {

            bl->surfs[index] = surface_none;
            ship->get_chunk_containing(rc->x, rc->y, rc->z)->render_chunk.valid = false;

            /* cause the other side to exist */
            block *other_side = ship->get_block(rc->px, rc->py, rc->pz);

            if (!other_side) {
                /* expand: note: we shouldn't ever actually have to do this... */
            }
            else {
                other_side->surfs[index ^ 1] = surface_none;
                ship->get_chunk_containing(rc->px, rc->py, rc->pz)->render_chunk.valid = false;
            }

        }
    }

    virtual void preview(raycast_info *rc)
    {
        block *bl = ship->get_block(rc->x, rc->y, rc->z);
        int index = normal_to_surface_index(rc);

        if (bl && bl->surfs[index] != surface_none) {

            per_object->val.world_matrix = mat_position(rc->x, rc->y, rc->z);
            per_object->upload();

            glUseProgram(remove_overlay_shader);
            glEnable(GL_POLYGON_OFFSET_FILL);
            glPolygonOffset(-0.1, -0.1);
            draw_mesh(surfs_hw[index]);
            glPolygonOffset(0, 0);
            glDisable(GL_POLYGON_OFFSET_FILL);
            glUseProgram(simple_shader);
        }
    }
};


struct add_block_entity_tool : public tool
{
    /* TODO: ent type */

    virtual void use(raycast_info *rc)
    {
        block *bl = ship->get_block(rc->px, rc->py, rc->pz);

        /* can only build on the side of an existing scaffold */
        if (bl && rc->block->type == block_support) {
            bl->type = block_entity;
            /* dirty the chunk -- TODO: do we really have to do this when changing a cell from
             * empty -> entity? */
            ship->get_chunk_containing(rc->px, rc->py, rc->pz)->render_chunk.valid = false;
        }

        /* TODO: flesh this out a bit */
        ship->get_chunk_containing(rc->px, rc->py, rc->pz)->entities.push_back(
            new entity(rc->px, rc->py, rc->pz, frobnicator_hw)
            );
    }

    virtual void preview(raycast_info *rc)
    {
        block *bl = ship->get_block(rc->px, rc->py, rc->pz);

        /* frobnicator can only be placed in empty space, on a scaffold */
        if (bl && rc->block->type == block_support) {
            per_object->val.world_matrix = mat_position(rc->px, rc->py, rc->pz);
            per_object->upload();

            draw_mesh(frobnicator_hw);

            /* draw a block overlay as well around the frobnicator */
            glUseProgram(add_overlay_shader);
            draw_mesh(scaffold_hw);
            glUseProgram(simple_shader);
        }
    }
};


tool *tools[] = {
    NULL,   /* tool 0 isnt a tool (currently) */
    new add_block_tool(),
    new remove_block_tool(),
    new add_surface_tool(surface_wall),
    new remove_surface_tool(),
    new add_block_entity_tool(),
    new add_surface_tool(surface_grate),
};


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

    player.eye = player.pos + glm::vec3(0, 0, EYE_OFFSET_Z);

    glm::mat4 proj = glm::perspective(45.0f, (float)wnd.width / wnd.height, 0.01f, 1000.0f);
    glm::mat4 view = glm::lookAt(player.eye, player.eye + player.dir, glm::vec3(0, 0, 1));
    per_camera->val.view_proj_matrix = proj * view;
    per_camera->upload();

    tool *t = tools[player.selected_slot];

    /* both tool use and overlays need the raycast itself */
    raycast_info rc;
    ship->raycast(player.eye.x, player.eye.y, player.eye.z, player.dir.x, player.dir.y, player.dir.z, &rc);

    /* tool use */
    if (player.use && !player.last_use && rc.hit && t) {
        t->use(&rc);
    }

    /* walk all the chunks -- TODO: only walk chunks that might contribute to the view */
    for (int k = ship->chunks.zo; k < ship->chunks.zo + ship->chunks.zd; k++) {
        for (int j = ship->chunks.yo; j < ship->chunks.yo + ship->chunks.yd; j++) {
            for (int i = ship->chunks.xo; i < ship->chunks.xo + ship->chunks.xd; i++) {
                chunk *ch = ship->get_chunk(i, j, k);
                if (ch) {
                    ch->prepare_render(i, j, k);
                }
            }
        }
    }

    for (int k = ship->chunks.zo; k < ship->chunks.zo + ship->chunks.zd; k++) {
        for (int j = ship->chunks.yo; j < ship->chunks.yo + ship->chunks.yd; j++) {
            for (int i = ship->chunks.xo; i < ship->chunks.xo + ship->chunks.xd; i++) {
                /* TODO: prepare all the matrices first, and do ONE upload */
                chunk *ch = ship->get_chunk(i, j, k);
                if (ch) {
                    per_object->val.world_matrix = mat_position(
                                (float)i * CHUNK_SIZE, (float)j * CHUNK_SIZE, (float)k * CHUNK_SIZE);
                    per_object->upload();
                    draw_mesh(ch->render_chunk.mesh);
                }
            }
        }
    }

    /* walk all the entities in the (visible) chunks */
    for (int k = ship->chunks.zo; k < ship->chunks.zo + ship->chunks.zd; k++) {
        for (int j = ship->chunks.yo; j < ship->chunks.yo + ship->chunks.yd; j++) {
            for (int i = ship->chunks.xo; i < ship->chunks.xo + ship->chunks.xd; i++) {
                chunk *ch = ship->get_chunk(i, j, k);
                if (ch) {
                    for (std::vector<entity *>::const_iterator it = ch->entities.begin(); it != ch->entities.end(); it++) {
                        entity *e = *it;

                        /* TODO: batch these matrix uploads too! */
                        per_object->val.world_matrix = mat_position(e->x, e->y, e->z);
                        per_object->upload();

                        draw_mesh(e->mesh);
                    }
                }
            }
        }
    }

    /* tool preview */
    if (rc.hit && t) {
        t->preview(&rc);
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
    if (keys[SDL_SCANCODE_6])
        player.selected_slot = 6;
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
