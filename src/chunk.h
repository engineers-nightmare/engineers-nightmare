#pragma once

#include "block.h"
#include "fixed_cube.h"

#include <vector>

#define CHUNK_SIZE 8

struct hw_mesh;
struct entity;

class btTriangleMesh;
class btCollisionShape;
class btRigidBody;

struct render_chunk {
    hw_mesh *mesh = NULL;
    bool valid = false;


    btTriangleMesh *phys_mesh = NULL;
    btCollisionShape *phys_shape = NULL;
    btRigidBody *phys_body = NULL;
};

struct chunk {
    /* with a CHUNK_SIZE of 8
     * we have 8^3 blocks
     * this means a chunk represents
     * 8m^3
     */
    fixed_cube<block, CHUNK_SIZE> blocks;

    /* rendering information */
    struct render_chunk render_chunk;

    /* entities */
    std::vector<entity *> entities;

    void prepare_render(int _x, int _y, int _z);
};

