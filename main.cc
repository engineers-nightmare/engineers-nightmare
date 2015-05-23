#include <stdio.h>
#include <SDL.h>
#include <SDL_image.h>

#ifndef _WIN32
#include <err.h> /* errx */
#else
#include "src/winerr.h"
#endif

#include <epoxy/gl.h>
#include <algorithm>

#include <libconfig.h>

#include <unordered_map>

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

/* rebase button */
#define EN_BUTTON(x) (x - INPUT_EOK)

void configureBindings();

bool exit_requested = false;

auto hfov = DEG2RAD(90.f);

struct wnd {
    SDL_Window *ptr;
    SDL_GLContext gl_ctx;
    int width;
    int height;
} wnd;


enum input_action {
    action_left,
    action_right,
    action_forward,
    action_back,
    action_jump,
    action_use,
    action_menu,
    action_reset,
    action_crouch,
    action_gravity,
    action_tool_next,
    action_tool_prev,
    action_use_tool,
    action_slot1,
    action_slot2,
    action_slot3,
    action_slot4,
    action_slot5,
    action_slot6,
    action_slot7,
    action_slot8,
    action_slot9,

    num_actions,
};

typedef struct input_action_lookup_t { const char* name; input_action action; } input_action_lookup_t;
static const input_action_lookup_t action_lookup_table[] = {
    { "action_left",      action_left },
    { "action_right",     action_right },
    { "action_forward",   action_forward },
    { "action_back",      action_back },
    { "action_jump",      action_jump },
    { "action_use",       action_use },
    { "action_menu",      action_menu },
    { "action_reset",     action_reset },
    { "action_crouch",    action_crouch },
    { "action_gravity",   action_gravity },
    { "action_tool_next", action_tool_next },
    { "action_tool_prev", action_tool_prev },
    { "action_use_tool",  action_use_tool },
    { "action_slot1",     action_slot1 },
    { "action_slot2",     action_slot2 },
    { "action_slot3",     action_slot3 },
    { "action_slot4",     action_slot4 },
    { "action_slot5",     action_slot5 },
    { "action_slot6",     action_slot6 },
    { "action_slot7",     action_slot7 },
    { "action_slot8",     action_slot8 },
    { "action_slot9",     action_slot9 },
};

input_action lookup_input_action(const char *name) {
    for (const input_action_lookup_t *input = action_lookup_table; input->name != NULL; ++input) {
        if (strcmp(input->name, name) == 0) {
            return input->action;
        }
    }
    return (input_action)-1;
}

