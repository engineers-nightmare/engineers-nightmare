#include "server_common.h"

extern hw_mesh *door_hw;

entity_type entity_types[] = {
    { "Door", "mesh/single_door_frame.dae", 2, false, 2 },
    { "Frobnicator", "mesh/frobnicator.dae", 3, false, 1 },
    { "Light", "mesh/panel_4x4.dae", 8, true, 1 },
    { "Warning Light", "mesh/warning_light.dae", 8, true, 1 },
    { "Display Panel", "mesh/panel_4x4.dae", 7, true, 1 },
    { "Switch", "mesh/panel_1x1.dae", 9, true, 1 },
    { "Plaidnicator", "mesh/frobnicator.dae", 13, false, 1 },
    { "Pressure Sensor 1", "mesh/panel_1x1.dae", 12, true, 1 },
    { "Pressure Sensor 2", "mesh/panel_1x1.dae", 14, true, 1 },
    { "Sensor Comparator", "mesh/panel_1x1.dae", 13, true, 1 },
    { "Proximity Sensor", "mesh/panel_1x1.dae", 3, true, 1 },
    { "Flashlight", "mesh/no_place.dae", 3, true, 1 },
};

void
remove_ents_from_surface(glm::ivec3 b, int face, physics *phy)
{
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

        auto type = &entity_types[*type_man.get_instance_data(ce).type];

        if (p.x == b.x && p.y == b.y && p.z <= b.z && p.z + type->height > b.z && f == face) {
            destroy_entity(ce, phy);
            it = ch->entities.erase(it);

            block *bl = ship->get_block(p);
            assert(bl);
            bl->surf_space[face] = 0;   /* we've popped *everything* off, it must be empty now */
        }
        else {
            ++it;
        }
    }
}


glm::mat4
mat_block_face(glm::ivec3 p, int face, float rotation)
{
    auto norm = glm::vec3(surface_index_to_normal(face));
    auto pos = glm::vec3(p) + glm::vec3(0.5f) + 0.5f * norm;
    return glm::rotate(mat_rotate_mesh(pos, -norm), rotation, glm::vec3(0, 0, 1));
}


glm::mat4
mat_block_face(glm::ivec3 p, int face)
{
    return mat_block_face(p, face, 0);
}


