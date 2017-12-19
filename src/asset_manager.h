#pragma once

#include <unordered_map>
#include <string>
#include <array>
#include "soloud.h"
#include "soloud_audiosource.h"

#include "mesh.h"
#include "block.h"
#include "textureset.h"

class asset_manager
{
    std::unordered_map<std::string, mesh_data> meshes;
    std::array<std::array<std::string, face_count>, 256> surf_to_mesh;
    std::array<std::string, 256> surf_type_to_mesh;

    std::unordered_map<std::string, texture_set *> skyboxes {};
    std::unordered_map<std::string, SoLoud::AudioSource *> sounds {};
     
    void load_asset_manifest(char const *filename);

public:
    texture_set *render_textures{ nullptr };

    asset_manager();
    ~asset_manager() = default;

    void load_assets();

    const mesh_data & get_mesh(const std::string & mesh) const;

    const mesh_data & get_surface_mesh(unsigned surface_type) const;

    const std::string & get_surface_mesh_name(unsigned surface_type) const;

    SoLoud::AudioSource * get_sound(const std::string &);

    void bind_render_textures(int i);

    void bind_skybox(const std::string &skybox, int i);
};
