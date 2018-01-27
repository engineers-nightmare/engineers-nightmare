#include "asset_manager.h"
#include "tinydir.h"
#include "common.h"
#include <libconfig.h>
#include "libconfig_shim.h"
#include <soloud_wav.h>
#include <soloud_wavstream.h>

asset_manager::asset_manager()
    : meshes(),
      surf_to_mesh() {

    surf_to_mesh[surface_xm][surface_wall] = "x_quad";
    surf_to_mesh[surface_xp][surface_wall] = "x_quad_p";
    surf_to_mesh[surface_ym][surface_wall] = "y_quad";
    surf_to_mesh[surface_yp][surface_wall] = "y_quad_p";
    surf_to_mesh[surface_zm][surface_wall] = "z_quad";
    surf_to_mesh[surface_zp][surface_wall] = "z_quad_p";

    surf_to_mesh[surface_xm][surface_grate] = "grate_x_quad";
    surf_to_mesh[surface_xp][surface_grate] = "grate_x_quad_p";
    surf_to_mesh[surface_ym][surface_grate] = "grate_y_quad";
    surf_to_mesh[surface_yp][surface_grate] = "grate_y_quad_p";
    surf_to_mesh[surface_zm][surface_grate] = "grate_z_quad";
    surf_to_mesh[surface_zp][surface_grate] = "grate_z_quad_p";

    surf_to_mesh[surface_xm][surface_glass] = "glass_x_quad";
    surf_to_mesh[surface_xp][surface_glass] = "glass_x_quad_p";
    surf_to_mesh[surface_ym][surface_glass] = "glass_y_quad";
    surf_to_mesh[surface_yp][surface_glass] = "glass_y_quad_p";
    surf_to_mesh[surface_zm][surface_glass] = "glass_z_quad";
    surf_to_mesh[surface_zp][surface_glass] = "glass_z_quad_p";

    surf_type_to_mesh[surface_wall] = "surface_wall";
    surf_type_to_mesh[surface_grate] = "surface_grate";
    surf_type_to_mesh[surface_glass] = "surface_glass";
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

            double scale;
            if (config_setting_lookup_float(asset_setting, "scale", &scale)) {
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
        mesh.second.upload_mesh();
        mesh.second.load_physics();
    }

    render_textures = new texture_set(GL_TEXTURE_2D_ARRAY, RENDER_DIM, 2);
}

const mesh_data & asset_manager::get_mesh(const std::string & mesh) const {
    return meshes.at(mesh);
}

const mesh_data & asset_manager::get_surface_mesh(unsigned surface_type) const {
    return meshes.at(surf_type_to_mesh[surface_type]);
}

const std::string & asset_manager::get_surface_mesh_name(unsigned surface_type) const {
    return surf_type_to_mesh[surface_type];
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

