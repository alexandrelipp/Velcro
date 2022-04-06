//
// Created by alexa on 2022-03-20.
//

#pragma once


#include "RenderLayer.h"
#include "../Objects/UniformBuffer.h"
#include "../Objects/ShaderStorageBuffer.h"
#include "../Objects/Texture.h"
#include "../../Scene/Scene.h"

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

private:
    void createPipelineLayout();
    void createDescriptorSets();

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
    int _selectedEntity = -1;
    std::shared_ptr<Scene> _scene = nullptr;

    ImGuizmo::OPERATION _operation = ImGuizmo::OPERATION::TRANSLATE;

    //Texture _texture{};
};

