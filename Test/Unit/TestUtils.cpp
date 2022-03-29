//
// Created by alexa on 2022-03-28.
//

#include <catch2/catch_test_macros.hpp>
#include <core/Utils/UtilsTemplate.h>

TEST_CASE( "VectorSizeByte", "[UtilsTemplate]" ) {
    std::vector<int> ok = {1, 2, 3};
    REQUIRE(utils::vectorSizeByte(ok) == sizeof(int) * 3);

    ok.clear();
    REQUIRE(utils::vectorSizeByte(ok) == 0);
}

