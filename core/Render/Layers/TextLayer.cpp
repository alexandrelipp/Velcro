//
// Created by alexa on 2022-04-21.
//

#include "TextLayer.h"
#include "../../Utils/UtilsFile.h"
#include "backends/imgui_impl_vulkan.h"
#include "../Factory/FactoryModel.h"
#include "../../Utils/UtilsTemplate.h"

#include <msdf-atlas-gen/msdf-atlas-gen.h>

using namespace msdf_atlas;

TextLayer::TextLayer(VkRenderPass renderPass) {
    std::string filePath = "../../../core/Assets/Fonts/OpenSans/OpenSans-Regular.ttf";
    VK_ASSERT(utils::fileExists(filePath), "Font file does not exist");

    generateAtlas(filePath);

    std::vector<TexVertex2> vertices;
    FactoryModel::createTexturedSquare2(vertices);
    _vertexBuffer.init(_vrd, vertices.data(), utils::vectorSizeByte(vertices));

    // describe
    std::vector<Factory::Descriptor> descriptors = {
            {
                    .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    .shaderStage = VK_SHADER_STAGE_FRAGMENT_BIT,
                    .info = std::vector<VkDescriptorImageInfo>{
                            VkDescriptorImageInfo{
                                    .sampler = _texture.getSampler(),
                                    .imageView = _texture.getImageView(),
                                    .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
                            }
                    }
            }
    };

    /// create descriptor sets
    std::tie(_descriptorSetLayout, _pipelineLayout, _descriptorPool, _descriptorSets) =
            Factory::createDescriptorSets(_vrd, descriptors, {});

    // describe attribute input
    VkVertexInputBindingDescription bindingDescription = {
            .binding = 0,
            .stride = sizeof(TexVertex2),
            .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
    };
    std::vector<VkVertexInputAttributeDescription> inputDescriptions = VertexBuffer::inputAttributeDescriptions({
        {.offset = (uint32_t)offsetof(TexVertex2, position), .format = typeToFormat<decltype(TexVertex2::position)>()},
        {.offset = (uint32_t)offsetof(TexVertex2, uv),       .format = typeToFormat<decltype(TexVertex2::uv)>()},
                                                                                                                });

    Factory::GraphicsPipelineProps props = {
            .vertexInputBinding = &bindingDescription,
            .vertexInputAttributes = inputDescriptions,
            .shaders =  {
                    .vertex = "TextV.spv",
                    .fragment = "TextF.spv"
            },
            .enableDepthTest = VK_FALSE,
            .sampleCountMSAA = _vrd->sampleCount
    };
    _graphicsPipeline = Factory::createGraphicsPipeline(_vrd->device, _swapchainExtent, renderPass, _pipelineLayout, props);

}

TextLayer::~TextLayer() {

}

void TextLayer::update(float dt, uint32_t commandBufferIndex, const glm::mat4& pv) {
}

void TextLayer::onEvent(Event& event) {

}

void TextLayer::fillCommandBuffer(VkCommandBuffer commandBuffer, uint32_t commandBufferIndex) {
    bindPipelineAndDS(commandBuffer, commandBufferIndex);
    _vertexBuffer.bind(commandBuffer);
    vkCmdDraw(commandBuffer, 6, 1, 0, 0);
}

void TextLayer::onImGuiRender() {

}

bool TextLayer::generateAtlas(const std::string& fontFilename) {
    bool success = false;
    // Initialize instance of FreeType library
    if (msdfgen::FreetypeHandle *ft = msdfgen::initializeFreetype()) {
        // Load font file
        if (msdfgen::FontHandle *font = msdfgen::loadFont(ft, fontFilename.c_str())) {
            // Storage for glyph geometry and their coordinates in the atlas
            std::vector<GlyphGeometry> glyphs;
            // FontGeometry is a helper class that loads a set of glyphs from a single font.
            // It can also be used to get additional font metrics, kerning information, etc.
            FontGeometry fontGeometry(&glyphs);
            // Load a set of character glyphs:
            // The second argument can be ignored unless you mix different font sizes in one atlas.
            // In the last argument, you can specify a charset other than ASCII.
            // To load specific glyph indices, use loadGlyphs instead.
            fontGeometry.loadCharset(font, 1.0, Charset::ASCII);

            // Apply MSDF edge coloring. See edge-coloring.h for other coloring strategies.
            // TODO : renable??
//            const double maxCornerAngle = 3.0;
//            for (GlyphGeometry &glyph : glyphs)
//                glyph.edgeColoring(&msdfgen::edgeColoringInkTrap, maxCornerAngle, 0);

            // TightAtlasPacker class computes the layout of the atlas.
            TightAtlasPacker packer;
            // Set atlas parameters:
            // setDimensions or setDimensionsConstraint to find the best value
            packer.setDimensionsConstraint(TightAtlasPacker::DimensionsConstraint::SQUARE);
            // setScale for a fixed size or setMinimumScale to use the largest that fits
            packer.setMinimumScale(24.0);
            // setPixelRange or setUnitRange
            packer.setPixelRange(2.0);
            packer.setMiterLimit(1.0);
            // Compute atlas layout - pack glyphs
            packer.pack(glyphs.data(), glyphs.size());
            // Get final atlas dimensions
            int width = 0, height = 0;
            packer.getDimensions(width, height);
            // The ImmediateAtlasGenerator class facilitates the generation of the atlas bitmap.
            ImmediateAtlasGenerator<
                    float, // pixel type of buffer for individual glyphs depends on generator function
                    1, // number of atlas color channels
                    &sdfGenerator, // function to generate bitmaps for individual glyphs
                    BitmapAtlasStorage<byte, 1> // class that stores the atlas bitmap
                    // For example, a custom atlas storage class that stores it in VRAM can be used.
            > generator(width, height);
            // GeneratorAttributes can be modified to change the generator's default settings.
            GeneratorAttributes attributes;
            generator.setAttributes(attributes);
            generator.setThreadCount(4);
            // Generate atlas bitmap
            generator.generate(glyphs.data(), glyphs.size());

            const auto& atlasStorage = generator.atlasStorage();
            const auto bitmap = (const msdfgen::Bitmap<unsigned char, 1>&)atlasStorage;

            auto data = (const unsigned char*)bitmap;

            Texture::TextureDesc desc = {
                    .width = (uint32_t)bitmap.width(),
                    .height = (uint32_t)bitmap.height(),
                    .imageFormat = VK_FORMAT_R8_UNORM,
                    //.imageFormat = VK_FORMAT_R8G8B8A8_SRGB,
                    .data = std::vector<char>(data, data + height * width)
            };
            _texture.init(desc, *_vrd, true);

            SPDLOG_INFO("Bitmap size {} {}", bitmap.width(), bitmap.height());

            // The atlas bitmap can now be retrieved via atlasStorage as a BitmapConstRef.
            // The glyphs array (or fontGeometry) contains positioning data for typesetting text.
            //success = myProject::submitAtlasBitmapAndLayout(generator.atlasStorage(), glyphs);
            // Cleanup
            msdfgen::destroyFont(font);
            success = true;

        }
        msdfgen::deinitializeFreetype(ft);
    }
    SPDLOG_INFO("Success {}", success);
    return success;
}
