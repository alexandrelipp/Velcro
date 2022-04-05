//
// Created by alexa on 2022-03-20.
//

#pragma once

#include "RenderLayer.h"
#include "../Objects/UniformBuffer.h"
#include "../Objects/ShaderStorageBuffer.h"
#include "../Objects/Texture.h"


class LineLayer : public RenderLayer {
public:
    LineLayer(VkRenderPass renderPass);
    virtual ~LineLayer();

    virtual void fillCommandBuffer(VkCommandBuffer commandBuffer, uint32_t commandBufferIndex) override;
    virtual void update(float dt, uint32_t commandBufferIndex, const glm::mat4& pv) override;
    virtual void onImGuiRender() override;

    void line(const glm::vec3& p1, const glm::vec3& p2, const glm::vec4& c);
    void plane3d(const glm::vec3& orig, const glm::vec3& v1, const glm::vec3& v2,
                 int n1, int n2, float s1, float s2, const glm::vec4& color, const glm::vec4& outlineColor);

private:
    static constexpr uint32_t MAX_LINE_COUNT = 65000;

    void createPipelineLayout();
    void createDescriptorSets();

    struct VertexData {
        glm::vec3 position = glm::vec3(0.f);
        glm::vec4 color = glm::vec4(1.f);
    };

private:
    // points
    ShaderStorageBuffer _pointsSSBO{}; // TODO : probably want dynamic buffer
    std::vector<VertexData> _lines{};

    std::array<UniformBuffer, MAX_FRAMES_IN_FLIGHT> _mvpUniformBuffers{};
};