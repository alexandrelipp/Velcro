//
// Created by alexa on 2022-04-11.
//

#include "SelectedMeshLayer.h"

#include "../Factory/FactoryVulkan.h"
#include "../Factory/FactoryModel.h"
#include "../../Utils/UtilsVulkan.h"
#include "../../Utils/UtilsTemplate.h"
#include "../../Application.h"
#include "../../Utils/UtilsMath.h"
#include "../../events/KeyEvent.h"

// Better way to do this using post processing : http://geoffprewett.com/blog/software/opengl-outline/

SelectedMeshLayer::SelectedMeshLayer(VkRenderPass renderPass, const Props& props) : _scene(props.scene) {
    // init the uniform buffers
    for (auto& buffer : _vpUniformBuffers)
        buffer.init(_vrd->device, _vrd->physicalDevice, sizeof(glm::mat4));

    // describe descriptors
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
                            VkDescriptorBufferInfo {props.vertices.getBuffer(), 0, props.vertices.getSize()},
                            VkDescriptorBufferInfo {props.vertices.getBuffer(), 0, props.vertices.getSize()},
                    }
            },
            {
                    .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                    .shaderStage = VK_SHADER_STAGE_VERTEX_BIT,
                    .info = std::array<VkDescriptorBufferInfo, MAX_FRAMES_IN_FLIGHT>{
                            VkDescriptorBufferInfo {props.indices.getBuffer(), 0, props.indices.getSize()},
                            VkDescriptorBufferInfo {props.indices.getBuffer(), 0, props.indices.getSize()},
                    }
            },
            {
                    .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                    .shaderStage = VK_SHADER_STAGE_VERTEX_BIT,
                    .info = std::array<VkDescriptorBufferInfo, MAX_FRAMES_IN_FLIGHT>{
                            VkDescriptorBufferInfo {props.meshTransformBuffers[0].getBuffer(), 0, props.meshTransformBuffers[0].getSize()},
                            VkDescriptorBufferInfo {props.meshTransformBuffers[1].getBuffer(), 0, props.meshTransformBuffers[1].getSize()},
                    }
            },
    };

    // push constant for factor of outline thickness
    _scaleFactor = {
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
            .offset = 0,
            .size = sizeof(float),
    };

    // create descriptors
    std::tie(_descriptorSetLayout, _pipelineLayout, _descriptorPool, _descriptorSets) =
            Factory::createDescriptorSets(_vrd, descriptors, {_scaleFactor});

    // Fill up front stencil state as described by the spec :
    // https://www.khronos.org/registry/vulkan/specs/1.3/html/chap26.html#fragops-stencil
    VkStencilOpState frontStencilState = {
            .failOp = VK_STENCIL_OP_KEEP,      // will be set dynamically
            .passOp = VK_STENCIL_OP_KEEP,      // will be set dynamically
            .depthFailOp = VK_STENCIL_OP_KEEP, // operation if stencil passes, but depth fails. Will never happen, depth test is disabled
            .compareOp = VK_COMPARE_OP_NEVER,  // will be set dynamically
            .compareMask = 0xffffffff,         // compare mask
            .writeMask = 0xffffffff,           // always write
            .reference = 0,
    };

    // set up specialization info to inject outline color in shader (when compiling it). Apparently can only be a scalar so we need 4
    std::array<VkSpecializationMapEntry, 4> mapEntries{};

    for (uint32_t i = 0; i < mapEntries.size(); ++i) {
        mapEntries[i].constantID = i;
        mapEntries[i].offset = i * sizeof(float);
        mapEntries[i].size = sizeof(float);
    }
    VkSpecializationInfo specializationInfo = {
            .mapEntryCount = mapEntries.size(),
            .pMapEntries = mapEntries.data(),
            .dataSize = sizeof(OUTLINE_COLOR),
            .pData = &OUTLINE_COLOR // TODO : address of constexpr ??
    };

    // create graphics pipeline
    Factory::GraphicsPipelineProps factoryProps = {
            .shaders =  {
                    .vertex = "SelectedMeshV.spv",
                    .fragment = "SelectedMeshF.spv",
                    .fragmentSpec = &specializationInfo
            },
            .enableDepthTest = VK_FALSE, // depth dest is disabled
            .enableStencilTest = VK_TRUE,
            .frontStencilState = frontStencilState,
            .sampleCountMSAA = _vrd->sampleCount,
            .dynamicStates = {
                    VK_DYNAMIC_STATE_STENCIL_OP, // we dynamically change the stencil operation
                    }
    };
    _graphicsPipeline = Factory::createGraphicsPipeline(_vrd->device, _swapchainExtent, renderPass, _pipelineLayout, factoryProps);

}

SelectedMeshLayer::~SelectedMeshLayer() {
    // destroy the buffers
    for (auto& buffer : _vpUniformBuffers)
        buffer.destroy(_vrd->device);
}

