//
// Created by alexa on 2022-03-22.
//

#pragma once

#include "RenderLayer.h"


class ImGuiLayer : public RenderLayer {
public:
    ImGuiLayer(VkRenderPass renderPass);

    void begin();
    void end();

    virtual ~ImGuiLayer();

    virtual void fillCommandBuffer(VkCommandBuffer commandBuffer, uint32_t currentImage) override;

    virtual void update(float dt, uint32_t currentImage) override;


};