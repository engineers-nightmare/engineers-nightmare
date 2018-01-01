#include <btBulletDynamicsCommon.h>
#include <stdio.h>

#include "player.h"
#include "physics.h"
#include "utils/debugdraw.h"

#define MOVE_SPEED  0.07f
#define CROUCH_FACTOR 0.4f
#define AIR_CONTROL_FACTOR 0.25f
#include <algorithm>
#include <glm/ext.hpp>

std::unique_ptr<btSphereShape> innerReachSphere;
std::unique_ptr<btRigidBody> innerReachCollider;

/* a simple constructor hacked together based on
 * http://bulletphysics.org/mediawiki-1.5.8/index.php/Hello_World
 */
physics::physics(player *p)
    :
    broadphase(new btDbvtBroadphase()),
    collisionConfiguration(new btDefaultCollisionConfiguration()),
    dispatcher(new btCollisionDispatcher(collisionConfiguration.get())),
    solver(new btSequentialImpulseConstraintSolver),
    dynamicsWorld(new btDiscreteDynamicsWorld(
        dispatcher.get(),
        broadphase.get(),
        solver.get(),
        collisionConfiguration.get()))
{
    /* some default gravity
     * z is up and down
     */
    this->dynamicsWorld->setGravity(btVector3(0, 0, 0));

    /* store a pointer to our player so physics can drive his position */
    this->pl = p;

    rb_controller = std::make_unique<en_rb_controller>(mat_position(pl->pos));
    dynamicsWorld->addRigidBody(rb_controller.get());

    innerReachSphere = std::make_unique<btSphereShape>(0.8f);
    btTransform t{};
    auto *ms = new btDefaultMotionState(t);
    btRigidBody::btRigidBodyConstructionInfo
        ci(0, ms, innerReachSphere.get(), btVector3(0, 0, 0));
    innerReachCollider = std::make_unique<btRigidBody>(ci);
    // NOTE: not added to world!
}

void
physics::tick_controller(float dt)
{
    /* messy input -> char controller binding
     * TODO: untangle.
     */

    auto m = glm::mat4_cast(glm::normalize(pl->rot));
    auto right = glm::vec3(m[0]);
    auto up = glm::vec3(m[1]);

    if (pl->jump_state == player::on_structure) {
        
        rb_controller->setDamping(.5f, 0.f);
        rb_controller->applyCentralImpulse(vec3_to_bt(right * pl->move.x) * 5.f);
        rb_controller->applyCentralImpulse(vec3_to_bt(pl->dir * pl->move.y) * 5.f);
        rb_controller->applyCentralImpulse(vec3_to_bt(up * pl->move.z) * 5.f);

        if (pl->jump) {
            pl->jump_depth = pl->thing;
            pl->jump_state = player::leaving_structure;
            rb_controller->applyCentralImpulse(vec3_to_bt(glm::normalize(pl->dir) * 125.0f));
            rb_controller->setDamping(0.f, 0.f);
        }
    }
}

struct reach_callback : btCollisionWorld::ContactResultCallback {
    glm::vec3 deepest{};
    float p{ 0 };

    bool needsCollision(btBroadphaseProxy *proxy) const override {
        btCollisionObject const *co = (btCollisionObject const *)proxy->m_clientObject;
        if (!co->isStaticObject())
            return false;

        return btCollisionWorld::ContactResultCallback::needsCollision(proxy);
    }

    void addPoint(btVector3 pt, float dist) {
        if (dist < p) {
            deepest = bt_to_vec3(pt);
            p = dist;
        }
    }

    btScalar addSingleResult(btManifoldPoint& cp,
        btCollisionObjectWrapper const *obj0, int part0, int index0,
        btCollisionObjectWrapper const *obj1, int part1, int index1) override {

        if (obj0->m_collisionObject == innerReachCollider.get()) {
            addPoint(cp.m_localPointA, cp.m_distance1);
        }
        else if (obj1->m_collisionObject == innerReachCollider.get()) {
            addPoint(cp.m_localPointB, cp.m_distance1);
        }

        return 1.0f;
    }
};

void
physics::tick(float dt)
{
    dynamicsWorld->stepSimulation(dt, 10);

    auto trans = bt_to_mat4(rb_controller->getWorldTransform());
    pl->pos = trans[3];

    btTransform reachTransform{ btQuaternion{ 0, 0, 0, 1 }, vec3_to_bt(pl->pos) };
    innerReachCollider->setWorldTransform(reachTransform);
    reach_callback inner{};
    dynamicsWorld->contactTest(innerReachCollider.get(), inner);

    pl->thing = inner.p;
    pl->ui_dirty = true;

    switch (pl->jump_state) {
    case player::on_structure: {
        auto limit = 0.1f;
        if (inner.p > -limit && inner.p < 0.f) {
            auto factor = 1.f - inner.p / -limit;
            auto dir = glm::normalize(inner.deepest);
            auto proj = glm::dot(dir, bt_to_vec3(rb_controller->getLinearVelocity()));
            if (proj < 0)
                rb_controller->applyCentralImpulse(vec3_to_bt(-120.f * proj * dir));
        }
    } break;

    case player::leaving_structure: {
        if (inner.p < pl->jump_depth) {
            // jump didnt leave reach volume. we're still on structure.
            pl->jump_state = player::on_structure;
        }
        if (inner.p == 0.f) {
            pl->jump_state = player::off_structure;
        }
    } break;

    case player::off_structure: {
        if (inner.p < 0.f) {
            pl->jump_state = player::on_structure;
        }
    } break;
    }
}
