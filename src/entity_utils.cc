//
// Created by caleb on 11/23/17.
//

#include <libconfig.h>
#include <glm/ext.hpp>
#include "libconfig_shim.h"
#include "entity_utils.h"
#include "asset_manager.h"
#include "component/component_system_manager.h"
#include "tinydir.h"

extern ship_space *ship;

extern asset_manager asset_man;
extern component_system_manager component_system_man;
extern player pl;

std::vector<std::string> entity_names{};
std::unordered_map<std::string, entity_data> entity_stubs{};

void
load_entities() {
    std::vector<std::string> files;

    tinydir_dir dir{};
    tinydir_open(&dir, "entities");

    while (dir.has_next)
    {
        tinydir_file file{};
        tinydir_readfile(&dir, &file);

        tinydir_next(&dir);

        if (file.is_dir || strcmp(file.extension, "ent") != 0) {
            continue;
        }

        files.emplace_back(file.path);
    }

    tinydir_close(&dir);

    auto failed_load = false;

    for (auto &f : files) {
        config_t cfg{};
        config_setting_t *entity_config_setting = nullptr;
        config_init(&cfg);

        if (!config_read_file(&cfg, f.c_str())) {
            printf("%s:%d - %s reading %s\n", config_error_file(&cfg),
                   config_error_line(&cfg), config_error_text(&cfg), f.c_str());

            config_destroy(&cfg);

            assert(false);
        }

        entity_data entity{};

        // required to be a valid entity
        std::unordered_map<std::string, bool> required_dependencies;
        auto req_deps = component_stub::get_required_dependencies();
        for (auto &&dep : req_deps) {
            required_dependencies[dep] = false;
        }

        // component-defined dependencies
        std::unordered_map<std::string, std::unordered_map<std::string, bool>> dependencies;

        printf("Loading entity from %s\n", f.c_str());

        entity_config_setting = config_lookup(&cfg, "entity");
        if (entity_config_setting != nullptr) {
            /* http://www.hyperrealm.com/libconfig/libconfig_manual.html
            * states
            *  > int config_setting_length (const config_setting_t * setting)
            *  > This function returns the number of settings in a group,
            *  > or the number of elements in a list or array.
            *  > For other types of settings, it returns 0.
            *
            * so the count can only ever be positive, despite the return type being int
            */
            auto components = config_setting_lookup(entity_config_setting, "components");
            auto components_count = (unsigned)config_setting_length(components);

            for (unsigned i = 0; i < components_count; ++i) {
                auto component = config_setting_get_elem(components, i);
                printf("  Component: %s\n", component->name);

                auto stub_ptr = component_system_man.managers.get_stub(component->name, component).release();

                auto type_stub = dynamic_cast<type_component_stub*>(stub_ptr);
                if (type_stub) {
                    entity.name = type_stub->name;
                }

                auto deps = stub_ptr->get_dependencies();
                dependencies[component->name] = {};
                for (auto &&dep : deps) {
                    dependencies[component->name][dep] = false;
                }

                entity.components.emplace_back(stub_ptr);
            }

            std::set<std::string> keys;
            for (auto &&comp : dependencies) {
                keys.insert(comp.first);
            }

            for (auto &&comp : dependencies) {
                for (auto &&dep : comp.second) {
                    dep.second = keys.count(dep.first) != 0;
                }
            }

            for (auto &&dep: required_dependencies) {
                dep.second = keys.count(dep.first) != 0;
            }

            auto valid = true;
            for (auto &&comp : dependencies) {
                for (auto &&dep : comp.second) {
                    if (!dep.second) {
                        valid = false;
                        failed_load = true;
                        printf ("!! Entity \"%s\" in %s missing '%s' component dependency [%s]\n", entity.name.c_str(), f.c_str(), comp.first.c_str(), dep.first.c_str());
                    }
                }
            }

            for (auto &req : required_dependencies) {
                if (!req.second) {
                    printf("!! Entity \"%s\" in %s missing required [%s] component\n", entity.name.c_str(), f.c_str(), req.first.c_str());
                    valid = false;
                    failed_load = true;
                }
            }

            if (valid) {
                entity_stubs[entity.name] = std::move(entity);
            }
        }

        config_destroy(&cfg);
    }

    assert(failed_load != true);

    for (auto &entity : entity_stubs) {
        std::string name = entity.first;
        entity_names.push_back(name);
    }
}

unsigned c_entity::entities_id_ref = 1;

c_entity
spawn_entity(const std::string &name, glm::mat4 mat) {
    auto ce = c_entity::spawn();

    auto & entity = entity_stubs[name];

    auto render_stub = entity.get_component<renderable_component_stub>();
    assert(render_stub);

    for (auto &comp : entity.components) {
        comp->assign_component_to_entity(ce);
    }

    auto &pos_man = component_system_man.managers.position_component_man;
    auto &physics_man = component_system_man.managers.physics_component_man;

    if (physics_man.exists(ce)) {
        auto physics = physics_man.get_instance_data(ce);
        *physics.rigid = nullptr;
        std::string m = *physics.mesh;
        auto const &phys_mesh = asset_man.get_mesh(m);
        build_rigidbody(mat, phys_mesh.phys_shape, physics.rigid);
        /* so that we can get back to the entity from a phys raycast */
        /* TODO: these should really come from a dense pool rather than the generic allocator */
        auto per = new phys_ent_ref;
        per->ce = ce;
        (*physics.rigid)->setUserPointer(per);
    }

    auto pos = pos_man.get_instance_data(ce);
    *pos.mat = mat;

    return ce;
}

