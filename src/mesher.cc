#include "tinydir.h"

#include <epoxy/gl.h>
#include <string>

#include <glm/glm.hpp>
#include <vector>
#include <btBulletDynamicsCommon.h>
#include <BulletCollision/CollisionShapes/btShapeHull.h>
#include <unordered_map>
#include <glm/ext.hpp>

#include "asset_manager.h"
#include "chunk.h"

extern asset_manager asset_man;

extern physics *phy;

static void
stamp_at_offset(std::vector<vertex> *verts, std::vector<unsigned> *indices,
                sw_mesh *src, glm::vec3 offset)
{
    auto index_base = (unsigned)verts->size();

    for (unsigned int i = 0; i < src->num_vertices; i++) {
        vertex v = src->verts[i];
        v.x += offset.x;
        v.y += offset.y;
        v.z += offset.z;
        verts->push_back(v);
    }

    for (unsigned int i = 0; i < src->num_indices; i++)
        indices->push_back(index_base + src->indices[i]);
}

static void
stamp_at_mat(std::vector<vertex> *verts, std::vector<unsigned> *indices,
                sw_mesh *src, glm::mat4 mat)
{
    auto index_base = (unsigned)verts->size();

    for (unsigned int i = 0; i < src->num_vertices; i++) {
        auto v = src->verts[i];
        auto nv = mat * glm::vec4(v.x, v.y, v.z, 1);
        v.x = nv.x;
        v.y = nv.y;
        v.z = nv.z;

        auto norm = glm::unpackSnorm3x10_1x2(v.normal_packed);
        norm.w = 0;
        norm = mat * norm;
        v.normal_packed = glm::packSnorm3x10_1x2(norm);

        verts->push_back(v);
    }

    for (unsigned int i = 0; i < src->num_indices; i++)
        indices->push_back(index_base + src->indices[i]);
}

void build_rigidbody(const glm::mat4 &m, btCollisionShape *shape, btRigidBody **rb) {
    if (*rb) {
        /* We already have a rigid body set up; just swap out its collision shape. */
        (*rb)->setCollisionShape(shape);
    }
    else {
        /* Rigid body doesn't exist yet -- build one, along with all the motionstate junk */
        btTransform t;
        t.setFromOpenGLMatrix(glm::value_ptr(m));
        auto *ms = new btDefaultMotionState(t);
        btRigidBody::btRigidBodyConstructionInfo
            ci(0, ms, shape, btVector3(0, 0, 0));
        *rb = new btRigidBody(ci);
        (*rb)->setSleepingThresholds(0.1f, 0.1f);
        (*rb)->setCollisionFlags((*rb)->getCollisionFlags() | btCollisionObject::CF_STATIC_OBJECT);
        phy->dynamicsWorld->addRigidBody(*rb);
    }
}

void convert_static_rb_to_dynamic(btRigidBody *rb, float mass) {
    phy->dynamicsWorld->removeRigidBody(rb);
    rb->setCollisionFlags(rb->getCollisionFlags() & ~btCollisionObject::CF_STATIC_OBJECT);
    btVector3 inertia(0, 0, 0);
    rb->getCollisionShape()->calculateLocalInertia(mass, inertia);
    rb->setMassProps(mass, inertia);
    rb->updateInertiaTensor();
    phy->dynamicsWorld->addRigidBody(rb);
    rb->activate();
}


void
build_static_physics_mesh(sw_mesh const * src, btTriangleMesh **mesh, btCollisionShape **shape)
{
    btTriangleMesh *phys = nullptr;
    btCollisionShape *new_shape = nullptr;

    if (src->num_indices) {
        /* If we have some content in our mesh, transfer it to bullet */
        phys = new btTriangleMesh();
        phys->preallocateVertices(src->num_vertices);
        phys->preallocateIndices(src->num_indices);

        for (auto x = src->indices; x < src->indices + src->num_indices; /* */) {
            vertex v1 = src->verts[*x++];
            vertex v2 = src->verts[*x++];
            vertex v3 = src->verts[*x++];

            phys->addTriangle(btVector3(v1.x, v1.y, v1.z),
                btVector3(v2.x, v2.y, v2.z),
                btVector3(v3.x, v3.y, v3.z));
        }

        new_shape = new btBvhTriangleMeshShape(phys, true, true);
    }
    else {
        /* Empty mesh, just provide an empty shape. A zero-size mesh provokes a segfault inside
         * bullet, so avoid that. */
        new_shape = new btEmptyShape();
    }

    /* Throw away any old objects we've replaced. */
    if (*shape)
        delete *shape;
    *shape = new_shape;

    if (*mesh)
        delete *mesh;
    *mesh = phys;
}

