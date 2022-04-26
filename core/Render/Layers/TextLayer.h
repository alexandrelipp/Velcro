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
    static constexpr char FONT_FILENAME[] = "../../../core/Assets/Fonts/Roboto/Roboto-Regular.ttf";

private:
    VkRenderPass _renderPass = nullptr; ///< cached render pass for pipeline recreation
    ///< Tex coords of all the chars. NOTE:  Could be on device and no recomputed every frame
    std::array<HostSSBO, MAX_FRAMES_IN_FLIGHT> _texCoords{};
    std::array<HostSSBO, MAX_FRAMES_IN_FLIGHT> _charMVPs{}; ///< MVP's of all chars (1 / char)

    // renderer objects
    VertexBuffer  _vertexBuffer{};
    IndexBuffer   _indexBuffer{};
    Texture       _texture{};

    // multi-channel signed distance field settings
    float _minimumScale = 56.0;
    float _pixelRange = 5.0f;
    float _miterLimit = 1.0f;
    ImTextureID _textureId = nullptr;
    glm::vec2 _textureSize{};

    float _scale = 0.05f; ///< scale of the text

    ///< utf8 code of all the chars. Is it bad to store all utf8 chars as 4 bytes (uint32_t) ?. Save as string??
    std::vector<msdfgen::unicode_t> _chars{};

    struct GlyphData{
        ///< glyph box in the atlas. TODO : precompute coords!
        msdf_atlas::Rectangle rect{};
        ///< Quad plane bounds. Only b is used for now
        double l = 0.0, b = 0.0, r = 0.0 ,t = 0.0;
    };

    ///< map of unicode -> glyph Data. Could be a vector if all glyphs unicode are continuous and starting from 0 (not the case)
    std::unordered_map<msdfgen::unicode_t, GlyphData> _charMap;
    msdfgen::FontMetrics _fontMetrics{}; ///< Metrics about the font. Not currently used. Remove?
};

