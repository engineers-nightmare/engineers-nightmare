#include "tinydir.h"

#include <epoxy/gl.h>
#include <string>

#include <glm/glm.hpp>
#include <vector>
#include <btBulletDynamicsCommon.h>
#include <BulletCollision/CollisionShapes/btShapeHull.h>
#include <unordered_map>

#include "asset_manager.h"
#include "chunk.h"

extern asset_manager asset_man;

static void
stamp_at_offset(std::vector<vertex> *verts, std::vector<unsigned> *indices,
                sw_mesh *src, glm::vec3 offset)
{
    unsigned index_base = (unsigned)verts->size();

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


extern physics *phy;


void
build_static_physics_rb(int x, int y, int z, btCollisionShape *shape, btRigidBody **rb)
{
    if (*rb) {
        /* We already have a rigid body set up; just swap out its collision shape. */
        (*rb)->setCollisionShape(shape);
    }
    else {
        /* Rigid body doesn't exist yet -- build one, along with all the motionstate junk */
        btDefaultMotionState *ms = new btDefaultMotionState(
            btTransform(btQuaternion(0, 0, 0, 1), btVector3((float)x, (float)y, (float)z)));
        btRigidBody::btRigidBodyConstructionInfo
                    ci(0, ms, shape, btVector3(0, 0, 0));
        *rb = new btRigidBody(ci);
        phy->dynamicsWorld->addRigidBody(*rb);
    }
}

void build_static_physics_rb_mat(glm::mat4 *m, btCollisionShape *shape, btRigidBody **rb) {
    if (*rb) {
        /* We already have a rigid body set up; just swap out its collision shape. */
        (*rb)->setCollisionShape(shape);
    }
    else {
        /* Rigid body doesn't exist yet -- build one, along with all th motionstate junk */
        btTransform t;
        t.setFromOpenGLMatrix((float *)m);
        btDefaultMotionState *ms = new btDefaultMotionState(t);
        btRigidBody::btRigidBodyConstructionInfo
            ci(0, ms, shape, btVector3(0, 0, 0));
        *rb = new btRigidBody(ci);
        (*rb)->setSleepingThresholds(0.1f, 0.1f);
        phy->dynamicsWorld->addRigidBody(*rb);
    }
}

void
build_dynamic_physics_rb_mat(glm::mat4 *m, btConvexHullShape *shape, btRigidBody **rb, float mass)
{
    if (*rb) {
        /* We already have a rigid body set up; just swap out its collision shape. */
        (*rb)->setCollisionShape(shape);
    }
    else {
        /* Rigid body doesn't exist yet -- build one, along with all th motionstate junk */
        btTransform t;
        t.setFromOpenGLMatrix((float *)m);
        auto *ms = new btDefaultMotionState(t);
        btVector3 inertia(0, 0, 0);
        shape->calculateLocalInertia(mass, inertia);
        btRigidBody::btRigidBodyConstructionInfo
            ci(mass, ms, shape, inertia);
        *rb = new btRigidBody(ci);
        (*rb)->setSleepingThresholds(0.1f, 0.1f);
        phy->dynamicsWorld->addRigidBody(*rb);
    }
}

void convert_static_rb_to_dynamic(btRigidBody **rb, float mass) {
    phy->dynamicsWorld->removeRigidBody(*rb);
    btVector3 inertia(0, 0, 0);
    (*rb)->getCollisionShape()->calculateLocalInertia(mass, inertia);
    (*rb)->setMassProps(mass, inertia);
    phy->dynamicsWorld->addRigidBody(*rb);
    (*rb)->activate();
}


void
build_static_physics_mesh(sw_mesh const * src, btTriangleMesh **mesh, btCollisionShape **shape)
{
    btTriangleMesh *phys = NULL;
    btCollisionShape *new_shape = NULL;

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
build_dynamic_physics_mesh(sw_mesh const * src, btTriangleMesh **mesh, btConvexHullShape **shape)
{
    btTriangleMesh *phys = nullptr;
    btConvexHullShape *new_shape = nullptr;

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

    if (*mesh)
        delete *mesh;
    *mesh = phys;
}

void activate_neighbors(btRigidBody *const *rb) {
    auto cache = phy->dynamicsWorld->getPairCache();
    auto* dispatcher = phy->dynamicsWorld->getDispatcher();
    auto* proxy = (*rb)->getBroadphaseHandle();

    // activate overlapping pairs
    struct activator_callback : btOverlapCallback
    {
        btBroadphaseProxy *find;
        btOverlappingPairCache *pair_cache;
        btDispatcher *disp;

        activator_callback(btBroadphaseProxy *p, btOverlappingPairCache *c, btDispatcher *d) :
            find(p), pair_cache(c), disp(d) {}

        bool processOverlap(btBroadphasePair& pair) override {
            auto wake_other = [this](btBroadphasePair& other, btBroadphaseProxy* p) {
                auto coll_obj = static_cast<btCollisionObject*>(p->m_clientObject);
                coll_obj->activate();
                pair_cache->cleanOverlappingPair(other, disp);
            };
            if (pair.m_pProxy0 == find) {
                wake_other(pair, pair.m_pProxy1);
            } else if (pair.m_pProxy1 == find) {
                wake_other(pair, pair.m_pProxy0);
            }
            return false;
        }
    } activator(proxy, phy->dynamicsWorld->getPairCache(), dispatcher);

    cache->processAllOverlappingPairs(&activator, dispatcher);
}

void
teardown_dynamic_physics_setup(btTriangleMesh **mesh, btConvexHullShape **shape, btRigidBody **rb)
{
    /* cleanly teardown a dynamic physics object such that build_dynamic_physics_setup() will
     * properly reconstruct everything */

    if (rb && *rb) {
        activate_neighbors(rb);

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

void
teardown_static_physics_setup(btTriangleMesh **mesh, btCollisionShape **shape, btRigidBody **rb)
{
    /* cleanly teardown a static physics object such that build_static_physics_setup() will
     * properly reconstruct everything */

    if (rb && *rb) {
        activate_neighbors(rb);

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
    const mesh_data * frame_mesh;
} frame_render_data;

void
mesher_init()
{
    frame_render_data.frame_mesh = &asset_man.get_mesh("frame");
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
                            auto mesh = &asset_man.get_surface_mesh(surf, b->surfs[surf]);
                            stamp_at_offset(&verts, &indices, mesh->sw, glm::vec3(i, j, k));
                        }
                    }
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
                            auto mesh = &asset_man.get_surface_mesh(surf, b->surfs[surf]);
                            stamp_at_offset(&verts, &indices, mesh->sw, glm::vec3(i, j, k));
                        }
                    }
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

    build_static_physics_rb(x * CHUNK_SIZE,
        y * CHUNK_SIZE,
        z * CHUNK_SIZE,
        this->phys_chunk.phys_shape,
        &this->phys_chunk.phys_body);
}
