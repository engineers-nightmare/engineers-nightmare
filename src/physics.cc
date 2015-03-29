#include <btBulletDynamicsCommon.h>
#include <stdio.h>

#include "player.h"
#include "physics.h"

/* a simple constructor hacked together based on
 * http://bulletphysics.org/mediawiki-1.5.8/index.php/Hello_World
 */
physics::physics(player *pl){
    /* a broadspace filters out obvious non-colliding pairs
     * before the more expensive collision detection algorithm sees them
     *
     * FIXME currently using default broadphase
     * see http://bulletphysics.org/mediawiki-1.5.8/index.php/Broadphase
     * for more information
     */
    this->broadphase = new btDbvtBroadphase();

    this->collisionConfiguration = new btDefaultCollisionConfiguration();
    this->dispatcher = new btCollisionDispatcher(this->collisionConfiguration);

    /* the magic sauce that makes everything else work */
    this->solver = new btSequentialImpulseConstraintSolver;

    /* our actual world */
    this->dynamicsWorld =
        new btDiscreteDynamicsWorld(this->dispatcher,
                                    this->broadphase,
                                    this->solver,
                                    this->collisionConfiguration);

    /* some default gravity
     * z is up and down
     */
    this->dynamicsWorld->setGravity(btVector3(0, 0, -10));

    /* store a pointer to our player so physics can drive his position */
    this->pl= pl;

    /* FIXME set player height to physics height */
    pl->pos.x = 4;
    pl->pos.y = 4;
    pl->pos.z = 50;

    /* FIXME setup player rigid body */
    this->playerShape = new btSphereShape(1);
    /* we start our player at z50 */
    this->playerMotionState =
        new btDefaultMotionState(btTransform(btQuaternion(0, 0, 0, 1),
                                             btVector3(4, 4, 50)));
    btScalar mass = 1;
    btVector3 playerInertia(0, 0, 0);
    this->playerShape->calculateLocalInertia(mass, playerInertia);

    /* FIXME eewww */
    btRigidBody::btRigidBodyConstructionInfo playerRigidBodyCI(mass, playerMotionState, playerShape, playerInertia);
    /* FIXME eewww */
    this->playerRigidBody = new btRigidBody(playerRigidBodyCI);
    this->dynamicsWorld->addRigidBody(this->playerRigidBody);

    /* FIXME setup world static floor */
    /* we want our plane at z = 1 */
    this->groundShape = new btStaticPlaneShape(btVector3(0, 0, 1), 1);
    this->groundMotionState =
        new btDefaultMotionState(btTransform(btQuaternion(0, 0, 0, 1),
                                             btVector3(0, 0, 1)));
    /* FIXME eewww */
    btRigidBody::btRigidBodyConstructionInfo
                    groundRigidBodyCI(0, groundMotionState, groundShape, btVector3(0, 0, 0));
    /* FIXME eewww */
    this->groundRigidBody = new btRigidBody(groundRigidBodyCI);
    this->dynamicsWorld->addRigidBody(this->groundRigidBody);


}

physics::~physics(){
    delete(this->broadphase);

    delete(this->collisionConfiguration);
    delete(this->dispatcher);

    delete(this->solver);

    delete(this->dynamicsWorld);

    delete(this->playerShape);
    delete(this->playerMotionState);
    delete(this->playerRigidBody);

    delete(this->groundShape);
    delete(this->groundMotionState);
    delete(this->groundRigidBody);
}

void
physics::tick(){
    double x, y, z;

    dynamicsWorld->stepSimulation(1 / 60.f, 10);

    btTransform trans;
    this->playerRigidBody->getMotionState()->getWorldTransform(trans);
    x = trans.getOrigin().getX();
    y = trans.getOrigin().getY();
    z = trans.getOrigin().getZ();

    /* FIXME assign trans x, y, z to player */
    this->pl->pos.x = x;
    this->pl->pos.y = y;
    this->pl->pos.z = z;

    // printf("The player is now at x '%f', y '%f', z '%f'\n", x, y, z);
}

