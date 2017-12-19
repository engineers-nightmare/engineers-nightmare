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
        collisionConfiguration.get())),
    ghostObj(new btPairCachingGhostObject())
{
    /* some default gravity
     * z is up and down
     */
    this->dynamicsWorld->setGravity(btVector3(0, 0, 0));

    /* store a pointer to our player so physics can drive his position */
    this->pl = p;

    /* setup player rigid body */
    /* player height is 2 * radius + height
     * therefore, height for capsule is
     * total height - 2 * radius
     */
    auto standHeight = std::max(0.f, PLAYER_STAND_HEIGHT - 2 * PLAYER_RADIUS);
    standShape.reset(new btCapsuleShapeZ(PLAYER_RADIUS, standHeight));
    auto crouchHeight = std::max(0.f, PLAYER_CROUCH_HEIGHT - 2 * PLAYER_RADIUS);
    crouchShape.reset(new btCapsuleShapeZ(PLAYER_RADIUS, crouchHeight));
    float maxStepHeight = 0.15f;

    /* setup the character controller. this gets a bit fiddly. */
    btTransform startTransform;
    startTransform.setIdentity();
    startTransform.setOrigin(btVector3(pl->pos.x, pl->pos.y, pl->pos.z));
    ghostObj->setWorldTransform(startTransform);
    broadphase->getOverlappingPairCache()->setInternalGhostPairCallback(new btGhostPairCallback());
    ghostObj->setCollisionShape(standShape.get());
    ghostObj->setCollisionFlags(btCollisionObject::CF_CHARACTER_OBJECT);
    controller.reset(new en_char_controller(ghostObj.get(), standShape.get(), crouchShape.get(), btScalar(maxStepHeight)));

    dynamicsWorld->addCollisionObject(ghostObj.get(), btBroadphaseProxy::CharacterFilter,
            btBroadphaseProxy::StaticFilter|btBroadphaseProxy::DefaultFilter);
    dynamicsWorld->addAction(controller.get());
    controller->setUpAxis(2);
}

void
physics::tick_controller(float dt)
{
    /* messy input -> char controller binding
     * TODO: untangle.
     */

    float c = cosf(this->pl->angle);
    float s = sinf(this->pl->angle);

    btVector3 fwd(c, s, 0);
    btVector3 right(s, -c, 0);

    if (pl->gravity) {
        pl->disable_gravity ^= true;

        if( this->pl->disable_gravity ){
            this->controller->setGravity(0);
        } else {
            /* http://bulletphysics.org/Bullet/BulletFull/btKinematicCharacterController_8cpp_source.html : 144
             * 3G acceleration.
             */
            this->controller->setGravity(9.8f * 3);
        }

        pl->ui_dirty = true;
    }

    float speed = MOVE_SPEED;
    if (!this->controller->onGround())
        speed *= AIR_CONTROL_FACTOR;
    else if (this->controller->isCrouching())
        speed *= CROUCH_FACTOR;

    pl->height = this->controller->isCrouching() ? PLAYER_CROUCH_HEIGHT : PLAYER_STAND_HEIGHT;

    fwd *= this->pl->move.y * speed;
    right *= this->pl->move.x * speed;

    this->controller->setWalkDirection(fwd + right);

    if (pl->jump && this->controller->onGround())
        this->controller->jump();

    if (pl->reset) {
        /* reset position (for debug) */
        this->controller->warp(btVector3(3.0f, 2.0f, 3.0f));
    }

    if (pl->crouch) {
        this->controller->crouch(dynamicsWorld.get());
    }
    else if (pl->crouch_end) {
        this->controller->crouchEnd();
    }
}

void
physics::tick(float dt)
{
    dynamicsWorld->stepSimulation(dt, 10);

    btTransform trans = this->ghostObj->getWorldTransform();

    this->pl->pos.x = trans.getOrigin().getX();
    this->pl->pos.y = trans.getOrigin().getY();
    this->pl->pos.z = trans.getOrigin().getZ();
}

void BulletDebugDraw::drawSphere(const btVector3 &p, btScalar radius, const btVector3 &color) {
    dd::sphere(glm::value_ptr(bt_to_vec3(p)), color, radius);
}

void BulletDebugDraw::drawLine(const btVector3 &from, const btVector3 &to, const btVector3 &color) {
    dd::line(glm::value_ptr(bt_to_vec3(from)), glm::value_ptr(bt_to_vec3(to)), color);

}

void BulletDebugDraw::drawContactPoint(const btVector3 &PointOnB, const btVector3 &normalOnB, btScalar distance,
                                       int lifeTime, const btVector3 &color) {

}

void BulletDebugDraw::reportErrorWarning(const char *warningString) {

}

void BulletDebugDraw::draw3dText(const btVector3 &location, const char *textString) {

}

void BulletDebugDraw::setDebugMode(int debugMode) {

}

int BulletDebugDraw::getDebugMode() const {
    return 1;
}
