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
#include "src/text.h"


#define APP_NAME    "Engineer's Nightmare"
#define DEFAULT_WIDTH   1024
#define DEFAULT_HEIGHT  768


#define WORLD_TEXTURE_DIMENSION     32
#define MAX_WORLD_TEXTURES          64

#define MOUSE_Y_LIMIT   1.54
#define MOUSE_X_SENSITIVITY -0.001
#define MOUSE_Y_SENSITIVITY -0.001

// 1 for ordinary use, -1 to invert mouse. TODO: settings...
#define MOUSE_INVERT_LOOK 1

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
    GLenum target;

    texture_set(GLenum target, int dim, int array_size) : texobj(0), dim(dim), array_size(array_size), target(target) {
        glGenTextures(1, &texobj);
        glBindTexture(target, texobj);
        glTexStorage3D(target,
                       1,   /* no mips! I WANT YOUR EYES TO BLEED -- todo, fix this. */
                       GL_RGBA8, dim, dim, array_size);
        glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    }

    void bind(int texunit)
    {
        glActiveTexture(GL_TEXTURE0 + texunit);
        glBindTexture(target, texobj);
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
        glBindTexture(target, texobj);

        /* just blindly upload as if it's RGBA/UNSIGNED_BYTE. TODO: support weirder things */
        glTexSubImage3D(target, 0,
                        0, 0, slot,
                        dim, dim, 1,
                        surf->format->BytesPerPixel == 4 ? GL_RGBA : GL_RGB,
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
GLuint simple_shader, add_overlay_shader, remove_overlay_shader, ui_shader;
GLuint sky_shader;
shader_params<per_camera_params> *per_camera;
shader_params<per_object_params> *per_object;
texture_set *world_textures;
texture_set *skybox;
ship_space *ship;
player player;
physics *phy;
unsigned char const *keys;
hw_mesh *scaffold_hw;
hw_mesh *surfs_hw[6];
text_renderer *text;


struct entity_type
{
    sw_mesh *sw;
    hw_mesh *hw;
    char const *name;
    btTriangleMesh *phys_mesh;
    btCollisionShape *phys_shape;
};


entity_type entity_types[2];


struct entity
{
    /* TODO: replace this completely, it's silly. */
    int x, y, z;
    entity_type *type;

    btRigidBody *phys_body;

    entity(int x, int y, int z, entity_type *type)
        : x(x), y(y), z(z), type(type), phys_body(0)
    {
        build_static_physics_rb(x, y, z, type->phys_shape, &phys_body);

        /* so that we can get back to the entity from a phys raycast */
        phys_body->setUserPointer(this);
    }

    ~entity()
    {
        teardown_static_physics_setup(NULL, NULL, &phys_body);
    }

    void use()
    {
        /* used by the player */
        printf("player using the %s at %d %d %d\n",
               type->name, x, y, z);
    }
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

    entity_types[0].sw = load_mesh("mesh/frobnicator.obj");
    set_mesh_material(entity_types[0].sw, 3);
    entity_types[0].hw = upload_mesh(entity_types[0].sw);
    entity_types[0].name = "Frobnicator";
    build_static_physics_mesh(entity_types[0].sw, &entity_types[0].phys_mesh, &entity_types[0].phys_shape);

    entity_types[1].sw = load_mesh("mesh/panel_4x4.obj");
    set_mesh_material(entity_types[1].sw, 7);
    entity_types[1].hw = upload_mesh(entity_types[1].sw);
    entity_types[1].name = "Display Panel (4x4)";
    build_static_physics_mesh(entity_types[1].sw, &entity_types[1].phys_mesh, &entity_types[1].phys_shape);

    simple_shader = load_shader("shaders/simple.vert", "shaders/simple.frag");
    add_overlay_shader = load_shader("shaders/add_overlay.vert", "shaders/simple.frag");
    remove_overlay_shader = load_shader("shaders/remove_overlay.vert", "shaders/simple.frag");
    ui_shader = load_shader("shaders/ui.vert", "shaders/ui.frag");
    sky_shader = load_shader("shaders/sky.vert", "shaders/sky.frag");

    scaffold_hw = upload_mesh(scaffold_sw);         /* needed for overlay */

    glUseProgram(simple_shader);

    per_camera = new shader_params<per_camera_params>;
    per_object = new shader_params<per_object_params>;

    per_object->val.world_matrix = glm::mat4(1);    /* identity */

    per_camera->bind(0);
    per_object->bind(1);

    world_textures = new texture_set(GL_TEXTURE_2D_ARRAY, WORLD_TEXTURE_DIMENSION, MAX_WORLD_TEXTURES);
    world_textures->load(0, "textures/white.png");
    world_textures->load(1, "textures/scaffold.png");
    world_textures->load(2, "textures/plate.png");
    world_textures->load(3, "textures/frobnicator.png");
    world_textures->load(4, "textures/grate.png");
    world_textures->load(5, "textures/red.png");
    world_textures->load(6, "textures/text_example.png");
    world_textures->load(7, "textures/display.png");

    skybox = new texture_set(GL_TEXTURE_CUBE_MAP_ARRAY, 2048, 6);
    skybox->load(0, "textures/sky_right1.png");
    skybox->load(1, "textures/sky_left2.png");
    skybox->load(2, "textures/sky_top3.png");
    skybox->load(3, "textures/sky_bottom4.png");
    skybox->load(4, "textures/sky_front5.png");
    skybox->load(5, "textures/sky_back6.png");

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
    player.last_left_button = player.left_button = false;
    player.last_crouch = player.crouch = false;
    player.ui_dirty = true;
    player.disable_gravity = false;

    phy = new physics(&player);

    glEnable(GL_CULL_FACE);
    glFrontFace(GL_CW);

    text = new text_renderer("fonts/pixelmix.ttf", 16);
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
    virtual void get_description(char *str) = 0;
};


struct add_block_tool : public tool
{
    virtual void use(raycast_info *rc)
    {
        if (rc->inside) return; /* n/a */

        /* ensure we can access this x,y,z */
        ship->ensure_block(rc->px, rc->py, rc->pz);

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
        if (rc->inside) return; /* n/a */

        block *bl = ship->get_block(rc->px, rc->py, rc->pz);

        /* can only build on the side of an existing scaffold */
        if ((!bl || bl->type == block_empty) && rc->block->type == block_support) {
            per_object->val.world_matrix = mat_position(rc->px, rc->py, rc->pz);
            per_object->upload();

            glUseProgram(add_overlay_shader);
            draw_mesh(scaffold_hw);
            glUseProgram(simple_shader);
        }
    }

    virtual void get_description(char *str)
    {
        strcpy(str, "Place Scaffolding");
    }
};


struct remove_block_tool : public tool
{
    virtual void use(raycast_info *rc)
    {
        if (rc->inside) return; /* n/a */

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
        if (rc->inside) return; /* n/a */

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

    virtual void get_description(char *str)
    {
        strcpy(str, "Remove Scaffolding");
    }
};


struct add_surface_tool : public tool
{
    surface_type st;
    add_surface_tool(surface_type st) : st(st) {}

    bool can_use(block *bl, block *other, int index)
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

        if (can_use(bl, other_side, index)) {

            if (!other_side) {
                ship->ensure_block(rc->px, rc->py, rc->pz);
                other_side = ship->get_block(rc->px, rc->py, rc->pz);
            }

            bl->surfs[index] = this->st;
            ship->get_chunk_containing(rc->x, rc->y, rc->z)->render_chunk.valid = false;

            other_side->surfs[index ^ 1] = this->st;
            ship->get_chunk_containing(rc->px, rc->py, rc->pz)->render_chunk.valid = false;

        }
    }

    virtual void preview(raycast_info *rc)
    {
        block *bl = ship->get_block(rc->x, rc->y, rc->z);
        int index = normal_to_surface_index(rc);
        block *other_side = ship->get_block(rc->px, rc->py, rc->pz);

        if (can_use(bl, other_side, index)) {
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

    virtual void get_description(char *str)
    {
        sprintf(str, "Place surface type %d\n", (int)st);
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

    virtual void get_description(char *str)
    {
        strcpy(str, "Remove surface");
    }
};


struct add_block_entity_tool : public tool
{
    entity_type *type;

    add_block_entity_tool(entity_type *type) : type(type) {}

    virtual void use(raycast_info *rc)
    {
        if (rc->inside) return; /* n/a */

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
            new entity(rc->px, rc->py, rc->pz, type)
            );
    }

    virtual void preview(raycast_info *rc)
    {
        if (rc->inside) return; /* n/a */

        block *bl = ship->get_block(rc->px, rc->py, rc->pz);

        /* frobnicator can only be placed in empty space, on a scaffold */
        if (bl && rc->block->type == block_support) {
            per_object->val.world_matrix = mat_position(rc->px, rc->py, rc->pz);
            per_object->upload();

            draw_mesh(type->hw);

            /* draw a block overlay as well around the frobnicator */
            glUseProgram(add_overlay_shader);
            draw_mesh(scaffold_hw);
            glUseProgram(simple_shader);
        }
    }

    virtual void get_description(char *str)
    {
        sprintf(str, "Place %s", type->name);
    }
};


struct empty_hands_tool : public tool
{
    virtual void use(raycast_info *rc)
    {
    }

    virtual void preview(raycast_info *rc)
    {
    }

    virtual void get_description(char *str)
    {
        strcpy(str, "(empty hands)");
    }
};


tool *tools[] = {
    NULL,   /* tool 0 isnt a tool (currently) */
    new add_block_tool(),
    new remove_block_tool(),
    new add_surface_tool(surface_wall),
    new add_surface_tool(surface_grate),
    new add_surface_tool(surface_text),
    new remove_surface_tool(),
    new add_block_entity_tool(&entity_types[0]),
    new add_block_entity_tool(&entity_types[1]),
    new empty_hands_tool(),
};


void
rebuild_ui()
{
    text->reset();

    float w = 0;
    float h = 0;
    char buf[256];
    char buf2[512];

    /* Tool name down the bottom */
    tool *t = tools[player.selected_slot];

    if (t) {
        t->get_description(buf);
    }
    else {
        strcpy(buf, "(no tool)");
    }

    sprintf(buf2, "Left mouse button: %s", buf);
    text->measure(buf2, &w, &h);
    text->add(buf2, -w/2, -400);

    /* Gravity state (temp) */
    w = 0; h = 0;
    sprintf(buf, "Gravity: %s (G to toggle)", player.disable_gravity ? "OFF" : "ON");
    text->measure(buf, &w, &h);
    text->add(buf, -w/2, -430);

    text->upload();
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
    if (player.left_button && !player.last_left_button && rc.hit && t) {
        t->use(&rc);
    }

    /* interact with ents */
    entity *hit_ent = phys_raycast(player.eye.x, player.eye.y, player.eye.z,
                                   player.dir.x, player.dir.y, player.dir.z,
                                   2 /* dist */, phy->ghostObj, phy->dynamicsWorld);

    if (player.use && !player.last_use && hit_ent) {
        hit_ent->use();
    }

    world_textures->bind(0);

    /* walk all the chunks -- TODO: only walk chunks that might contribute to the view */
    for (int k = ship->min_z; k <= ship->max_z; k++) {
        for (int j = ship->min_y; j <= ship->max_y; j++) {
            for (int i = ship->min_x; i <= ship->max_x; i++) {
                chunk *ch = ship->get_chunk(i, j, k);
                if (ch) {
                    ch->prepare_render(i, j, k);
                }
            }
        }
    }

    for (int k = ship->min_z; k <= ship->max_z; k++) {
        for (int j = ship->min_y; j <= ship->max_y; j++) {
            for (int i = ship->min_x; i <= ship->max_x; i++) {
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
    for (int k = ship->min_z; k <= ship->max_z; k++) {
        for (int j = ship->min_y; j <= ship->max_y; j++) {
            for (int i = ship->min_x; i <= ship->max_x; i++) {
                chunk *ch = ship->get_chunk(i, j, k);
                if (ch) {
                    for (std::vector<entity *>::const_iterator it = ch->entities.begin(); it != ch->entities.end(); it++) {
                        entity *e = *it;

                        /* TODO: batch these matrix uploads too! */
                        per_object->val.world_matrix = mat_position(e->x, e->y, e->z);
                        per_object->upload();

                        draw_mesh(e->type->hw);
                    }
                }
            }
        }
    }

    /* tool preview */
    if (rc.hit && t) {
        t->preview(&rc);
    }


    /* draw the sky */
    glUseProgram(sky_shader);
    skybox->bind(0);
    glDrawArrays(GL_TRIANGLES, 0, 3);


    if (player.ui_dirty) {
        rebuild_ui();
        player.ui_dirty = false;
    }


    text->upload();

    glDisable(GL_DEPTH_TEST);

    glUseProgram(ui_shader);
    text->draw();
    glUseProgram(simple_shader);

    glEnable(GL_DEPTH_TEST);
}


void
set_slot(int slot)
{
    player.selected_slot = slot;
    player.ui_dirty = true;
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

    player.last_crouch = player.crouch;
    player.crouch = keys[SDL_SCANCODE_LCTRL];

    player.last_gravity = player.gravity;
    player.gravity = keys[SDL_SCANCODE_G];

    if (keys[SDL_SCANCODE_1]) set_slot(1);
    if (keys[SDL_SCANCODE_2]) set_slot(2);
    if (keys[SDL_SCANCODE_3]) set_slot(3);
    if (keys[SDL_SCANCODE_4]) set_slot(4);
    if (keys[SDL_SCANCODE_5]) set_slot(5);
    if (keys[SDL_SCANCODE_6]) set_slot(6);
    if (keys[SDL_SCANCODE_7]) set_slot(7);
    if (keys[SDL_SCANCODE_8]) set_slot(8);
    if (keys[SDL_SCANCODE_9]) set_slot(9);

    /* Current state of the mouse buttons. */
    unsigned int mouse_buttons = SDL_GetRelativeMouseState(NULL, NULL);
    player.last_left_button = player.left_button;
    player.left_button = mouse_buttons & SDL_BUTTON(1);
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
                player.elev += MOUSE_Y_SENSITIVITY * MOUSE_INVERT_LOOK * e.motion.yrel;

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
