#include "component/component_system_manager.h"
#include "entity.h"
#include "mesh.h"

// TODO: refactor this to a seperate file
extern glm::mat4 mat_block_face(int x, int y, int z, int face);
// TODO: also not great
extern entity_type entity_types[5];

entity::entity(int x, int y, int z, unsigned type, int face) {
    ce = c_entity::spawn();

    auto mat = mat_block_face(x, y, z, face);

    auto et = &entity_types[type];

    type_man.assign_entity(ce);
    type_man.type(ce) = type;

    physics_man.assign_entity(ce);
    build_static_physics_rb_mat(&mat, et->phys_shape, &physics_man.rigid(ce));
    /* so that we can get back to the entity from a phys raycast */
    physics_man.rigid(ce)->setUserPointer(this);
    physics_man.mesh(ce) = et->phys_mesh;
    physics_man.collision(ce) = et->phys_shape;

    surface_man.assign_entity(ce);
    surface_man.block(ce) = glm::ivec3(x, y, z);
    surface_man.face(ce) = face;

    pos_man.assign_entity(ce);
    pos_man.position(ce) = glm::vec3(x, y, z);
    pos_man.mat(ce) = mat;

    render_man.assign_entity(ce);
    render_man.mesh(ce) = *et->hw;

    // frobnicator
    if (type == 0) {
        power_man.assign_entity(ce);
        //default to powered state for now
        power_man.powered(ce) = true;

        switchable_man.assign_entity(ce);
        switchable_man.enabled(ce) = false;

        gas_man.assign_entity(ce);
        gas_man.flow_rate(ce) = 0.1f;
        gas_man.max_pressure(ce) = 1.0f;
    }
    // display panel
    else if (type == 1) {
        power_man.assign_entity(ce);
        //default to powered state for now
        power_man.powered(ce) = true;

        light_man.assign_entity(ce);
        light_man.intensity(ce) = 0.15f;
    }
    // light
    else if (type == 2) {
        power_man.assign_entity(ce);
        //default to powered state for now
        power_man.powered(ce) = true;

        switchable_man.assign_entity(ce);
        switchable_man.enabled(ce) = true;

        light_man.assign_entity(ce);
        light_man.intensity(ce) = 1.f;
    }
    // switch
    else if (type == 3) {
        switch_man.assign_entity(ce);
    }
}

entity::~entity() {
}
