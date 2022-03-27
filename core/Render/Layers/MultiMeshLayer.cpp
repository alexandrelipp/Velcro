//
// Created by alexa on 2022-03-26.
//

#include <imgui.h>
#include "MultiMeshLayer.h"

#include "../Factory/FactoryVulkan.h"
#include "../Factory/FactoryModel.h"

#include "../../Application.h"

MultiMeshLayer::MultiMeshLayer(VkRenderPass renderPass) {
    // static assert making sure no padding is added to our struct
    static_assert(sizeof(InstanceData) == sizeof(InstanceData::transform) + sizeof(InstanceData::indexOffset) +
        sizeof(InstanceData::materialIndex) + sizeof(InstanceData::meshIndex));

    // init the uniform buffers
    for (auto& buffer : _mvpUniformBuffers)
        buffer.init(_vrd->device, _vrd->physicalDevice, sizeof(glm::mat4));

    _scene = std::make_shared<Scene>("NanoWorld");
    FactoryModel::importFromFile("../../../core/Assets/Models/Nano/nanosuit.obj", _scene);

    static_assert(sizeof(Vertex) == sizeof(Vertex::position) + sizeof(Vertex::normal) + sizeof(Vertex::uv));

    auto [vertices, vtxSize] = _scene->getVerticesData();
    auto [indices, idxSize] = _scene->getIndicesData();

    // init the vertices ssbo
    _vertices.init(_vrd->device, _vrd->physicalDevice, vtxSize);
    VK_ASSERT(_vertices.setData(_vrd->device, _vrd->physicalDevice, _vrd->graphicsQueue, _vrd->commandPool,
                                vertices, vtxSize), "set data failed");

    // init the indices ssbo
    _indices.init(_vrd->device, _vrd->physicalDevice, idxSize);
    VK_ASSERT(_indices.setData(_vrd->device, _vrd->physicalDevice, _vrd->graphicsQueue, _vrd->commandPool,
                               indices, idxSize), "set data failed");


    // add all meshes as indirect commands
    std::vector<VkDrawIndirectCommand> indirectCommands;
    const auto& meshes = _scene->getMeshes();
    for (const auto& mesh : meshes){
        indirectCommands.push_back({
            .vertexCount = mesh.indexCount,
            .instanceCount = 1,
            .firstVertex = mesh.firstVertexIndex,
            .firstInstance = 0
        });
    }

//    VkDrawIndirectCommand indirectCommand = {
//            .vertexCount = (uint32_t)indices.size(),
//            .instanceCount = 1,
//            .firstVertex = 0,
//            .firstInstance = 0
//    };

    _indirectCommandBuffer.init(_vrd->device, _vrd->physicalDevice, indirectCommands.size() * sizeof(indirectCommands[0]), VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT);
    VK_ASSERT(_indirectCommandBuffer.setData(_vrd->device, _vrd->physicalDevice, _vrd->graphicsQueue, _vrd->commandPool,
                      indirectCommands.data(), indirectCommands.size() * sizeof(indirectCommands[0])), "set data failed");


    // init the statue texture
    _texture.init("../../../core/Assets/Models/duck/textures/Duck_baseColor.png", _vrd->device, _vrd->physicalDevice, _vrd->graphicsQueue, _vrd->commandPool);

    // create our graphics pipeline
    createPipelineLayout();
    createDescriptorSets();
    Factory::GraphicsPipelineProps props = {
            .shaders =  {
                    .vertex = "multiV.spv",
                    .fragment = "multiF.spv"
            }
    };
    _graphicsPipeline = Factory::createGraphicsPipeline(_vrd->device, _swapchainExtent, renderPass, _pipelineLayout, props);
}

MultiMeshLayer::~MultiMeshLayer() {
    // destroy the buffers
    for (auto& buffer : _mvpUniformBuffers)
        buffer.destroy(_vrd->device);

    _indirectCommandBuffer.destroy(_vrd->device);
    _vertices.destroy(_vrd->device);
    _indices.destroy(_vrd->device);

    _texture.destroy(_vrd->device);
}

void MultiMeshLayer::fillCommandBuffer(VkCommandBuffer commandBuffer, uint32_t currentImage) {
    // bind pipeline and render
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _graphicsPipeline);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipelineLayout,
                            0, 1, &_descriptorSets[currentImage], 0, nullptr);

    vkCmdDrawIndirect(commandBuffer, _indirectCommandBuffer.getBuffer(), 0, 1, sizeof(VkDrawIndirectCommand));
}

void MultiMeshLayer::update(float dt, uint32_t currentImage, const glm::mat4& pv) {
    static float time = 0.f;
    time += dt;
    glm::mat4 m = glm::rotate(
            glm::translate(glm::mat4(1.0f), glm::vec3(0.f, 0.5f, -1.5f)) * glm::rotate(glm::mat4(1.f), glm::pi<float>(),
                                                                                       glm::vec3(1, 0, 0)),
            time,
            glm::vec3(0.0f, 1.0f, 0.0f)
    );

    glm::mat4 mvp = pv * m;
    VK_ASSERT(_mvpUniformBuffers[currentImage].setData(_vrd->device, glm::value_ptr(mvp), sizeof(mvp)), "Failed to dat");
}

void MultiMeshLayer::onImGuiRender() {
    displayHierarchy(0);
}

