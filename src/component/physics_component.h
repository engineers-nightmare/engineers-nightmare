#pragma once

#include <btBulletDynamicsCommon.h>

#include "component_manager.h"

// physics component
// rigid     -- rigid body
// btRigidBody *
// mesh      -- static mesh
// btTriangleMesh *
// collision -- collision shape
// btCollisionShape *

struct physics_component_manager : component_manager {
    struct power_instance_data {
        c_entity *entity;
        btRigidBody **rigid;
        btTriangleMesh **mesh;
        btCollisionShape **collision;
    } instance_pool;

    void create_component_instance_data(unsigned count) override;

    void destroy_instance(instance i) override;

    void entity(c_entity const &e) override;

    btRigidBody *& rigid(c_entity const &e) {
        auto inst = lookup(e);

        return instance_pool.rigid[inst.index];
    }

    btTriangleMesh *& mesh(c_entity const &e) {
        auto inst = lookup(e);

        return instance_pool.mesh[inst.index];
    }

    btCollisionShape *& collision(c_entity const &e) {
        auto inst = lookup(e);

        return instance_pool.collision[inst.index];
    }
};
