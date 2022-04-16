//
// Created by alexa on 2022-03-26.
//

#include "MultiMeshLayer.h"

#include "../Factory/FactoryVulkan.h"
#include "../Factory/FactoryModel.h"
#include "../../Utils/UtilsMath.h"
#include "../../Application.h"
#include "../../events/KeyEvent.h"
#include "../../Utils/UtilsTemplate.h"


MultiMeshLayer::MultiMeshLayer(VkRenderPass renderPass) {
    // static assert making sure no padding is added to our struct
    static_assert(sizeof(InstanceData) == sizeof(InstanceData::transform) + sizeof(InstanceData::indexOffset) +
        sizeof(InstanceData::materialIndex) + sizeof(InstanceData::meshIndex));

    // init the uniform buffers
    for (auto& buffer : _vpUniformBuffers)
        buffer.init(_vrd->device, _vrd->physicalDevice, sizeof(glm::mat4));

    _scene = std::make_shared<Scene>("NanoWorld");
    //FactoryModel::importFromFile("../../../core/Assets/Models/Nano/nanosuit.obj", _scene);
    //FactoryModel::importFromFile("../../../core/Assets/Models/utahTeapot.fbx", _scene);
    //FactoryModel::importFromFile("../../../core/Assets/Models/engine.fbx", _scene);
    // https://sketchfab.com/3d-models/low-poly-truck-car-drifter-f3750246b6564607afbefc61cb1683b1
    FactoryModel::importFromFile("../../../core/Assets/Models/Drifter/source/Jeep_done.fbx", _scene);
    // nice little model but the default scale is stupid
    auto& rootTransform = _scene->getTransform(1);
    rootTransform.scale *= 0.01f;
    rootTransform.needUpdateModelMatrix = true;
    _scene->setDirtyTransform(1);

    FactoryModel::importFromFile("../../../core/Assets/Models/Bell Huey.fbx", _scene);
    //FactoryModel::importFromFile("../../../core/Assets/Models/duck/scene.gltf", _scene);

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

    // vector containing the indices of all meshes
    std::vector<uint32_t> materialIndices;

    // add all meshes as indirect commands
    std::vector<VkDrawIndirectCommand> indirectCommands;
    const auto& meshes = _scene->getMeshes();
    for (uint32_t i = 0; i < meshes.size(); ++i){
        indirectCommands.push_back({
               .vertexCount = meshes[i].indexCount,
               .instanceCount = 1,
               .firstVertex = meshes[i].firstVertexIndex,
               .firstInstance = i
       });
        materialIndices.push_back(meshes[i].materialIndex);
    }

    // set the commands in the command buffer
    _indirectCommandBuffer.init(_vrd, utils::vectorSizeByte(indirectCommands),
                                indirectCommands.data(), VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT);

    // init the mesh metadata buffer
    _meshMetadata.init(_vrd, utils::vectorSizeByte(materialIndices), materialIndices.data());

    // init the transform buffers
    for (auto& buffer : _meshTransformBuffers){
        buffer.init(_vrd->device, _vrd->physicalDevice, meshes.size() * sizeof(glm::mat4), true);
    }

    // init the materials buffer
    const auto& materials = _scene->getMaterials();
    _materialsSSBO.init(_vrd, utils::vectorSizeByte(materials), (void*)materials.data());

    // init the statue texture
    //_texture.init("../../../core/Assets/Models/duck/textures/Duck_baseColor.png", _vrd->device, _vrd->physicalDevice, _vrd->graphicsQueue, _vrd->commandPool);

    // create our graphics pipeline
    createDescriptors();
    Factory::GraphicsPipelineProps props = {
            .shaders =  {
                    .vertex = "multiV.spv",
                    .fragment = "multiF.spv"
            },
            .sampleCountMSAA = _vrd->sampleCount
    };
    _graphicsPipeline = Factory::createGraphicsPipeline(_vrd->device, _swapchainExtent, renderPass, _pipelineLayout, props);

    // create the selected mesh layer
    SelectedMeshLayer::Props selectedMeshProps = {
        .scene = _scene,
        .vertices = _vertices,
        .indices = _indices,
        .meshTransformBuffers = _meshTransformBuffers
    };
    _selectedMeshLayer = std::make_shared<SelectedMeshLayer>(renderPass, selectedMeshProps);
}

MultiMeshLayer::~MultiMeshLayer() {
    // destroy the buffers
    for (auto& buffer : _vpUniformBuffers)
        buffer.destroy(_vrd->device);

    for (auto& buffer : _meshTransformBuffers)
        buffer.destroy(_vrd->device);

    _indirectCommandBuffer.destroy(_vrd->device);
    _vertices.destroy(_vrd->device);
    _indices.destroy(_vrd->device);
    _meshMetadata.destroy(_vrd->device);
    _materialsSSBO.destroy(_vrd->device);

    //_texture.destroy(_vrd->device);
}

