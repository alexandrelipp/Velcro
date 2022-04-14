//
// Created by alexa on 2022-03-26.
//


#include "MultiMeshLayer.h"

#include "../Factory/FactoryVulkan.h"
#include "../Factory/FactoryModel.h"

#include "../../Utils/UtilsMath.h"

#include "../../Application.h"
#include "../../events/KeyEvent.h"


#include <imgui.h>

MultiMeshLayer::MultiMeshLayer(VkRenderPass renderPass) {
    // static assert making sure no padding is added to our struct
    static_assert(sizeof(InstanceData) == sizeof(InstanceData::transform) + sizeof(InstanceData::indexOffset) +
        sizeof(InstanceData::materialIndex) + sizeof(InstanceData::meshIndex));

    // init the uniform buffers
    for (auto& buffer : _vpUniformBuffers)
        buffer.init(_vrd->device, _vrd->physicalDevice, sizeof(glm::mat4));

    _scene = std::make_shared<Scene>("NanoWorld");
    //FactoryModel::importFromFile("../../../core/Assets/Models/Nano/nanosuit.obj", _scene);
    FactoryModel::importFromFile("../../../core/Assets/Models/utahTeapot.fbx", _scene);
    //FactoryModel::importFromFile("../../../core/Assets/Models/engine.fbx", _scene);
    //FactoryModel::importFromFile("../../../core/Assets/Models/Bell Huey.fbx", _scene);
    //FactoryModel::importFromFile("../../../core/Assets/Models/duck/scene.gltf", _scene);

    static_assert(sizeof(Vertex) == sizeof(Vertex::position) + sizeof(Vertex::normal) + sizeof(Vertex::uv));

    auto [vertices, vtxSize] = _scene->getVerticesData();
    auto [indices, idxSize] = _scene->getIndicesData();

    // init the vertices ssbo
    _vertices.init(_vrd->device, _vrd->physicalDevice, vtxSize);
    VK_ASSERT(_vertices.setData(_vrd->device, _vrd->physicalDevice, _vrd->graphicsQueue, _vrd->commandPool,
                                vertices, vtxSize), "set data failed");

    // init the indices ssbo
    _indices.init(_vrd->device, _vrd->physicalDevice, idxSize);
    VK_ASSERT(_indices.setData(_vrd->device, _vrd->physicalDevice, _vrd->graphicsQueue, _vrd->commandPool,
                               indices, idxSize), "set data failed");


    // add all meshes as indirect commands
    std::vector<VkDrawIndirectCommand> indirectCommands;
    const auto& meshes = _scene->getMeshes();
    for (uint32_t i = 0; i < meshes.size(); ++i){
        indirectCommands.push_back({
               .vertexCount = meshes[i].indexCount,
               .instanceCount = 1,
               .firstVertex = meshes[i].firstVertexIndex,
               .firstInstance = i
       });
    }

    // set the commands in the command buffer
    _indirectCommandBuffer.init(_vrd->device, _vrd->physicalDevice, indirectCommands.size() * sizeof(indirectCommands[0]),
                                false, VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT);
    VK_ASSERT(_indirectCommandBuffer.setData(_vrd->device, _vrd->physicalDevice, _vrd->graphicsQueue, _vrd->commandPool,
                      indirectCommands.data(), indirectCommands.size() * sizeof(indirectCommands[0])), "set data failed");

    // init the transform buffers
    for (auto& buffer : _meshTransformBuffers){
        buffer.init(_vrd->device, _vrd->physicalDevice, meshes.size() * sizeof(glm::mat4), true);
    }

    // init the statue texture
    //_texture.init("../../../core/Assets/Models/duck/textures/Duck_baseColor.png", _vrd->device, _vrd->physicalDevice, _vrd->graphicsQueue, _vrd->commandPool);

    // create our graphics pipeline
    createDescriptors();
    Factory::GraphicsPipelineProps props = {
            .shaders =  {
                    .vertex = "multiV.spv",
                    .fragment = "multiF.spv"
            },
            .sampleCountMSAA = _vrd->sampleCount
    };
    _graphicsPipeline = Factory::createGraphicsPipeline(_vrd->device, _swapchainExtent, renderPass, _pipelineLayout, props);

    SelectedMeshLayer::Props selectedMeshProps = {
        .scene = _scene,
        .vertices = _vertices,
        .indices = _indices,
        .meshTransformBuffers = _meshTransformBuffers
    };

    // create the selected mesh layer
    _selectedMeshLayer = std::make_shared<SelectedMeshLayer>(renderPass, selectedMeshProps);
}