void
build_dynamic_physics_mesh(sw_mesh const * src, btCollisionShape **shape)
{
    btConvexHullShape *new_shape = nullptr;

    if (src->num_indices) {
        auto chs = btConvexHullShape((const btScalar*)src->verts, src->num_vertices, sizeof(vertex));
        auto sh = btShapeHull(&chs);
        btScalar margin = chs.getMargin();
        sh.buildHull(margin);
        new_shape = new btConvexHullShape((const btScalar*)sh.getVertexPointer(), sh.numVertices());
    }
    else {
        /* Empty mesh. Invalid. */
        assert(false);
    }

    /* Throw away any old objects we've replaced. */
    if (*shape)
        delete *shape;
    *shape = new_shape;
}

void
teardown_physics_setup(btTriangleMesh **mesh, btCollisionShape **shape, btRigidBody **rb)
{
    /* cleanly teardown a physics object such that build_rigidbody() will
     * properly reconstruct everything */

    if (rb && *rb) {
        phy->dynamicsWorld->removeRigidBody(*rb);
        delete *rb;
        *rb = nullptr;
    }

    if (shape && *shape) {
        delete *shape;
        *shape = nullptr;
    }

    if (mesh && *mesh) {
        delete *mesh;
        *mesh = nullptr;
    }
}

static struct {
    mesh_data const * frame_mesh;
    mesh_data const * frame_corner_mesh;
    mesh_data const * frame_invcorner_mesh;
    mesh_data const * frame_sloped_mesh;
    glm::mat4 corner_matrices[8];
    glm::mat4 sloped_matrices[8];
    glm::mat4 extra_matrices[4];
} frame_render_data;

void
mesher_init()
{
    frame_render_data.frame_mesh = &asset_man.get_mesh("frame");
    frame_render_data.frame_corner_mesh = &asset_man.get_mesh("frame-corner");
    frame_render_data.frame_invcorner_mesh = &asset_man.get_mesh("frame-invcorner");
    frame_render_data.frame_sloped_mesh = &asset_man.get_mesh("frame-sloped");

    static float const rots[] = { 0.f, 90.f, 270.f, 180.f };

    for (auto i = 0; i < 8; i++) {
        frame_render_data.corner_matrices[i] = glm::translate(glm::vec3(0.5f, 0.5f, 0.5f)) *
            glm::eulerAngleZ(glm::radians(rots[i & 3])) *
            glm::eulerAngleY(glm::radians((i & 4) ? 90.f : 0.f)) *
            glm::translate(glm::vec3(-0.5f, -0.5f, -0.5f));
    }

    for (auto i = 0; i < 8; i++) {
        frame_render_data.sloped_matrices[i] = glm::translate(glm::vec3(0.5f, 0.5f, 0.5f)) *
            glm::eulerAngleZ(glm::radians(rots[i & 3])) *
            glm::eulerAngleY(glm::radians((i & 4) ? 180.f : 0.f)) *
            glm::translate(glm::vec3(-0.5f, -0.5f, -0.5f));
    }

    for (auto i = 0; i < 4; i++) {
        frame_render_data.extra_matrices[i] = glm::translate(glm::vec3(0.5f, 0.5f, 0.5f)) *
            glm::eulerAngleZ(glm::radians(rots[i & 3])) *
            glm::eulerAngleY(glm::radians(90.f)) *
            glm::translate(glm::vec3(-0.5f, -0.5f, -0.5f));
    }
}

glm::mat4
get_corner_matrix(block_type type, glm::ivec3 pos) {
    glm::mat4 mat;
    if ((type & ~3) == block_slope_extra_base) {
        mat = frame_render_data.extra_matrices[type & 3];
    }
    else if ((type & ~7) == block_slope_base) {
        mat = frame_render_data.sloped_matrices[type & 7];
    }
    else {
        mat = frame_render_data.corner_matrices[type & 7];
    }
    mat[3][0] += pos.x;
    mat[3][1] += pos.y;
    mat[3][2] += pos.z;
    return mat;
}

