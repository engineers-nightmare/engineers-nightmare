#pragma once

#include <unordered_map>
#include <string>
#include <array>
#include <unordered_map>
#include "soloud.h"
#include "soloud_audiosource.h"

#include "mesh.h"
#include "block.h"
#include "textureset.h"

struct surface_kind {
    mesh_data const *visual_mesh {};
    mesh_data const *physics_mesh {};
    mesh_data const *popped_physics_mesh  {};
    char const *legacy_mesh_name {};
    char const *legacy_physics_mesh_name {};
    char const *legacy_popped_physics_mesh_name {};
    char const *name {};
};

class asset_manager
{
    std::unordered_map<std::string, mesh_data> meshes;
    std::array<std::string, 256> surf_type_to_mesh;

    std::unordered_map<std::string, texture_set *> skyboxes {};
    std::unordered_map<std::string, SoLoud::AudioSource *> sounds {};
     
    void load_asset_manifest(char const *filename);

public:
    texture_set *render_textures{ nullptr };
    std::unordered_map<unsigned, surface_kind> surf_kinds{};

    asset_manager();
    ~asset_manager() = default;

    void load_assets();

    const mesh_data & get_mesh(const std::string & mesh) const;

    SoLoud::AudioSource * get_sound(const std::string &);

    void bind_render_textures(int i);

    void bind_skybox(const std::string &skybox, int i);
};
