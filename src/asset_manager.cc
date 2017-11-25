#include "asset_manager.h"
#include "tinydir.h"
#include "common.h"
#include <libconfig.h>
#include "libconfig_shim.h"

asset_manager::asset_manager() : meshes(), surface_index_to_mesh_name() {
    surface_index_to_mesh_name[surface_xm] = "x_quad";
    surface_index_to_mesh_name[surface_xp] = "x_quad_p";
    surface_index_to_mesh_name[surface_ym] = "y_quad";
    surface_index_to_mesh_name[surface_yp] = "y_quad_p";
    surface_index_to_mesh_name[surface_zm] = "z_quad";
    surface_index_to_mesh_name[surface_zp] = "z_quad_p";
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

        if (!strcmp(asset_type, "mesh")) {
            char const *asset_name;
            config_setting_lookup_string(asset_setting, "name", &asset_name);
            char const *asset_file;
            config_setting_lookup_string(asset_setting, "file", &asset_file);

            auto & m = meshes[asset_name] = mesh_data{ asset_file };
        }
        else {
            printf("Unknown asset type `%s`\n", asset_type);
            continue;
        }
    }

    config_destroy(&cfg);
}

void asset_manager::load_meshes() {
    auto asset_files = get_file_list("assets", [](tinydir_file const &f) { return !strcmp(f.extension, "manifest"); });
    for (auto const &f : asset_files) {
        load_asset_manifest(f.path);
    }

    for (auto &mesh : meshes) {
        mesh.second.upload_mesh();
        mesh.second.load_physics();
    }
}

void asset_manager::load_textures() {
    world_textures = new texture_set(GL_TEXTURE_2D_ARRAY, WORLD_TEXTURE_DIMENSION, MAX_WORLD_TEXTURES);
    render_textures = new texture_set(GL_TEXTURE_2D_ARRAY, RENDER_DIM, 2);
    skybox = new texture_set(GL_TEXTURE_CUBE_MAP, 2048, 6);

    auto files = get_file_list("textures", [](tinydir_file const &f) { return true; });

    printf("Loading textures\n");
    unsigned cnt = 0;
    for (auto &f: files) {
        if (strstr(f.name, "sky_") == f.name) {
            continue;
        }
        printf("  %s\n", f.path);
        world_textures->load(cnt, f.path);
        world_texture_to_index[f.name] = cnt;

        cnt++;
    }

    render_texture_to_index["render"] = 0;
    render_texture_to_index["render2"] = 1;

    printf("Loading skybox\n");
    skybox->load(0, "textures/sky_right1.png");
    skybox->load(1, "textures/sky_left2.png");
    skybox->load(2, "textures/sky_top3.png");
    skybox->load(3, "textures/sky_bottom4.png");
    skybox->load(4, "textures/sky_front5.png");
    skybox->load(5, "textures/sky_back6.png");
}

unsigned asset_manager::get_world_texture_index(const std::string & tex) const {
    return world_texture_to_index.at(tex);
}

unsigned asset_manager::get_render_texture_index(const std::string & tex) const {
    return render_texture_to_index.at(tex);
}

mesh_data & asset_manager::get_mesh(const std::string & mesh) {
    return meshes.at(mesh);
}

mesh_data &asset_manager::get_surface_mesh(unsigned surface_index) {
    return meshes.at(surface_index_to_mesh_name[surface_index]);
}

void asset_manager::bind_world_textures(int i) {
    world_textures->bind(i);
}

void asset_manager::bind_render_textures(int i) {
    render_textures->bind(i);
}

void asset_manager::bind_skybox(int i) {
    skybox->bind(i);
}