/* fairly ugly. non-keyboard inputs go at bottom
keyboard entries trimmed down from SDL list */
enum en_input {
    INPUT_A              = SDL_SCANCODE_A,
    INPUT_B              = SDL_SCANCODE_B,
    INPUT_C              = SDL_SCANCODE_C,
    INPUT_D              = SDL_SCANCODE_D,
    INPUT_E              = SDL_SCANCODE_E,
    INPUT_F              = SDL_SCANCODE_F,
    INPUT_G              = SDL_SCANCODE_G,
    INPUT_H              = SDL_SCANCODE_H,
    INPUT_I              = SDL_SCANCODE_I,
    INPUT_J              = SDL_SCANCODE_J,
    INPUT_K              = SDL_SCANCODE_K,
    INPUT_L              = SDL_SCANCODE_L,
    INPUT_M              = SDL_SCANCODE_M,
    INPUT_N              = SDL_SCANCODE_N,
    INPUT_O              = SDL_SCANCODE_O,
    INPUT_P              = SDL_SCANCODE_P,
    INPUT_Q              = SDL_SCANCODE_Q,
    INPUT_R              = SDL_SCANCODE_R,
    INPUT_S              = SDL_SCANCODE_S,
    INPUT_T              = SDL_SCANCODE_T,
    INPUT_U              = SDL_SCANCODE_U,
    INPUT_V              = SDL_SCANCODE_V,
    INPUT_W              = SDL_SCANCODE_W,
    INPUT_X              = SDL_SCANCODE_X,
    INPUT_Y              = SDL_SCANCODE_Y,
    INPUT_Z              = SDL_SCANCODE_Z,
    INPUT_1              = SDL_SCANCODE_1,
    INPUT_2              = SDL_SCANCODE_2,
    INPUT_3              = SDL_SCANCODE_3,
    INPUT_4              = SDL_SCANCODE_4,
    INPUT_5              = SDL_SCANCODE_5,
    INPUT_6              = SDL_SCANCODE_6,
    INPUT_7              = SDL_SCANCODE_7,
    INPUT_8              = SDL_SCANCODE_8,
    INPUT_9              = SDL_SCANCODE_9,
    INPUT_0              = SDL_SCANCODE_0,
    INPUT_RETURN         = SDL_SCANCODE_RETURN,
    INPUT_ESCAPE         = SDL_SCANCODE_ESCAPE,
    INPUT_BACKSPACE      = SDL_SCANCODE_BACKSPACE,
    INPUT_TAB            = SDL_SCANCODE_TAB,
    INPUT_SPACE          = SDL_SCANCODE_SPACE,
    INPUT_MINUS          = SDL_SCANCODE_MINUS,
    INPUT_EQUALS         = SDL_SCANCODE_EQUALS,
    INPUT_LEFTBRACKET    = SDL_SCANCODE_LEFTBRACKET,
    INPUT_RIGHTBRACKET   = SDL_SCANCODE_RIGHTBRACKET,
    INPUT_BACKSLASH      = SDL_SCANCODE_BACKSLASH,
    INPUT_NONUSHASH      = SDL_SCANCODE_NONUSHASH,
    INPUT_SEMICOLON      = SDL_SCANCODE_SEMICOLON,
    INPUT_APOSTROPHE     = SDL_SCANCODE_APOSTROPHE,
    INPUT_GRAVE          = SDL_SCANCODE_GRAVE,
    INPUT_COMMA          = SDL_SCANCODE_COMMA,
    INPUT_PERIOD         = SDL_SCANCODE_PERIOD,
    INPUT_SLASH          = SDL_SCANCODE_SLASH,
    INPUT_CAPSLOCK       = SDL_SCANCODE_CAPSLOCK,
    INPUT_F1             = SDL_SCANCODE_F1,
    INPUT_F2             = SDL_SCANCODE_F2,
    INPUT_F3             = SDL_SCANCODE_F3,
    INPUT_F4             = SDL_SCANCODE_F4,
    INPUT_F5             = SDL_SCANCODE_F5,
    INPUT_F6             = SDL_SCANCODE_F6,
    INPUT_F7             = SDL_SCANCODE_F7,
    INPUT_F8             = SDL_SCANCODE_F8,
    INPUT_F9             = SDL_SCANCODE_F9,
    INPUT_F10            = SDL_SCANCODE_F10,
    INPUT_F11            = SDL_SCANCODE_F11,
    INPUT_F12            = SDL_SCANCODE_F12,
    INPUT_PRINTSCREEN    = SDL_SCANCODE_PRINTSCREEN,
    INPUT_SCROLLLOCK     = SDL_SCANCODE_SCROLLLOCK,
    INPUT_PAUSE          = SDL_SCANCODE_PAUSE,
    INPUT_INSERT         = SDL_SCANCODE_INSERT,
    INPUT_HOME           = SDL_SCANCODE_HOME,
    INPUT_PAGEUP         = SDL_SCANCODE_PAGEUP,
    INPUT_DELETE         = SDL_SCANCODE_DELETE,
    INPUT_END            = SDL_SCANCODE_END,
    INPUT_PAGEDOWN       = SDL_SCANCODE_PAGEDOWN,
    INPUT_RIGHT          = SDL_SCANCODE_RIGHT,
    INPUT_LEFT           = SDL_SCANCODE_LEFT,
    INPUT_DOWN           = SDL_SCANCODE_DOWN,
    INPUT_UP             = SDL_SCANCODE_UP,
    INPUT_NUMLOCKCLEAR   = SDL_SCANCODE_NUMLOCKCLEAR,
    INPUT_KP_DIVIDE      = SDL_SCANCODE_KP_DIVIDE,
    INPUT_KP_MULTIPLY    = SDL_SCANCODE_KP_MULTIPLY,
    INPUT_KP_MINUS       = SDL_SCANCODE_KP_MINUS,
    INPUT_KP_PLUS        = SDL_SCANCODE_KP_PLUS,
    INPUT_KP_ENTER       = SDL_SCANCODE_KP_ENTER,
    INPUT_KP_1           = SDL_SCANCODE_KP_1,
    INPUT_KP_2           = SDL_SCANCODE_KP_2,
    INPUT_KP_3           = SDL_SCANCODE_KP_3,
    INPUT_KP_4           = SDL_SCANCODE_KP_4,
    INPUT_KP_5           = SDL_SCANCODE_KP_5,
    INPUT_KP_6           = SDL_SCANCODE_KP_6,
    INPUT_KP_7           = SDL_SCANCODE_KP_7,
    INPUT_KP_8           = SDL_SCANCODE_KP_8,
    INPUT_KP_9           = SDL_SCANCODE_KP_9,
    INPUT_KP_0           = SDL_SCANCODE_KP_0,
    INPUT_KP_PERIOD      = SDL_SCANCODE_KP_PERIOD,
    INPUT_NONUSBACKSLASH = SDL_SCANCODE_NONUSBACKSLASH,
    INPUT_LCTRL          = SDL_SCANCODE_LCTRL,
    INPUT_LSHIFT         = SDL_SCANCODE_LSHIFT,
    INPUT_LALT           = SDL_SCANCODE_LALT,
    INPUT_LGUI           = SDL_SCANCODE_LGUI,
    INPUT_RCTRL          = SDL_SCANCODE_RCTRL,
    INPUT_RSHIFT         = SDL_SCANCODE_RSHIFT,
    INPUT_RALT           = SDL_SCANCODE_RALT,
    INPUT_RGUI           = SDL_SCANCODE_RGUI,

    /* end of keyboard */
    INPUT_EOK            = SDL_NUM_SCANCODES,

    INPUT_MOUSE_LEFT     = INPUT_EOK + SDL_BUTTON_LEFT,
    INPUT_MOUSE_MIDDLE   = INPUT_EOK + SDL_BUTTON_MIDDLE,
    INPUT_MOUSE_RIGHT    = INPUT_EOK + SDL_BUTTON_RIGHT,
    INPUT_MOUSE_THUMB1   = INPUT_EOK + SDL_BUTTON_X1,
    INPUT_MOUSE_THUMB2   = INPUT_EOK + SDL_BUTTON_X2,
    INPUT_MOUSE_WHEELDOWN,
    INPUT_MOUSE_WHEELUP,

    /* end of mouse */
    INPUT_EOM,

    INPUT_NUM_INPUTS,
};

