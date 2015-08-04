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
    hw_mesh *mesh = nullptr;
    bool valid = false;


    btTriangleMesh *phys_mesh = nullptr;
    btCollisionShape *phys_shape = nullptr;
    btRigidBody *phys_body = nullptr;
};

struct topo_info {
    topo_info *p;
    int rank;
    int size;   /* if p==this, then the number of blocks in this cc */
};

struct chunk {
    /* with a CHUNK_SIZE of 8
     * we have 8^3 blocks
     * this means a chunk represents
     * 8m^3
     */
    fixed_cube<block, CHUNK_SIZE> blocks;
    fixed_cube<topo_info, CHUNK_SIZE> topo;

    /* rendering information */
    struct render_chunk render_chunk;

    /* entities */
    std::vector<entity *> entities;

    void prepare_render(int _x, int _y, int _z);
};

/* must be called once before the mesher can be used */
void mesher_init();
