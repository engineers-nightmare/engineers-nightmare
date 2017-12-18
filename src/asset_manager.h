#pragma once

#include <unordered_map>
#include <string>
#include <array>

#include "mesh.h"
#include "block.h"
#include "textureset.h"

class asset_manager
{
    std::unordered_map<std::string, mesh_data> meshes;
    std::array<std::array<std::string, face_count>, 256> surf_to_mesh;

    texture_set *render_textures{nullptr};
    std::unordered_map<std::string, texture_set *> skyboxes {};

    void load_asset_manifest(char const *filename);

public:
    asset_manager();
    ~asset_manager() = default;

    void load_assets();

    mesh_data & get_mesh(const std::string &);

    mesh_data & get_surface_mesh(unsigned surface_index, unsigned surface_type);

    void bind_render_textures(int i);

    void bind_skybox(const std::string &skybox, int i);
};
