#pragma once

#include "block.h"
#include "fixed_cube.h"
#include "mesh.h"
#include "component/c_entity.h"

#include <vector>

#define CHUNK_SIZE 4

class btTriangleMesh;
class btCollisionShape;
class btRigidBody;

struct render_chunk {
    hw_mesh *mesh = nullptr;
    bool valid = false;
};

struct phys_chunk {
    btTriangleMesh *phys_mesh = nullptr;
    btCollisionShape *phys_shape = nullptr;
    btRigidBody *phys_body = nullptr;
    bool valid = false;
};

struct topo_info {
    topo_info *p;
    int rank;
    int size;   /* if p==this, then the number of blocks in this cc */
};

struct chunk {
    fixed_cube<block, CHUNK_SIZE> blocks;
    fixed_cube<topo_info, CHUNK_SIZE> topo;

    /* rendering information */
    struct render_chunk render_chunk;
    struct phys_chunk phys_chunk;

    void prepare_render();
    void prepare_phys(int x, int y, int z);

    void dirty() {
        render_chunk.valid = false;
        phys_chunk.valid = false;
    }
};

/* must be called once before the mesher can be used */
void mesher_init();
