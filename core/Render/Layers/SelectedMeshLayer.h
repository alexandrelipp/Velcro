//
// Created by alexa on 2022-04-11.
//

#pragma once

#include "RenderLayer.h"
#include "../Objects/UniformBuffer.h"
#include "../Objects/ShaderStorageBuffer.h"
#include "../Objects/Texture.h"
#include "../../Scene/Scene.h"


class SelectedMeshLayer : public RenderLayer {
public:
    SelectedMeshLayer(VkRenderPass renderPass);

    virtual ~SelectedMeshLayer();

    virtual void update(float dt, uint32_t commandBufferIndex, const glm::mat4& pv) override;
    virtual void onEvent(Event& event) override;

    virtual void fillCommandBuffer(VkCommandBuffer commandBuffer, uint32_t commandBufferIndex) override;
    virtual void onImGuiRender() override;

private:

};
