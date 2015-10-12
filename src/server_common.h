#include "mesh.h"
#include "chunk.h"
#include "ship_space.h"
#include "component/component_system_manager.h"
#include "physics.h"

extern sw_mesh *door_sw;
extern hw_mesh *door_hw;
extern ship_space *ship;

void remove_ents_from_surface(glm::ivec3 b, int face);
glm::mat4 mat_block_face(glm::ivec3 p, int face);

struct entity_type
{
    /* static */
    char const *name;
    char const *mesh;
    int material;
    bool placed_on_surface;
    int height;

    /* loader loop does these */
    sw_mesh *sw;
    hw_mesh *hw;
    btTriangleMesh *phys_mesh;
    btCollisionShape *phys_shape;
};


static entity_type entity_types[] = {
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

c_entity spawn_entity(glm::ivec3 p, unsigned type, int face);

void destroy_entity(c_entity e);
