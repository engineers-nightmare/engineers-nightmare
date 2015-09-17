#pragma once

#include <btBulletDynamicsCommon.h>

#include "component_manager.h"

// physics component
// rigid     -- rigid body
// btRigidBody *

struct physics_component_manager : component_manager {
    struct instance_data {
        c_entity *entity;
        btRigidBody **rigid;
    } instance_pool;

    void create_component_instance_data(unsigned count) override;

    void destroy_instance(instance i) override;

    void entity(c_entity const &e) override;

    btRigidBody *& rigid(c_entity const &e) {
        auto inst = lookup(e);

        return instance_pool.rigid[inst.index];
    }
};
