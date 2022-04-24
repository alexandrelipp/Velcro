//
// Created by alexa on 2022-04-21.
//

#include "RenderLayer.h"
#include "../Objects/Texture.h"
#include "../Objects/VertexBuffer.h"
#include "../Objects/ShaderStorageBuffer.h"
#include "../Objects/IndexBuffer.h"

#include <imgui.h>

#include <msdf-atlas-gen/msdf-atlas-gen.h>


class TextLayer : public RenderLayer {
public:
    TextLayer(VkRenderPass renderPass);

    virtual ~TextLayer();

    virtual void update(float dt, uint32_t commandBufferIndex, const glm::mat4& pv) override;
    virtual void onEvent(Event& event) override;


    virtual void fillCommandBuffer(VkCommandBuffer commandBuffer, uint32_t commandBufferIndex) override;
    virtual void onImGuiRender() override;

private:
    void createGraphicsPipeline();

    bool generateAtlasSDF(const std::string& fontFilename);
    bool generateAtlasMSDF(const std::string& fontFilename);

private:
    static constexpr uint32_t MAX_CHAR = 200;

private:
    VkRenderPass _renderPass = nullptr; ///< cached render pass for pipeline recreation
    // TODO : device?
    std::array<HostSSBO, MAX_FRAMES_IN_FLIGHT> _texCoords{};
    VertexBuffer _vertexBuffer{};
    IndexBuffer _indexBuffer{};
    Texture _texture{};
    ImTextureID _textureId = nullptr;
    glm::vec2 _textureSize{};

    float _minimumScale = 56.0;
    float _pixelRange = 5.0f;
    float _miterLimit = 1.0f;


    float _scale = 1.f;

    // TODO :make vector also prob not neccesary to store glyp?
    std::unordered_map<msdfgen::unicode_t, msdf_atlas::GlyphGeometry> _charMap;
    std::vector<msdfgen::unicode_t> _chars{};

    std::array<HostSSBO, MAX_FRAMES_IN_FLIGHT> _charMVPs{};

    msdfgen::FontMetrics _fontMetrics{};


};

