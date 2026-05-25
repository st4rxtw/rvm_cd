#pragma once
#include <cstdint>

namespace rvm {

struct SpriteAnimation {
    char    name[16]{};
    uint8_t numFrames{};
    uint8_t animationSpeed{};
    uint8_t loopPosition{};
    uint8_t rotationFlag{};
    int32_t frameListOffset{};
};

}
