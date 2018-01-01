#pragma once

#include <LinearMath/btVector3.h>

extern btTransform mat4_to_bt(const glm::mat4 &m);

ATTRIBUTE_ALIGNED16(class) en_rb_controller : public btRigidBody {
public:
    explicit en_rb_controller(const glm::mat4 &m) :
                    btRigidBody(72.0f, new btDefaultMotionState(btTransform(mat4_to_bt(m))),
                    new btSphereShape(0.42f)) {
        setSleepingThresholds(0, 0);
    }
};

