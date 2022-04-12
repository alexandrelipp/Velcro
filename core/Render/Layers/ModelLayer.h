//
// Created by alexa on 2022-03-20.
//

#pragma once

#include "RenderLayer.h"
#include "../Objects/UniformBuffer.h"
#include "../Objects/ShaderStorageBuffer.h"
#include "../Objects/Texture.h"


class ModelLayer : public RenderLayer {
public:
    ModelLayer(VkRenderPass renderPass);
    virtual ~ModelLayer();

    virtual void fillCommandBuffer(VkCommandBuffer commandBuffer, uint32_t commandBufferIndex) override;
    virtual void onEvent(Event& event) override;
    virtual void update(float dt, uint32_t commandBufferIndex, const glm::mat4& pv) override;
    virtual void onImGuiRender() override;

private:
    void createDescriptors();

private:
    struct SelectedTransforms{
        glm::mat4 mvp = glm::mat4(1.f);
        glm::mat4 mvpScale = glm::mat4(1.f);
    };


    // Buffers
    std::array<UniformBuffer, MAX_FRAMES_IN_FLIGHT> _mvpUniformBuffers{};
    ShaderStorageBuffer _vertices{};
    ShaderStorageBuffer _indices{};
    uint32_t _indexCount = 0;

    Texture _texture{};
};