typedef struct input_lookup_t { const char* name; en_input action; } input_lookup_t;
static const input_lookup_t input_lookup_table[] = {
    { "INPUT_A",               INPUT_A },
    { "INPUT_B",               INPUT_B },
    { "INPUT_C",               INPUT_C },
    { "INPUT_D",               INPUT_D },
    { "INPUT_E",               INPUT_E },
    { "INPUT_F",               INPUT_F },
    { "INPUT_G",               INPUT_G },
    { "INPUT_H",               INPUT_H },
    { "INPUT_I",               INPUT_I },
    { "INPUT_J",               INPUT_J },
    { "INPUT_K",               INPUT_K },
    { "INPUT_L",               INPUT_L },
    { "INPUT_M",               INPUT_M },
    { "INPUT_N",               INPUT_N },
    { "INPUT_O",               INPUT_O },
    { "INPUT_P",               INPUT_P },
    { "INPUT_Q",               INPUT_Q },
    { "INPUT_R",               INPUT_R },
    { "INPUT_S",               INPUT_S },
    { "INPUT_T",               INPUT_T },
    { "INPUT_U",               INPUT_U },
    { "INPUT_V",               INPUT_V },
    { "INPUT_W",               INPUT_W },
    { "INPUT_X",               INPUT_X },
    { "INPUT_Y",               INPUT_Y },
    { "INPUT_Z",               INPUT_Z },
    { "INPUT_1",               INPUT_1 },
    { "INPUT_2",               INPUT_2 },
    { "INPUT_3",               INPUT_3 },
    { "INPUT_4",               INPUT_4 },
    { "INPUT_5",               INPUT_5 },
    { "INPUT_6",               INPUT_6 },
    { "INPUT_7",               INPUT_7 },
    { "INPUT_8",               INPUT_8 },
    { "INPUT_9",               INPUT_9 },
    { "INPUT_0",               INPUT_0 },
    { "INPUT_RETURN",          INPUT_RETURN },
    { "INPUT_ESCAPE",          INPUT_ESCAPE },
    { "INPUT_BACKSPACE",       INPUT_BACKSPACE },
    { "INPUT_TAB",             INPUT_TAB },
    { "INPUT_SPACE",           INPUT_SPACE },
    { "INPUT_MINUS",           INPUT_MINUS },
    { "INPUT_EQUALS",          INPUT_EQUALS },
    { "INPUT_LEFTBRACKET",     INPUT_LEFTBRACKET },
    { "INPUT_RIGHTBRACKET",    INPUT_RIGHTBRACKET },
    { "INPUT_BACKSLASH",       INPUT_BACKSLASH },
    { "INPUT_NONUSHASH",       INPUT_NONUSHASH },
    { "INPUT_SEMICOLON",       INPUT_SEMICOLON },
    { "INPUT_APOSTROPHE",      INPUT_APOSTROPHE },
    { "INPUT_GRAVE",           INPUT_GRAVE },
    { "INPUT_COMMA",           INPUT_COMMA },
    { "INPUT_PERIOD",          INPUT_PERIOD },
    { "INPUT_SLASH",           INPUT_SLASH },
    { "INPUT_CAPSLOCK",        INPUT_CAPSLOCK },
    { "INPUT_F1",              INPUT_F1 },
    { "INPUT_F2",              INPUT_F2 },
    { "INPUT_F3",              INPUT_F3 },
    { "INPUT_F4",              INPUT_F4 },
    { "INPUT_F5",              INPUT_F5 },
    { "INPUT_F6",              INPUT_F6 },
    { "INPUT_F7",              INPUT_F7 },
    { "INPUT_F8",              INPUT_F8 },
    { "INPUT_F9",              INPUT_F9 },
    { "INPUT_F10",             INPUT_F10 },
    { "INPUT_F11",             INPUT_F11 },
    { "INPUT_F12",             INPUT_F12 },
    { "INPUT_PRINTSCREEN",     INPUT_PRINTSCREEN },
    { "INPUT_SCROLLLOCK",      INPUT_SCROLLLOCK },
    { "INPUT_PAUSE",           INPUT_PAUSE },
    { "INPUT_INSERT",          INPUT_INSERT },
    { "INPUT_HOME",            INPUT_HOME },
    { "INPUT_PAGEUP",          INPUT_PAGEUP },
    { "INPUT_DELETE",          INPUT_DELETE },
    { "INPUT_END",             INPUT_END },
    { "INPUT_PAGEDOWN",        INPUT_PAGEDOWN },
    { "INPUT_RIGHT",           INPUT_RIGHT },
    { "INPUT_LEFT",            INPUT_LEFT },
    { "INPUT_DOWN",            INPUT_DOWN },
    { "INPUT_UP",              INPUT_UP },
    { "INPUT_NUMLOCKCLEAR",    INPUT_NUMLOCKCLEAR },
    { "INPUT_KP_DIVIDE",       INPUT_KP_DIVIDE },
    { "INPUT_KP_MULTIPLY",     INPUT_KP_MULTIPLY },
    { "INPUT_KP_MINUS",        INPUT_KP_MINUS },
    { "INPUT_KP_PLUS",         INPUT_KP_PLUS },
    { "INPUT_KP_ENTER",        INPUT_KP_ENTER },
    { "INPUT_KP_1",            INPUT_KP_1 },
    { "INPUT_KP_2",            INPUT_KP_2 },
    { "INPUT_KP_3",            INPUT_KP_3 },
    { "INPUT_KP_4",            INPUT_KP_4 },
    { "INPUT_KP_5",            INPUT_KP_5 },
    { "INPUT_KP_6",            INPUT_KP_6 },
    { "INPUT_KP_7",            INPUT_KP_7 },
    { "INPUT_KP_8",            INPUT_KP_8 },
    { "INPUT_KP_9",            INPUT_KP_9 },
    { "INPUT_KP_0",            INPUT_KP_0 },
    { "INPUT_KP_PERIOD",       INPUT_KP_PERIOD },
    { "INPUT_NONUSBACKSLASH",  INPUT_NONUSBACKSLASH },
    { "INPUT_LCTRL",           INPUT_LCTRL },
    { "INPUT_LSHIFT",          INPUT_LSHIFT },
    { "INPUT_LALT",            INPUT_LALT },
    { "INPUT_LGUI",            INPUT_LGUI },
    { "INPUT_RCTRL",           INPUT_RCTRL },
    { "INPUT_RSHIFT",          INPUT_RSHIFT },
    { "INPUT_RALT",            INPUT_RALT },
    { "INPUT_RGUI",            INPUT_RGUI },
    { "INPUT_MOUSE_LEFT",      INPUT_MOUSE_LEFT },
    { "INPUT_MOUSE_MIDDLE",    INPUT_MOUSE_MIDDLE },
    { "INPUT_MOUSE_RIGHT",     INPUT_MOUSE_RIGHT },
    { "INPUT_MOUSE_THUMB1",    INPUT_MOUSE_THUMB1 },
    { "INPUT_MOUSE_THUMB2",    INPUT_MOUSE_THUMB2 },
    { "INPUT_MOUSE_WHEELDOWN", INPUT_MOUSE_WHEELDOWN },
    { "INPUT_MOUSE_WHEELUP",   INPUT_MOUSE_WHEELUP },
};