void MultiMeshLayer::createPipelineLayout() {
    // create the pipeline layout
    std::array<VkDescriptorSetLayoutBinding, 4> layoutBindings = {
            VkDescriptorSetLayoutBinding{
                    .binding = 0,
                    .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                    .descriptorCount = 1,
                    .stageFlags = VK_SHADER_STAGE_VERTEX_BIT
            },
            VkDescriptorSetLayoutBinding{
                    .binding = 1,
                    .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                    .descriptorCount = 1,
                    .stageFlags = VK_SHADER_STAGE_VERTEX_BIT
            },
            VkDescriptorSetLayoutBinding{
                    .binding = 2,
                    .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                    .descriptorCount = 1,
                    .stageFlags = VK_SHADER_STAGE_VERTEX_BIT
            },
            VkDescriptorSetLayoutBinding{
                    .binding = 3,
                    .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    .descriptorCount = 1,
                    .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
            }
    };

    VkDescriptorSetLayoutCreateInfo descriptorLayoutCI = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .bindingCount = layoutBindings.size(),
            .pBindings = layoutBindings.data()
    };

    VK_CHECK(vkCreateDescriptorSetLayout(_vrd->device, &descriptorLayoutCI, nullptr, &_descriptorSetLayout));


    // create pipeline layout (used for uniform and push constants)
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &_descriptorSetLayout;
    pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
    pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

    VK_CHECK(vkCreatePipelineLayout(_vrd->device, &pipelineLayoutInfo, nullptr, &_pipelineLayout));
}

void MultiMeshLayer::createDescriptorSets() {
    _descriptorPool = Factory::createDescriptorPool(_vrd->device, FB_COUNT, 1, 2, 1);

    std::array<VkDescriptorSetLayout, FB_COUNT> layouts = {_descriptorSetLayout, _descriptorSetLayout, _descriptorSetLayout};

    VkDescriptorSetAllocateInfo descriptorSetAI = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .descriptorPool = _descriptorPool,
            .descriptorSetCount = (uint32_t)layouts.size(),
            .pSetLayouts = layouts.data()
    };

    VK_CHECK(vkAllocateDescriptorSets(_vrd->device, &descriptorSetAI, _descriptorSets.data()));

    for (size_t i = 0; i < _descriptorSets.size(); ++i) {
        // create write descriptor set for each descriptor set
        std::vector<VkWriteDescriptorSet> writeDescriptorSets;

        // push back descriptor write for UBO (1 buffer/image)
        VkDescriptorBufferInfo bufferInfo = {
                .buffer = _mvpUniformBuffers[i].getBuffer(),
                .offset = 0,
                .range = _mvpUniformBuffers[i].getSize()
        };
        writeDescriptorSets.push_back({
                                              .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                                              .dstSet = _descriptorSets[i],
                                              .dstBinding = 0,
                                              .dstArrayElement = 0,
                                              .descriptorCount = 1,
                                              .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                              .pBufferInfo = &bufferInfo
                                      });

        // push back descriptor write for vertices SSBO (1 buffer)
        VkDescriptorBufferInfo verticesInfo = {
                .buffer = _vertices.getBuffer(),
                .offset = 0,
                .range = _vertices.getSize()
        };
        writeDescriptorSets.push_back({
                                              .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                                              .dstSet = _descriptorSets[i],
                                              .dstBinding = 1,
                                              .dstArrayElement = 0,
                                              .descriptorCount = 1,
                                              .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                                              .pBufferInfo = &verticesInfo
                                      });

        // push back descriptor write for indices SSBO (1 buffer)
        VkDescriptorBufferInfo indicesInfo = {
                .buffer = _indices.getBuffer(),
                .offset = 0,
                .range = _indices.getSize()
        };
        writeDescriptorSets.push_back({
                                              .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                                              .dstSet = _descriptorSets[i],
                                              .dstBinding = 2,
                                              .dstArrayElement = 0,
                                              .descriptorCount = 1,
                                              .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                                              .pBufferInfo = &indicesInfo
                                      });

        // push back descriptor write for image/sampler
        VkDescriptorImageInfo imageInfo = {
                .sampler = _texture.getSampler(),
                .imageView = _texture.getImageView(),
                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        };
        writeDescriptorSets.push_back({
                                              .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                                              .dstSet = _descriptorSets[i],
                                              .dstBinding = 3,
                                              .dstArrayElement = 0,
                                              .descriptorCount = 1,
                                              .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                              .pImageInfo = &imageInfo
                                      });

        // update the descriptor sets with the created decriptor writes
        vkUpdateDescriptorSets(_vrd->device, writeDescriptorSets.size(), writeDescriptorSets.data(), 0, nullptr);
    }
}

void MultiMeshLayer::displayHierarchy(int entity) {
    if (entity == -1)
        return;

    // default flags and additional flag if selected
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth |ImGuiTreeNodeFlags_DefaultOpen;
//    if (entity == _selectedEntity)
//        flags |= ImGuiTreeNodeFlags_Selected | ImGuiTreeNodeFlags_DefaultOpen;

    // get tag and hierarchy
    HierarchyComponent& hc = _scene->getHierarchy(entity);
    if (hc.firstChild == -1)
        flags |= ImGuiTreeNodeFlags_Leaf;

    // check if the node is opened and if it's the selected entity
    bool opened = ImGui::TreeNodeEx((void*) entity, flags, _scene->getName(entity).c_str());

    ImGui::PushID((int)entity);

//    if (ImGui::IsItemClicked())
//        _selectedEntity = entity;

    if (opened) {
        for (int e = hc.firstChild; e != -1; e = _scene->getHierarchy(e).nextSibling)
            displayHierarchy(e);
        ImGui::TreePop();
    }

    ImGui::PopID();
}
