#include "../tinydir.h"

#include <epoxy/gl.h>

#include "../component/component_system_manager.h"
#include "../asset_manager.h"
#include "../common.h"
#include "../ship_space.h"
#include "../mesh.h"
#include "../player.h"
#include "tools.h"

#include <libconfig.h>
#include "../libconfig_shim.h"


extern GLuint overlay_shader;
extern GLuint simple_shader;

extern ship_space *ship;

extern asset_manager asset_man;
extern component_system_manager component_system_man;
extern player pl;


std::vector<std::string> entity_names{};
std::unordered_map<std::string, std::shared_ptr<entity_data>> entity_stubs{};


void load_entities() {
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

        auto entity = std::make_shared<entity_data>();

        // required to be a valid entity
        std::unordered_map<std::string, bool> found{
            { "type",               false },
            { "renderable",         false },
            { "physics",            false },
            { "relative_position",  false },
            { "surface_attachment", false },
        };

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

                entity->components.emplace_back(stub_ptr);

                auto type_stub = dynamic_cast<type_component_stub*>(stub_ptr);
                if (type_stub) {
                    entity->name = type_stub->name;

                    found[component->name] = true;
                }

                auto render_stub = dynamic_cast<renderable_component_stub*>(stub_ptr);
                if (render_stub) {
                    found[component->name] = true;
                }

                auto pos_stub = dynamic_cast<relative_position_component_stub*>(stub_ptr);
                if (pos_stub) {
                    found[component->name] = true;
                }

                auto physics_stub = dynamic_cast<physics_component_stub*>(stub_ptr);
                if (physics_stub) {
                    found[component->name] = true;
                }

                auto surf_stub = dynamic_cast<surface_attachment_component_stub*>(stub_ptr);
                if (surf_stub) {
                    found[component->name] = true;
                }
            }

            auto valid = true;
            for (auto &find : found) {
                if (!find.second) {
                    printf("!! Entity %s in %s missing %s component\n", f.c_str(), entity->name.c_str(), find.first.c_str());
                    valid = false;
                }
            }

            if (valid) {
                entity_stubs[entity->name] = std::move(entity);
            }
        }
    }

    for (auto &entity : entity_stubs) {
        std::string name = entity.first;
        entity_names.push_back(name);
    }
}

c_entity
spawn_entity(const std::string &name, glm::ivec3 p, int face) {
    auto ce = c_entity::spawn();

    auto mat = mat_block_face(p, face);

    auto entity = entity_stubs[name];

    auto render_stub = entity->get_component<renderable_component_stub>();
    assert(render_stub);

    for (auto &comp : entity->components) {
        comp->assign_component_to_entity(ce);
    }

    auto &pos_man = component_system_man.managers.relative_position_component_man;
    auto &physics_man = component_system_man.managers.physics_component_man;
    auto &surface_man = component_system_man.managers.surface_attachment_component_man;
    auto &render_man = component_system_man.managers.renderable_component_man;

    auto physics = physics_man.get_instance_data(ce);
    *physics.rigid = nullptr;
    std::string m = *physics.mesh;
    auto const &phys_mesh = asset_man.get_mesh(m);
    build_static_physics_rb_mat(&mat, phys_mesh.phys_shape, physics.rigid);
    /* so that we can get back to the entity from a phys raycast */
    /* TODO: these should really come from a dense pool rather than the generic allocator */
    auto per = new phys_ent_ref;
    per->ce = ce;
    (*physics.rigid)->setUserPointer(per);

    auto surface = surface_man.get_instance_data(ce);
    *surface.block = p;
    *surface.face = face;

    auto pos = pos_man.get_instance_data(ce);
    *pos.position = p;
    *pos.mat = mat;

    auto render = render_man.get_instance_data(ce);
    *render.material = asset_man.get_world_texture_index(render_stub->material);

    return ce;
}


void
use_action_on_entity(ship_space *ship, c_entity ce) {
    auto &pos_man = component_system_man.managers.relative_position_component_man;
    auto &switch_man = component_system_man.managers.switch_component_man;
    auto &type_man = component_system_man.managers.type_component_man;

    /* used by the player */
    assert(pos_man.exists(ce) || !"All [usable] entities probably need position");

    auto pos = *pos_man.get_instance_data(ce).position;
    auto type = *type_man.get_instance_data(ce).name;
    printf("player using the %s at %f %f %f\n",
        type, pos.x, pos.y, pos.z);

    if (switch_man.exists(ce)) {
        /* publish new state on all attached comms wires */
        auto & enabled = *switch_man.get_instance_data(ce).enabled;
        enabled ^= true;

        comms_msg msg;
        msg.originator = ce;
        msg.desc = comms_msg_type_switch_state;
        msg.data = enabled ? 1.f : 0.f;
        publish_msg(ship, ce, msg);
    }
}

