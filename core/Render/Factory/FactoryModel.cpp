//
// Created by alexa on 2022-03-19.
//

#include "FactoryModel.h"

#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/Importer.hpp>


namespace FactoryModel {
    bool createDuckModel(std::vector<TexVertex>& vertices, std::vector<uint32_t>& indices) {
        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile("../../../core/Assets/Models/duck/scene.gltf",
                                                 aiProcess_Triangulate |
                                                 aiProcess_JoinIdenticalVertices);
        if (scene == nullptr || scene->mRootNode == nullptr)
            return false;

        SPDLOG_INFO("Mesh count {}", scene->mNumMeshes);

        for(int i = 0; i < scene->mNumMeshes; ++i){
            auto& mesh = scene->mMeshes[i];
            // insert pos and tc
            for (int j = 0; j < mesh->mNumVertices; ++j){
                aiVector3D pos = mesh->mVertices[j];
                aiVector3D tc = mesh->mTextureCoords[0][j];
                vertices.push_back({.position = {pos.x, pos.y, pos.z}, .uv = {tc.x, tc.y}});
            }

            // insert indices
            for (int j = 0; j < mesh->mNumFaces; ++j){
                auto face = mesh->mFaces[j];
                for (int k = 0; k < face.mNumIndices; ++k)
                    indices.emplace_back(face.mIndices[k]);
            }
        }
        return true;
    }

    bool createTexturedSquare(std::vector<TexVertex>& vertices, std::vector<uint32_t>& indices) {
        vertices = {
                {glm::vec3(-0.5f, -0.5f, 0.f), glm::vec2(0.f, 0.f),},// top left
                {glm::vec3(0.5f, -0.5f, 0.f),  glm::vec2(1.f, 0.f),},// top right
                {glm::vec3(0.5f, 0.5f, 0.f),   glm::vec2(1.f, 1.f),},// bottom right
                {glm::vec3(-0.5f, 0.5f, 0.f),  glm::vec2(0.f, 1.f),},// bottom left
        };

        indices = {
                0, 1, 2, 2, 3, 0
        };
        return true;
    }
};


