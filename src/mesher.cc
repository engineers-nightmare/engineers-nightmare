#include <epoxy/gl.h>

#include "chunk.h"
#include "mesh.h"
#include "physics.h"

#include <glm/glm.hpp>
#include <vector>       // HISSSSSSS
#include <btBulletDynamicsCommon.h>

/* TODO: sensible container for these things, once we have variants */
extern sw_mesh *scaffold_sw;
extern sw_mesh *surfs_sw[6];


static void
stamp_at_offset(std::vector<vertex> *verts, std::vector<unsigned> *indices,
                sw_mesh *src, glm::vec3 offset, int mat)
{
    unsigned index_base = verts->size();

    for (unsigned int i = 0; i < src->num_vertices; i++) {
        vertex v = src->verts[i];
        v.x += offset.x;
        v.y += offset.y;
        v.z += offset.z;
        v.mat = mat;
        verts->push_back(v);
    }

    for (unsigned int i = 0; i < src->num_indices; i++)
        indices->push_back(index_base + src->indices[i]);
}


extern physics *phy;


void
chunk::prepare_render(int _x, int _y, int _z)
{
    if (this->render_chunk.valid)
        return;     // nothing to do here.

    static const int surface_type_to_material[] = { 0, 2, 4 };

    std::vector<vertex> verts;
    std::vector<unsigned> indices;

    for (int k = 0; k < CHUNK_SIZE; k++)
        for (int j = 0; j < CHUNK_SIZE; j++)
            for (int i = 0; i < CHUNK_SIZE; i++) {
                block *b = this->blocks.get(i, j, k);

                if (b->type == block_support) {
                    // TODO: block detail, variants, types, surfaces
                    stamp_at_offset(&verts, &indices, scaffold_sw, glm::vec3(i, j, k), 1);
                }

                for (int surf = 0; surf < 6; surf++) {
                    if (b->surfs[surf] != surface_none) {
                        stamp_at_offset(&verts, &indices, surfs_sw[surf], glm::vec3(i, j, k),
                                surface_type_to_material[b->surfs[surf]]);
                    }
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

    // physics! This is a pretty stupid way to do it, but can redo...
    btTriangleMesh *phys = new btTriangleMesh();
    phys->preallocateVertices(verts.size());
    phys->preallocateIndices(indices.size());

    for (std::vector<unsigned>::const_iterator x = indices.begin(); x != indices.end();) {
        vertex v1 = verts[*x++];
        vertex v2 = verts[*x++];
        vertex v3 = verts[*x++];

        phys->addTriangle(btVector3(v1.x, v1.y, v1.z),
                          btVector3(v2.x, v2.y, v2.z),
                          btVector3(v3.x, v3.y, v3.z));
    }

    btBvhTriangleMeshShape *shape = new btBvhTriangleMeshShape(phys, true, true);

    if (this->render_chunk.phys_body)
        this->render_chunk.phys_body->setCollisionShape(shape);
    else {
        btDefaultMotionState *ms = new btDefaultMotionState(
            btTransform(btQuaternion(0, 0, 0, 1),
                        btVector3(_x * CHUNK_SIZE, _y * CHUNK_SIZE, _z * CHUNK_SIZE)));
        btRigidBody::btRigidBodyConstructionInfo
                    ci(0, ms, shape, btVector3(0, 0, 0));
        this->render_chunk.phys_body = new btRigidBody(ci);
        phy->dynamicsWorld->addRigidBody(this->render_chunk.phys_body);
    }

    if (this->render_chunk.phys_shape)
        delete this->render_chunk.phys_shape;
    this->render_chunk.phys_shape = shape;

    if (this->render_chunk.phys_mesh)
        delete this->render_chunk.phys_mesh;
    this->render_chunk.phys_mesh = phys;
}

