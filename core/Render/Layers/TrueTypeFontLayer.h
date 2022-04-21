//
// Created by alexa on 2022-04-19.
//

#pragma once

#include "RenderLayer.h"
#include "../Objects/Texture.h"
#include "../Objects/VertexBuffer.h"


#include <imgui.h>

class TrueTypeFontLayer : public RenderLayer{
public:
    TrueTypeFontLayer(VkRenderPass renderPass);
    virtual ~TrueTypeFontLayer();

    virtual void update(float dt, uint32_t commandBufferIndex, const glm::mat4& pv) override;
    virtual void onEvent(Event& event) override;


    virtual void fillCommandBuffer(VkCommandBuffer commandBuffer, uint32_t commandBufferIndex) override;
    virtual void onImGuiRender() override;

private:
    Texture::TextureDesc createCustom();
    Texture::TextureDesc createBitmapFont();
    VertexBuffer _vertexBuffer{};
    Texture _texture{};
    ImTextureID _textureId = nullptr;

};