en_input lookup_input(const char *name) {
    for (const input_lookup_t *input = input_lookup_table; input->name != NULL; ++input) {
        if (strcmp(input->name, name) == 0) {
            return input->action;
        }
    }
    return (en_input)-1;
}

enum slot_cycle_direction {
    cycle_next,
    cycle_prev,
};

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


struct light_field {
    GLuint texobj;
    unsigned char data[128*128*128];

    light_field() : texobj(0) {
        glGenTextures(1, &texobj);
        glBindTexture(GL_TEXTURE_3D, texobj);
        glTexStorage3D(GL_TEXTURE_3D, 1, GL_R8, 128, 128, 128);
    }

    void bind(int texunit)
    {
        glActiveTexture(GL_TEXTURE0 + texunit);
        glBindTexture(GL_TEXTURE_3D, texobj);
    }

    void upload()
    {
        /* TODO: toroidal addressing + partial updates, to make these uploads VERY small */
        /* TODO: experiment with buffer texture rather than 3D, so we can have the light field
         * persistently mapped in our address space */

        /* DSA would be nice -- for now, we'll just disturb the tex0 binding */
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_3D, texobj);

        glTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 0,
                        128, 128, 128,
                        GL_RED,
                        GL_UNSIGNED_BYTE,
                        data);
    }
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
/* one larger than it needs to be due to SDL starting numbering at 1 */
unsigned int mouse_buttons[INPUT_EOM - INPUT_EOK];
hw_mesh *scaffold_hw;
hw_mesh *surfs_hw[6];
text_renderer *text;
light_field *light;
entity *use_entity = 0;


glm::mat4
mat_position(float x, float y, float z)
{
    return glm::translate(glm::mat4(1), glm::vec3(x, y, z));
}

glm::mat4
mat_block_face(int x, int y, int z, int face)
{
    switch (face) {
    case surface_zp:
        return glm::rotate(glm::translate(glm::mat4(1), glm::vec3(x, y + 1, z + 1)), (float)M_PI, glm::vec3(1.0f, 0.0f, 0.0f));
    case surface_zm:
        return glm::translate(glm::mat4(1), glm::vec3(x, y, z));
    case surface_xp:
        return glm::rotate(glm::translate(glm::mat4(1), glm::vec3(x + 1, y, z)), -(float)M_PI/2, glm::vec3(0.0f, 1.0f, 0.0f));
    case surface_xm:
        return glm::rotate(glm::translate(glm::mat4(1), glm::vec3(x, y, z + 1)), (float)M_PI/2, glm::vec3(0.0f, 1.0f, 0.0f));
    case surface_yp:
        return glm::rotate(glm::translate(glm::mat4(1), glm::vec3(x, y + 1, z)), (float)M_PI/2, glm::vec3(1.0f, 0.0f, 0.0f));
    case surface_ym:
        return glm::rotate(glm::translate(glm::mat4(1), glm::vec3(x, y, z + 1)), -(float)M_PI/2, glm::vec3(1.0f, 0.0f, 0.0f));

    default:
        return glm::mat4(1);    /* unreachable */
    }
}


struct entity_type
{
    sw_mesh *sw;
    hw_mesh *hw;
    char const *name;
    btTriangleMesh *phys_mesh;
    btCollisionShape *phys_shape;
};


entity_type entity_types[3];


struct entity
{
    /* TODO: replace this completely, it's silly. */
    int x, y, z;
    entity_type *type;
    btRigidBody *phys_body;
    int face;
    glm::mat4 mat;

