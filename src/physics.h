#pragma once

#include <btBulletDynamicsCommon.h>
#include <BulletCollision/CollisionDispatch/btGhostObject.h>
#include <BulletDynamics/Character/btKinematicCharacterController.h>

#include "player.h"

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

    /* a pointer to our player object in main
     * static struct {
     *      float angle;
     *      float elev;
     *      glm::vec3 pos;
     *      glm::vec3 dir;
     * } player;
     */
    player *pl;

    /* character control guff */
    btConvexShape *playerShape;
    btPairCachingGhostObject *ghostObj;
    btKinematicCharacterController *controller;

    /* our static `floor` */
    btCollisionShape *groundShape;
    btDefaultMotionState *groundMotionState;
    btRigidBody *groundRigidBody;

    /* initialise our physics state */
    physics(player *pl);

    ~physics();

    /* call each physics tick */
    void tick();
};

