#pragma once

#include "component/component_manager.h"
#include "physics.h"

struct entity_type
{
    sw_mesh *sw;
    hw_mesh *hw;
    char const *name;
    btTriangleMesh *phys_mesh;
    btCollisionShape *phys_shape;
};

struct entity
{
    /* TODO: replace this completely, it's silly. */
    c_entity ce;

    entity(int x, int y, int z, unsigned type, int face);
    ~entity();
};