MultiMeshLayer::~MultiMeshLayer() {
    // destroy the buffers
    for (auto& buffer : _vpUniformBuffers)
        buffer.destroy(_vrd->device);

    for (auto& buffer : _meshTransformBuffers)
        buffer.destroy(_vrd->device);

    _indirectCommandBuffer.destroy(_vrd->device);
    _vertices.destroy(_vrd->device);
    _indices.destroy(_vrd->device);

    //_texture.destroy(_vrd->device);
}

void MultiMeshLayer::fillCommandBuffer(VkCommandBuffer commandBuffer, uint32_t commandBufferIndex) {
    Camera* camera = Application::getApp()->getRenderer()->getCamera();
    // bind pipeline
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _graphicsPipeline);

    glm::vec4 value = glm::vec4(*camera->getPosition(), _specularS);
    // push the camera pos and bind descriptor sets
    vkCmdPushConstants(commandBuffer, _pipelineLayout, _cameraPosPC.stageFlags, _cameraPosPC.offset, _cameraPosPC.size, &value);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipelineLayout,
                            0, 1, &_descriptorSets[commandBufferIndex], 0, nullptr);

    // render
    vkCmdDrawIndirect(commandBuffer, _indirectCommandBuffer.getBuffer(), 0,
                      _scene->getMeshes().size(), sizeof(VkDrawIndirectCommand));
}

void MultiMeshLayer::update(float dt, uint32_t commandBufferIndex, const glm::mat4& pv) {
    _scene->propagateTransforms();
    glm::mat4 nec = pv; // TODO : necessary??
    VK_ASSERT(_vpUniformBuffers[commandBufferIndex].setData(_vrd->device, glm::value_ptr(nec), sizeof(pv)), "Failed to dat");
    const auto& transforms = _scene->getMeshTransforms();
    _meshTransformBuffers[commandBufferIndex].setData(_vrd->device, _vrd->physicalDevice, _vrd->graphicsQueue, _vrd->commandPool,
                                                      (void*)transforms.data(), transforms.size() * sizeof(transforms[0]));
}

void MultiMeshLayer::onEvent(Event& event) {
    switch (event.getType()) {
        case Event::Type::KEY_PRESSED:{
            KeyPressedEvent* keyPressedEvent = (KeyPressedEvent*)&event;
            if (keyPressedEvent->getRepeatCount() != 0)
                return;
            switch(keyPressedEvent->getKeyCode()){
                case KeyCode::D:
//                    if (isEditing() && Application::getApp()->isKeyPressed(KeyCode::LeftControl)){
//                        if (SceneEditor::getSelectedEntity() != entt::null){
//                            _editorScene->duplicateEntity(SceneEditor::getSelectedEntity());
//                        }
//                    }
                    break;
                case KeyCode::S:
                    if (!Application::getApp()->isKeyPressed(KeyCode::LeftControl)){
                        _operation = ImGuizmo::OPERATION::SCALE;
                        return;
                    }
                    break;
                case KeyCode::G:
                    _operation = ImGuizmo::OPERATION::TRANSLATE;
                    break;
                case KeyCode::R:
                    _operation = ImGuizmo::OPERATION::ROTATE;
                    break;
                default:
                    break;
            }
        }
        case Event::Type::MOUSE_PRESSED:{
//            auto mouseButtonPressedEvent = (MouseButtonPressedEvent*)&event;
//            if (_viewPortHovered && !ImGuizmo::IsOver() && mouseButtonPressedEvent->getMouseButton() == MouseCode::ButtonLeft){
//                SceneEditor::setSelectedEntity(_hoveredEntity);
//            }
            break;
        }

        default:
            break;
    }
}

