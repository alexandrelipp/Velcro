//
// Created by alexa on 2022-03-28.
//

#include <catch2/catch_test_macros.hpp>
#include <core/Utils/UtilsTemplate.h>
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

