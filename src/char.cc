/*
   Bullet Continuous Collision Detection and Physics Library
   Copyright (c) 2003-2008 Erwin Coumans  http://bulletphysics.com

   This software is provided 'as-is', without any express or implied warranty.
   In no event will the authors be held liable for any damages arising from the use of this software.
   Permission is granted to anyone to use this software for any purpose, 
   including commercial applications, and to alter it and redistribute it freely, 
   subject to the following restrictions:

   1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
   2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
   3. This notice may not be removed or altered from any source distribution.
   */

/* Modified and extended for Engineer's Nightmare by chrisf */


#include "LinearMath/btIDebugDraw.h"
#include "BulletCollision/CollisionDispatch/btGhostObject.h"
#include "BulletCollision/BroadphaseCollision/btCollisionAlgorithm.h"

#include "physics.h"

// static helper method
    static btVector3
getNormalizedVector(const btVector3& v)
{
    btVector3 n(0, 0, 0);

    if (v.length() > SIMD_EPSILON) {
        n = v.normalized();
    }
    return n;
}


///@todo Interact with dynamic objects,
///Ride kinematicly animated platforms properly
///More realistic (or maybe just a config option) falling
/// -> Should integrate falling velocity manually and use that in stepDown()
///Support jumping
///Support ducking
class en_ray_result_callback : public btCollisionWorld::ClosestRayResultCallback
{
public:
    en_ray_result_callback(btCollisionObject* me, btVector3 const & from, btVector3 const &to)
        : btCollisionWorld::ClosestRayResultCallback(from, to),
        m_me(me) {}

    btScalar addSingleResult(btCollisionWorld::LocalRayResult& rayResult, bool normalInWorldSpace) override {
        if (rayResult.m_collisionObject == m_me)
            return 1.0;

        return ClosestRayResultCallback::addSingleResult (rayResult, normalInWorldSpace);
    }

protected:
    btCollisionObject* m_me;
};

class en_convex_result_callback : public btCollisionWorld::ClosestConvexResultCallback
{
public:
    en_convex_result_callback (btCollisionObject* me, const btVector3& up, btScalar minSlopeDot)
        : btCollisionWorld::ClosestConvexResultCallback(btVector3(0.0, 0.0, 0.0), btVector3(0.0, 0.0, 0.0)),
        m_me(me),
        m_up(up),
        m_minSlopeDot(minSlopeDot) {}

    virtual btScalar addSingleResult(btCollisionWorld::LocalConvexResult& convexResult,bool normalInWorldSpace) override {
        if (convexResult.m_hitCollisionObject == m_me)
            return btScalar(1.0);

        if (!convexResult.m_hitCollisionObject->hasContactResponse())
            return btScalar(1.0);

        btVector3 hitNormalWorld;
        if (normalInWorldSpace)
        {
            hitNormalWorld = convexResult.m_hitNormalLocal;
        } else
        {
            ///need to transform normal into worldspace
            hitNormalWorld = convexResult.m_hitCollisionObject->getWorldTransform().getBasis()*convexResult.m_hitNormalLocal;
        }

        btScalar dotUp = m_up.dot(hitNormalWorld);
        if (dotUp < m_minSlopeDot) {
            return btScalar(1.0);
        }

        return ClosestConvexResultCallback::addSingleResult (convexResult, normalInWorldSpace);
    }
protected:
    btCollisionObject* m_me;
    const btVector3 m_up;
    btScalar m_minSlopeDot;
};

/* Not part of CC, but reuses the same callbacks etc. */
bool
phys_raycast_world(glm::vec3 start, glm::vec3 end, btCollisionObject *ignore, btCollisionWorld *world,
                   raycast_info_world *rc)
{
    rc->hit = false;
    rc->entity = c_entity{0};

    btVector3 start_bt = vec3_to_bt(start);
    btVector3 end_bt = vec3_to_bt(end);
    en_ray_result_callback callback(ignore, start_bt, end_bt);
    world->rayTest(start_bt, end_bt, callback);

    if (callback.hasHit()) {
        rc->hit = true;
        rc->hitCoord = bt_to_vec3(callback.m_hitPointWorld);
        rc->hitNormal = bt_to_vec3(callback.m_hitNormalWorld);
        if (callback.m_collisionObject->getUserPointer()) {
            rc->entity = ((phys_ent_ref *) callback.m_collisionObject->getUserPointer())->ce;
        }
    }

    return rc->hit;
}
