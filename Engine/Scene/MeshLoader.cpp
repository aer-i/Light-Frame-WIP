#include "MeshLoader.hpp" 
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <meshoptimizer.h>
#include <stdexcept>

auto MeshLoader::loadMesh(std::string_view path, bool flipUV) -> std::vector<MeshComponent>
{
    m_result.clear();

    auto importer{ Assimp::Importer{} };
    auto const* scene{ importer.ReadFile(path.data(), aiProcess_Triangulate | aiProcess_GenNormals | (flipUV ? aiProcess_FlipUVs : 0)) };

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
    {
        std::runtime_error(importer.GetErrorString());
    }

    processNode(scene->mRootNode, scene);

    return m_result;
}

auto MeshLoader::processNode(aiNode* pNode, aiScene const* pScene) -> void
{
    for (unsigned int i = 0; i < pNode->mNumMeshes; i++)
    {
        aiMesh *mesh = pScene->mMeshes[pNode->mMeshes[i]]; 
        m_result.push_back(processMesh(mesh, pScene));			
    }
    
    for (unsigned int i = 0; i < pNode->mNumChildren; i++)
    {
        processNode(pNode->mChildren[i], pScene);
    }
}

auto MeshLoader::processMesh(aiMesh* pMesh, aiScene const* pScene) -> MeshComponent
{
    MeshComponent result;
    result.indexOffset = static_cast<u32>(m_indices.size());
    result.vertexOffset = static_cast<u32>(m_positions.size() / 3);

    for (u32 i = 0; i < pMesh->mNumVertices; ++i)
    {
        m_positions.emplace_back(pMesh->mVertices[i].x);
        m_positions.emplace_back(pMesh->mVertices[i].y);
        m_positions.emplace_back(pMesh->mVertices[i].z);

        m_normals.emplace_back(static_cast<u8>(pMesh->mNormals[i].x * 127.f + 127.5f));
        m_normals.emplace_back(static_cast<u8>(pMesh->mNormals[i].y * 127.f + 127.5f));
        m_normals.emplace_back(static_cast<u8>(pMesh->mNormals[i].z * 127.f + 127.5f));

        m_uvs.emplace_back(pMesh->mTextureCoords[0] ? pMesh->mTextureCoords[0][i].x : 0.f);
        m_uvs.emplace_back(pMesh->mTextureCoords[0] ? pMesh->mTextureCoords[0][i].y : 0.f);
    }

    for (u32 i = 0; i < pMesh->mNumFaces; ++i)
    {
        aiFace face = pMesh->mFaces[i];

        for (unsigned int j = 0; j < face.mNumIndices; ++j)
        {
            m_indices.emplace_back(face.mIndices[j]);
        }
    }

    result.indexCount = m_indices.size() - result.indexOffset;
    result.indexCount = m_positions.size() / 3 - result.vertexOffset;
    
    return result;
}