void
chunk::prepare_render()
{
    if (this->render_chunk.valid)
        return;     // nothing to do here.

    std::vector<vertex> verts;
    std::vector<unsigned> indices;

    for (unsigned k = 0; k < CHUNK_SIZE; k++) {
        for (unsigned j = 0; j < CHUNK_SIZE; j++) {
            for (unsigned i = 0; i < CHUNK_SIZE; i++) {
                block *b = this->blocks.get(i, j, k);

                if (b->type == block_frame) {
                    // TODO: block detail, variants, types, surfaces
                    stamp_at_offset(&verts, &indices, frame_render_data.frame_mesh->sw, glm::vec3(i, j, k));

                    // Only frame side of surface gets generated
                    for (unsigned surf = 0; surf < 6; surf++) {
                        if (b->surfs[surf] != surface_none) {
                            auto mesh = asset_man.surf_kinds[b->surfs[surf]].visual_mesh;
                            auto mat = mat_block_surface({i, j, k}, surf ^ 1);
                            stamp_at_mat(&verts, &indices, mesh->sw, mat);
                        }
                    }
                }
                else if ((b->type & ~7) == block_corner_base) {
                    stamp_at_mat(&verts, &indices, frame_render_data.frame_corner_mesh->sw,
                        get_corner_matrix(b->type, { i, j, k }));
                }
                else if ((b->type & ~7) == block_invcorner_base) {
                    stamp_at_mat(&verts, &indices, frame_render_data.frame_invcorner_mesh->sw,
                        get_corner_matrix(b->type, { i, j, k }));
                }
                else if ((b->type & ~7) == block_slope_base) {
                    stamp_at_mat(&verts, &indices, frame_render_data.frame_sloped_mesh->sw,
                        get_corner_matrix(b->type, { i, j, k }));
                }
                else if ((b->type & ~3) == block_slope_extra_base) {
                    stamp_at_mat(&verts, &indices, frame_render_data.frame_sloped_mesh->sw,
                        get_corner_matrix(b->type, { i, j, k }));
                }
            }
        }
    }

    /* wrap the vectors in a temporary sw_mesh */
    sw_mesh m{};
    m.verts = &verts[0];
    m.indices = &indices[0];
    m.num_vertices = (unsigned)verts.size();
    m.num_indices = (unsigned)indices.size();

    // TODO: try to reuse memory
    if (this->render_chunk.mesh) {
        free_mesh(this->render_chunk.mesh);
        delete this->render_chunk.mesh;
    }

    this->render_chunk.mesh = upload_mesh(&m);
    this->render_chunk.valid = true;
}


void
chunk::prepare_phys(int x, int y, int z)
{
    if (this->phys_chunk.valid)
        return;     // nothing to do here.

    std::vector<vertex> verts;
    std::vector<unsigned> indices;

    for (unsigned k = 0; k < CHUNK_SIZE; k++) {
        for (unsigned j = 0; j < CHUNK_SIZE; j++) {
            for (unsigned i = 0; i < CHUNK_SIZE; i++) {
                block *b = this->blocks.get(i, j, k);

                if (b->type == block_frame) {
                    // TODO: block detail, variants, types, surfaces
                    stamp_at_offset(&verts, &indices, frame_render_data.frame_mesh->sw, glm::vec3(i, j, k));

                    // Only generate in blocks that have framing
                    for (unsigned surf = 0; surf < 6; surf++) {
                        if (b->surfs[surf] != surface_none) {
                            auto mesh = asset_man.surf_kinds[b->surfs[surf]].physics_mesh;
                            auto mat = mat_block_surface({i, j, k}, surf ^ 1);
                            stamp_at_mat(&verts, &indices, mesh->sw, mat);
                        }
                    }
                }
                else if ((b->type & ~7) == block_corner_base) {
                    stamp_at_mat(&verts, &indices, frame_render_data.frame_corner_mesh->sw,
                        get_corner_matrix(b->type, { i, j, k }));
                }
                else if ((b->type & ~7) == block_invcorner_base) {
                    stamp_at_mat(&verts, &indices, frame_render_data.frame_invcorner_mesh->sw,
                        get_corner_matrix(b->type, { i, j, k }));
                }
                else if ((b->type & ~7) == block_slope_base) {
                    stamp_at_mat(&verts, &indices, frame_render_data.frame_sloped_mesh->sw,
                        get_corner_matrix(b->type, { i, j, k }));
                }
                else if ((b->type & ~3) == block_slope_extra_base) {
                    stamp_at_mat(&verts, &indices, frame_render_data.frame_sloped_mesh->sw,
                        get_corner_matrix(b->type, { i, j, k }));
                }
            }
        }
    }

    /* wrap the vectors in a temporary sw_mesh */
    sw_mesh m{};
    m.verts = &verts[0];
    m.indices = &indices[0];
    m.num_vertices = (unsigned)verts.size();
    m.num_indices = (unsigned)indices.size();

    this->phys_chunk.valid = true;

    build_static_physics_mesh(&m,
        &this->phys_chunk.phys_mesh,
        &this->phys_chunk.phys_shape);

    auto mat = mat_position({CHUNK_SIZE * x, CHUNK_SIZE * y, CHUNK_SIZE * z});
    build_rigidbody(mat, this->phys_chunk.phys_shape, &this->phys_chunk.phys_body);
}
