#pragma once

#include <btBulletDynamicsCommon.h>
#include <BulletCollision/CollisionDispatch/btGhostObject.h>
#include <glm/glm.hpp>
#include <memory>
#include "char.h"
#include "component/c_entity.h"
#include "common.h"


/* in player.h */
struct player;


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
    std::unique_ptr<btDiscreteDynamicsWorld> dynamicsWorld;

    player *pl;

    /* character control guff */

    /* initialise our physics state */
    physics(player *pl);

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
build_dynamic_physics_mesh(sw_mesh const * src, btConvexHullShape **shape);

void
teardown_physics_setup(btTriangleMesh **mesh, btConvexHullShape **shape, btRigidBody **rb);

bool
phys_raycast_world(glm::vec3 start, glm::vec3 end,
                   btCollisionObject *ignore, btCollisionWorld *world, raycast_info_world *rc);

static inline glm::vec3
bt_to_vec3(btVector3 const &v)
{
    return glm::vec3(v.x(), v.y(), v.z());
}

static inline btVector3
vec3_to_bt(const glm::vec3 &v)
{
    return btVector3(v.x, v.y, v.z);
}

static inline btTransform
mat4_to_bt(const glm::mat4 &m)
{
    btMatrix3x3 bm(
        m[0][0], m[1][0], m[2][0],
        m[0][1], m[1][1], m[2][1],
        m[0][2], m[1][2], m[2][2] );
    return btTransform(bm, btVector3(m[3][0], m[3][1], m[3][2]));
}

static inline glm::mat4
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
