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
    std::unique_ptr<btConvexShape> standShape, crouchShape;
    std::unique_ptr<btPairCachingGhostObject> ghostObj;
    std::unique_ptr<en_char_controller> controller;

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
build_static_physics_rb(int x, int y, int z, btCollisionShape *shape, btRigidBody **rb);

void
build_static_physics_rb_mat(glm::mat4 *m, btCollisionShape *shape, btRigidBody **rb);


void
build_static_physics_mesh(sw_mesh const * src, btTriangleMesh **mesh, btCollisionShape **shape);


void
teardown_static_physics_setup(btTriangleMesh **mesh, btCollisionShape **shape, btRigidBody **rb);

bool
phys_raycast_entity(glm::vec3 start, glm::vec3 end,
                    btCollisionObject *ignore, btCollisionWorld *world, raycast_info_entity *rc);

bool
phys_raycast_generic(glm::vec3 start, glm::vec3 end,
                     btCollisionObject *ignore, btCollisionWorld *world, raycast_info_generic *rc);

static inline glm::vec3
bt_to_glm(btVector3 const &v)
{
    return glm::vec3(v.x(), v.y(), v.z());
}

static inline btVector3
glm_to_bt(glm::vec3 v)
{
    return btVector3(v.x, v.y, v.z);
}
