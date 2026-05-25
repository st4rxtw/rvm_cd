#pragma once
#include <cstdint>

namespace rvm {

struct InputResult {
    uint8_t up{};
    uint8_t down{};
    uint8_t left{};
    uint8_t right{};
    uint8_t buttonA{};
    uint8_t buttonB{};
    uint8_t buttonC{};
    uint8_t start{};
    uint8_t touchDown[4]{};
    int32_t touchX[4]{};
    int32_t touchY[4]{};
    int32_t touchID[4]{};
    int32_t touches{};
};

}
