#include <epoxy/gl.h>

#include "../common.h"
#include "../ship_space.h"
#include "../mesh.h"
#include "../block.h"
#include "tools.h"


extern GLuint add_overlay_shader;
extern GLuint remove_overlay_shader;
extern GLuint simple_shader;

extern ship_space *ship;

extern hw_mesh *surfs_hw[6];

bool
add_surface_tool::can_use(block *bl, block *other, int index) {
    if (bl && bl->surfs[index] != surface_none) return false; /* already a surface here */
    return (bl && bl->type == block_frame) || (other && other->type == block_frame);
}

void
add_surface_tool::use(raycast_info *rc) {
    if (!rc->hit)
        return;

    block *bl = rc->block;

    int index = normal_to_surface_index(rc);
    block *other_side = ship->get_block(rc->p);

    if (can_use(bl, other_side, index)) {
        ship->set_surface(rc->bl, rc->p, (surface_index)index, st);

        mark_lightfield_update(rc->bl);
        mark_lightfield_update(rc->p);
    }
}


void
add_surface_tool::alt_use(raycast_info *rc) {}


void
add_surface_tool::long_use(raycast_info *rc) {}


void
add_surface_tool::cycle_mode() {
    switch (st) {
    case surface_wall:
        st = surface_grate;
        break;
    case surface_grate:
        st = surface_glass;
        break;
    case surface_glass:
        st = surface_wall;
        break;
    default:
        st = surface_wall;
        break;
    }
}


void
add_surface_tool::preview(raycast_info *rc, frame_data *frame) {
    if (!rc->hit)
        return;

    block *bl = ship->get_block(rc->bl);
    int index = normal_to_surface_index(rc);
    block *other_side = ship->get_block(rc->p);

    if (can_use(bl, other_side, index)) {
        auto mat = frame->alloc_aligned<glm::mat4>(1);
        *mat.ptr = mat_position(rc->bl);
        mat.bind(1, frame);

        glUseProgram(add_overlay_shader);
        glEnable(GL_POLYGON_OFFSET_FILL);
        draw_mesh(surfs_hw[index]);
        glDisable(GL_POLYGON_OFFSET_FILL);
        glUseProgram(simple_shader);
    }
}


void
add_surface_tool::get_description(char *str) {
    sprintf(str, "Place surface type %d\n", (int)st);
}
