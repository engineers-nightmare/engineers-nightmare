#include <btBulletDynamicsCommon.h>
#include <stdio.h>

#include "player.h"
#include "physics.h"

#define GROUND_Z 1
#define PLAYER_START_X 4
#define PLAYER_START_Y 4
#define PLAYER_START_Z 50

#define MOVE_SPEED  0.5

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

    /* set player height to physics height */
    pl->pos.x = PLAYER_START_X;
    pl->pos.y = PLAYER_START_Y;
    pl->pos.z = PLAYER_START_Z;


    /* setup player rigid body */
    this->playerShape = new btSphereShape(0.7);
    float maxStepHeight = 0.5f;

    /* setup the character controller. this gets a bit fiddly. */
    btTransform startTransform;
    startTransform.setIdentity();
    startTransform.setOrigin(btVector3(pl->pos.x, pl->pos.y, pl->pos.z));
    this->ghostObj = new btPairCachingGhostObject();
    this->ghostObj->setWorldTransform(startTransform);
    this->broadphase->getOverlappingPairCache()->setInternalGhostPairCallback(new btGhostPairCallback());
    this->ghostObj->setCollisionShape(this->playerShape);
    this->ghostObj->setCollisionFlags(btCollisionObject::CF_CHARACTER_OBJECT);
    this->controller = new btKinematicCharacterController(this->ghostObj, this->playerShape, btScalar(maxStepHeight));

    this->dynamicsWorld->addCollisionObject(this->ghostObj, btBroadphaseProxy::CharacterFilter,
            btBroadphaseProxy::StaticFilter|btBroadphaseProxy::DefaultFilter);
    this->dynamicsWorld->addAction(this->controller);
    this->controller->setUpAxis(2);


    /* setup world static floor */
    /* we want our plane at z = 1 */
    this->groundShape = new btStaticPlaneShape(btVector3(0, 0, GROUND_Z), 1);
    this->groundMotionState =
        new btDefaultMotionState(btTransform(btQuaternion(0, 0, 0, 1),
                                             btVector3(0, 0, 0)));
    btRigidBody::btRigidBodyConstructionInfo
                    groundRigidBodyCI(0, groundMotionState, groundShape, btVector3(0, 0, 0));
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
    delete(this->ghostObj);
    delete(this->controller);

    delete(this->groundShape);
    delete(this->groundMotionState);
    delete(this->groundRigidBody);
}

void
physics::tick(){
    double x, y, z;

    double c = cos(this->pl->angle);
    double s = sin(this->pl->angle);

    btVector3 fwd(c, s, 0);
    btVector3 right(s, -c, 0);

    fwd *= this->pl->move.y * MOVE_SPEED;
    right *= this->pl->move.x * MOVE_SPEED;

    this->controller->setWalkDirection(fwd + right);

    if (!pl->last_jump && pl->jump && this->controller->canJump())
        this->controller->jump();

    dynamicsWorld->stepSimulation(1 / 60.f, 10);

    btTransform trans = this->ghostObj->getWorldTransform();
    x = trans.getOrigin().getX();
    y = trans.getOrigin().getY();
    z = trans.getOrigin().getZ();

    /* FIXME assign trans x, y, z to player */
    this->pl->pos.x = x;
    this->pl->pos.y = y;
    this->pl->pos.z = z;

//    printf("The player is now at x '%f', y '%f', z '%f'\n", x, y, z);
}

