//
// Created by alexa on 2022-03-22.
//

#pragma once

#include "RenderLayer.h"


class ImGuiLayer : public RenderLayer {
public:
    ImGuiLayer(VkRenderPass renderPass);

    virtual ~ImGuiLayer();

    virtual void fillCommandBuffer(VkCommandBuffer commandBuffer, uint32_t commandBufferIndex) override;
    virtual void update(float dt, uint32_t commandBufferIndex, const glm::mat4& pv) override;
    virtual void onImGuiRender() override;

    void begin();
    void end();




};