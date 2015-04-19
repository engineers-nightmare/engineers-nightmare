#include <epoxy/gl.h>

#include "chunk.h"
#include "mesh.h"
#include "physics.h"

#include <glm/glm.hpp>
#include <vector>       // HISSSSSSS
#include <btBulletDynamicsCommon.h>

/* TODO: sensible container for these things, once we have variants */
extern sw_mesh *scaffold_sw;
extern sw_mesh *x_quad_sw, *y_quad_sw, *z_quad_sw;


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


extern physics *phy;


void
chunk::prepare_render(int _x, int _y, int _z)
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

                if (b->surfs[surface_xm] == surface_wall) {
                    stamp_at_offset(&verts, &indices, x_quad_sw, glm::vec3(i, j, k));
                }

                if (b->surfs[surface_xp] == surface_wall) {

                    stamp_at_offset(&verts, &indices, x_quad_sw, glm::vec3(i+1, j, k));
                }

                if (b->surfs[surface_ym] == surface_wall) {
                    stamp_at_offset(&verts, &indices, y_quad_sw, glm::vec3(i, j, k));
                }

                if (b->surfs[surface_yp] == surface_wall) {
                    stamp_at_offset(&verts, &indices, y_quad_sw, glm::vec3(i, j+1, k));
                }

                if (b->surfs[surface_zm] == surface_wall) {
                    stamp_at_offset(&verts, &indices, z_quad_sw, glm::vec3(i, j, k));
                }

                if (b->surfs[surface_zp] == surface_wall) {
                    stamp_at_offset(&verts, &indices, z_quad_sw, glm::vec3(i, j, k+1));
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

