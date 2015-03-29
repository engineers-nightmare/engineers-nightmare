#include <epoxy/gl.h>

#include "chunk.h"
#include "mesh.h"

#include <glm/glm.hpp>
#include <vector>       // HISSSSSSS

/* TODO: sensible container for these things, once we have variants */
extern sw_mesh *scaffold_sw;


static void
stamp_at_offset(std::vector<vertex> *verts, std::vector<unsigned> *indices,
                sw_mesh *src, glm::vec3 offset)
{
    unsigned index_base = verts->size();

    for (int i = 0; i < src->num_vertices; i++) {
        vertex v = src->verts[i];
        v.x += offset.x;
        v.y += offset.y;
        v.z += offset.z;
        verts->push_back(v);
    }

    for (int i = 0; i < src->num_indices; i++)
        indices->push_back(index_base + src->indices[i]);
}


void
chunk::prepare_render()
{
    if (this->render_chunk.valid)
        return;     // nothing to do here.

    std::vector<vertex> verts;
    std::vector<unsigned> indices;

    for (int k = 0; k < CHUNK_SIZE; k++)
        for (int j = 0; j < CHUNK_SIZE; j++)
            for (int i = 0; i < CHUNK_SIZE; i++) {
                block *b = this->blocks.get(i, j, k);

                if (b->type == block_support) {
                    // TODO: block detail, variants, types, surfaces
                    stamp_at_offset(&verts, &indices, scaffold_sw, glm::vec3(i, j, k));
                }
            }

    /* wrap the vectors in a temporary sw_mesh */
    sw_mesh m;
    m.verts = &verts[0];
    m.indices = &indices[0];
    m.num_vertices = verts.size();
    m.num_indices = indices.size();

    // TODO: try to reuse memory
    if (this->render_chunk.mesh) {
        free_mesh(this->render_chunk.mesh);
        delete this->render_chunk.mesh;
    }

    this->render_chunk.mesh = upload_mesh(&m);
    this->render_chunk.valid = true;
}