void
attach_entity_to_surface(c_entity ce, glm::ivec3 p, int face) {
    auto &surface_man = component_system_man.managers.surface_attachment_component_man;
    if (surface_man.exists(ce)) {
        auto surface = surface_man.get_instance_data(ce);
        *surface.block = p;
        *surface.face = face;
        *surface.attached = true;
    }
}

extern physics *phy;
c_entity
spawn_floating_generic_entity(glm::mat4 mat, const std::string &mesh, const std::string &phys_mesh) {
    auto ce = c_entity::spawn();

    auto &pos_man = component_system_man.managers.position_component_man;
    auto &physics_man = component_system_man.managers.physics_component_man;
    auto &surface_man = component_system_man.managers.surface_attachment_component_man;
    auto &render_man = component_system_man.managers.renderable_component_man;
    auto &type_man = component_system_man.managers.type_component_man;

    pos_man.assign_entity(ce);
    physics_man.assign_entity(ce);
    surface_man.assign_entity(ce);
    render_man.assign_entity(ce);
    type_man.assign_entity(ce);

    auto physics = physics_man.get_instance_data(ce);
    *physics.rigid = nullptr;
    *physics.mesh = phys_mesh.c_str();
    *physics.mass = 0.2f;
    auto const &pm = asset_man.get_mesh(phys_mesh);
    build_rigidbody(mat, pm.phys_shape, physics.rigid);
    /* so that we can get back to the entity from a phys raycast */
    /* TODO: these should really come from a dense pool rather than the generic allocator */
    auto per = new phys_ent_ref;
    per->ce = ce;
    (*physics.rigid)->setUserPointer(per);

    auto surface = surface_man.get_instance_data(ce);
    *surface.attached = false;

    auto pos = pos_man.get_instance_data(ce);
    *pos.mat = mat;

    auto render = render_man.get_instance_data(ce);
    *render.mesh = mesh.c_str();
    *render.draw = true;

    auto type = type_man.get_instance_data(ce);
    // todo: fix this
    *type.name = "Generic";

    pop_entity_off(ce);

    return ce;
}

void
destroy_entity(c_entity e) {
    // clean up `e` itself
    auto &physics_man = component_system_man.managers.physics_component_man;

    if (physics_man.exists(e)) {
        auto phys_data = physics_man.get_instance_data(e);
        delete (phys_ent_ref *)(*phys_data.rigid)->getUserPointer();
        teardown_physics_setup(nullptr, nullptr, phys_data.rigid);
    }

    component_system_man.managers.destroy_entity_instance(e);

    auto &parent_man = component_system_man.managers.parent_component_man;
    // clean up the whole hierarchy descended from `e`
    for (auto i = 0u; i < parent_man.buffer.num; /* */) {
        if (parent_man.instance_pool.parent[i] == e) {
            destroy_entity(parent_man.instance_pool.entity[i]);
        }
        else {
            ++i;
        }
    }
}

void pop_entity_off(c_entity entity) {
    auto &convert = component_system_man.managers.convert_on_pop_component_man;
    auto &pos = component_system_man.managers.position_component_man;
    auto &phys = component_system_man.managers.physics_component_man;

    if (convert.exists(entity)) {
        // rip out the matrix, despawn the old entity, spawn one in its place of the target type.
        // this path is used for large entities which get packed up into "kits" when popped.
        auto target = *convert.get_instance_data(entity).type;
        auto mat = *pos.get_instance_data(entity).mat;
        destroy_entity(entity);
        auto new_ent = spawn_entity(target, mat);
        auto ph = phys.get_instance_data(new_ent);
        convert_static_rb_to_dynamic(*ph.rigid, *ph.mass);
    }
    else {
        // detach the existing entity retaining the rest of its state. this path is used for small
        // entities whose popped representation is the same as their attached representation.
        auto &sam = component_system_man.managers.surface_attachment_component_man;
        auto ph = phys.get_instance_data(entity);
        auto sa = sam.get_instance_data(entity);
        *sa.attached = false;
        convert_static_rb_to_dynamic(*ph.rigid, *ph.mass);
    }
}

void
set_entity_matrix(c_entity ce, glm::mat4 mat) {
    auto &phys_man = component_system_man.managers.physics_component_man;
    auto &pos_man = component_system_man.managers.position_component_man;

    auto phy = phys_man.get_instance_data(ce);
    auto pos = pos_man.get_instance_data(ce);

    (*phy.rigid)->setWorldTransform(mat4_to_bt(mat));
    *(pos.mat) = mat;
}

void
use_action_on_entity(ship_space *ship, c_entity ce) {
    auto &pos_man = component_system_man.managers.position_component_man;
    auto &switch_man = component_system_man.managers.switch_component_man;
    auto &cwire_man = component_system_man.managers.wire_comms_component_man;

    /* used by the player */
    assert(pos_man.exists(ce) || !"All [usable] entities probably need position");
    assert(switch_man.exists(ce) || !"All [usable] entities need switch comp");

    if (switch_man.exists(ce)) {
        /* publish new state on all attached comms wires */
        auto & enabled = *switch_man.get_instance_data(ce).enabled;
        enabled ^= true;

        comms_msg msg{
            ce,
            comms_msg_type_switch_state,
            enabled ? 1.f : 0.f,
        };
        publish_msg(ship, *cwire_man.get_instance_data(ce).network, msg);
    }
}
