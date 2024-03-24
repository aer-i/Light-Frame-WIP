#pragma once
#include "Mesh.hpp"
#include <string_view>
#include <assimp/scene.h>
#include <vector>

class MeshLoader
{
public:
    auto loadMesh(std::string_view path, bool flipUV) -> std::vector<Mesh>;

private:
    auto processNode(aiNode* pNode, aiScene const* pScene) -> void;
    auto processMesh(aiMesh* pMesh, aiScene const* pScene) -> Mesh;

public:
    std::vector<u32> indices;
    std::vector<f32> positions;
    std::vector<f32> uvs;
    std::vector<u8>  normals;

private:
    struct M
    {
        std::vector<Mesh> result;
    } m;
};