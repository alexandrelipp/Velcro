//
// Created by alexa on 2022-03-19.
//

#pragma once

#include <glm/glm.hpp>
#include <assimp/scene.h>
#include "../../Scene/Scene.h"

struct TexVertex{
    glm::vec3 position;
    glm::vec2 uv;
};

struct TexVertex2{
    glm::vec2 position;
    glm::vec2 uv;
};

class FactoryModel {
public:
    static bool createDuckModel(std::vector<TexVertex>& vertices, std::vector<uint32_t>& indices);

    // NOTE : These 2 methods will work badly if in 3d space, where y axis is from bottom top top. Don't use with projections!
    static bool createTexturedSquare(std::vector<TexVertex>& vertices, std::vector<uint32_t>& indices);
    static bool createTexturedSquare2(std::vector<TexVertex2>& vertices);

    static void importFromFile(const std::string& path, std::shared_ptr<Scene> scene);

private:
    static void traverseNodeRecursive(aiNode* node, int parentEntity, int level, const std::shared_ptr<Scene>& scene);

    // Helper methods
    static bool createMeshComponent(int aiMeshIndex, MeshComponent& mc, std::shared_ptr<Scene> scene);
    static std::string getMeshName(int meshIndex);
    static glm::mat4 convertAiMat4(const aiMatrix4x4& mat);
    static glm::vec3 convertAiColor3D(const aiColor3D& color);


    static inline const aiScene* _aiScene = nullptr; ///<cached scene
    ///< index of the first material in the scene. Relevant if importing multiple models to a scene
    static inline uint32_t _firstMaterialIndex = 0;
    static inline std::string _filePath; ///< cached filepath

    // constants
    ///< Minimum ambient color of a material. Necessary because a lot of assimp materials have 0 as ambient color.
    /// Note: we could also have a maximum ambient color
    static constexpr glm::vec3 MATERIAL_MIN_AMBIENT = glm::vec3(0.1f);
};