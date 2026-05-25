#pragma once
#include <cstdint>

namespace rvm {

struct FontCharacter {
    int32_t  id{};
    int16_t  left{};
    int16_t  top{};
    int16_t  xSize{};
    int16_t  ySize{};
    int16_t  xPivot{};
    int16_t  yPivot{};
    int16_t  xAdvance{};
};

}
