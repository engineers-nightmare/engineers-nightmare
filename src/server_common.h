#include "chunk.h"
#include "ship_space.h"
#include "component/component_system_manager.h"
#include "physics.h"

extern ship_space *ship;

void remove_ents_from_surface(glm::ivec3 b, int face, physics *phy);
glm::mat4 mat_block_face(glm::ivec3 p, int face);
glm::mat4 mat_block_face(glm::ivec3 p, int face, float rotation);
c_entity spawn_entity(glm::ivec3 p, unsigned type, int face, physics *phy);
c_entity spawn_entity(glm::ivec3 p, unsigned type, int face, physics *phy, float rotation);
void destroy_entity(c_entity e, physics *phy);

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


extern entity_type entity_types[12];