c_entity 
spawn_entity(glm::ivec3 p, unsigned type, int face, physics *phy, float rotation) {
    auto ce = c_entity::spawn();

    auto mat = mat_block_face(p, face, rotation);

    auto et = &entity_types[type];

    type_man.assign_entity(ce);
    auto type_comp = type_man.get_instance_data(ce);
    *type_comp.type = type;

    physics_man.assign_entity(ce);
    auto physics = physics_man.get_instance_data(ce);
    *physics.rigid = nullptr;
    build_static_physics_rb_mat(&mat, phy, et->phys_shape, physics.rigid);

    /* so that we can get back to the entity from a phys raycast */
    /* TODO: these should really come from a dense pool rather than the generic allocator */
    auto per = new phys_ent_ref;
    per->ce = ce;
    (*physics.rigid)->setUserPointer(per);

    surface_man.assign_entity(ce);
    auto surface = surface_man.get_instance_data(ce);
    *surface.block = p;
    *surface.face = face;

    pos_man.assign_entity(ce);
    auto pos = pos_man.get_instance_data(ce);
    *pos.position = p;
    *pos.mat = mat;
    *pos.rotation = rotation;

    /* hack to not render a mesh for the flashlight */
    /* todo: handle entities that don't need to be rendered*/
    if (type != 11) {
        render_man.assign_entity(ce);
        auto render = render_man.get_instance_data(ce);
        *render.mesh = et->hw;
    }

    // door
    if (type == 0) {
        power_man.assign_entity(ce);
        auto power = power_man.get_instance_data(ce);
        *power.powered = false;
        *power.required_power = 8;
        *power.max_required_power = 8;

        door_man.assign_entity(ce);
        auto door = door_man.get_instance_data(ce);
        *door.mesh = door_hw;
        *door.pos = 1.0f;
        *door.desired_pos = 1.0f;
        *door.height = et->height;

        reader_man.assign_entity(ce);
        auto reader = reader_man.get_instance_data(ce);
        *reader.name = "desired state";
        reader.source->id = 0;
        *reader.desc = nullptr;
        *reader.data = 1.0f;
    }
    // frobnicator
    else if (type == 1) {
        power_man.assign_entity(ce);
        auto power = power_man.get_instance_data(ce);
        *power.powered = false;
        *power.required_power = 12;
        *power.max_required_power = 12;

        gas_man.assign_entity(ce);
        auto gas = gas_man.get_instance_data(ce);
        *gas.flow_rate = 0.1f;
        *gas.max_pressure = 1.0f;
        *gas.enabled = true;
    }
    // light
    else if (type == 2) {
        power_man.assign_entity(ce);
        auto power = power_man.get_instance_data(ce);
        *power.powered = false;
        *power.required_power = 6;
        *power.max_required_power = 6;

        light_man.assign_entity(ce);
        auto light = light_man.get_instance_data(ce);
        *light.intensity = 1.0f;

        reader_man.assign_entity(ce);
        auto reader = reader_man.get_instance_data(ce);
        *reader.name = "light brightness";
        reader.source->id = 0;
        *reader.desc = nullptr;
        *reader.data = 1.0f;
    }
    // warning light
    else if (type == 3) {
        power_man.assign_entity(ce);
        auto power = power_man.get_instance_data(ce);
        *power.powered = false;
        *power.required_power = 6;
        *power.max_required_power = 6;

        light_man.assign_entity(ce);
        auto light = light_man.get_instance_data(ce);
        *light.intensity = 1.0f;

        reader_man.assign_entity(ce);
        auto reader = reader_man.get_instance_data(ce);
        *reader.name = "light brightness";
        reader.source->id = 0;
        *reader.desc = comms_msg_type_sensor_comparison_state;      // temp until we have discriminator tool
        *reader.data = 1.0f;
    }
    // display panel
    else if (type == 4) {
        power_man.assign_entity(ce);
        auto power = power_man.get_instance_data(ce);
        *power.powered = false;
        *power.required_power = 4;
        *power.max_required_power = 4;

        light_man.assign_entity(ce);
        auto light = light_man.get_instance_data(ce);
        *light.intensity = 0.15f;

        reader_man.assign_entity(ce);
        auto reader = reader_man.get_instance_data(ce);
        *reader.name = "light brightness";
        reader.source->id = 0;
        *reader.desc = nullptr;
        *reader.data = 0.15f;
    }
    // switch
    else if (type == 5) {
        switch_man.assign_entity(ce);
        auto sw = switch_man.get_instance_data(ce);
        *sw.enabled = true;
    }
    // plaidnicator
    else if (type == 6) {
        power_provider_man.assign_entity(ce);
        auto power_provider = power_provider_man.get_instance_data(ce);
        *power_provider.max_provided = 12;
        *power_provider.provided = 12;
    }
    // pressure sensor 1
    else if (type == 7) {
        pressure_man.assign_entity(ce);
        auto pressure = pressure_man.get_instance_data(ce);
        *pressure.pressure = 0.0f;
        *pressure.type = 1;
    }
    // pressure sensor 2
    else if (type == 8) {
        pressure_man.assign_entity(ce);
        auto pressure = pressure_man.get_instance_data(ce);
        *pressure.pressure = 0.0f;
        *pressure.type = 2;
    }
    // sensor comparator
    else if (type == 9) {
        comparator_man.assign_entity(ce);
        auto comparator = comparator_man.get_instance_data(ce);
        *comparator.compare_epsilon = 0.0001f;
    }
    // proximity sensor
    else if (type == 10) {
        power_man.assign_entity(ce);
        auto power = power_man.get_instance_data(ce);
        *power.powered = false;
        *power.required_power = 1;
        *power.max_required_power = 1;

        proximity_man.assign_entity(ce);
        auto proximity_sensor = proximity_man.get_instance_data(ce);
        *proximity_sensor.range = 5;
        *proximity_sensor.is_detected = false;
    }
    // flashlight
    else if (type == 11) {
        power_man.assign_entity(ce);
        auto power = power_man.get_instance_data(ce);
        *power.powered = false; /* Flashlight starts off */
        *power.required_power = 0;
        *power.max_required_power = 0;

        light_man.assign_entity(ce);
        auto light = light_man.get_instance_data(ce);
        *light.intensity = 0.75f;

        reader_man.assign_entity(ce);
        auto reader = reader_man.get_instance_data(ce);
        *reader.name = "flashlight brightness";
        reader.source->id = 0;
        *reader.desc = nullptr;
        *reader.data = 0.75f;
    }

    return ce;
}


c_entity 
spawn_entity(glm::ivec3 p, unsigned type, int face, physics *phy) {
    return spawn_entity(p, type, face, phy, 0);
}


void
destroy_entity(c_entity e, physics *phy)
{
    /* removing block influence from this ent */
    /* this should really be componentified */
    if (surface_man.exists(e)) {
        auto b = *surface_man.get_instance_data(e).block;
        auto type = &entity_types[*type_man.get_instance_data(e).type];

        for (auto i = 0; i < type->height; i++) {
            auto p = b + glm::ivec3(0, 0, i);
            block *bl = ship->get_block(p);
            assert(bl);
            if (bl->type == block_entity) {
                printf("emptying %d,%d,%d on remove of ent\n", p.x, p.y, p.z);
                bl->type = block_empty;

                for (auto face = 0; face < 6; face++) {
                    /* unreserve all the space */
                    bl->surf_space[face] = 0;
                }
            }
        }
    }

    if (door_man.exists(e)) {
        /* make sure the door is /open/ -- no magic surfaces left lying around. */
        set_door_state(ship, e, surface_none);
    }

    if (physics_man.exists(e)) {
        auto phys_data = physics_man.get_instance_data(e);
        phys_ent_ref *per = (phys_ent_ref *)(*phys_data.rigid)->getUserPointer();
        if (per) {
            delete per;
        }

        teardown_static_physics_setup(phy, nullptr, nullptr, phys_data.rigid);
    }

    comparator_man.destroy_entity_instance(e);
    gas_man.destroy_entity_instance(e);
    light_man.destroy_entity_instance(e);
    physics_man.destroy_entity_instance(e);
    pos_man.destroy_entity_instance(e);
    power_man.destroy_entity_instance(e);
    power_provider_man.destroy_entity_instance(e);
    pressure_man.destroy_entity_instance(e);
    render_man.destroy_entity_instance(e);
    surface_man.destroy_entity_instance(e);
    switch_man.destroy_entity_instance(e);
    type_man.destroy_entity_instance(e);
    door_man.destroy_entity_instance(e);
    reader_man.destroy_entity_instance(e);
    proximity_man.destroy_entity_instance(e);

    remove_attaches_for_entity(ship, e);
}
