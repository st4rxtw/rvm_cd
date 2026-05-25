#pragma once
#include <cstdint>

namespace rvm {

struct Face3D {
    int32_t a{};
    int32_t b{};
    int32_t c{};
    int32_t d{};
    int32_t color{};
    uint8_t flag{};
};

}
