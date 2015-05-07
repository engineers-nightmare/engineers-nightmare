#pragma once

#include <btBulletDynamicsCommon.h>
#include <BulletCollision/CollisionDispatch/btGhostObject.h>
#include <BulletDynamics/Character/btKinematicCharacterController.h>


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
    btConvexShape *playerShape;
    btPairCachingGhostObject *ghostObj;
    btKinematicCharacterController *controller;

    /* initialise our physics state */
    physics(player *pl);

    ~physics();

    /* call each physics tick */
    void tick();
};


struct sw_mesh;


void
build_static_physics_setup(int _x, int _y, int _z, sw_mesh const * src,
                           btTriangleMesh **mesh, btCollisionShape **shape, btRigidBody **rb);


void
teardown_static_physics_setup(btTriangleMesh **mesh, btCollisionShape **shape, btRigidBody **rb);
