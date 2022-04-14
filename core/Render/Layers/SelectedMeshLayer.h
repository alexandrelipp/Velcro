//
// Created by alexa on 2022-04-11.
//

#pragma once

#include "RenderLayer.h"
#include "../Objects/UniformBuffer.h"
#include "../Objects/ShaderStorageBuffer.h"
#include "../Objects/Texture.h"
#include "../../Scene/Scene.h"


class SelectedMeshLayer : public RenderLayer {
public:
    struct Props{
        std::shared_ptr<Scene> scene;
        ShaderStorageBuffer vertices;
        ShaderStorageBuffer indices;
        std::array<ShaderStorageBuffer, MAX_FRAMES_IN_FLIGHT> meshTransformBuffers{};
    };

public:
    SelectedMeshLayer(VkRenderPass renderPass, const Props& props);

    virtual ~SelectedMeshLayer();

    virtual void update(float dt, uint32_t commandBufferIndex, const glm::mat4& pv) override;
    virtual void onEvent(Event& event) override;

    virtual void fillCommandBuffer(VkCommandBuffer commandBuffer, uint32_t commandBufferIndex) override;
    virtual void onImGuiRender() override;

    void setSelectedEntity(int selectedEntity);

private:
//    struct SelectedMeshMVP{
//        glm::mat4 original = glm::mat4(1.f);
//        glm::mat4 scaledUp = glm::mat4(1.f);
//    };
    // Buffers
    std::array<UniformBuffer, MAX_FRAMES_IN_FLIGHT> _vpUniformBuffers{};
    std::shared_ptr<Scene> _scene = nullptr;

    std::vector<MeshComponent*> _selectedMeshes;
    int _selectedEntity = -1;

    static float constexpr MAG_STENCIL_FACTOR = 1.04f;

};
