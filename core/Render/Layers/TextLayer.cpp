//
// Created by alexa on 2022-04-21.
//

#include "TextLayer.h"
#include "../../Utils/UtilsFile.h"
#include "backends/imgui_impl_vulkan.h"
//#include "../Factory/FactoryModel.h"
#include "../../Utils/UtilsTemplate.h"


// TODO : remove!
using namespace msdf_atlas;

TextLayer::TextLayer(VkRenderPass renderPass) : _renderPass(renderPass) {
    std::string filePath = "../../../core/Assets/Fonts/Roboto/Roboto-Regular.ttf";
    VK_ASSERT(utils::fileExists(filePath), "Font file does not exist");

    //generateMSDF(filePath);
    //generateAtlasSDF(filePath);
    generateAtlasMSDF(filePath);

    // NOTE. The y axis is cartesian (since the camera pv inverts the y axis). This works because the text is rendered
    // in 3d space. If the text was rendered as an overlay the y axis would need to be from the vulkan coordinate system
    std::vector<glm::vec2> vertices = {
            {-0.5f,   0.5f},   // top left
            { 0.5f,   0.5f},   // top right
            { 0.5f,  -0.5f, }, // bottom right
            {-0.5f,  -0.5f,},  // bottom left
    };

    std::vector<uint16_t> indices = {
            1, 0, 2, 2, 0, 3
    };

    _vertexBuffer.init(_vrd, vertices.data(), utils::vectorSizeByte(vertices));
    _indexBuffer.init(_vrd, VK_INDEX_TYPE_UINT16, indices.data(), indices.size());



    for (auto& buffer : _charMVPs){
        buffer.init(_vrd, sizeof(glm::mat4) * MAX_CHAR, nullptr);
    }

    _chars = {'a', 'b'};

    for (auto& buffer : _texCoords)
        buffer.init(_vrd, MAX_CHAR * sizeof(glm::vec2) * 4, nullptr);

    // describe
    std::vector<Factory::Descriptor> descriptors = {
            {
                    .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                    .shaderStage = VK_SHADER_STAGE_VERTEX_BIT,
                    .info = std::array<VkDescriptorBufferInfo, MAX_FRAMES_IN_FLIGHT>{
                            VkDescriptorBufferInfo {_charMVPs[0].getBuffer(), 0, _charMVPs[0].getSize()},
                            VkDescriptorBufferInfo {_charMVPs[1].getBuffer(), 0, _charMVPs[1].getSize()},
                    }
            },
            {
                .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                .shaderStage = VK_SHADER_STAGE_VERTEX_BIT,
                .info = std::array<VkDescriptorBufferInfo, MAX_FRAMES_IN_FLIGHT>{
                        VkDescriptorBufferInfo {_texCoords[0].getBuffer(), 0, _texCoords[0].getSize()},
                        VkDescriptorBufferInfo {_texCoords[1].getBuffer(), 0, _texCoords[1].getSize()},
                }
            },
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
            },
    };

    /// create descriptor sets
    std::tie(_descriptorSetLayout, _pipelineLayout, _descriptorPool, _descriptorSets) =
            Factory::createDescriptorSets(_vrd, descriptors, {});

    createGraphicsPipeline();
}

TextLayer::~TextLayer() {
    VkDevice device = _vrd->device;
    _texture.destroy(device);
    for (auto& buffer : _texCoords)
        buffer.destroy(device);
    for (auto& buffer : _charMVPs)
        buffer.destroy(device);
}

