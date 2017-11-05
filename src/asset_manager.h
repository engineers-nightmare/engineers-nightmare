#pragma once

#include <unordered_map>
#include <string>
#include <array>

#include "mesh.h"
#include "block.h"

struct asset_manager
{
    asset_manager();
    ~asset_manager() {}

    std::unordered_map<std::string, mesh_data> meshes;
    std::array<std::string, face_count> surface_index_to_mesh;

    void load_meshes();
};

