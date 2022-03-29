//
// Created by alexa on 2022-03-28.
//

#include <catch2/catch_test_macros.hpp>
#include <core/Scene/Scene.h>

TEST_CASE( "Create", "[Scene]" ) {
    Scene scene("test");
    auto test = &scene;
    REQUIRE(test != nullptr);
}


