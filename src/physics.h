#pragma once

#include <btBulletDynamicsCommon.h>
#include <BulletCollision/CollisionDispatch/btGhostObject.h>
#include <BulletSoftBody/btSoftRigidDynamicsWorld.h>
#include <glm/glm.hpp>
#include <memory>
#include <vector>

#include "char.h"
#include "component/c_entity.h"
#include "common.h"
#include "block.h"

/* in player.h */
struct player;

extern btVector3 vec3_to_bt(const glm::vec3 &v);
extern btTransform mat4_to_bt(const glm::mat4 &m);

ATTRIBUTE_ALIGNED16(class) en_rb_tether_point : public btRigidBody {
public:
    static constexpr float radius = 0.01f;
    explicit en_rb_tether_point(const glm::mat4 &m) : btRigidBody(1.1f, new btDefaultMotionState(btTransform(mat4_to_bt(m))), new btSphereShape(radius), btVector3(0, 0, 0)) {
        setCollisionFlags(getCollisionFlags() & ~btCollisionObject::CF_STATIC_OBJECT);
//        btVector3 inertia(0, 0, 0);
//        getCollisionShape()->calculateLocalInertia(0.01f, inertia);
//        setMassProps(0.01f, inertia);
        activate();
    }
};

/* a simple physics world hacked together by referencing
 * http://bulletphysics.org/mediawiki-1.5.8/index.php/Hello_World
 */
struct physics {
    /* all of our incantation state
     * FIXME we probably don't need to keep half of this
     */
    std::unique_ptr<btBroadphaseInterface> broadphase;
    std::unique_ptr<btDefaultCollisionConfiguration> collisionConfiguration;
    std::unique_ptr<btCollisionDispatcher> dispatcher;
    std::unique_ptr<btSequentialImpulseConstraintSolver> solver;
    std::unique_ptr<btSoftRigidDynamicsWorld> dynamicsWorld;

    player *pl;

    /* character control guff */
    std::unique_ptr<en_rb_controller> rb_controller;

    struct tether_point {
        std::unique_ptr<en_rb_tether_point> point{};
        std::unique_ptr<btPoint2PointConstraint> constraint{};
        virtual void detach(btSoftRigidDynamicsWorld *world) {
            world->removeConstraint(constraint.get());
            world->removeRigidBody(point.get());
            constraint.reset();
            point.reset();
        };
    };

    struct tether_point_entity : tether_point {
        c_entity entity;

        tether_point_entity(btSoftRigidDynamicsWorld *world, c_entity entity, btRigidBody *entity_rb, glm::vec3 p) : entity(entity) {
            point = std::make_unique<en_rb_tether_point>(mat_position(p));
            world->addRigidBody(point.get());
            constraint = std::make_unique<btPoint2PointConstraint>(*entity_rb, *point.get(), vec3_to_bt(p) - entity_rb->getWorldTransform().getOrigin(), btVector3(0, 0, 0));
            constraint->setDbgDrawSize(5.0f);

            world->addConstraint(constraint.get(), false);
        }

        void detach(btSoftRigidDynamicsWorld *world) override {
            tether_point::detach(world);
            entity.id = 0;
        }
    };

    struct tether_point_rb : tether_point {
        tether_point_rb(btSoftRigidDynamicsWorld *world, btRigidBody *entity_rb, glm::vec3 p) {
            point = std::make_unique<en_rb_tether_point>(mat_position(p));
            world->addRigidBody(point.get());
            constraint = std::make_unique<btPoint2PointConstraint>(*entity_rb, *point.get(), vec3_to_bt(p) - entity_rb->getWorldTransform().getOrigin(), btVector3(0, 0, 0));
            constraint->setDbgDrawSize(5.0f);

            world->addConstraint(constraint.get(), false);
        }

        void detach(btSoftRigidDynamicsWorld *world) override {
            tether_point::detach(world);
        }
    };

    struct tether_point_surface : tether_point {
        glm::ivec3 block;
        surface_index surf;

