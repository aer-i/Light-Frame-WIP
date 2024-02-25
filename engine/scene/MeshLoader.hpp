#pragma once
#include <vector>
#include <string_view>
#include <assimp/scene.h>
#include "MeshComponent.hpp"
#include "Renderer.hpp"

class MeshLoader
{
public:
    auto loadMesh(std::string_view path, bool flipUV) -> std::vector<MeshComponent>;

private:
    auto processNode(aiNode* pNode, aiScene const* pScene) -> void;
    auto processMesh(aiMesh* pMesh, aiScene const* pScene) -> MeshComponent;

public:
    std::vector<u32>           m_indices;
    std::vector<f32>           m_positions;
    std::vector<f32>           m_uvs;
    std::vector<u8>            m_normals;
    std::vector<MeshComponent> m_result;
};