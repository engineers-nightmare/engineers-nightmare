#include "asset_manager.h"
#include "tinydir.h"
#include "common.h"
#include <libconfig.h>
#include "libconfig_shim.h"
#include "soloud_wav.h"
#include "soloud_wavstream.h"

asset_manager::asset_manager()
    : meshes() {
}

template<typename Func>
std::vector<tinydir_file> get_file_list(char const *path, Func f) {
    std::vector<tinydir_file> files;
    tinydir_dir dir{};
    tinydir_open(&dir, path);

    for (; dir.has_next; tinydir_next(&dir)) {
        tinydir_file file{};
        tinydir_readfile(&dir, &file);

        if (file.is_dir || !f(file)) {
            continue;
        }

        files.emplace_back(file);
    }

    tinydir_close(&dir);

    return files;
}

struct asset_type {
    static constexpr const char* mesh {"mesh"};
    static constexpr const char* texture {"texture"};
    static constexpr const char* skybox {"skybox"};
    static constexpr const char* sound { "sound" };
};

void asset_manager::load_asset_manifest(char const *filename) {
    config_t cfg{};
    config_init(&cfg);

    printf("Loading from manifest `%s`\n", filename);

    if (!config_read_file(&cfg, filename)) {
        printf("Failed to load asset manifest `%s`: %s:%d\n", filename, config_error_text(&cfg),
            config_error_line(&cfg));
        config_destroy(&cfg);
        return;
    }

    auto assets_setting = config_lookup(&cfg, "assets");
    auto num_assets = config_setting_length(assets_setting);

    for (auto i = 0; i < num_assets; i++) {
        auto asset_setting = config_setting_get_elem(assets_setting, i);
        char const *asset_type;
        config_setting_lookup_string(asset_setting, "type", &asset_type);

        if (!strcmp(asset_type, asset_type::mesh)) {
            char const *asset_name;
            config_setting_lookup_string(asset_setting, "name", &asset_name);
            char const *asset_file;
            config_setting_lookup_string(asset_setting, "file", &asset_file);

            auto & m = meshes[asset_name] = mesh_data{ asset_file };

            double density;
            if (config_setting_lookup_float(asset_setting, "density", &density)) {
                double scale = 1.0 / density;
                // TODO: if -ve scale, or we add nonuniform sometime, then
                // I need to fix the normals too
                for (auto i = 0u; i < m.sw->num_vertices; i++) {
                    m.sw->verts[i].x *= (float)scale;
                    m.sw->verts[i].y *= (float)scale;
                    m.sw->verts[i].z *= (float)scale;
                }
            }

            auto offset = config_setting_lookup(asset_setting, "offset");
            if (offset) {
                auto ox = (float)config_setting_get_float_elem(offset, 0);
                auto oy = (float)config_setting_get_float_elem(offset, 1);
                auto oz = (float)config_setting_get_float_elem(offset, 2);
                for (auto i = 0u; i < m.sw->num_vertices; i++) {
                    m.sw->verts[i].x += ox;
                    m.sw->verts[i].y += oy;
                    m.sw->verts[i].z += oz;
                }
            }

            int nonconvex;
            if (config_setting_lookup_bool(asset_setting, "nonconvex", &nonconvex)) {
                m.nonconvex = nonconvex != 0;
            }
        }
        else if (!strcmp(asset_type, asset_type::skybox)) {
            char const *asset_name;
            config_setting_lookup_string(asset_setting, "name", &asset_name);
            int asset_dim;
            config_setting_lookup_int(asset_setting, "dimensions", &asset_dim);

            skyboxes[asset_name] = new texture_set(GL_TEXTURE_CUBE_MAP, asset_dim, 6);

            auto files = config_setting_lookup(asset_setting, "files");

            auto files_count = config_setting_length(files);
            assert(files_count == 6);

            for (unsigned files_index = 0; files_index < (unsigned)files_count; ++files_index) {
                config_setting_t *input = config_setting_get_elem(files, files_index);
                auto file = config_setting_get_string(input);
                skyboxes[asset_name]->load(file);
            }
        }
        else if (!strcmp(asset_type, asset_type::sound)) {
            char const *asset_name;
            config_setting_lookup_string(asset_setting, "name", &asset_name);
            char const *asset_file;
            config_setting_lookup_string(asset_setting, "file", &asset_file);
            int streaming;
            config_setting_lookup_bool(asset_setting, "streaming", &streaming);

            if (streaming) {
                auto sound = new SoLoud::WavStream();
                sounds[asset_name] = sound;
                sound->load(asset_file);
            }
            else {
                auto sound = new SoLoud::Wav();
                sounds[asset_name] = sound;
                sound->load(asset_file);
            }
        }
        else {
            printf("Unknown asset type `%s`\n", asset_type);
            continue;
        }
    }

    config_destroy(&cfg);
}

