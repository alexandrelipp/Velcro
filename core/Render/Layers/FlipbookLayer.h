//
// Created by alexa on 2022-04-06.
//

#pragma once

#include "RenderLayer.h"
#include "../Objects/Texture.h"


class FlipbookLayer : public RenderLayer {

public:
    FlipbookLayer(VkRenderPass renderPass);
    virtual ~FlipbookLayer();

    virtual void update(float dt, uint32_t commandBufferIndex, const glm::mat4& pv) override;
    virtual void onEvent(Event& event) override;

    virtual void fillCommandBuffer(VkCommandBuffer commandBuffer, uint32_t commandBufferIndex) override;
    virtual void onImGuiRender() override;

private:
    struct Animation {
        uint32_t textureIndex;
    };
    std::vector<Texture> _textures;
    std::vector<Animation> _animations;

};