void MultiMeshLayer::onImGuiRender() {
    ImGui::Begin("Scene");
    displayHierarchy(0);
    ImGui::End();

    if (_selectedEntity == -1)
        return;

    ImGui::Begin("Selected");

    auto& transform = _scene->getTransform(_selectedEntity);
    bool needUpdate = false;
    needUpdate |= ImGui::DragFloat3("Position", glm::value_ptr(transform.position), 0.01f);
    needUpdate |= ImGui::DragFloat3("Rotation", glm::value_ptr(transform.rotation), 0.01f);
    needUpdate |= ImGui::DragFloat3("Scale", glm::value_ptr(transform.scale), 0.01f, 0.f);
    ImGui::SameLine();
    float factor = transform.scale.x;


    if (ImGui::DragFloat("##Unifrorm", &factor, 0.01f)){
        transform.scale += transform.scale * (factor - transform.scale.x);
        transform.needUpdateModelMatrix = true;
        _scene->setDirtyTransform(_selectedEntity);
    }

    if (needUpdate) {
        transform.needUpdateModelMatrix = true;
        _scene->setDirtyTransform(_selectedEntity);
    }

    ImGui::End();

    ImGui::Begin("Specular");
    ImGui::DragFloat("s", &_specularS, 0.1f, 0.f, 10.f);

    ImGui::End();

    displayGuizmo(_selectedEntity);
}

std::shared_ptr<SelectedMeshLayer> MultiMeshLayer::getSelectedMeshLayer() {
    return _selectedMeshLayer;
}

void MultiMeshLayer::createDescriptors() {
    std::vector<Factory::Descriptor> descriptors = {
            {
                    .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                    .shaderStage = VK_SHADER_STAGE_VERTEX_BIT,
                    .info = std::array<VkDescriptorBufferInfo, MAX_FRAMES_IN_FLIGHT>{
                            VkDescriptorBufferInfo {_vpUniformBuffers[0].getBuffer(), 0, _vpUniformBuffers[0].getSize()},
                            VkDescriptorBufferInfo {_vpUniformBuffers[1].getBuffer(), 0, _vpUniformBuffers[1].getSize()},
                    }
            },
            {
                    .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                    .shaderStage = VK_SHADER_STAGE_VERTEX_BIT,
                    .info = std::array<VkDescriptorBufferInfo, MAX_FRAMES_IN_FLIGHT>{
                            VkDescriptorBufferInfo {_vertices.getBuffer(), 0, _vertices.getSize()},
                            VkDescriptorBufferInfo {_vertices.getBuffer(), 0, _vertices.getSize()},
                    }
            },
            {
                    .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                    .shaderStage = VK_SHADER_STAGE_VERTEX_BIT,
                    .info = std::array<VkDescriptorBufferInfo, MAX_FRAMES_IN_FLIGHT>{
                            VkDescriptorBufferInfo {_indices.getBuffer(), 0, _indices.getSize()},
                            VkDescriptorBufferInfo {_indices.getBuffer(), 0, _indices.getSize()},
                    }
            },
            {
                    .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                    .shaderStage = VK_SHADER_STAGE_VERTEX_BIT,
                    .info = std::array<VkDescriptorBufferInfo, MAX_FRAMES_IN_FLIGHT>{
                            VkDescriptorBufferInfo {_meshTransformBuffers[0].getBuffer(), 0, _meshTransformBuffers[0].getSize()},
                            VkDescriptorBufferInfo {_meshTransformBuffers[1].getBuffer(), 0, _meshTransformBuffers[1].getSize()},
                    }
            },
    };

    // create fragment push constant for camera pos
    _cameraPosPC = {
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
            .offset = 0,               // must be multiple of 4 (offset into push constant block)
            .size = sizeof(glm::vec3) + sizeof(_specularS), // must be multiple of 4
    };

    // create descriptors
    std::tie(_descriptorSetLayout, _pipelineLayout, _descriptorPool, _descriptorSets) =
            Factory::createDescriptorSets(_vrd, descriptors, {_cameraPosPC});
}

