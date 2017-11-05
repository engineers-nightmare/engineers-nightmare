#include "asset_manager.h"
#include "tinydir.h"

asset_manager::asset_manager() : meshes(), surface_index_to_mesh() {
    surface_index_to_mesh[surface_xm] = "x_quad.dae";
    surface_index_to_mesh[surface_xp] = "x_quad_p.dae";
    surface_index_to_mesh[surface_ym] = "y_quad.dae";
    surface_index_to_mesh[surface_yp] = "y_quad_p.dae";
    surface_index_to_mesh[surface_zm] = "z_quad.dae";
    surface_index_to_mesh[surface_zp] = "z_quad_p.dae";
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
