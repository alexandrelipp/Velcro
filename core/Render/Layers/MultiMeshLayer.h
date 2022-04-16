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
    DeviceSSBO _vertices{};
    DeviceSSBO _indices{};
    DeviceSSBO _indirectCommandBuffer{};
    DeviceSSBO _meshMetadata{};
    DeviceSSBO _materialsSSBO{};

    VkPushConstantRange _cameraPosPC;

    // Scene
    std::shared_ptr<Scene> _scene = nullptr;

    ///< Selected mesh layer
    std::shared_ptr<SelectedMeshLayer> _selectedMeshLayer = nullptr;

    //Texture _texture{};
};