void asset_manager::load_assets() {
    auto asset_files = get_file_list("assets", [](tinydir_file const &f) { return !strcmp(f.extension, "manifest"); });
    for (auto const &f : asset_files) {
        load_asset_manifest(f.path);
    }

    for (auto &mesh : meshes) {
        mesh.second.hw = upload_mesh(mesh.second.sw);
        if (mesh.second.nonconvex) {
            build_static_physics_mesh(mesh.second.sw, &mesh.second.phys_mesh, &mesh.second.phys_shape);
        }
        else {
            build_dynamic_physics_mesh(mesh.second.sw, &mesh.second.phys_shape);
        }
    }

    render_textures = new texture_set(GL_TEXTURE_2D_ARRAY, RENDER_DIM, 32);

    surf_kinds[surface_wall].visual_mesh = &get_mesh("surface_wall");
    surf_kinds[surface_wall].physics_mesh = &get_mesh("surface_wall");
    surf_kinds[surface_wall].popped_physics_mesh = &get_mesh("surface_wall_popped");
    surf_kinds[surface_wall].legacy_mesh_name = "surface_wall";
    surf_kinds[surface_wall].legacy_physics_mesh_name = "surface_wall";
    surf_kinds[surface_wall].legacy_popped_physics_mesh_name = "surface_wall_popped";
    surf_kinds[surface_wall].name = "Wall";

    surf_kinds[surface_glass].visual_mesh = &get_mesh("surface_glass");
    surf_kinds[surface_glass].physics_mesh = &get_mesh("surface_wall");
    surf_kinds[surface_glass].popped_physics_mesh = &get_mesh("surface_wall_popped");
    surf_kinds[surface_glass].legacy_mesh_name = "surface_glass";
    surf_kinds[surface_glass].legacy_physics_mesh_name = "surface_wall";
    surf_kinds[surface_glass].legacy_popped_physics_mesh_name = "surface_wall_popped";
    surf_kinds[surface_glass].name = "Glass";

    surf_kinds[surface_grate].visual_mesh = &get_mesh("surface_grate");
    surf_kinds[surface_grate].physics_mesh = &get_mesh("surface_wall");
    surf_kinds[surface_grate].popped_physics_mesh = &get_mesh("surface_wall_popped");
    surf_kinds[surface_grate].legacy_mesh_name = "surface_grate";
    surf_kinds[surface_grate].legacy_physics_mesh_name = "surface_wall";
    surf_kinds[surface_grate].legacy_popped_physics_mesh_name = "surface_wall_popped";
    surf_kinds[surface_grate].name = "Grate";
}

const mesh_data & asset_manager::get_mesh(const std::string & mesh) const {
    return meshes.at(mesh);
}

SoLoud::AudioSource * asset_manager::get_sound(const std::string & sound) {
    return sounds.at(sound);
}

void asset_manager::bind_render_textures(int i) {
    render_textures->bind(i);
}

void asset_manager::bind_skybox(const std::string & skybox, int i) {
    skyboxes[skybox]->bind(i);
}
