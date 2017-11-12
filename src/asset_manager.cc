#include "asset_manager.h"
#include "tinydir.h"

#define WORLD_TEXTURE_DIMENSION     32
#define MAX_WORLD_TEXTURES          64

asset_manager::asset_manager() : meshes(), surface_index_to_mesh_name() {
    surface_index_to_mesh_name[surface_xm] = "x_quad.dae";
    surface_index_to_mesh_name[surface_xp] = "x_quad_p.dae";
    surface_index_to_mesh_name[surface_ym] = "y_quad.dae";
    surface_index_to_mesh_name[surface_yp] = "y_quad_p.dae";
    surface_index_to_mesh_name[surface_zm] = "z_quad.dae";
    surface_index_to_mesh_name[surface_zp] = "z_quad_p.dae";
}

void asset_manager::load_meshes() {
    std::vector<tinydir_file> files;

    tinydir_dir dir{};
    tinydir_open(&dir, "mesh");

    while (dir.has_next)
    {
        tinydir_file file{};
        tinydir_readfile(&dir, &file);

        tinydir_next(&dir);

        if (file.is_dir || strcmp(file.extension, "dae") != 0) {
            continue;
        }

        files.emplace_back(file);
    }

    tinydir_close(&dir);

    printf("Loading meshes\n");
    for (auto &f : files) {
        meshes.emplace(f.name, mesh_data{ f.path }).second;
    }

    auto proj_mesh = meshes["sphere.dae"];
    for (auto i = 0u; i < proj_mesh.sw->num_vertices; ++i) {
        proj_mesh.sw->verts[i].x *= 0.01f;
        proj_mesh.sw->verts[i].y *= 0.01f;
        proj_mesh.sw->verts[i].z *= 0.01f;
    }
    set_mesh_material(proj_mesh.sw, 11);

    set_mesh_material(meshes["attach.dae"].sw, 10);
    set_mesh_material(meshes["no_place.dae"].sw, 11);
    set_mesh_material(meshes["wire.dae"].sw, 12);
    set_mesh_material(meshes["single_door.dae"].sw, 2);
    set_mesh_material(meshes["wire.dae"].sw, 12);

    for (auto &mesh : meshes) {
        mesh.second.upload_mesh();
        mesh.second.load_physics();
    }
}

void asset_manager::load_textures() {
    world_textures = new texture_set(GL_TEXTURE_2D_ARRAY, WORLD_TEXTURE_DIMENSION, MAX_WORLD_TEXTURES);
    skybox = new texture_set(GL_TEXTURE_CUBE_MAP, 2048, 6);

    std::vector<tinydir_file> files;

    tinydir_dir dir{};
    tinydir_open(&dir, "textures");

    while (dir.has_next)
    {
        tinydir_file file{};
        tinydir_readfile(&dir, &file);

        tinydir_next(&dir);

        if (file.is_dir) {
            continue;
        }

        files.emplace_back(file);
    }

    tinydir_close(&dir);

    printf("Loading textures\n");
    auto cnt = 0;
    for (auto &f: files) {
        if (strstr(f.name, "sky_") == f.name) {
            continue;
        }
        printf("  %s\n", f.path);
        world_textures->load(cnt, f.path);
        texture_to_index.emplace(std::pair<std::string, unsigned>{f.name, cnt});

        cnt++;
    }

    printf("Loading skybox\n");
    skybox->load(0, "textures/sky_right1.png");
    skybox->load(1, "textures/sky_left2.png");
    skybox->load(2, "textures/sky_top3.png");
    skybox->load(3, "textures/sky_bottom4.png");
    skybox->load(4, "textures/sky_front5.png");
    skybox->load(5, "textures/sky_back6.png");
}

unsigned asset_manager::get_texture_index(const std::string & tex) const {
    return texture_to_index.at(tex);
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

void asset_manager::bind_skybox(int i) {
    skybox->bind(i);
}

