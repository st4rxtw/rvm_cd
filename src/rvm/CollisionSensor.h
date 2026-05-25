#pragma once
#include <cstdint>

namespace rvm {

struct CollisionSensor {
    int32_t xPos{};
    int32_t yPos{};
    int32_t angle{};
    uint8_t collided{};
    uint8_t flags{};
};

}
