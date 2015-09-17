#include <epoxy/gl.h>

#include "../ship_space.h"
#include "../shader_params.h"
#include "../mesh.h"
#include "../block.h"
#include "tools.h"


extern GLuint add_overlay_shader;
extern GLuint remove_overlay_shader;
extern GLuint simple_shader;

extern void
mark_lightfield_update(int x, int y, int z);

extern ship_space *ship;

extern glm::mat4
mat_position(float x, float y, float z);

extern hw_mesh *surfs_hw[6];

extern void
remove_ents_from_surface(int x, int y, int z, int face);

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
    block *other_side = ship->get_block(rc->px, rc->py, rc->pz);

    if (can_use(bl, other_side, index)) {

        if (!bl) {
            ship->ensure_block(rc->x, rc->y, rc->z);
            bl = ship->get_block(rc->x, rc->y, rc->z);
        }

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

        ship->update_topology_for_add_surface(rc->x, rc->y, rc->z, rc->px, rc->py, rc->pz, index);

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
add_surface_tool::preview(raycast_info *rc) {
    if (!rc->hit)
        return;

    block *bl = ship->get_block(rc->x, rc->y, rc->z);
    int index = normal_to_surface_index(rc);
    block *other_side = ship->get_block(rc->px, rc->py, rc->pz);

    if (can_use(bl, other_side, index)) {
        per_object->val.world_matrix = mat_position((float)rc->x, (float)rc->y, (float)rc->z);
        per_object->upload();

        glUseProgram(add_overlay_shader);
        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(-0.1f, -0.1f);
        draw_mesh(surfs_hw[index]);
        glPolygonOffset(0.0f, 0.0f);
        glDisable(GL_POLYGON_OFFSET_FILL);
        glUseProgram(simple_shader);
    }
}


void
add_surface_tool::get_description(char *str) {
    sprintf(str, "Place surface type %d\n", (int)st);
}
