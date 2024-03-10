#pragma once
#include <vector>
#include "MeshLoader.hpp"

struct ModelComponent
{
    ModelComponent() = delete;
    ~ModelComponent() = default;
    ModelComponent(MeshLoader& loader, std::string_view path, bool flipUV)
    {
        meshes = loader.loadMesh(path, flipUV);
    }

    std::vector<MeshComponent> meshes;
};