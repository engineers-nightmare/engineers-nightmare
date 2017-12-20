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


#pragma once

#include <LinearMath/btVector3.h>

extern btTransform mat4_to_bt(const glm::mat4 &m);

ATTRIBUTE_ALIGNED16(class) en_rb_controller : public btRigidBody {
public:
    explicit en_rb_controller(const glm::mat4 &m) :
                    btRigidBody(72.0f, new btDefaultMotionState(btTransform(mat4_to_bt(m))),
                    new btSphereShape(0.23f)) {
        setSleepingThresholds(0, 0);
    }
};

