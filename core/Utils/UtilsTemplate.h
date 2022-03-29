#pragma once

#include <typeinfo>

namespace utils{

    template<typename T>
    uint32_t vectorSizeByte(const std::vector<T>& vec){
        if (vec.empty())
            return 0;
        return vec.size() * sizeof(vec[0]);
    }

}