void TextLayer::update(float dt, uint32_t commandBufferIndex, const glm::mat4& pv) {
    if (_textureId == nullptr)
        _textureId = (ImTextureID)ImGui_ImplVulkan_AddTexture(_texture.getSampler(), _texture.getImageView(),
                                                              VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL);

    // nothing to do if not chars
    if(_chars.empty())
        return;

    std::vector<glm::vec2> coords(_chars.size() * 4);
    std::vector<glm::mat4> mvps(_chars.size());
    float offset = 0.f;
    for(uint32_t i = 0; i < _chars.size(); ++i){
        auto rect = _charMap[_chars[i]].getBoxRect();
        // half a pixel added to prevent atlas bleeding
        glm::vec2 topLeft = {(rect.x + 0.5f)/_textureSize.x, (_textureSize.y - rect.h - rect.y + 0.5f)/_textureSize.y};

        // one pixel removed to prevent atlas bleeding
        glm::vec2 size = {(rect.w - 1)/ _textureSize.x, (rect.h -1) / _textureSize.y};

        coords[i * 4 + 0] =  topLeft;                            // top left
        coords[i * 4 + 1]= {topLeft.x + size.x, topLeft.y};     // top right
        coords[i * 4 + 2] =  topLeft + size;                    // bottom right
        coords[i * 4 + 3] = {topLeft.x, topLeft.y + size.y};    // bottom left

        double l,b,r,t;
        _charMap[_chars[i]].getQuadPlaneBounds(l,b,r,t);

        auto& glyph = _charMap[_chars[i]];


        //_charMap[_chars[i]].getBoxTranslate()

        //mvps[i] = pv * glm::scale(glm::translate(glm::mat4(1.f), {offset + l, (b * size.y), 0.f}), glm::vec3(_scale * size.x, _scale * size.y, 1));
        //mvps[i] = pv * glm::scale(glm::translate(glm::mat4(1.f), {offset, b, 0.f}), glm::vec3(rect.w, rect.h, 1));
        double test = t - b;
        mvps[i] = pv * glm::scale(glm::translate(glm::mat4(1.f), {offset, test * 10, 0.f}), glm::vec3(rect.w, rect.h, 1));
        //mvps[i] = pv * glm::translate(glm::mat4(1.f), {offset, 0.f, 0.f});
        // TODO : use size instead!
        //offset += _scale * size.x;
        offset += _scale * rect.w;
    }
    _texCoords[commandBufferIndex].setData(_vrd, coords.data(), utils::vectorSizeByte(coords));
    _charMVPs[commandBufferIndex].setData(_vrd, mvps.data(), utils::vectorSizeByte(mvps));
}

void TextLayer::onEvent(Event& event) {

}

void TextLayer::fillCommandBuffer(VkCommandBuffer commandBuffer, uint32_t commandBufferIndex) {
    // nothing to do if not chars
    if(_chars.empty())
        return;
    bindPipelineAndDS(commandBuffer, commandBufferIndex);
    _vertexBuffer.bind(commandBuffer);
    _indexBuffer.bind(commandBuffer);
    vkCmdDrawIndexed(commandBuffer, 6, _chars.size(), 0, 0, 0);
}

void TextLayer::onImGuiRender() {
    ImGui::Begin("Atlas Setting");
    static char buffer[256];
    memset(buffer, 0, sizeof(buffer));
    // FIXME : won't work with unicode?
    for (uint32_t i = 0; i < _chars.size(); ++i){
        buffer[i] = _chars[i];
    }
    if (ImGui::InputText("Text", buffer, sizeof(buffer))){
        auto size = strlen(buffer);
        _chars.clear();
        for (uint32_t i = 0; i < size; ++i){
            _chars.push_back(buffer[i]);
        }
    }


    ImGui::DragFloat("Scale", &_scale, 0.1f);

    ImGui::DragFloat("Minimum Scale", &_minimumScale, 0.5f, 1.f, 100.f);
    ImGui::DragFloat("Pixel range", &_pixelRange, 0.25f, 1.f, 20.f);
    ImGui::DragFloat("Miter limit", &_miterLimit, 0.25f, 1.f, 20.f);

    if (ImGui::Button("Apply")){
        SPDLOG_INFO("Updating texture info");
        vkDeviceWaitIdle(_vrd->device);
        _texture.destroy(_vrd->device);
        generateAtlasMSDF("../../../core/Assets/Fonts/Roboto/Roboto-Regular.ttf");

        VkDescriptorImageInfo imageInfo = {
            .sampler = _texture.getSampler(),
            .imageView = _texture.getImageView(),
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        };

        for (auto& descriptorSet : _descriptorSets) {
            VkWriteDescriptorSet writeDescriptorSet = {
                    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                    .dstSet = descriptorSet,
                    .dstBinding = 2,
                    .dstArrayElement = 0,
                    .descriptorCount = 1,
                    .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    .pImageInfo = &imageInfo
            };
            vkUpdateDescriptorSets(_vrd->device, 1, &writeDescriptorSet, 0, nullptr);
        }

        // FIXME : This is a memory leak. The older descriptor never gets deallocated. Would probably require an imgui fix
        _textureId = (ImTextureID)ImGui_ImplVulkan_AddTexture(_texture.getSampler(), _texture.getImageView(),
                                                              VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL);

        // destroy pipeline and create a new one
        vkDestroyPipeline(_vrd->device, _graphicsPipeline, nullptr);
        createGraphicsPipeline();
    }
    ImGui::Image(_textureId, *(ImVec2*)&_textureSize);

    //ImGui::DragFloat2("Position", glm::value_ptr(_atlasTransform), 0.01f);
    ImGui::End();
}


