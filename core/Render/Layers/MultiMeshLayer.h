//
// Created by alexa on 2022-03-20.
//

#pragma once


#include "RenderLayer.h"
#include "../Objects/UniformBuffer.h"
#include "../Objects/ShaderStorageBuffer.h"
#include "../Objects/Texture.h"
#include "../../Scene/Scene.h"
#include "SelectedMeshLayer.h"

// NOTE : ImGui must be included before ImGuizmo
#include <imgui.h>
#include <ImGuizmo/ImGuizmo.h>


class MultiMeshLayer : public RenderLayer {
public:
    MultiMeshLayer(VkRenderPass renderPass);
    virtual ~MultiMeshLayer();


    virtual void fillCommandBuffer(VkCommandBuffer commandBuffer, uint32_t commandBufferIndex) override;
    virtual void update(float dt, uint32_t commandBufferIndex, const glm::mat4& pv) override;
    virtual void onEvent(Event& event) override;
    virtual void onImGuiRender() override;

    std::shared_ptr<SelectedMeshLayer> getSelectedMeshLayer();

private:
    void createDescriptors();

    // TODO : should this be done in the selected mesh layer ??
    void displayHierarchy(int entity);
    void displayGuizmo(int selectedEntity);

private:
    struct InstanceData{
        glm::mat4 transform;
        uint32_t meshIndex;
        uint32_t materialIndex;
        uint32_t indexOffset;
    };

    // Buffers
    std::array<UniformBuffer, MAX_FRAMES_IN_FLIGHT> _vpUniformBuffers{};
    std::array<ShaderStorageBuffer, MAX_FRAMES_IN_FLIGHT> _meshTransformBuffers{};
    ShaderStorageBuffer _vertices{};
    ShaderStorageBuffer _indices{};
    ShaderStorageBuffer _indirectCommandBuffer{};

    VkPushConstantRange _cameraPosPC;
    float _specularS = 0.5f;

    // Scene
    int _selectedEntity = -1; // TODO  Remove since we have selected mesh layer??
    std::shared_ptr<Scene> _scene = nullptr;

    // current operation done with the guizmo (translate, rotate or scale)
    ImGuizmo::OPERATION _operation = ImGuizmo::OPERATION::TRANSLATE;

    // TODO : assign!!
    std::shared_ptr<SelectedMeshLayer> _selectedMeshLayer = nullptr;

    //Texture _texture{};
};

