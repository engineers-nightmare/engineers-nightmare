#pragma once

#include <LinearMath/btIDebugDraw.h>

#include "glm/glm/ext.hpp"
#include "utils/debugdraw.h"

class BulletDebugDraw : public btIDebugDraw
{
public:
    void drawSphere(const btVector3 &p, btScalar radius, const btVector3 &color) override {
        dd::sphere(glm::value_ptr(bt_to_vec3(p)), color, radius);
    }

    void drawLine(const btVector3 &from, const btVector3 &to, const btVector3 &color) override {
        dd::line(glm::value_ptr(bt_to_vec3(from)), glm::value_ptr(bt_to_vec3(to)), color);

    }

    void drawContactPoint(const btVector3 &PointOnB, const btVector3 &normalOnB, btScalar distance,
        int lifeTime, const btVector3 &color) override {

    }

    void reportErrorWarning(const char *warningString) override {

    }

    void draw3dText(const btVector3 &location, const char *textString) override {

    }

    void setDebugMode(int debugMode) override {

    }

    int getDebugMode() const override {
        return 1;
    }

};
