#include <epoxy/gl.h>

#include "../common.h"
#include "../ship_space.h"
#include "../shader_params.h"
#include "../mesh.h"
#include "../block.h"
#include "tools.h"


extern GLuint add_overlay_shader;
extern GLuint remove_overlay_shader;
extern GLuint simple_shader;

extern void
mark_lightfield_update(glm::ivec3 p);

extern ship_space *ship;

extern hw_mesh *surfs_hw[6];

extern void
remove_ents_from_surface(glm::ivec3 p, int face);

bool
add_surface_tool::can_use(block *bl, block *other, int index) {
    if (bl && bl->surfs[index] != surface_none) return false; /* already a surface here */
    return (bl && bl->type == block_support) || (other && other->type == block_support);
}

void
add_surface_tool::use(raycast_info *rc) {
    if (!rc->hit)
        return;

    block *bl = rc->block;

    int index = normal_to_surface_index(rc);
    block *other_side = ship->get_block(rc->p);

    if (can_use(bl, other_side, index)) {

        if (!bl) {
            ship->ensure_block(rc->bl);
            bl = ship->get_block(rc->bl);
        }

        if (!other_side) {
            ship->ensure_block(rc->p);
            other_side = ship->get_block(rc->p);
        }

        bl->surfs[index] = this->st;
        ship->get_chunk_containing(rc->bl)->render_chunk.valid = false;

        other_side->surfs[index ^ 1] = this->st;
        ship->get_chunk_containing(rc->p)->render_chunk.valid = false;

        mark_lightfield_update(rc->bl);
        mark_lightfield_update(rc->p);

        ship->update_topology_for_add_surface(rc->bl, rc->p, index);

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
        per_object->val.world_matrix = mat_position(rc->bl);
        per_object->upload();

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
