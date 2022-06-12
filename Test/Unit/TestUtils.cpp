//
// Created by alexa on 2022-03-28.
//

#include <catch2/catch_test_macros.hpp>
#include <core/Utils/UtilsTemplate.h>
#include <core/Utils/UtilsVulkan.h>
#include <core/Utils/UtilsMath.h>
#include <msdf-atlas-gen/utf8.h>

#include <string>

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
    VkPhysicalDeviceFeatures deviceFeatures{}, requestedFeatures{};
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

TEST_CASE( "Stub", "[UtilsVulkan]"){
    using namespace std;
    string text = "今!é123";
    //for(size_t i = 0; i < text.length();)
//    {
//        int cplen = 1;
//        if((text[i] & 0xf8) == 0xf0) cplen = 4;
//        else if((text[i] & 0xf0) == 0xe0) cplen = 3;
//        else if((text[i] & 0xe0) == 0xc0) cplen = 2;
//        if((i + cplen) > text.length()) cplen = 1;
//
//        cout << text.substr(i, cplen) << endl;
//        i += cplen;
//    }

    std::vector<msdf_atlas::unicode_t> unicodes;
    msdf_atlas::utf8Decode(unicodes, text.c_str());
    for (auto u : unicodes)
        std::cout << u << std::endl;



};