/* todo: support free-placed entities*/
void
place_entity_attaches(raycast_info* rc, int index, c_entity e) {
    auto &render_man = component_system_man.managers.renderable_component_man;

    auto mesh_name = *render_man.get_instance_data(e).mesh;
    auto sw = asset_man.get_mesh(mesh_name).sw;

    for (auto wire_index = 0; wire_index < num_wire_types; ++wire_index) {
        auto wt = (wire_type)wire_index;
        for (auto i = 0u; i < sw->num_attach_points[wt]; ++i) {
            auto mat = mat_block_face(rc->p, index ^ 1) * sw->attach_points[wt][i];
            auto attach_index = (unsigned)ship->wire_attachments[wt].size();
            wire_attachment wa = { mat, attach_index, 0, true };

            ship->wire_attachments[wt].push_back(wa);
            ship->entity_to_attach_lookups[wt][e].insert(attach_index);
        }
    }
}

struct add_entity_tool : tool {
    enum class RotateMode {
        AxisAligned,
        Stepped45,
        Stepped15,
    } rotate_mode{RotateMode::AxisAligned};

    enum class PlaceMode {
        BlockSnapped,
        HalfBlockSnapped,
        FreeForm,
    } place_mode{PlaceMode::BlockSnapped};

    unsigned entity_name_index = 0;

    bool can_use(raycast_info *rc) {
        return true;
    }

    void use(raycast_info *rc) override {
        if (!can_use(rc)) {
            return;
        }
    }

    void alt_use(raycast_info *rc) override {
        switch (rotate_mode) {
            case RotateMode::AxisAligned: {
                rotate_mode = RotateMode::Stepped45;
                break;
            }
            case RotateMode::Stepped45: {
                rotate_mode = RotateMode::Stepped15;
                break;
            }
            case RotateMode::Stepped15: {
                rotate_mode = RotateMode::AxisAligned;
                break;
            }
        }
    }

    void long_use(raycast_info *rc) override {

    }

    void long_alt_use(raycast_info *rc) override {

    }

    void cycle_mode() override {
        switch (place_mode) {
            case PlaceMode::BlockSnapped: {
                place_mode = PlaceMode::HalfBlockSnapped;
                break;
            }
            case PlaceMode::HalfBlockSnapped: {
                place_mode = PlaceMode::FreeForm;
                break;
            }
            case PlaceMode::FreeForm: {
                place_mode = PlaceMode::BlockSnapped;
                break;
            }
        }
    }

    void preview(raycast_info *rc, frame_data *frame) override {
    }

    void get_description(char *str) override {
        auto name = entity_names[entity_name_index];
        const char *rotate = nullptr;
        switch (rotate_mode) {
            case RotateMode::AxisAligned: {
                rotate = "Axis Aligned";
                break;
            }
            case RotateMode::Stepped45: {
                rotate = "Stepped @ 45";
                break;
            }
            case RotateMode::Stepped15: {
                rotate = "Stepped @ 15";
                break;
            }
        }

        const char *place = nullptr;
        switch (place_mode) {
            case PlaceMode::BlockSnapped: {
                place = "Block Snapped";
                break;
            }
            case PlaceMode::HalfBlockSnapped: {
                place = "Half Block Snapped";
                break;
            }
            case PlaceMode::FreeForm: {
                place = "Free Form";
                break;
            }
        }
        sprintf(str, "Place %s \nRotating %s \nPlacing %s", name.c_str(), rotate, place);
    }
};

tool *tool::create_add_entity_tool() { return new add_entity_tool; }

struct add_block_entity_tool : tool
{
    unsigned type = 0;

    bool can_use(raycast_info *rc) {
        if (!rc->hit || rc->inside)
            return false;

        /* don't allow placements that would cause the player to end up inside the ent and get stuck */
        if (rc->p == get_coord_containing(pl.eye) ||
            rc->p == get_coord_containing(pl.pos))
            return false;

        /* block ents can only be placed in empty space, on a frame */
        if (!rc->block || rc->block->type != block_frame) {
            return false;
        }

        //todo: fix height here. hardcoded is wrong
        auto height = 1;
        for (auto i = 0; i < height; i++) {
            block *bl = ship->get_block(rc->p + glm::ivec3(0, 0, i));
            if (bl) {
                /* check for surface ents that would conflict */
                for (unsigned short face : bl->surf_space)
                    if (face)
                        return false;
            }
        }

        return true;
    }

    void use(raycast_info *rc) override {
        if (!can_use(rc))
            return;

        auto name = entity_names[type];
        chunk *ch = ship->get_chunk_containing(rc->p);
        auto e = spawn_entity(name, rc->p, surface_zm);
        ch->entities.push_back(e);

        //todo: fix height here. hardcoded is wrong
        auto height = 1;
        for (auto i = 0; i < height; i++) {
            auto p = rc->p + glm::ivec3(0, 0, i);
            block *bl = ship->ensure_block(p);
            bl->type = block_entity;
            printf("taking block %d,%d,%d\n", p.x, p.y, p.z);

            /* consume ALL the space on the surfaces */
            for (unsigned short &face : bl->surf_space) {
                face = (unsigned short)~0;
            }
        }

        place_entity_attaches(rc, surface_zp, e);
    }

