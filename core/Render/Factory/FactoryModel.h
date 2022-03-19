//
// Created by alexa on 2022-03-19.
//

#pragma once

#include <glm/glm.hpp>

struct TexVertex{
    glm::vec3 position;
    glm::vec2 uv;
};

namespace FactoryModel {
    bool createDuckModel(std::vector<TexVertex>& vertices, std::vector<uint32_t>& indices);

    bool createTexturedSquare(std::vector<TexVertex>& vertices, std::vector<uint32_t>& indices);
};