void TextLayer::createGraphicsPipeline() {
    // describe attribute input
    VkVertexInputBindingDescription bindingDescription = {
            .binding = 0,
            .stride = sizeof(glm::vec2),
            .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
    };
    std::vector<VkVertexInputAttributeDescription> inputDescriptions = VertexBuffer::inputAttributeDescriptions({
            {.offset = 0, .format = typeToFormat<glm::vec2>()},
    });

    // set up specialization info to inject unit range in shader (when compiling it)
    std::array<VkSpecializationMapEntry, 2> mapEntries{};
    for (uint32_t i = 0; i < mapEntries.size(); ++i) {
        mapEntries[i].constantID = i;
        mapEntries[i].offset = i * sizeof(float);
        mapEntries[i].size = sizeof(float);
    }

    glm::vec2 unitRange = glm::vec2(_pixelRange)/_textureSize;

    // add data
    VkSpecializationInfo specializationInfo = {
            .mapEntryCount = mapEntries.size(),
            .pMapEntries = mapEntries.data(),
            .dataSize = sizeof(unitRange),
            .pData = &unitRange
    };

    Factory::GraphicsPipelineProps props = {
            .vertexInputBinding = &bindingDescription,
            .vertexInputAttributes = inputDescriptions,
            .shaders =  {
                    .vertex = "TextV.spv",
                    .fragment = "TextF.spv",
                    .fragmentSpec = &specializationInfo
            },
            //.enableDepthTest = VK_FALSE, // TODO  : enable!
            //.enableBackFaceCulling = VK_FALSE, // TODO  : renable!
            .sampleCountMSAA = _vrd->sampleCount
    };
    _graphicsPipeline = Factory::createGraphicsPipeline(_vrd->device, _swapchainExtent, _renderPass, _pipelineLayout, props);
}


bool TextLayer::generateAtlasSDF(const std::string& fontFilename) {
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
            const double maxCornerAngle = 3.0;
            for (GlyphGeometry &glyph : glyphs)
                glyph.edgeColoring(&msdfgen::edgeColoringInkTrap, maxCornerAngle, 0);

            // TightAtlasPacker class computes the layout of the atlas.
            TightAtlasPacker packer;
            // Set atlas parameters:
            // setDimensions or setDimensionsConstraint to find the best value
            packer.setDimensionsConstraint(TightAtlasPacker::DimensionsConstraint::SQUARE);
            // setScale for a fixed size or setMinimumScale to use the largest that fits
            packer.setMinimumScale(_minimumScale);
            // setPixelRange or setUnitRange
            packer.setPixelRange(_pixelRange);
            packer.setMiterLimit(_miterLimit);

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
            const auto& bitmap = (const msdfgen::Bitmap<byte, 1>&)atlasStorage;

            std::vector<byte> pixels(bitmap.width()*bitmap.height());
            for (int y = 0; y < bitmap.height(); ++y)
                memcpy(&pixels[bitmap.width()*y], bitmap(0, bitmap.height()-y-1), bitmap.width());

            Texture::TextureDesc desc = {
                    .width = (uint32_t)bitmap.width(),
                    .height = (uint32_t)bitmap.height(),
                    .imageFormat = VK_FORMAT_R8_UNORM,
                    .data = *(std::vector<char>*)&pixels, //std::vector<char>(data, data + height * width)
            };
            _texture.init(desc, *_vrd, true);

            _textureSize = {(float)bitmap.width(), (float)bitmap.height()};


            msdfgen::destroyFont(font);
            success = true;

        }
        msdfgen::deinitializeFreetype(ft);
    }
    SPDLOG_INFO("Success {}", success);
    return success;
}

