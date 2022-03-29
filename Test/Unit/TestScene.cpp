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

TEST_CASE( "Children", "[Scene]" ){
    // create a scene and make sure a root get implicitly created
    Scene scene("test");
    REQUIRE(!scene.getName(0).empty());

    // add first scene node
    int newEntity = scene.addSceneNode(0, 0, "bob");
    REQUIRE(newEntity == 1);
}


