#pragma once

#include <btBulletDynamicsCommon.h>
#include <BulletCollision/CollisionDispatch/btGhostObject.h>
#include <glm/glm.hpp>
#include "char.h"
#include "component/c_entity.h"


/* in player.h */
struct player;


/* a simple physics world hacked together by referencing
 * http://bulletphysics.org/mediawiki-1.5.8/index.php/Hello_World
 */
struct physics {
    /* all of our incantation state
     * FIXME we probably don't need to keep half of this
     */
    btBroadphaseInterface *broadphase;
    btDefaultCollisionConfiguration *collisionConfiguration;
    btCollisionDispatcher *dispatcher;
    btSequentialImpulseConstraintSolver *solver;
    btDiscreteDynamicsWorld *dynamicsWorld;

    player *pl;

    /* character control guff */
    btConvexShape *standShape;
    btConvexShape *crouchShape;
    btPairCachingGhostObject *ghostObj;
    en_char_controller *controller;

    /* initialise our physics state */
    physics(player *pl);

    ~physics();

    /* call each physics tick */
    void tick_controller(float dt);
    void tick(float dt);
};


struct sw_mesh;


struct generic_raycast_info {
    bool hit;
    glm::vec3 hitCoord;         /* world hit coord */
    glm::vec3 hitNormal;        /* world hit normal */
    glm::vec3 toHit;
    glm::vec3 fromHit;
};

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


phys_ent_ref *
phys_raycast(glm::vec3 start, glm::vec3 end,
             btCollisionObject *ignore, btCollisionWorld *world);

generic_raycast_info
phys_raycast_generic(glm::vec3 start, glm::vec3 end,
                     btCollisionObject *ignore, btCollisionWorld *world);

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
