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

// Alternate/better way to do this using post processing : http://geoffprewett.com/blog/software/opengl-outline/
// The currently used produces bad results for meshes with transforms not centered at the mesh center. The transform of the mesh
// could corrected to be at its center : https://github.com/alexandrelipp/Velcro/issues/22

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
            .reference = 0,                    // Reference used as A in the compare op (after being and`ed with the compare mask)
    };                                         // Note : B is the stencil value, after being and`ed as well with the compare mask

    // set up specialization info to inject outline color in shader (when compiling it). Apparently can only be a scalar so we need 4
    std::array<VkSpecializationMapEntry, 4> mapEntries{};
    for (uint32_t i = 0; i < mapEntries.size(); ++i) {
        mapEntries[i].constantID = i;
        mapEntries[i].offset = i * sizeof(float);
        mapEntries[i].size = sizeof(float);
    }

    // add data
    VkSpecializationInfo specializationInfo = {
            .mapEntryCount = mapEntries.size(),
            .pMapEntries = mapEntries.data(),
            .dataSize = sizeof(OUTLINE_COLOR),
            .pData = &OUTLINE_COLOR // seems to work (address of constexpr)
                    // https://stackoverflow.com/questions/14872240/unique-address-for-constexpr-variable
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
    // upload projection view matrix
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

    // if defined, all the mesh outlines will be rendered. If not only the outline of all the meshes will be visible
#define RENDER_ALL_OUTLINES
#ifdef RENDER_ALL_OUTLINES
    // For each mesh :
    // 1. Render Mesh with stencil test that always fail but write to stencil
    // 2. Render scaled up mesh. Only outlined pixels will pass the stencil test
    // 3. Decrement (effectively clearing) stencil for next mesh
    for (auto mesh : _selectedMeshes) {
        // render mesh at its scale
        float value = 1.f;
        vkCmdPushConstants(commandBuffer, _pipelineLayout, _scaleFactor.stageFlags, _scaleFactor.offset, _scaleFactor.size,
                           &value);
        // will always fail, but will write to stencil buffer
        vkCmdSetStencilOp(commandBuffer, VK_STENCIL_FACE_FRONT_BIT,
                          VK_STENCIL_OP_INCREMENT_AND_CLAMP, // Fail OP -> increment stencil value
                          VK_STENCIL_OP_INCREMENT_AND_CLAMP, // Pass OP (never happens)
                          VK_STENCIL_OP_KEEP,                // Depth fail OP (never happens, no depth test)
                          VK_COMPARE_OP_GREATER);            // Always fail : Reference is 0 (nothing greater then 0)

        vkCmdDraw(commandBuffer, mesh->indexCount, 1, mesh->firstVertexIndex, mesh->meshIndex);

        // scale up mesh by the factor
        value = MAG_OUTLINE_FACTOR;
        vkCmdPushConstants(commandBuffer, _pipelineLayout, _scaleFactor.stageFlags, _scaleFactor.offset, _scaleFactor.size,
                           &value);

        // will only pass for pixels in outline
        vkCmdSetStencilOp(commandBuffer, VK_STENCIL_FACE_FRONT_BIT,
                          VK_STENCIL_OP_DECREMENT_AND_CLAMP,    // Pass OP -> decrement stencil value for next mesh
                          VK_STENCIL_OP_DECREMENT_AND_CLAMP,    // Fail OP -> decrement stencil value for next mesh
                          VK_STENCIL_OP_KEEP,                   // Depth fail OP (never happens, no depth test)
                          VK_COMPARE_OP_EQUAL);                 // Reference is 0. Only the pixels with a stencil value of 0
                                                                // (outline pixels) will pass
        vkCmdDraw(commandBuffer, mesh->indexCount, 1, mesh->firstVertexIndex, mesh->meshIndex);
    }

#else

    // render mesh at its scale
    float value = 1.f;
    vkCmdPushConstants(commandBuffer, _pipelineLayout, _scaleFactor.stageFlags, _scaleFactor.offset, _scaleFactor.size,
                       &value);
    // will always fail, but will write to stencil buffer
    vkCmdSetStencilOp(commandBuffer, VK_STENCIL_FACE_FRONT_BIT, VK_STENCIL_OP_INCREMENT_AND_CLAMP,
                      VK_STENCIL_OP_INCREMENT_AND_CLAMP,
                      VK_STENCIL_OP_KEEP, VK_COMPARE_OP_GREATER);
    for (auto mesh : _selectedMeshes) {
        vkCmdDraw(commandBuffer, mesh->indexCount, 1, mesh->firstVertexIndex, mesh->meshIndex);
    }

    // scale up mesh by the factor
    value = MAG_OUTLINE_FACTOR;
    vkCmdPushConstants(commandBuffer, _pipelineLayout, _scaleFactor.stageFlags, _scaleFactor.offset, _scaleFactor.size,
                       &value);

    // will only pass for pixels in outline
    vkCmdSetStencilOp(commandBuffer, VK_STENCIL_FACE_FRONT_BIT, VK_STENCIL_OP_KEEP, VK_STENCIL_OP_REPLACE,
                      VK_STENCIL_OP_KEEP, VK_COMPARE_OP_EQUAL);
    for (auto mesh : _selectedMeshes) {
        vkCmdDraw(commandBuffer, mesh->indexCount, 1, mesh->firstVertexIndex, mesh->meshIndex);
    }
#endif
}

void SelectedMeshLayer::onImGuiRender() {
    // display the scene hierarchy (starting from the root -> 0)
    ImGui::Begin("Scene");
    displayHierarchy(0);
    ImGui::End();

    // nothing to do if no selected entity
    if (_selectedEntity == -1)
        return;

    // display drag floats to control selected entity transform
    ImGui::Begin("Selected");
    auto& transform = _scene->getTransform(_selectedEntity);
    bool needUpdate = false;
    needUpdate |= ImGui::DragFloat3("Position", glm::value_ptr(transform.position), 0.01f);
    needUpdate |= ImGui::DragFloat3("Rotation", glm::value_ptr(transform.rotation), 0.01f);
    needUpdate |= ImGui::DragFloat3("Scale", glm::value_ptr(transform.scale), 0.01f, 0.f);

    // display scale drag (uniform scaling)
    ImGui::SameLine();
    float factor = transform.scale.x;

    // ## means its only used as a key (uniform won't be visible)
    if (ImGui::DragFloat("##Uniform", &factor, 0.01f)){
        transform.scale += transform.scale * (factor - transform.scale.x);
        transform.needUpdateModelMatrix = true;
        _scene->setDirtyTransform(_selectedEntity);
    }

    // notify the scene if the selected entity transform has changed
    if (needUpdate) {
        transform.needUpdateModelMatrix = true;
        _scene->setDirtyTransform(_selectedEntity);
    }
    ImGui::End();

    // finally, display the guizmo
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

    // set the selected entity if clicked on
    if (ImGui::IsItemClicked()) {
        _selectedEntity = entity;
        setSelectedEntity(_selectedEntity);
    }

    // recursively traverse all the children if the node is opened
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