        tether_point_surface(btSoftRigidDynamicsWorld *world, glm::vec3 p, glm::ivec3 bl, surface_index s) : block(bl), surf(s) {
            point = std::make_unique<en_rb_tether_point>(mat_position(p));
            world->addRigidBody(point.get());
            constraint = std::make_unique<btPoint2PointConstraint>(*point.get(), btVector3(0, 0, 0));
            constraint->setDbgDrawSize(5.0f);

            world->addConstraint(constraint.get(), false);
        }

        void detach(btSoftRigidDynamicsWorld *world) override {
            tether_point::detach(world);
        }
    };

    class s_tether {
        std::unique_ptr<btSoftBody> sb_tether;
        std::vector<std::unique_ptr<btRigidBody>> rb_tether_pieces;

        struct tether_end {
            std::unique_ptr<tether_point> tp;
            glm::vec3 point;
        };
        std::array<tether_end, 2> tether_ends;

        enum class attach_state {
            detached,
            attached_one,
            attached_both,
        } state = attach_state::detached;

        void build_tether(btSoftRigidDynamicsWorld *world);

    public:
        enum class side {
            none,
            one,
            two,
        };

        bool is_attached() { return state == attach_state::attached_both; }
        btSoftBody *get_softbody() { return sb_tether.get(); }
        void attach_to_entity(btSoftRigidDynamicsWorld *world, glm::vec3 point, btRigidBody *entity_rb, c_entity entity);
        void attach_to_surface(btSoftRigidDynamicsWorld *world, glm::vec3 point, glm::ivec3 block, surface_index surf);
        void detach(btSoftRigidDynamicsWorld *world);

        bool is_attached_to_entity(c_entity entity);

        bool is_attached_to_surface(glm::ivec3 block, surface_index surf, glm::ivec3 other_block, surface_index other_surf);

        void attach_to_rb(btSoftRigidDynamicsWorld *world, glm::vec3 point, en_rb_controller *rb);
    } tether;

    /* initialise our physics state */
    explicit physics(player *pl);

    /* call each physics tick */
    void tick_controller(float dt);
    void tick(float dt);
};


struct sw_mesh;

/* Identifies an entity and a mesh/meshpart, to be referenced by the physics system. */
struct phys_ent_ref
{
    c_entity ce;
};

void
build_rigidbody(const glm::mat4 &m, btCollisionShape *shape, btRigidBody **rb);

void convert_static_rb_to_dynamic(btRigidBody *rb, float mass);

void
build_static_physics_mesh(sw_mesh const * src, btTriangleMesh **mesh, btCollisionShape **shape);

void
build_dynamic_physics_mesh(sw_mesh const * src, btCollisionShape **shape);

void
teardown_physics_setup(btTriangleMesh **mesh, btCollisionShape **shape, btRigidBody **rb);

bool
phys_raycast_world(glm::vec3 start, glm::vec3 end,
                   btCollisionObject *ignore, btCollisionWorld *world, raycast_info_world *rc);

inline glm::vec3
bt_to_vec3(btVector3 const &v)
{
    return glm::vec3(v.x(), v.y(), v.z());
}

inline btVector3
vec3_to_bt(const glm::vec3 &v)
{
    return btVector3(v.x, v.y, v.z);
}

inline btTransform
mat4_to_bt(const glm::mat4 &m)
{
    btMatrix3x3 bm(
        m[0][0], m[1][0], m[2][0],
        m[0][1], m[1][1], m[2][1],
        m[0][2], m[1][2], m[2][2] );
    return btTransform(bm, btVector3(m[3][0], m[3][1], m[3][2]));
}

inline glm::mat4
bt_to_mat4(const btTransform &t)
{
    btVector3 v0 = t.getBasis().getColumn(0);
    btVector3 v1 = t.getBasis().getColumn(1);
    btVector3 v2 = t.getBasis().getColumn(2);
    btVector3 v3 = t.getOrigin();

    glm::mat4 m(
        v0.x(), v0.y(), v0.z(), 0.f,
        v1.x(), v1.y(), v1.z(), 0.f,
        v2.x(), v2.y(), v2.z(), 0.f,
        v3.x(), v3.y(), v3.z(), 1.f );
    return m;
}
