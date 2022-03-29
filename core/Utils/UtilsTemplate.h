#pragma once

#include <typeinfo>

namespace utils{

    template<typename T>
    uint32_t vectorSizeByte(const std::vector<T>& vec){
        if (vec.empty())
            return 0;
        return vec.size() * sizeof(vec[0]);
    }

    /// returns true if the two given floating (simple or double precision) numbers are almost equal
    template<typename T>
    bool almostEqual(T a, T b) {
        // why are we multiplying by 10??
        return fabs(a - b) < std::numeric_limits<T>::epsilon() * 10;
    }
}