    void alt_use(raycast_info *rc) override {}

    void long_use(raycast_info *rc) override {}

    void cycle_mode() override {
        type++;
        if (type >= entity_names.size()) {
            type = 0;
        }
    }

    void preview(raycast_info *rc, frame_data *frame) override {
        if (!can_use(rc))
            return;

        auto render = entity_stubs[entity_names[type]]->get_component<renderable_component_stub>();

        auto mat = frame->alloc_aligned<mesh_instance>(1);
        mat.ptr->world_matrix = mat_position(glm::vec3(rc->p) + glm::vec3(0.5f, 0.5f, 0.0f));
        mat.ptr->material = asset_man.get_world_texture_index(render->material);
        mat.bind(1, frame);

        auto mesh = asset_man.get_mesh(render->mesh);
        draw_mesh(mesh.hw);

        auto material = asset_man.get_world_texture_index("white.png");

        auto mat_overlay = frame->alloc_aligned<mesh_instance>(1);
        mat_overlay.ptr->world_matrix = mat_position(glm::vec3(rc->p));
        mat_overlay.ptr->material = material;
        mat_overlay.bind(1, frame);

        auto &frame_mesh = asset_man.get_mesh("initial_frame.dae");

        /* draw a block overlay as well around the block */
        glUseProgram(overlay_shader);
        draw_mesh(frame_mesh.hw);
        glUseProgram(simple_shader);
    }

    void get_description(char *str) override {
        auto name = entity_names[type];
        sprintf(str, "Place %s", name.c_str());
    }
};

tool *tool::create_add_block_entity_tool() { return new add_block_entity_tool; }


struct add_surface_entity_tool : tool
{
    unsigned type = 0;

    bool can_use(raycast_info *rc) {
        if (!rc->hit)
            return false;

        block *bl = rc->block;

        if (!bl)
            return false;

        int index = normal_to_surface_index(rc);

        if (~bl->surfs[index] & surface_phys)
            return false;

        block *other_side = ship->get_block(rc->p);
        auto required_space = (unsigned short)~0; /* TODO: make this a prop of the type + subblock placement */

        if (other_side->surf_space[index ^ 1] & required_space) {
            /* no room on the surface */
            return false;
        }

        return true;
    }

    void use(raycast_info *rc) override {
        if (!can_use(rc))
            return;

        int index = normal_to_surface_index(rc);

        block *other_side = ship->get_block(rc->p);
        auto required_space = (unsigned short)~0; /* TODO: make this a prop of the type + subblock placement */

        chunk *ch = ship->get_chunk_containing(rc->p);
        /* the chunk we're placing into is guaranteed to exist, because there's
        * a surface facing into it */
        assert(ch);

        auto name = entity_names[type];
        auto e = spawn_entity(name, rc->p, index ^ 1);
        ch->entities.push_back(e);

        /* take the space. */
        other_side->surf_space[index ^ 1] |= required_space;

        place_entity_attaches(rc, index, e);
    }

    void alt_use(raycast_info *rc) override {}

    void long_use(raycast_info *rc) override {}

    void cycle_mode() override {
        type++;
        if (type >= entity_names.size()) {
            type = 0;
        }
    }

    void preview(raycast_info *rc, frame_data *frame) override {
        if (!can_use(rc))
            return;

        auto index = normal_to_surface_index(rc);
        auto render = entity_stubs[entity_names[type]]->get_component<renderable_component_stub>();

        auto mat = frame->alloc_aligned<mesh_instance>(1);
        mat.ptr->world_matrix = mat_block_face(rc->p, index ^ 1);
        mat.ptr->material = asset_man.get_world_texture_index(render->material);
        mat.bind(1, frame);

        auto mesh = asset_man.get_mesh(render->mesh);
        draw_mesh(mesh.hw);

        /* draw a surface overlay here too */
        /* TODO: sub-block placement granularity -- will need a different overlay */
        auto surf_mesh = asset_man.get_surface_mesh(index);
        auto material = asset_man.get_world_texture_index("white.png");

        auto mat2 = frame->alloc_aligned<mesh_instance>(1);
        mat2.ptr->world_matrix = mat_position(glm::vec3(rc->bl));
        mat2.ptr->material = material;
        mat2.bind(1, frame);

        glUseProgram(overlay_shader);
        glEnable(GL_POLYGON_OFFSET_FILL);
        draw_mesh(surf_mesh.hw);
        glDisable(GL_POLYGON_OFFSET_FILL);
        glUseProgram(simple_shader);
    }

    void get_description(char *str) override {
        auto name = entity_names[type];
        sprintf(str, "Place %s on surface", name.c_str());
    }
};

tool *tool::create_add_surface_entity_tool() { return new add_surface_entity_tool; }