void MultiMeshLayer::displayHierarchy(int entity) {
    if (entity == -1)
        return;

    // default flags and additional flag if selected
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth |ImGuiTreeNodeFlags_DefaultOpen;
    if (entity == _selectedEntity)
        flags |= ImGuiTreeNodeFlags_Selected | ImGuiTreeNodeFlags_DefaultOpen;

    // get tag and hierarchy
    HierarchyComponent& hc = _scene->getHierarchy(entity);
    if (hc.firstChild == -1)
        flags |= ImGuiTreeNodeFlags_Leaf;

    // check if the node is opened and if it's the selected entity
    bool opened = ImGui::TreeNodeEx((void*) entity, flags, _scene->getName(entity).c_str());

    ImGui::PushID((int)entity);

    if (ImGui::IsItemClicked()) {
        _selectedEntity = entity;
        _selectedMeshLayer->setSelectedEntity(_selectedEntity);
    }

    if (opened) {
        for (int e = hc.firstChild; e != -1; e = _scene->getHierarchy(e).nextSibling)
            displayHierarchy(e);
        ImGui::TreePop();
    }

    ImGui::PopID();
}

void MultiMeshLayer::displayGuizmo(int selectedEntity) {

    if(selectedEntity == -1){
        SPDLOG_ERROR("Cannot display guizmo of null entity or null scene");
        return;
    }

    Camera* camera = Application::getApp()->getRenderer()->getCamera();

    // set up imguizmo
    ImGuizmo::SetOrthographic(false);
    ImGuizmo::SetRect(0, 0, (float)Application::getApp()->getWindowWidth(), (float)Application::getApp()->getWindowHeight());

    // add optional snapping
    float* snapValue = nullptr;
    if (Application::getApp()->isKeyPressed(KeyCode::LeftControl)){
        // This value could be set in UI
        float constexpr snapT = 0.25f;
        static float snap[3] = {snapT, snapT, snapT};
        snapValue = snap;
    }

    // get the transform
    auto& ntc =  _scene->getTransform(selectedEntity);
    glm::mat4 transform = ntc.worldTransform;

    // the Y is flipped for vulkan (see camera!), but ImGuizmo does not expect it to be flipped, so we flip it back!
    glm::mat4 projectionMatrix = *(glm::mat4*)camera->getProjectionMatrix();
    projectionMatrix[1][1] *= -1;

    // we add a snapping value if ctrl is pressed
    ImGuizmo::Manipulate(camera->getViewMatrix(), glm::value_ptr(projectionMatrix),
                         _operation, ImGuizmo::MODE::LOCAL, glm::value_ptr(transform), nullptr, snapValue);


    if (ImGuizmo::IsUsing()) {
        auto& hc = _scene->getHierarchy(selectedEntity);

        // updated local transform
        glm::mat4 localTransform;

        // if we don't have a parent, simply set the updated transform as the local transform
        if (hc.parent == -1){
            localTransform = transform;
        }
        // if we have a parent, we need to take in count its world transform
        else {
            auto& parentH = _scene->getHierarchy(hc.parent);
            auto& parentTC = _scene->getTransform(hc.parent);
            localTransform = glm::inverse(parentTC.worldTransform) * transform;
        }

        // decompose our transform into the position and the scale
        glm::vec3 rotation;
        utils::decomposeTransform(localTransform, ntc.position, rotation, ntc.scale);

        // this should prevent gimbal lock
        glm::vec3 deltaRotation = rotation - ntc.rotation;
        ntc.rotation += deltaRotation;
        ntc.needUpdateModelMatrix = true;

        // notify the scene about the change
        _scene->setDirtyTransform(selectedEntity);
    }
}