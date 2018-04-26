#include <btBulletDynamicsCommon.h>
#include <BulletSoftBody/btSoftRigidDynamicsWorld.h>
#include <BulletSoftBody/btSoftBodyHelpers.h>
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
    dynamicsWorld(new btSoftRigidDynamicsWorld(
        dispatcher.get(),
        broadphase.get(),
        solver.get(),
        collisionConfiguration.get()))
{
    this->dynamicsWorld->setGravity(btVector3(0, 0, 0));

    auto &softBodyWorldInfo = dynamicsWorld->getWorldInfo();
    softBodyWorldInfo.m_broadphase = dynamicsWorld->getBroadphase();
    softBodyWorldInfo.m_dispatcher = dynamicsWorld->getDispatcher();
    softBodyWorldInfo.m_gravity = dynamicsWorld->getGravity();
    softBodyWorldInfo.m_sparsesdf.Initialize();

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
        
        rb_controller->setDamping(.8f, 0.f);

        auto move = pl->move;
        auto len = glm::length(move);
        if (len > 1.f) {
            move /= len;
        }
        
        rb_controller->applyCentralImpulse(vec3_to_bt(right * move.x) * 2.f);
        rb_controller->applyCentralImpulse(vec3_to_bt(pl->dir * move.y) * 2.f);
        rb_controller->applyCentralImpulse(vec3_to_bt(up * move.z) * 2.f);

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
            auto dir = glm::normalize(inner.deepest);
            auto proj = glm::dot(dir, bt_to_vec3(rb_controller->getLinearVelocity()));
            if (proj < 0)
                rb_controller->applyCentralImpulse(vec3_to_bt(-120.f * proj * dir));
        }

        // somehow detached, now we're off-structure.
        if (inner.p == 0.f) {
            pl->jump_state = player::off_structure;
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

std::unique_ptr<physics::tether_point_entity>
create_tether_point_on_entity(btSoftRigidDynamicsWorld *world, c_entity entity, btRigidBody *entity_rb, glm::vec3 pos) {
    return std::move(std::make_unique<physics::tether_point_entity>(world, entity, entity_rb, pos));
}

std::unique_ptr<physics::tether_point_rb>
create_tether_point_on_rb(btSoftRigidDynamicsWorld *world, btRigidBody *entity_rb, glm::vec3 pos) {
    return std::move(std::make_unique<physics::tether_point_rb>(world, entity_rb, pos));
}

std::unique_ptr<physics::tether_point_surface>
create_tether_point_on_surface(btSoftRigidDynamicsWorld *world, glm::vec3 pos, glm::ivec3 block, surface_index surf) {
    return std::move(std::make_unique<physics::tether_point_surface>(world, pos, block, surf));
}

void physics::s_tether::attach_to_entity(btSoftRigidDynamicsWorld *world, glm::vec3 point, btRigidBody *entity_rb, c_entity entity) {
    auto tp = create_tether_point_on_entity(world, entity, entity_rb, point);

    switch (state) {
        case attach_state::detached: {
            tether_ends[0].tp = std::move(tp);
            tether_ends[0].point = point;
            state = attach_state::attached_one;
            break;
        }
        case attach_state::attached_one:
        case attach_state::attached_both: {
            tether_ends[1].tp = std::move(tp);
            tether_ends[1].point = point;
            state = attach_state::attached_both;

            build_tether(world);
            break;
        }
    }
}

void physics::s_tether::attach_to_rb(btSoftRigidDynamicsWorld *world, glm::vec3 point, en_rb_controller *rb) {
    auto tp = create_tether_point_on_rb(world, rb, point);

    switch (state) {
        case attach_state::detached: {
            tether_ends[0].tp = std::move(tp);
            tether_ends[0].point = point;
            state = attach_state::attached_one;
            break;
        }
        case attach_state::attached_one:
        case attach_state::attached_both: {
            tether_ends[1].tp = std::move(tp);
            tether_ends[1].point = point;
            state = attach_state::attached_both;

            build_tether(world);
            break;
        }
    }
}

void physics::s_tether::attach_to_surface(btSoftRigidDynamicsWorld *world, glm::vec3 point, glm::ivec3 block, surface_index surf) {
    auto tp = create_tether_point_on_surface(world, point, block, surf);

    switch (state) {
        case attach_state::detached: {
            tether_ends[0].tp = std::move(tp);
            tether_ends[0].point = point;
            state = attach_state::attached_one;
            break;
        }
        case attach_state::attached_one:
        case attach_state::attached_both: {
            tether_ends[1].tp = std::move(tp);
            tether_ends[1].point = point;
            state = attach_state::attached_both;

            build_tether(world);
            break;
        }
    }
}

void physics::s_tether::build_tether(btSoftRigidDynamicsWorld *world) {
    if (state != attach_state::attached_both) {
        return;
    }

    auto &a_end = tether_ends[0];
    auto &b_end = tether_ends[1];

    auto ao = vec3_to_bt(a_end.point);
    auto bo = vec3_to_bt(b_end.point);
    auto resolution = std::max(1.0f, (ao - bo).length() / 0.025f);
    printf("Resolution= %f\n", resolution);
    sb_tether.reset(btSoftBodyHelpers::CreateRope(world->getWorldInfo(), ao, bo, resolution, 0));

    auto capsule = new btSphereShape{0.005f};
    auto size = sb_tether->m_nodes.size() - 1;
    for (int k = 0; k < size; k++) {
        auto node1 = sb_tether->m_nodes[k];
        auto node2 = sb_tether->m_nodes[k + 1];

        auto pos = bt_to_vec3(node1.m_x);
        auto dir = glm::normalize(bt_to_vec3(node2.m_x) - pos);
        auto mat = mat_rotate_mesh(pos, dir);
        btRigidBody *rb = nullptr;
        build_rigidbody(mat, capsule, &rb);

        convert_static_rb_to_dynamic(rb, 1.f / size);

        sb_tether->appendAnchor(k, rb, true);

        rb_tether_pieces.emplace_back(rb);
    }

    sb_tether->setTotalMass(1.f);

    sb_tether->appendAnchor(0, a_end.tp->point.get(), true);
    sb_tether->appendAnchor(sb_tether->m_nodes.size()-1, b_end.tp->point.get(), true);

    sb_tether->m_cfg.kDP = 0.005f;
    sb_tether->m_cfg.kSHR = 1;
    sb_tether->m_cfg.kCHR = 1;
    sb_tether->m_cfg.kKHR = 1;
    sb_tether->m_cfg.piterations = 16;
    sb_tether->m_cfg.citerations = 16;
    sb_tether->m_cfg.diterations = 16;
    sb_tether->m_cfg.viterations = 16;

    world->addSoftBody(sb_tether.get());
}

void physics::s_tether::detach(btSoftRigidDynamicsWorld *world) {
    world->removeSoftBody(sb_tether.get());
    sb_tether.reset();

    for (auto &rb : rb_tether_pieces) {
        world->removeRigidBody(rb.get());
        rb.reset();
    }
    rb_tether_pieces.resize(0);

    tether_ends[0].tp->detach(world);
    tether_ends[1].tp->detach(world);

    tether_ends[0].tp.reset();
    tether_ends[1].tp.reset();

    state = attach_state::detached;
}

bool physics::s_tether::is_attached_to_entity(c_entity entity) {
    auto ts1 = dynamic_cast<physics::tether_point_entity*>(tether_ends[0].tp.get());
    auto ts2 = dynamic_cast<physics::tether_point_entity*>(tether_ends[1].tp.get());

    return (ts1 && ts1->entity == entity) || (ts2 && ts2->entity == entity);
}

bool physics::s_tether::is_attached_to_surface(glm::ivec3 block, surface_index surf, glm::ivec3 other_block, surface_index other_surf) {
    auto ts1 = dynamic_cast<physics::tether_point_surface*>(tether_ends[0].tp.get());
    auto ts2 = dynamic_cast<physics::tether_point_surface*>(tether_ends[1].tp.get());

    return
        ts1 && ((ts1->block == block && ts1->surf == surf) || (ts1->block == other_block && ts1->surf == other_surf)) ||
        (ts2 && ((ts2->block == block && ts2->surf == surf) || (ts2->block == other_block && ts2->surf == other_surf)));

}
