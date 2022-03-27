#pragma once

#include <typeinfo>

namespace utils{

    uint32_t vectorSizeByte(const auto& vec){
        if (vec.empty())
            return 0;
        return vec.size() * sizeof(vec[0]);
    }

}