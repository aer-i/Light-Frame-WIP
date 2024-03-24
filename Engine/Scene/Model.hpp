#pragma once
#include "MeshLoader.hpp"
#include <vector>

struct Model
{
    Model() = delete;
    ~Model() = default;

    Model(MeshLoader& loader, std::string_view path, bool flipUV)
    {
        meshes = loader.loadMesh(path, flipUV);
    }

    std::vector<Mesh> meshes;
};