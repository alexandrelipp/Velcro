//
// Created by Alex on 2022-02-20.
//

#pragma once

#include "Components.hpp"

#include <array>
#include <vector>
#include <unordered_map>
#include <functional>
#include <unordered_set>

// TODO : padding?
struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 uv = glm::vec2(0.f);
   // int meshIndex;
};

class Scene {
    // these friends class are pretty stupid
    friend class FactoryModel;
//    friend class ShaderMaterial;
public:
    Scene(std::string name);

    std::string& getName(int entity);

    // Note : Maybe this needs to be redesigned. It is unsafe to return a pointer in a vector. If the buffer of the vector
    // needs to be reallocated and the pointer / reference is used, the program will likely crash or unexpected result
    HierarchyComponent& getHierarchy(int entity);
    TransformComponent& getTransform(int entity);
    MeshComponent* getMesh(int entity);

    int addSceneNode(int parent, int level, const std::string& name);

    /// creates a mesh for the given entityID and returns the created mesh
    MeshComponent& createMesh(int entityID);

    void setTransform(int entity, const glm::mat4& transform);
    void setDirtyTransform(int entity);

    void propagateTransforms();

    void traverseRecursive(int entity, std::function<void(int entt)> foo);

    /// pair of vertex* / size(in bytes)
    std::pair<Vertex*, uint32_t> getVerticesData();

    /// return pair of index* / size(in bytes)
    std::pair<uint32_t*, uint32_t> getIndicesData();

    /// return pair of transform* / size(in bytes)
    const std::vector<glm::mat4>& getMeshTransforms();

    const std::vector<MeshComponent>& getMeshes();
    const std::vector<Material>& getMaterials();

private:

    /// ubiquitous components
    std::vector<HierarchyComponent> _hierarchies;
    std::vector<TransformComponent> _transforms;
    std::vector<std::string> _entityNames;

    /// map of entity to meshID
    std::unordered_map<int, int> _meshesMap;
    std::vector<glm::mat4> _meshTransforms; ///< transforms to be uploaded to gpu
    std::vector<MeshComponent> _meshes;

    ///< vector of material data and material name. Index in vector corresponds to the meshes material index
    std::vector<Material> _materials;
    std::vector<std::string> _materialsNames;

    std::string _name;          ///< name of the scene

    static constexpr uint32_t MAX_LEVELS = 16;


    /// used to only recompute necessary transforms, note : it might be faster/simpler to always recompute all transforms
    /// in certain case. Profile to get an accurate answer. If done so, the transforms must be sorted by level
    std::array<std::unordered_set<int>, MAX_LEVELS> _changedTransforms;

    std::vector<Vertex> _vertices;
    std::vector<uint32_t> _indices;

};






