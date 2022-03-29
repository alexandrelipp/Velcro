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

    // get root (don't return by ref, we push right after! -> moves all element in vector)
    HierarchyComponent root = scene.getHierarchy(0);

    // add first scene node
    int firstChild = scene.addSceneNode(0, 0, "First");
    REQUIRE(firstChild == 1);
    HierarchyComponent fcc = scene.getHierarchy(firstChild);
    REQUIRE(fcc.parent == 0);
    root = scene.getHierarchy(0);
    REQUIRE(root.firstChild == firstChild);

    // add second child
    int secondChild = scene.addSceneNode(0, 1, "Second");
    REQUIRE(secondChild == 2);
    HierarchyComponent scc = scene.getHierarchy(secondChild);
    REQUIRE(scc.parent == 0);
    REQUIRE(root.firstChild == firstChild);
    fcc = scene.getHierarchy(firstChild);
    REQUIRE(fcc.nextSibling == secondChild);

    // add grand child
    int grandChild = scene.addSceneNode(secondChild, 2, "grand child");
    REQUIRE(grandChild == 3);
    HierarchyComponent gcc = scene.getHierarchy(grandChild);
    REQUIRE(gcc.parent == secondChild);
    scc = scene.getHierarchy(secondChild);
    REQUIRE(scc.firstChild == grandChild);
}


