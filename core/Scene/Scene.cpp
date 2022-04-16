//
// Created by Alex on 2022-02-20.
//

#include "Scene.h"

#include "../Utils/UtilsMath.h"

Scene::Scene(std::string name) : _name(std::move(name)){
    HierarchyComponent root = {};
    root.level = 0;

    _hierarchies.push_back(root);
    _transforms.emplace_back();
    _entityNames.emplace_back("Root");

    SPDLOG_INFO("Vertex Size {}", sizeof(Vertex));
}

int Scene::addSceneNode(int parent, int level, const std::string& name) {
    // set the root as the parent if none specified
    if (parent == -1)
        parent = 0;

    // add the ubiquitous components (except hie that will be added later)
    _transforms.emplace_back();
    _entityNames.push_back(name);

    // create new hierarchy component
    int newEntity = _hierarchies.size();
    HierarchyComponent newComponent = {};

    // only the root can have a level of 0
    if (level == 0)
        level = 1;
    HierarchyComponent& pc = _hierarchies[parent];

    // sanity checks, should never happen
    if (pc.level + 1 != level){
        SPDLOG_INFO("Specified level is not valid with parent's level. Correcting it\n");
        level = pc.level + 1;
    }
    if (level > MAX_LEVELS)
        throw std::runtime_error("Level is too big");

    // assign level + parent to new node
    newComponent.level = level;
    newComponent.parent = parent;

    // if we it's the first child, simply set the newEntity as the firstChild
    if (pc.firstChild == -1){
        pc.firstChild = newEntity;
    }
    // if we are not, set the newEntity as the nextSibling of the lastChild of the parent
    else{
        for(int e = pc.firstChild; e != -1; e = _hierarchies[e].nextSibling){
            if (_hierarchies[e].nextSibling == -1){
                _hierarchies[e].nextSibling = newEntity;
                break;
            }
        }
    }

    _hierarchies.push_back(newComponent);
    return newEntity;
}


std::string& Scene::getName(int entity) {
    return _entityNames[entity];
}

HierarchyComponent& Scene::getHierarchy(int entity) {
    return _hierarchies[entity];
}

TransformComponent& Scene::getTransform(int entity) {
    return _transforms[entity];
}

MeshComponent* Scene::getMesh(int entity) {
    auto it = _meshesMap.find(entity);
    if (it == _meshesMap.end())
        return nullptr;
    return &_meshes[it->second];
}

/// creates a mesh for the given entityID and returns the created mesh
MeshComponent& Scene::createMesh(int entityID){
    // check if the mesh already exists
    if (_meshesMap.find(entityID) != _meshesMap.end()){
        SPDLOG_INFO("Mesh already exists\n");
        return _meshes[_meshesMap[entityID]];
    }
    // create new mesh
    int newMeshID = _meshTransforms.size();
    _meshTransforms.emplace_back(1.f);
    _meshes.emplace_back();
    _meshes.back().meshIndex = newMeshID;

    // associate entity with new mesh
    _meshesMap[entityID] = newMeshID;
    return _meshes.back();
}

void Scene::setTransform(int entity, const glm::mat4& transform){
    if (entity == -1)
        throw std::runtime_error("Invalid entity");
    auto& tc = getTransform(entity);
    tc.localTransform = transform;
    utils::decomposeTransform(transform, tc.position, tc.rotation, tc.scale);
    tc.needUpdateModelMatrix = false;

    // recursively mark this entity and all of his children as dirty
    traverseRecursive(entity, [this](int entity){
        auto& hie = _hierarchies[entity];
        _changedTransforms[hie.level].insert(entity);
    });
}

void Scene::setDirtyTransform(int entity) {
    traverseRecursive(entity, [this](int entity){
        auto& hie = _hierarchies[entity];
        _changedTransforms[hie.level].insert(entity);
    });
}


void Scene::propagateTransforms() {
    // special logic for root (no parent)
    if (!_changedTransforms[0].empty()){
        int root = *_changedTransforms[0].begin();
        VK_ASSERT(root == 0, "no bueno amigo");

        auto& tc = getTransform(root);
        // update the local transform if required
        if (tc.needUpdateModelMatrix){
            tc.localTransform = utils::calculateModelMatrix(tc.position, tc.scale, tc.rotation);
            tc.needUpdateModelMatrix = false;
        }
        tc.worldTransform = tc.localTransform;
        _changedTransforms[0].clear();
    }

    // propagate the transforms, going down the levels
    for (int i = 1; i < MAX_LEVELS; ++i){
        if (_changedTransforms[i].empty())
            continue;

        for (auto entity : _changedTransforms[i]){
            auto& hie = _hierarchies[entity];
            auto& tc = _transforms[entity];

            // update the local transform if required
            if (tc.needUpdateModelMatrix){
                tc.localTransform = utils::calculateModelMatrix(tc.position, tc.scale, tc.rotation);
                tc.needUpdateModelMatrix = false;
            }

            // compute world transform using parent
            tc.worldTransform = _transforms[hie.parent].worldTransform * tc.localTransform;

            // update the mesh transform at well if it exists
            auto it = _meshesMap.find(entity);
            if (it != _meshesMap.end()){
                _meshTransforms[it->second] = tc.worldTransform;
            }
        }
        _changedTransforms[i].clear();
    }
}

void Scene::traverseRecursive(int entity, std::function<void(int)> foo) {
    if (entity == -1)
        return;
    // pass by ref for stuff like deleting
    auto hie = _hierarchies[entity];
    foo(entity);

    for (int e = hie.firstChild; e != -1; e = _hierarchies[e].nextSibling)
        traverseRecursive(e, foo);
}

std::pair<Vertex*, uint32_t> Scene::getVerticesData() {
    return std::make_pair(_vertices.data(), _vertices.size() * sizeof(_vertices[0]));
}


std::pair<uint32_t*, uint32_t> Scene::getIndicesData() {
    return std::make_pair(_indices.data(), _indices.size() * sizeof(_indices[0]));
}

const std::vector<MeshComponent>& Scene::getMeshes() {
    return _meshes;
}

const std::vector<glm::mat4>& Scene::getMeshTransforms() {
    return _meshTransforms;
}

const std::vector<Material>& Scene::getMaterials() {
    return _materials;
}