bool TextLayer::generateAtlasMSDF(const std::string& fontFilename) {
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
            const double maxCornerAngle = 3.0;
            for (GlyphGeometry &glyph : glyphs)
                glyph.edgeColoring(&msdfgen::edgeColoringInkTrap, maxCornerAngle, 0);
            // TightAtlasPacker class computes the layout of the atlas.
            TightAtlasPacker packer;

            // setDimensions or setDimensionsConstraint to find the best value
            packer.setDimensionsConstraint(TightAtlasPacker::DimensionsConstraint::SQUARE);
            // setScale for a fixed size or setMinimumScale to use the largest that fits
            packer.setMinimumScale(_minimumScale);
            // setPixelRange or setUnitRange
            packer.setPixelRange(_pixelRange);
            packer.setMiterLimit(_miterLimit);

            // Compute atlas layout - pack glyphs
            packer.pack(glyphs.data(), glyphs.size());

            // Get final atlas dimensions
            int width = 0, height = 0;
            packer.getDimensions(width, height);

            // The ImmediateAtlasGenerator class facilitates the generation of the atlas bitmap.
            ImmediateAtlasGenerator<
                    float, // pixel type of buffer for individual glyphs depends on generator function
                    3, // number of atlas color channels
                    &msdfGenerator, // function to generate bitmaps for individual glyphs
                    BitmapAtlasStorage<byte, 3> // class that stores the atlas bitmap
                    // For example, a custom atlas storage class that stores it in VRAM can be used.
            > generator(width, height);
            // GeneratorAttributes can be modified to change the generator's default settings.
            GeneratorAttributes attributes;
            generator.setAttributes(attributes);
            generator.setThreadCount(4);
            // Generate atlas bitmap
            generator.generate(glyphs.data(), glyphs.size());

            auto& metrics = fontGeometry.getMetrics();
            //metrics.


            for (auto& glyph : glyphs){
                _charMap[glyph.getCodepoint()] = glyph;
                double l,b,r,t;
                glyph.getQuadPlaneBounds(l,b,r,t);
                float x = l, y = b, w = r, z = t;
                char c = glyph.getCodepoint();
                auto tr = glyph.getBoxTranslate();
                SPDLOG_INFO("Char {} bound l {:03.2f} b {:03.2f} r {:03.2f} t {:03.2f} Translate {:03.2f} t {:03.2f} ", c, x, y, w, z, tr.x, tr.y);
                //SPDLOG_INFO("Char {} Rect  {:03.2f}  {:03.2f} r {:03.2f} t {:03.2f} Translate {:03.2f} t {:03.2f} ", c, glyph.getBoxRect().x
            }
            auto test = glyphs[40];
            auto rect = test.getBoxRect();
            test.getCodepoint();

            const auto& atlasStorage = generator.atlasStorage();
            const auto& bitmap = (const msdfgen::Bitmap<byte, 3>&)atlasStorage;

            //const BitmapAtlasStorage<byte, 3>& bitmap = (const BitmapAtlasStorage<byte, 3>&)generator.atlasStorage();

            std::vector<byte> pixels3(3*bitmap.width() *bitmap.height());
            for (int y = 0; y < bitmap.height(); ++y)
                memcpy(&pixels3[3*bitmap.width()*y], bitmap(0, bitmap.height()-y-1), 3*bitmap.width());



            std::vector<byte> pixels4(4 * bitmap.width() * bitmap.height());
            int counter = 0;
            int i = 0;
            for (auto pixel : pixels3){
                pixels4[i++] = pixel;
                counter++;
                if (counter == 3){
                    counter = 0;
                    pixels4[i++] = 255;
                }
            }

            Texture::TextureDesc desc = {
                    .width = (uint32_t)bitmap.width(),
                    .height = (uint32_t)bitmap.height(),
                    .imageFormat = VK_FORMAT_R8G8B8A8_UNORM,
                    .data = *(std::vector<char>*)&pixels4, //std::vector<char>(data, data + height * width)
            };
            _texture.init(desc, *_vrd, true);

            _textureSize = {(float)bitmap.width(), (float)bitmap.height()};
            _fontMetrics = fontGeometry.getMetrics();

            //msdfgen::savePng(storage, "atlasMulti.png");


            // The atlas bitmap can now be retrieved via atlasStorage as a BitmapConstRef.
            // The glyphs array (or fontGeometry) contains positioning data for typesetting text.
            //success = myProject::submitAtlasBitmapAndLayout(generator.atlasStorage(), glyphs);
            // Cleanup
            msdfgen::destroyFont(font);
        }
        msdfgen::deinitializeFreetype(ft);
    }
    return success;
}