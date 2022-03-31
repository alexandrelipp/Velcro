//
// Created by alexa on 2022-03-19.
//

#include "FactoryModel.h"

#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/Importer.hpp>



bool FactoryModel::createDuckModel(std::vector<TexVertex>& vertices, std::vector<uint32_t>& indices) {
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

bool FactoryModel::createTexturedSquare(std::vector<TexVertex>& vertices, std::vector<uint32_t>& indices) {
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

void FactoryModel::importFromFile(const std::string& path, std::shared_ptr<Scene> scene) {
    Assimp::Importer importer;
    if (scene == nullptr)
        throw std::runtime_error("Scene is null");

    _aiScene = importer.ReadFile(path, aiProcess_Triangulate
                                       | aiProcess_JoinIdenticalVertices // without this, index buffer is useless
                                       | aiProcess_FlipWindingOrder // by default counter clockwise/ direct 11 wants clockwise
    );

    // TODO : prob want to remove this
    if (_aiScene == nullptr || _aiScene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !_aiScene->mRootNode){
        throw std::exception(importer.GetErrorString());
        _aiScene = nullptr;
        return;
    }

    SPDLOG_INFO("Num meshes {}", _aiScene->mNumMeshes);
    _filePath = path;

    traverseNodeRecursive(_aiScene->mRootNode, -1, 1, scene);
    scene->setDirtyTransform(0);

    SPDLOG_INFO("Num vertices {}", scene->_vertices.size());
    SPDLOG_INFO("Max index {}", *std::max_element(scene->_indices.begin(), scene->_indices.end()));

    // don't forget to reset the cached AiScene
    _aiScene = nullptr;
    _filePath.clear();
}

void FactoryModel::traverseNodeRecursive(aiNode* node, int parentEntity, int level, const std::shared_ptr<Scene>& scene) {
    // create the node entity and append its local transform
    int nodeEntity = scene->addSceneNode(parentEntity, level, node->mName.C_Str());

    scene->setTransform(nodeEntity, convertAiMat4(node->mTransformation));

    // increase the level for all mesh of node and children of node
    ++level;

    // iterate through the mesh of the node and add the mesh component to the scene
    for (int i = 0 ; i < node->mNumMeshes; ++i){
        int entityID = scene->addSceneNode(nodeEntity, level, getMeshName(node->mMeshes[i]));
        MeshComponent& mc = scene->createMesh(entityID);
        createMeshComponent(node->mMeshes[i], mc, scene);
    }

    // recursively traverse all children of the node
    for (int i = 0; i < node->mNumChildren; ++i){
        traverseNodeRecursive(node->mChildren[i], nodeEntity, level, scene);
    }
}

bool FactoryModel::createMeshComponent(int aiMeshIndex, MeshComponent& mc, std::shared_ptr<Scene> scene) {
    if (aiMeshIndex >= _aiScene->mNumMeshes)
        throw std::runtime_error("Index out of range");
    aiMesh* mesh = _aiScene->mMeshes[aiMeshIndex];

    // store the mesh first index (will be added to all indices since all indices of all meshes are stored continuously)
    uint32_t meshFirstVertexIndex = scene->_vertices.size();

    // get and add all vertices
    for (int i = 0; i < mesh->mNumVertices; ++i){
        auto& aiVertex = mesh->mVertices[i];
        auto& aiNormal = mesh->mNormals[i];
        Vertex vertex = {};
        //vertex.meshIndex = meshID;
        vertex.position.x = aiVertex.x;
        vertex.position.y = aiVertex.y;
        vertex.position.z = aiVertex.z;
        vertex.normal.x = aiNormal.x;
        vertex.normal.y = aiNormal.y;
        vertex.normal.z = aiNormal.z;
        //vertex.uv = // TODO : add UV!
        scene->_vertices.push_back(vertex);
    }

    // the index (in the index buffer) of the first index is the current index buffer size
    mc.firstVertexIndex = scene->_indices.size();

    // get and add all indices
    for (int i = 0; i < mesh->mNumFaces; ++i) {
        auto& face = mesh->mFaces[i];
        for (int j = 0; j < face.mNumIndices; ++j)
            scene->_indices.emplace_back(meshFirstVertexIndex + face.mIndices[j]);
    }

    // the index count is the current index buffer size - the first vertex index
    mc.indexCount = scene->_indices.size() - mc.firstVertexIndex;

    return true;
};

/// ai mats are row major, glm (and opengl) mats are column major ;  we can't type pun
glm::mat4 FactoryModel::convertAiMat4(const aiMatrix4x4& mat){
    return {         mat.a1, mat.b1, mat.c1, mat.d1,
                     mat.a2, mat.b2, mat.c2, mat.d2,
                     mat.a3, mat.b3, mat.c3, mat.d3,
                     mat.a4, mat.b4, mat.c4, mat.d4};
}

std::string FactoryModel::getMeshName(int meshIndex) {
    if (meshIndex >= _aiScene->mNumMeshes)
        throw std::runtime_error("Index out of range");
    aiMesh* mesh = _aiScene->mMeshes[meshIndex];
    return {mesh->mName.C_Str()};
}
