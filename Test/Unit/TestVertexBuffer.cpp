//
// Created by alexa on 2022-04-17.
//

#include <catch2/catch_test_macros.hpp>
#include <core/Render/Objects/VertexBuffer.h>
#include <core/Render/Factory/FactoryModel.h>

TEST_CASE( "typeToFormat", "[VertexBuffer]" ) {
    // test basic types
    REQUIRE(typeToFormat<glm::vec4>() == VK_FORMAT_R32G32B32A32_SFLOAT);
    REQUIRE(typeToFormat<glm::vec3>() == VK_FORMAT_R32G32B32_SFLOAT);
    REQUIRE(typeToFormat<glm::vec2>() == VK_FORMAT_R32G32_SFLOAT);

    // test with decltype
    REQUIRE(typeToFormat<decltype(Vertex::position)>() == VK_FORMAT_R32G32B32_SFLOAT);
    REQUIRE(typeToFormat<decltype(Vertex::uv)>() == VK_FORMAT_R32G32_SFLOAT);
}