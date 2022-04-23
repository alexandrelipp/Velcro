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
    ImTextureID _textureId = nullptr;
    ImVec2 _textureSize{};

    float _minimumScale = 24.0;
    float _pixelRange = 5.0;
    float _miterLimit = 2.0;

    glm::vec3 _atlasTransform = glm::vec3(0.f, 0.f, 1.f);
    VkPushConstantRange _atlasPushConstant{};
};