void SelectedMeshLayer::update(float dt, uint32_t commandBufferIndex, const glm::mat4& pv) {
    if (_selectedEntity == -1 || _selectedMeshes.empty() )
        return;
//    glm::mat4 model = _scene->getTransform(_selectedEntity).worldTransform;
//    SelectedMeshMVP mvps = {
//            .original = pv * model,
//            .scaledUp = pv * glm::scale(model, glm::vec3(MAG_STENCIL_FACTOR))
//    };
    _vpUniformBuffers[commandBufferIndex].setData(_vrd->device, (void*)&pv, sizeof(pv));
}

void SelectedMeshLayer::onEvent(Event& event) {
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

void SelectedMeshLayer::fillCommandBuffer(VkCommandBuffer commandBuffer, uint32_t commandBufferIndex) {
    // nothing to do if no selected entity
    if (_selectedEntity == -1 || _selectedMeshes.empty() )
        return;

    // bind the layer
    bindPipelineAndDS(commandBuffer, commandBufferIndex);

    // at the beginning of the render pass, the stencil buffer is cleared with 0's


    // TODO : fix!! we should always see all selected mesh
    float value = 1.f;
    vkCmdPushConstants(commandBuffer, _pipelineLayout, _scaleFactor.stageFlags, _scaleFactor.offset, _scaleFactor.size,
                       &value);
    for (auto mesh : _selectedMeshes) {
        //
        vkCmdSetStencilOp(commandBuffer, VK_STENCIL_FACE_FRONT_BIT, VK_STENCIL_OP_INCREMENT_AND_CLAMP,
                          VK_STENCIL_OP_INCREMENT_AND_CLAMP,
                          VK_STENCIL_OP_KEEP, VK_COMPARE_OP_GREATER);
        vkCmdDraw(commandBuffer, mesh->indexCount, 1, mesh->firstVertexIndex, mesh->meshIndex);
    }

    value = MAG_STENCIL_FACTOR;
    vkCmdPushConstants(commandBuffer, _pipelineLayout, _scaleFactor.stageFlags, _scaleFactor.offset, _scaleFactor.size,
                       &value);
    for (auto mesh : _selectedMeshes) {
        vkCmdSetStencilOp(commandBuffer, VK_STENCIL_FACE_FRONT_BIT, VK_STENCIL_OP_KEEP, VK_STENCIL_OP_REPLACE,
                          VK_STENCIL_OP_KEEP, VK_COMPARE_OP_EQUAL);
        vkCmdDraw(commandBuffer, mesh->indexCount, 1, mesh->firstVertexIndex, mesh->meshIndex);
    }

    return;


    // TODO : maybe don't change stencil op every time ??
    for (auto mesh : _selectedMeshes){
        float value = 1.f;
        vkCmdPushConstants(commandBuffer, _pipelineLayout, _scaleFactor.stageFlags, _scaleFactor.offset, _scaleFactor.size, &value);

        //
        vkCmdSetStencilOp(commandBuffer, VK_STENCIL_FACE_FRONT_BIT, VK_STENCIL_OP_INCREMENT_AND_CLAMP, VK_STENCIL_OP_INCREMENT_AND_CLAMP,
                          VK_STENCIL_OP_KEEP, VK_COMPARE_OP_GREATER);
        vkCmdDraw(commandBuffer, mesh->indexCount, 1, mesh->firstVertexIndex, mesh->meshIndex);

        value = 1.03f;
        vkCmdPushConstants(commandBuffer, _pipelineLayout, _scaleFactor.stageFlags, _scaleFactor.offset, _scaleFactor.size, &value);

        vkCmdSetStencilOp(commandBuffer, VK_STENCIL_FACE_FRONT_BIT, VK_STENCIL_OP_KEEP, VK_STENCIL_OP_REPLACE,
                          VK_STENCIL_OP_KEEP, VK_COMPARE_OP_EQUAL);
        vkCmdDraw(commandBuffer, mesh->indexCount, 1, mesh->firstVertexIndex, mesh->meshIndex);
    }



    //vkCmdSetStencilTestEnable(commandBuffer, VK_FALSE);
    //vkCmdSetStencilCompareMask(commandBuffer, VK_STENCIL_FACE_FRONT_BIT, 0xffffff);

}

void SelectedMeshLayer::onImGuiRender() {
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

    displayGuizmo(_selectedEntity);
}

void SelectedMeshLayer::setSelectedEntity(int selectedEntity) {
    // set selected entity
    _selectedEntity = selectedEntity;
    _selectedMeshes.clear();

    // nothing to do if no selected entity
    if (selectedEntity == -1)
        return;

    // append all mesh components that are child of the selected entity
    _scene->traverseRecursive(_selectedEntity, [this](int entity){
        MeshComponent* mesh = _scene->getMesh(entity);
        if (mesh != nullptr)
            _selectedMeshes.push_back(mesh);
    });
    SPDLOG_INFO("Selected mesh name {}", _scene->getName(selectedEntity));
}

void SelectedMeshLayer::displayHierarchy(int entity) {
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
        setSelectedEntity(_selectedEntity);
    }

    if (opened) {
        for (int e = hc.firstChild; e != -1; e = _scene->getHierarchy(e).nextSibling)
            displayHierarchy(e);
        ImGui::TreePop();
    }

    ImGui::PopID();
}

void SelectedMeshLayer::displayGuizmo(int selectedEntity) {

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

