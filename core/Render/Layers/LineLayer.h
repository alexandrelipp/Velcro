//
// Created by alexa on 2022-03-20.
//

#pragma once

#include "RenderLayer.h"
#include "../Objects/UniformBuffer.h"
#include "../Objects/ShaderStorageBuffer.h"
#include "../Objects/Texture.h"


class LineLayer : public RenderLayer {
public:
    LineLayer(VkRenderPass renderPass);
    virtual ~LineLayer();

    virtual void fillCommandBuffer(VkCommandBuffer commandBuffer, uint32_t currentImage) override;
    virtual void update(float dt, uint32_t currentImage) override;

private:
}