void MultiMeshLayer::fillCommandBuffer(VkCommandBuffer commandBuffer, uint32_t commandBufferIndex) {
    Camera* camera = Application::getApp()->getRenderer()->getCamera();
    // bind pipeline
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _graphicsPipeline);

    // push the camera pos and bind descriptor sets
    vkCmdPushConstants(commandBuffer, _pipelineLayout, _cameraPosPC.stageFlags, _cameraPosPC.offset, _cameraPosPC.size,
                       camera->getPosition());
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipelineLayout,
                            0, 1, &_descriptorSets[commandBufferIndex], 0, nullptr);

    // render
    vkCmdDrawIndirect(commandBuffer, _indirectCommandBuffer.getBuffer(), 0,
                      _scene->getMeshes().size(), sizeof(VkDrawIndirectCommand));
}

void MultiMeshLayer::update(float dt, uint32_t commandBufferIndex, const glm::mat4& pv) {
    _scene->propagateTransforms();
    glm::mat4 nec = pv; // TODO : necessary??
    VK_ASSERT(_vpUniformBuffers[commandBufferIndex].setData(_vrd->device, glm::value_ptr(nec), sizeof(pv)), "Failed to dat");
    const auto& transforms = _scene->getMeshTransforms();
    _meshTransformBuffers[commandBufferIndex].setData(_vrd->device, _vrd->physicalDevice, _vrd->graphicsQueue, _vrd->commandPool,
                                                      (void*)transforms.data(), transforms.size() * sizeof(transforms[0]));
}

void MultiMeshLayer::onEvent(Event& event) {

}

void MultiMeshLayer::onImGuiRender() {
//    ImGui::Begin("Specular");
//    ImGui::DragFloat("s", &_specularS, 0.1f, 0.f, 10.f);
//
//    ImGui::End();
}

std::shared_ptr<SelectedMeshLayer> MultiMeshLayer::getSelectedMeshLayer() {
    return _selectedMeshLayer;
}

void MultiMeshLayer::createDescriptors() {
    std::vector<Factory::Descriptor> descriptors = {
            {
                    .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                    .shaderStage = VK_SHADER_STAGE_VERTEX_BIT,
                    .info = std::array<VkDescriptorBufferInfo, MAX_FRAMES_IN_FLIGHT>{
                            VkDescriptorBufferInfo {_vpUniformBuffers[0].getBuffer(), 0, _vpUniformBuffers[0].getSize()},
                            VkDescriptorBufferInfo {_vpUniformBuffers[1].getBuffer(), 0, _vpUniformBuffers[1].getSize()},
                    }
            },
            {
                    .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                    .shaderStage = VK_SHADER_STAGE_VERTEX_BIT,
                    .info = std::array<VkDescriptorBufferInfo, MAX_FRAMES_IN_FLIGHT>{
                            VkDescriptorBufferInfo {_vertices.getBuffer(), 0, _vertices.getSize()},
                            VkDescriptorBufferInfo {_vertices.getBuffer(), 0, _vertices.getSize()},
                    }
            },
            {
                    .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                    .shaderStage = VK_SHADER_STAGE_VERTEX_BIT,
                    .info = std::array<VkDescriptorBufferInfo, MAX_FRAMES_IN_FLIGHT>{
                            VkDescriptorBufferInfo {_indices.getBuffer(), 0, _indices.getSize()},
                            VkDescriptorBufferInfo {_indices.getBuffer(), 0, _indices.getSize()},
                    }
            },
            {
                    .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                    .shaderStage = VK_SHADER_STAGE_VERTEX_BIT,
                    .info = std::array<VkDescriptorBufferInfo, MAX_FRAMES_IN_FLIGHT>{
                            VkDescriptorBufferInfo {_meshTransformBuffers[0].getBuffer(), 0, _meshTransformBuffers[0].getSize()},
                            VkDescriptorBufferInfo {_meshTransformBuffers[1].getBuffer(), 0, _meshTransformBuffers[1].getSize()},
                    }
            },
            {
                    .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                    .shaderStage = VK_SHADER_STAGE_VERTEX_BIT,
                    .info = std::array<VkDescriptorBufferInfo, MAX_FRAMES_IN_FLIGHT>{
                            VkDescriptorBufferInfo {_meshMetadata.getBuffer(), 0, _meshMetadata.getSize()},
                            VkDescriptorBufferInfo {_meshMetadata.getBuffer(), 0, _meshMetadata.getSize()},
                    }
            },
            {
                    .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                    .shaderStage = VK_SHADER_STAGE_FRAGMENT_BIT,
                    .info = std::array<VkDescriptorBufferInfo, MAX_FRAMES_IN_FLIGHT>{
                            VkDescriptorBufferInfo {_materialsSSBO.getBuffer(), 0, _materialsSSBO.getSize()},
                            VkDescriptorBufferInfo {_materialsSSBO.getBuffer(), 0, _materialsSSBO.getSize()},
                    }
            },
    };

    // create fragment push constant for camera pos
    _cameraPosPC = {
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
            .offset = 0,               // must be multiple of 4 (offset into push constant block)
            .size = sizeof(glm::vec3), // must be multiple of 4
    };

    // create descriptors
    std::tie(_descriptorSetLayout, _pipelineLayout, _descriptorPool, _descriptorSets) =
            Factory::createDescriptorSets(_vrd, descriptors, {_cameraPosPC});
}