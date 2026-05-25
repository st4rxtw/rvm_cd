#pragma once
#include <cstdint>

namespace rvm {

struct SpriteFrame {
    int32_t  left{};
    int32_t  top{};
    int32_t  xSize{};
    int32_t  ySize{};
    int32_t  xPivot{};
    int32_t  yPivot{};
    uint8_t  surfaceNum{};
    uint8_t  collisionBox{};
};

}