    entity(int x, int y, int z, entity_type *type, int face)
        : x(x), y(y), z(z), type(type), phys_body(0), face(face)
    {
        mat = mat_block_face(x, y, z, face);

        build_static_physics_rb_mat(&mat, type->phys_shape, &phys_body);

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
set_light_level(int x, int y, int z, int level)
{
    if (x < 0 || x >= 128) return;
    if (y < 0 || y >= 128) return;
    if (z < 0 || z >= 128) return;

    int p = x + y * 128 + z * 128 * 128;
    if (level < 0) level = 0;
    if (level > 255) level = 255;
    light->data[p] = level;
}


unsigned char
get_light_level(int x, int y, int z)
{
    if (x < 0 || x >= 128) return 0;
    if (y < 0 || y >= 128) return 0;
    if (z < 0 || z >= 128) return 0;

    return light->data[x + y*128 + z*128*128];
}


const int light_atten = 50;
/* as far as we can ever light from a light source */
const int max_light_prop = (255 + light_atten - 1) / light_atten;

bool need_lightfield_update = false;
glm::ivec3 lightfield_update_mins;
glm::ivec3 lightfield_update_maxs;


void
mark_lightfield_update(int x, int y, int z)
{
    glm::ivec3 center = glm::ivec3(x, y, z);
    glm::ivec3 half_extent = glm::ivec3(max_light_prop, max_light_prop, max_light_prop);
    if (need_lightfield_update) {
        lightfield_update_mins = center - half_extent;
        lightfield_update_maxs = center + half_extent;
    }
    else {
        lightfield_update_mins = glm::min(lightfield_update_mins,
                center - half_extent);
        lightfield_update_maxs = glm::max(lightfield_update_maxs,
                center + half_extent);
        need_lightfield_update = true;
    }
}


void
update_lightfield()
{
    if (!need_lightfield_update) {
        /* nothing to do here */
        return;
    }

    /* TODO: opt for case where we're JUST adding light -- no need to clear & rebuild */
    /* This is general enough to cope with occluders & lights being added and removed. */

    /* 1. remove all existing light in the box */
    for (int k = lightfield_update_mins.z; k <= lightfield_update_maxs.z; k++)
        for (int j = lightfield_update_mins.y; j <= lightfield_update_maxs.y; j++)
            for (int i = lightfield_update_mins.x; i <= lightfield_update_maxs.x; i++)
                set_light_level(i, j, k, 0);

    /* 2. inject sources. the box is guaranteed to be big enough for max propagation
     * for all sources we'll add here. */
    /* TODO: inject only required sources. */
    for (int k = ship->min_z; k <= ship->max_z; k++) {
        for (int j = ship->min_y; j <= ship->max_y; j++) {
            for (int i = ship->min_x; i <= ship->max_x; i++) {
                chunk *ch = ship->get_chunk(i, j, k);
                if (ch) {
                    for (auto e : ch->entities) {
                        /* TODO: only some entities should do this. */
                        if (e->type == &entity_types[2]) {
                            set_light_level(e->x, e->y, e->z, 255);
                        }
                    }
                }
            }
        }
    }

    /* 3. propagate max_light_prop times. this is guaranteed to be enough to cover
     * the sources' area of influence. */
    for (int pass = 0; pass < max_light_prop; pass++) {
        for (int k = lightfield_update_mins.z; k <= lightfield_update_maxs.z; k++) {
            for (int j = lightfield_update_mins.y; j <= lightfield_update_maxs.y; j++) {
                for (int i = lightfield_update_mins.x; i <= lightfield_update_maxs.x; i++) {
                    int level = get_light_level(i, j, k);

                    block *b = ship->get_block(i, j, k);
                    if (!b)
                        continue;

                    if (!b->surfs[surface_xm])
                        level = std::max(level, get_light_level(i - 1, j, k) - light_atten);
                    if (!b->surfs[surface_xp])
                        level = std::max(level, get_light_level(i + 1, j, k) - light_atten);

                    if (!b->surfs[surface_ym])
                        level = std::max(level, get_light_level(i, j - 1, k) - light_atten);
                    if (!b->surfs[surface_yp])
                        level = std::max(level, get_light_level(i, j + 1, k) - light_atten);

                    if (!b->surfs[surface_zm])
                        level = std::max(level, get_light_level(i, j, k - 1) - light_atten);
                    if (!b->surfs[surface_zp])
                        level = std::max(level, get_light_level(i, j, k + 1) - light_atten);

                    set_light_level(i, j, k, level);
                }
            }
        }
    }

    /* All done. */
    light->upload();
    need_lightfield_update = false;
}


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
    glDebugMessageCallback(reinterpret_cast<GLDEBUGPROC>(&gl_debug_callback), NULL);

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

    entity_types[2].sw = load_mesh("mesh/panel_4x4.obj");
    set_mesh_material(entity_types[2].sw, 8);
    entity_types[2].hw = upload_mesh(entity_types[2].sw);
    entity_types[2].name = "Light (4x4)";
    build_static_physics_mesh(entity_types[2].sw, &entity_types[2].phys_mesh, &entity_types[2].phys_shape);

    simple_shader = load_shader("shaders/simple.vert", "shaders/simple.frag");
    add_overlay_shader = load_shader("shaders/add_overlay.vert", "shaders/unlit.frag");
    remove_overlay_shader = load_shader("shaders/remove_overlay.vert", "shaders/unlit.frag");
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
    world_textures->load(8, "textures/light.png");

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

    configureBindings();

    player.angle = 0;
    player.elev = 0;
    player.pos = glm::vec3(3,2,2);
    player.selected_slot = 1;
    player.ui_dirty = true;
    player.disable_gravity = false;

    phy = new physics(&player);

    glEnable(GL_CULL_FACE);
    glFrontFace(GL_CW);

    text = new text_renderer("fonts/pixelmix.ttf", 16);

    printf("World vertex size: %zu bytes\n", sizeof(vertex));

    light = new light_field();
    light->bind(1);

    /* put some crap in the lightfield */
    memset(light->data, 0, sizeof(light->data));
    light->upload();
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
            mark_lightfield_update(rc->px, rc->py, rc->pz);
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


void
remove_ents_from_surface(int x, int y, int z, int face)
{
    chunk *ch = ship->get_chunk_containing(x, y, z);
    for (auto it = ch->entities.begin(); it != ch->entities.end(); /* */) {
        entity *e = *it;
        if (e->x == x && e->y == y && e->z == z && e->face == face) {
            delete e;
            it = ch->entities.erase(it);
        }
        else {
            it++;
        }
    }

    block *bl = ship->get_block(x, y, z);
    assert(bl);

    bl->surf_space[face] = 0;   /* we've popped *everything* off, it must be empty now */

    if (face == surface_zm) {

        if (bl->type == block_entity)
            bl->type = block_empty;

        ch->render_chunk.valid = false;
    }
}


struct remove_block_tool : public tool
{
    virtual void use(raycast_info *rc)
    {
        if (rc->inside) return; /* n/a */

        block *bl = rc->block;

        /* if there was a block entity here, find and remove it. block
         * ents are "attached" to the zm surface */
        if (bl->type == block_entity) {
            remove_ents_from_surface(rc->x, rc->y, rc->z, surface_zm);

            for (int face = 0; face < face_count; face++) {
                bl->surf_space[face] = 0;   /* we've just thrown away the block ent */
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

                    /* pop any dependent ents */
                    remove_ents_from_surface(rc->x, rc->y, rc->z, index);
                    remove_ents_from_surface(rx, ry, rz, index ^ 1);

                    mark_lightfield_update(rx, ry, rz);
                }
            }
        }

        /* dirty the chunk */
        ship->get_chunk_containing(rc->x, rc->y, rc->z)->render_chunk.valid = false;
        mark_lightfield_update(rc->x, rc->y, rc->z);
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

            mark_lightfield_update(rc->x, rc->y, rc->z);
            mark_lightfield_update(rc->px, rc->py, rc->pz);

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

            /* remove any ents using the surface */
            remove_ents_from_surface(rc->px, rc->py, rc->pz, index ^ 1);
            remove_ents_from_surface(rc->x, rc->y, rc->z, index);

            mark_lightfield_update(rc->x, rc->y, rc->z);
            mark_lightfield_update(rc->px, rc->py, rc->pz);
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

        if (bl) {
            for (int face = 0; face < face_count; face++)
                if (bl->surf_space[face])
                    return;
        }

        /* can only build on the side of an existing scaffold */
        if (bl && rc->block->type == block_support) {
            bl->type = block_entity;
            /* dirty the chunk -- TODO: do we really have to do this when changing a cell from
             * empty -> entity? */
            ship->get_chunk_containing(rc->px, rc->py, rc->pz)->render_chunk.valid = false;
        }

        /* TODO: flesh this out a bit */
        ship->get_chunk_containing(rc->px, rc->py, rc->pz)->entities.push_back(
            new entity(rc->px, rc->py, rc->pz, type, surface_zm)
            );

        if (bl) {
            /* consume ALL the space on the surfaces */
            for (int face = 0; face < face_count; face++)
                bl->surf_space[face] = ~0;
        }
    }

    virtual void preview(raycast_info *rc)
    {
        if (rc->inside) return; /* n/a */

        block *bl = ship->get_block(rc->px, rc->py, rc->pz);

        if (bl) {
            for (int face = 0; face < face_count; face++)
                if (bl->surf_space[face])
                    return;
        }

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


struct add_surface_entity_tool : public tool
{
    entity_type *type;

    add_surface_entity_tool(entity_type *type) : type(type) {}

    virtual void use(raycast_info *rc)
    {
        block *bl = rc->block;

        int index = normal_to_surface_index(rc);

        if (bl->surfs[index] == surface_none)
            return;

        block *other_side = ship->get_block(rc->px, rc->py, rc->pz);
        unsigned short required_space = ~0; /* TODO: make this a prop of the type + subblock placement */

        if (other_side->surf_space[index ^ 1] & required_space) {
            /* no room on the surface */
            return;
        }

        chunk *ch = ship->get_chunk_containing(rc->px, rc->py, rc->pz);
        /* the chunk we're placing into is guaranteed to exist, because there's
         * a surface facing into it */
        assert(ch);
        ch->entities.push_back(
            new entity(rc->px, rc->py, rc->pz, type, index ^ 1)
            );

        /* take the space. */
        other_side->surf_space[index ^ 1] |= required_space;

        /* mark lighting for rebuild around this point */
        mark_lightfield_update(rc->px, rc->py, rc->pz);
    }

    virtual void preview(raycast_info *rc)
    {
        block *bl = rc->block;

        int index = normal_to_surface_index(rc);

        if (bl->surfs[index] == surface_none)
            return;

        block *other_side = ship->get_block(rc->px, rc->py, rc->pz);

        unsigned short required_space = ~0; /* TODO: make this a prop of the type + subblock placement */

        if (other_side->surf_space[index ^ 1] & required_space) {
            /* no room on the surface */
            return;
        }

        per_object->val.world_matrix = mat_block_face(rc->px, rc->py, rc->pz, index ^ 1);
        per_object->upload();

        draw_mesh(type->hw);

        /* draw a surface overlay here too */
        /* TODO: sub-block placement granularity -- will need a different overlay */
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

    virtual void get_description(char *str)
    {
        sprintf(str, "Place %s on surface", type->name);
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
    new add_surface_entity_tool(&entity_types[1]),
    new add_surface_entity_tool(&entity_types[2]),
};


void
add_text_with_outline(char const *s, float x, float y)
{
    text->add(s, x - 2, y, 0, 0, 0);
    text->add(s, x + 2, y, 0, 0, 0);
    text->add(s, x, y - 2, 0, 0, 0);
    text->add(s, x, y + 2, 0, 0, 0);
    text->add(s, x, y, 1, 1, 1);
}


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

    add_text_with_outline(".", 0, 0);

    sprintf(buf2, "Left mouse button: %s", buf);
    text->measure(buf2, &w, &h);
    add_text_with_outline(buf2, -w/2, -400);

    /* Gravity state (temp) */
    w = 0; h = 0;
    sprintf(buf, "Gravity: %s (G to toggle)", player.disable_gravity ? "OFF" : "ON");
    text->measure(buf, &w, &h);
    add_text_with_outline(buf, -w/2, -430);

    /* Use key affordance */
    if (use_entity) {
        sprintf(buf2, "(E) Use the %s", use_entity->type->name);
        w = 0; h = 0;
        text->measure(buf2, &w, &h);
        add_text_with_outline(buf2, -w/2, -200);
    }

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

    auto vfov = hfov * (float)wnd.height / wnd.width;

    glm::mat4 proj = glm::perspective(vfov, (float)wnd.width / wnd.height, 0.01f, 1000.0f);
    glm::mat4 view = glm::lookAt(player.eye, player.eye + player.dir, glm::vec3(0, 0, 1));
    per_camera->val.view_proj_matrix = proj * view;
    per_camera->upload();

    tool *t = tools[player.selected_slot];

    /* no menu. exit for now */
    if (player.menu_requested) {
        exit_requested = true;
        player.menu_requested = false;
    }

    /* both tool use and overlays need the raycast itself */
    raycast_info rc;
    ship->raycast(player.eye.x, player.eye.y, player.eye.z, player.dir.x, player.dir.y, player.dir.z, &rc);

    /* tool use */
    if (player.use_tool && rc.hit && t) {
        t->use(&rc);
    }

    /* interact with ents */
    entity *hit_ent = phys_raycast(player.eye.x, player.eye.y, player.eye.z,
                                   player.dir.x, player.dir.y, player.dir.z,
                                   2 /* dist */, phy->ghostObj, phy->dynamicsWorld);

    if (hit_ent != use_entity) {
        use_entity = hit_ent;
        player.ui_dirty = true;
    }

    if (player.use && hit_ent) {
        hit_ent->use();
    }

    /* rebuild lighting if needed */
    update_lightfield();

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
                    for (auto e : ch->entities) {
                        /* TODO: batch these matrix uploads too! */
                        per_object->val.world_matrix = e->mat;
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
cycle_slot(slot_cycle_direction direction)
{
    auto num_tools = sizeof(tools) / sizeof(tools[0]);
    auto cur_slot = player.selected_slot;
    if (direction == cycle_next) {
        cur_slot++;
        if (cur_slot >= num_tools) {
            cur_slot = 0;
        }
    }
    else if (direction == cycle_prev) {
        cur_slot--;
        if (cur_slot < 0) {
            cur_slot = num_tools - 1;
        }
    }

    player.selected_slot = cur_slot;
    player.ui_dirty = true;
}

struct binding {
    std::vector<en_input> keyboard_inputs;
    std::vector<en_input> mouse_inputs;

    binding(en_input keyboard = INPUT_EOK, en_input mouse = INPUT_EOM)
    {
        bind(keyboard, mouse);
    }

    void bind(en_input keyboard = INPUT_EOK, en_input mouse = INPUT_EOM)
    {
        if (keyboard != INPUT_EOK) {
            keyboard_inputs.push_back(keyboard);
        }
        if (mouse != INPUT_EOM) {
            mouse_inputs.push_back(mouse);
        }
    }
};

struct action {
    input_action input;
    binding binds;

    bool active = false;        /* is action currently active */
    bool just_active = false;   /* did action go active this frame */
    bool just_inactive = false; /* did action go inactive this frame */
    Uint32 last_active = 0;     /* time of last activation */
    Uint32 current_active = 0;  /* duration of current activation */

    action()
    {
    }

    action(input_action a, binding b) :
        input(a), binds(b)
    {
    }

    void bind(en_input keyboard = INPUT_EOK, en_input mouse = INPUT_EOM)
    {
        binds.bind(keyboard, mouse);
    }
};

static std::unordered_map<input_action, action, std::hash<int>> en_actions;

void
configureBindings()
{
    const char *keysPath = "configs/keys.cfg";
    config_t cfg;
    config_setting_t *binds;

    config_init(&cfg);

    if (!config_read_file(&cfg, keysPath))
    {
        printf("%s:%d - %s reading %s\n", config_error_file(&cfg),
            config_error_line(&cfg), config_error_text(&cfg), keysPath);
        config_destroy(&cfg);
    }

    binds = config_lookup(&cfg, "binds");

    if (binds != NULL) {
        /* http://www.hyperrealm.com/libconfig/libconfig_manual.html
         * states
         *  > int config_setting_length (const config_setting_t * setting)
         *  > This function returns the number of settings in a group,
         *  > or the number of elements in a list or array.
         *  > For other types of settings, it returns 0.
         *
         * so this can only ever be positive, despite the return type being int
         */
        unsigned int count = config_setting_length(binds);

        for (unsigned int i = 0; i < count; ++i) {
            config_setting_t *bind, *inputs;
            bind = config_setting_get_elem(binds, i);

            const char *action_name;

            config_setting_lookup_string(bind, "action", &action_name);

            inputs = config_setting_lookup(bind, "inputs");
            /* config_setting_length will only ever be 0 or positive according
             * to the docs
             * */
            unsigned int  inputs_count = config_setting_length(inputs);
            const char **input_names = (const char**)malloc(sizeof(char*) * inputs_count);

            for (unsigned int input_index = 0; input_index < inputs_count; ++input_index) {
                config_setting_t *input = config_setting_get_elem(inputs, input_index);
                input_names[input_index] = config_setting_get_string(input);
            }

            unsigned int input_index = 0;
            input_action i_action = lookup_input_action(action_name);
            en_input input = lookup_input(input_names[input_index]);
            en_actions[i_action] = action(i_action, binding(input));

            for (input_index = 1; input_index < inputs_count; ++input_index) {
                input = lookup_input(input_names[input_index]);
                en_actions[i_action].bind(input);
            }

            free(input_names);
        }
    }


    //en_actions[action_left]      = action(action_right,     binding(INPUT_A));
    //en_actions[action_right]     = action(action_right,     binding(INPUT_D));
    // en_actions[action_forward]   = action(action_forward,   binding(INPUT_W));
    // en_actions[action_back]      = action(action_back,      binding(INPUT_S));
    // en_actions[action_jump]      = action(action_jump,      binding(INPUT_SPACE));
    // en_actions[action_use]       = action(action_use,       binding(INPUT_E));
    // en_actions[action_menu]      = action(action_menu,      binding(INPUT_ESCAPE));
    // en_actions[action_reset]     = action(action_reset,     binding(INPUT_R));
    // en_actions[action_crouch]    = action(action_crouch,    binding(INPUT_LCTRL));
    // en_actions[action_gravity]   = action(action_gravity,   binding(INPUT_G));
    en_actions[action_use_tool]  = action(action_use_tool,  binding(INPUT_EOK, INPUT_MOUSE_LEFT));
    en_actions[action_tool_next] = action(action_tool_next, binding(INPUT_EOK, INPUT_MOUSE_WHEELUP));
    en_actions[action_tool_prev] = action(action_tool_prev, binding(INPUT_EOK, INPUT_MOUSE_WHEELDOWN));
    // en_actions[action_slot1]     = action(action_slot1,     binding(INPUT_1));
    // en_actions[action_slot2]     = action(action_slot2,     binding(INPUT_2));
    // en_actions[action_slot3]     = action(action_slot3,     binding(INPUT_3));
    // en_actions[action_slot4]     = action(action_slot4,     binding(INPUT_4));
    // en_actions[action_slot5]     = action(action_slot5,     binding(INPUT_5));
    // en_actions[action_slot6]     = action(action_slot6,     binding(INPUT_6));
    // en_actions[action_slot7]     = action(action_slot7,     binding(INPUT_7));
    // en_actions[action_slot8]     = action(action_slot8,     binding(INPUT_8));
    // en_actions[action_slot9]     = action(action_slot9,     binding(INPUT_9));

    /* extra assign */
    // en_actions[action_crouch].bind(INPUT_C);
}


void
set_inputs() {
    auto now = SDL_GetTicks();

    for (auto &actionPair : en_actions) {
        bool pressed = false;
        auto action = &actionPair.second;
        auto binds = &action->binds;

        for (auto &key : binds->keyboard_inputs) {
            if (keys[key]) {
                pressed = true;
            }
        }

        for (auto &mouse : binds->mouse_inputs) {
            if (mouse_buttons[EN_BUTTON(mouse)]) {
                pressed |= true;
            }
        }

        if (action->active) {
            /* still pressed */
            if (pressed) {
                /* increase duration */
                action->current_active = now - action->last_active;

                /* ensure state integrity */
                action->just_active = false;
                action->just_inactive = false;
            }
            /* just released */
            else {
                action->active = false;

                action->current_active = 0;

                action->just_active = false;
                action->just_inactive = true;
            }
        }
        /* not currently pressed */
        else {
            /* just pressed */
            if (pressed) {
                action->active = true;

                action->just_active = true;
                action->just_inactive = false;
                action->last_active = now;
                action->current_active = 0;
            }
            /* still not pressed */
            else {
                action->active = false;
                action->just_inactive = false;
            }
        }
    }
}

void
handle_input()
{
    set_inputs();

    if (en_actions[action_menu].active) player.menu_requested = true;

    /* movement */
    auto moveX      = en_actions[action_right].active - en_actions[action_left].active;
    auto moveY      = en_actions[action_forward].active - en_actions[action_back].active;

    /* crouch */
    auto crouch     = en_actions[action_crouch].active;
    auto crouch_end = en_actions[action_crouch].just_inactive;

    /* momentary */
    auto jump       = en_actions[action_jump].just_active;
    auto reset      = en_actions[action_reset].just_active;
    auto use        = en_actions[action_use].just_active;
    auto slot1      = en_actions[action_slot1].just_active;
    auto slot2      = en_actions[action_slot2].just_active;
    auto slot3      = en_actions[action_slot3].just_active;
    auto slot4      = en_actions[action_slot4].just_active;
    auto slot5      = en_actions[action_slot5].just_active;
    auto slot6      = en_actions[action_slot6].just_active;
    auto slot7      = en_actions[action_slot7].just_active;
    auto slot8      = en_actions[action_slot8].just_active;
    auto slot9      = en_actions[action_slot9].just_active;
    auto gravity    = en_actions[action_gravity].just_active;
    auto use_tool   = en_actions[action_use_tool].just_active;
    auto next_tool  = en_actions[action_tool_next].just_active;
    auto prev_tool  = en_actions[action_tool_prev].just_active;

    /* persistent */


    player.move.x = moveX;
    player.move.y = moveY;

    player.jump       = jump;
    player.crouch     = crouch;
    player.reset      = reset;
    player.crouch_end = crouch_end;
    player.use        = use;
    player.gravity    = gravity;
    player.use_tool   = use_tool;

    if (next_tool) {
        cycle_slot(cycle_next);
    }
    if (prev_tool) {
        cycle_slot(cycle_prev);
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

    /* limit to unit vector */
    float len = glm::length(player.move);
    if (len > 0.0f)
        player.move = player.move / len;
}


void
run()
{
    for (;;) {
        auto sdl_buttons = SDL_GetRelativeMouseState(NULL, NULL);
        mouse_buttons[EN_BUTTON(INPUT_MOUSE_LEFT)]      = sdl_buttons & SDL_BUTTON(EN_BUTTON(INPUT_MOUSE_LEFT));
        mouse_buttons[EN_BUTTON(INPUT_MOUSE_MIDDLE)]    = sdl_buttons & SDL_BUTTON(EN_BUTTON(INPUT_MOUSE_MIDDLE));
        mouse_buttons[EN_BUTTON(INPUT_MOUSE_RIGHT)]     = sdl_buttons & SDL_BUTTON(EN_BUTTON(INPUT_MOUSE_RIGHT));
        mouse_buttons[EN_BUTTON(INPUT_MOUSE_THUMB1)]    = sdl_buttons & SDL_BUTTON(EN_BUTTON(INPUT_MOUSE_THUMB1));
        mouse_buttons[EN_BUTTON(INPUT_MOUSE_THUMB2)]    = sdl_buttons & SDL_BUTTON(EN_BUTTON(INPUT_MOUSE_THUMB2));
        mouse_buttons[EN_BUTTON(INPUT_MOUSE_WHEELDOWN)] = false;
        mouse_buttons[EN_BUTTON(INPUT_MOUSE_WHEELUP)]   = false;

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

            case SDL_MOUSEWHEEL:
                if (e.wheel.y != 0) {
                    e.wheel.y > 0
                        ? mouse_buttons[EN_BUTTON(INPUT_MOUSE_WHEELUP)] = true
                        : mouse_buttons[EN_BUTTON(INPUT_MOUSE_WHEELDOWN)] = true;
                }
                break;
            }
        }

        /* SDL_PollEvent above has already pumped the input, so current key state is available */
        handle_input();

        /* physics tick */
        phy->tick();

        update();

        SDL_GL_SwapWindow(wnd.ptr);

        if (exit_requested) return;
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
