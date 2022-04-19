//
// Created by alexa on 2022-04-19.
//

#include "TrueTypeFontLayer.h"

#include "../../Utils/UtilsFile.h"

#include <imgui/backends/imgui_impl_vulkan.h>

#include <ft2build.h>
#include FT_FREETYPE_H


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

    // init the statue texture
    _texture.init("../../../core/Assets/Textures/statue.jpg", *_vrd, true);
}

TrueTypeFontLayer::~TrueTypeFontLayer() {

}

void TrueTypeFontLayer::update(float dt, uint32_t commandBufferIndex, const glm::mat4& pv) {
    if (_textureId == nullptr)
        _textureId = (ImTextureID)ImGui_ImplVulkan_AddTexture(_texture.getSampler(), _texture.getImageView(),
                                                          VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL);

}

void TrueTypeFontLayer::onEvent(Event& event) {

}

void TrueTypeFontLayer::fillCommandBuffer(VkCommandBuffer commandBuffer, uint32_t commandBufferIndex) {

}

void TrueTypeFontLayer::onImGuiRender() {
    ImGui::Begin("Texture");

    ImGui::Image(_textureId, {100.f, 100.f});
    ImGui::End();
}
