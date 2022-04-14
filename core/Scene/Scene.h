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

private:

    /// ubiquitous components
    std::vector<HierarchyComponent> _hierarchies;
    std::vector<TransformComponent> _transforms;
    std::vector<std::string> _entityNames;

    /// map of entity to meshID
    std::unordered_map<int, int> _meshesMap;
    std::vector<glm::mat4> _meshTransforms; ///< transforms to be uploaded to gpu
    std::vector<MeshComponent> _meshes;

    /// map of node ID to materialID
    std::unordered_map<int, int> _materials;

    std::string _name;

    static constexpr uint32_t MAX_LEVELS = 16;


    /// used to only recompute necessary transforms, note : it might be faster/simpler to always recompute all transforms
    /// in certain case. Profile to get an accurate answer
    std::array<std::unordered_set<int>, MAX_LEVELS> _changedTransforms;

    std::vector<Vertex> _vertices;
    std::vector<uint32_t> _indices;

};






