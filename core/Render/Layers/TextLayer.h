//
// Created by alexa on 2022-04-21.
//

#include "RenderLayer.h"
#include "../Objects/Texture.h"
#include "../Objects/VertexBuffer.h"

#include <imgui.h>


class TextLayer : public RenderLayer {
public:
    TextLayer(VkRenderPass renderPass);

    virtual ~TextLayer();

    virtual void update(float dt, uint32_t commandBufferIndex, const glm::mat4& pv) override;
    virtual void onEvent(Event& event) override;


    virtual void fillCommandBuffer(VkCommandBuffer commandBuffer, uint32_t commandBufferIndex) override;
    virtual void onImGuiRender() override;

private:

    bool generateAtlas(const std::string& fontFilename);

private:
    VertexBuffer _vertexBuffer{};
    Texture _texture{};
};

