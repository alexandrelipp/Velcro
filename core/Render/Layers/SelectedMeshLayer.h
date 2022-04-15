//
// Created by alexa on 2022-04-11.
//

#pragma once

#include "RenderLayer.h"
#include "../Objects/UniformBuffer.h"
#include "../Objects/ShaderStorageBuffer.h"
#include "../Objects/Texture.h"
#include "../../Scene/Scene.h"

// NOTE : ImGui must be included before ImGuizmo
#include <imgui.h>
#include <ImGuizmo/ImGuizmo.h>

class SelectedMeshLayer : public RenderLayer {
public:
    struct Props{
        std::shared_ptr<Scene> scene;
        ShaderStorageBuffer vertices;
        ShaderStorageBuffer indices;
        std::array<ShaderStorageBuffer, MAX_FRAMES_IN_FLIGHT> meshTransformBuffers{};
    };

public:
    SelectedMeshLayer(VkRenderPass renderPass, const Props& props);

    virtual ~SelectedMeshLayer();

    virtual void update(float dt, uint32_t commandBufferIndex, const glm::mat4& pv) override;
    virtual void onEvent(Event& event) override;

    virtual void fillCommandBuffer(VkCommandBuffer commandBuffer, uint32_t commandBufferIndex) override;
    virtual void onImGuiRender() override;

    void setSelectedEntity(int selectedEntity);

private:
    // Methods to display selected entity
    void displayHierarchy(int entity);
    void displayGuizmo(int selectedEntity);

private:
    // Buffers
    std::array<UniformBuffer, MAX_FRAMES_IN_FLIGHT> _vpUniformBuffers{};
    VkPushConstantRange _scaleFactor{};
    std::shared_ptr<Scene> _scene = nullptr;

    std::vector<MeshComponent*> _selectedMeshes;
    int _selectedEntity = -1;

    // current operation done with the guizmo (translate, rotate or scale)
    ImGuizmo::OPERATION _operation = ImGuizmo::OPERATION::TRANSLATE;

    // constants
    static constexpr float MAG_OUTLINE_FACTOR = 1.04f;
    static constexpr glm::vec4 OUTLINE_COLOR = glm::vec4(1.0f, 1.0f, 0.f, 0.9f);
};
