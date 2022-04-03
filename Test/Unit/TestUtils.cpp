//
// Created by alexa on 2022-03-28.
//

#include <catch2/catch_test_macros.hpp>
#include <core/Utils/UtilsTemplate.h>
#include <core/Utils/UtilsVulkan.h>
#include <core/Utils/UtilsMath.h>

TEST_CASE( "VectorSizeByte", "[UtilsTemplate]" ) {
    std::vector<int> ok = {1, 2, 3};
    REQUIRE(utils::vectorSizeByte(ok) == sizeof(int) * 3);

    ok.clear();
    REQUIRE(utils::vectorSizeByte(ok) == 0);
}

TEST_CASE( "Almost Equal", "[UtilsMath]") {
    float a = 4.01f;
    float b = 5.03f;
    REQUIRE(a + b != 9.04f);
    REQUIRE(utils::almostEqual(a + b, 9.04f));
}

TEST_CASE( "IsFeaturesSupported", "[UtilsVulkan]") {
    // test with device + requested features all to false (defaults)
    VkPhysicalDeviceFeatures deviceFeatures, requestedFeatures;
    REQUIRE(utils::isFeaturesSupported(deviceFeatures, requestedFeatures));

    // one feature not supported
    requestedFeatures.drawIndirectFirstInstance = VK_TRUE;
    REQUIRE_FALSE(utils::isFeaturesSupported(deviceFeatures, requestedFeatures));

    // one feature supported
    deviceFeatures.drawIndirectFirstInstance = VK_TRUE;
    REQUIRE(utils::isFeaturesSupported(deviceFeatures, requestedFeatures));

    // last feature not supported
    requestedFeatures.inheritedQueries = VK_TRUE;
    REQUIRE_FALSE(utils::isFeaturesSupported(deviceFeatures, requestedFeatures));

    // last feature supported
    deviceFeatures.inheritedQueries = VK_TRUE;
    REQUIRE(utils::isFeaturesSupported(deviceFeatures, requestedFeatures));

    // test with r values
    REQUIRE_FALSE(utils::isFeaturesSupported({VK_TRUE, VK_FALSE, VK_TRUE}, {VK_TRUE, VK_TRUE, VK_TRUE}));
    REQUIRE(utils::isFeaturesSupported({VK_TRUE, VK_TRUE, VK_TRUE}, {VK_FALSE, VK_FALSE, VK_TRUE, VK_FALSE, VK_FALSE}));
    REQUIRE(utils::isFeaturesSupported({}, {}));
}
