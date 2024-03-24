#include "MeshLoader.hpp" 
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <meshoptimizer.h>
#include <stdexcept>

auto MeshLoader::loadMesh(std::string_view path, bool flipUV) -> std::vector<Mesh>
{
    m.result.clear();

    auto importer{ Assimp::Importer{} };
    auto const* scene{ importer.ReadFile(path.data(), aiProcess_Triangulate | aiProcess_GenNormals | (flipUV ? aiProcess_FlipUVs : 0)) };

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
    {
        std::runtime_error(importer.GetErrorString());
    }

    processNode(scene->mRootNode, scene);

    return m.result;
}

auto MeshLoader::processNode(aiNode* pNode, aiScene const* pScene) -> void
{
    for (auto i{ u32{} }; i < pNode->mNumMeshes; ++i)
    {
        aiMesh *mesh = pScene->mMeshes[pNode->mMeshes[i]]; 
        m.result.push_back(processMesh(mesh, pScene));			
    }
    
    for (auto i{ u32{} }; i < pNode->mNumChildren; ++i)
    {
        processNode(pNode->mChildren[i], pScene);
    }
}

auto MeshLoader::processMesh(aiMesh* pMesh, aiScene const* pScene) -> Mesh
{
    auto result{ Mesh{
        .vertexOffset = static_cast<u32>(uvs.size() >> 1),
        .indexOffset = static_cast<u32>(indices.size())
    }};

    positions.reserve(pMesh->mNumVertices * 3);
    normals.reserve(pMesh->mNumVertices * 3);
    uvs.reserve(pMesh->mNumVertices * 2);

    for (u32 i = 0; i < pMesh->mNumVertices; ++i)
    {
        positions.emplace_back(pMesh->mVertices[i].x);
        positions.emplace_back(pMesh->mVertices[i].y);
        positions.emplace_back(pMesh->mVertices[i].z);

        normals.emplace_back(static_cast<u8>(pMesh->mNormals[i].x * 127.f + 127.5f));
        normals.emplace_back(static_cast<u8>(pMesh->mNormals[i].y * 127.f + 127.5f));
        normals.emplace_back(static_cast<u8>(pMesh->mNormals[i].z * 127.f + 127.5f));

        uvs.emplace_back(pMesh->mTextureCoords[0] ? pMesh->mTextureCoords[0][i].x : 0.f);
        uvs.emplace_back(pMesh->mTextureCoords[0] ? pMesh->mTextureCoords[0][i].y : 0.f);
    }

    for (u32 i = 0; i < pMesh->mNumFaces; ++i)
    {
        aiFace face = pMesh->mFaces[i];

        for (auto j{ u32{} }; j < face.mNumIndices; ++j)
        {
            indices.emplace_back(face.mIndices[j]);
        }
    }

    result.indexCount = static_cast<u32>(indices.size()) - result.indexOffset,
    result.vertexCount = static_cast<u32>(uvs.size() >> 1) - result.vertexOffset;

    return result;
}