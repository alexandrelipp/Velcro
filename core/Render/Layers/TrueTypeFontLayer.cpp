//
// Created by alexa on 2022-04-19.
//

#include "TrueTypeFontLayer.h"

#include "../../Utils/UtilsFile.h"

#include <imgui/backends/imgui_impl_vulkan.h>

#include <ft2build.h>
#include FT_FREETYPE_H
#include "../../Application.h"
#include "../Factory/FactoryModel.h"
#include "../../Utils/UtilsTemplate.h"


TrueTypeFontLayer::TrueTypeFontLayer(VkRenderPass renderPass) {
    FT_Library library = nullptr;
    FT_Error error = FT_Init_FreeType(&library);
    VK_ASSERT(!error, "Failed to init free type library");

    std::string filePath = "../../../core/Assets/Fonts/OpenSans/OpenSans-Regular.ttf";
    VK_ASSERT(utils::fileExists(filePath), "Font file does not exist");

    //a face describes a typeface and style. Example Times New Roman Regular
    FT_Face face = nullptr;
    error = FT_New_Face(library, filePath.c_str(), 0, &face);

    VK_ASSERT(error != FT_Err_Unknown_File_Format, "Unkown file format");
    VK_ASSERT(!error, "Error when loading font");

    // set the pixel size. TODO : test with SET_CHAR_SIZE (does it automatically)
    error = FT_Set_Pixel_Sizes(face, 0, 16);
    VK_ASSERT(!error, "Failed to set pixel size");

    // get the glyph index. Note that if the given char has no glyph in the face, 0 is returned. This is known as the missing
    // glyph and is commonly displayed as a box or space
    uint32_t index = FT_Get_Char_Index(face, 0x0042);
    SPDLOG_INFO("Index {}", index);

    // load the glyph image into the slot
    error = FT_Load_Glyph( face , index , FT_LOAD_DEFAULT );
    VK_ASSERT(!error, "Error when loading font");

    error = FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL);
    VK_ASSERT(!error, "Error when rendering font");

    SPDLOG_INFO("Current IndexP{} ", face->glyph->glyph_index);

    Texture::TextureDesc desc = {
            .width = face->glyph->bitmap.width,
            .height = face->glyph->bitmap.rows,
            .imageFormat = VK_FORMAT_R8_SRGB,
    };


    desc.data = std::vector<char>(face->glyph->bitmap.buffer, face->glyph->bitmap.buffer + desc.width * desc.height);

    _texture.init(desc, *_vrd, true);

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
                    .vertex = "TTFV.spv",
                    .fragment = "TTFF.spv"
            },
            .enableDepthTest = VK_FALSE,
            .sampleCountMSAA = _vrd->sampleCount
    };
    _graphicsPipeline = Factory::createGraphicsPipeline(_vrd->device, _swapchainExtent, renderPass, _pipelineLayout, props);




#if 0

    // we need to find the character that goes below the baseline by the biggest value
    int maxUnderBaseline = 0, glyphIndex;
    for ( int i = 32 ; i < 127 ; ++i )
    {
        // get the glyph index from character code
        glyphIndex = FT_Get_Char_Index( face , i );

        // load the glyph image into the slot
        error = FT_Load_Glyph( face , glyphIndex , FT_LOAD_DEFAULT );
        if ( error )
        {
            std::cout << "BitmapFontGenerator > failed to load glyph, error code: " << error << std::endl;
        }

        // get the glyph metrics
        const FT_Glyph_Metrics& glyphMetrics = face->glyph->metrics;

        // find the character that reaches below the baseline by the biggest value
        int glyphHang = (glyphMetrics.horiBearingY- glyphMetrics.height)/64;
        if( glyphHang < maxUnderBaseline )
        {
            maxUnderBaseline = glyphHang;
        }
    }
#endif


    // init the statue texture
    //_texture.init("../../../core/Assets/Textures/statue.jpg", *_vrd, true);
}

TrueTypeFontLayer::~TrueTypeFontLayer() {
    VkDevice device = Application::getApp()->getRenderer()->getRenderDevice()->device;
    _texture.destroy(device);
}

void TrueTypeFontLayer::update(float dt, uint32_t commandBufferIndex, const glm::mat4& pv) {
    if (_textureId == nullptr)
        _textureId = (ImTextureID)ImGui_ImplVulkan_AddTexture(_texture.getSampler(), _texture.getImageView(),
                                                          VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL);

}

void TrueTypeFontLayer::onEvent(Event& event) {

}

void TrueTypeFontLayer::fillCommandBuffer(VkCommandBuffer commandBuffer, uint32_t commandBufferIndex) {
    bindPipelineAndDS(commandBuffer, commandBufferIndex);
    _vertexBuffer.bind(commandBuffer);
    vkCmdDraw(commandBuffer, 6, 1, 0, 0);
}

void TrueTypeFontLayer::onImGuiRender() {
    ImGui::Begin("Texture");

    ImGui::Image(_textureId, {100.f, 100.f});
    ImGui::End();
}
