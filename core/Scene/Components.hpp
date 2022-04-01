#pragma once

#include <glm/glm.hpp>


struct TransformComponent{
    glm::mat4 localTransform = glm::mat4(1.f);
    glm::mat4 worldTransform = glm::mat4(1.f); // Absolute transform

    // relative pos/rot/scale
    glm::vec3 position = glm::vec3(0.f);
    glm::vec3 rotation = glm::vec3(0.f);
    glm::vec3 scale    = glm::vec3(1.f);
    bool needUpdateModelMatrix = true;
};

struct HierarchyComponent {
    int level = -1;
    int parent = -1;
    int firstChild = -1;
    int nextSibling = -1;
};

struct MeshComponent {
    uint32_t firstVertexIndex = 0; ///< Index in the index buffer of the first vertex
    uint32_t indexCount = 0;       ///< Number of indices
};
