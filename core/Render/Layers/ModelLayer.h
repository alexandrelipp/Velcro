//
// Created by alexa on 2022-03-20.
//

#pragma once

#include "RenderLayer.h"
#include "../Objects/UniformBuffer.h"
#include "../Objects/ShaderStorageBuffer.h"
#include "../Objects/Texture.h"


class ModelLayer : public RenderLayer{
public:
    ModelLayer(VkRenderPass renderPass);
    virtual ~ModelLayer();

    virtual void fillCommandBuffer(VkCommandBuffer commandBuffer, uint32_t currentImage) override;
    virtual void update(float dt, uint32_t currentImage) override;

private:
    void createPipelineLayout();
    void createDescriptorSets();

private:
    // Buffers
    std::array<UniformBuffer, FB_COUNT> _mvpUniformBuffers{};
    ShaderStorageBuffer _vertices{};
    ShaderStorageBuffer _indices{};
    uint32_t _indexCount = 0;

    Texture _texture{};
};



