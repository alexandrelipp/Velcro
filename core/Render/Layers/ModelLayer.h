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
    ModelLayer(VkPhysicalDevice physicalDevice, VkRenderPass renderPass);
    virtual ~ModelLayer();

    virtual void fillCommandBuffer(VkCommandBuffer commandBuffer, uint32_t currentImage) override;

private:
    void createPipelineLayout();
    void createDescriptorSets();

private:
    // Buffers
    std::array<UniformBuffer, FB_COUNT> _mvpUniformBuffers{};
    glm::mat4 mvp = glm::mat4(1.f);
    ShaderStorageBuffer _vertices{};
    ShaderStorageBuffer _indices{};
    Texture _texture{};
    uint32_t _indexCount = 0;

};



