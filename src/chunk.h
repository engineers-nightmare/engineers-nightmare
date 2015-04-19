#pragma once

#include "block.h"
#include "fixed_cube.h"

#define CHUNK_SIZE 8

struct hw_mesh;

class btTriangleMesh;
class btBvhTriangleMeshShape;
class btRigidBody;

struct render_chunk {
    hw_mesh *mesh;
    bool valid;


    btTriangleMesh *phys_mesh;
    btBvhTriangleMeshShape *phys_shape;
    btRigidBody *phys_body;
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

    block * get_block(unsigned int x, unsigned int y, unsigned int z);

    void prepare_render(int _x, int _y, int _z);
};

