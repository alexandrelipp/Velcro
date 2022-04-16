//
// Created by alexa on 2022-03-20.
//

#pragma once

#include "RenderLayer.h"
#include "../Objects/UniformBuffer.h"
#include "../Objects/ShaderStorageBuffer.h"
#include "../Objects/Texture.h"
#include "../Objects/VertexBuffer.h"


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
    // Buffers
    std::array<UniformBuffer, MAX_FRAMES_IN_FLIGHT> _mvpUniformBuffers{};
    VertexBuffer _vertexBuffer{};
    //DeviceSSBO _vertices{};
    //DeviceSSBO _indices{};
    uint32_t _indexCount = 0;  // TODO : remove : store in index buffer
    uint32_t _vertexCount = 0; // TODO : remove!!

    Texture _texture{};
};



