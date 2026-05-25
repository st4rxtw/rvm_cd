#pragma once
#include <cstdint>

namespace rvm {

struct LayoutMap {
    uint16_t tileMap[65536]{};
    uint8_t  lineScrollRef[32768]{};
    int32_t  parallaxFactor{};
    int32_t  scrollSpeed{};
    int32_t  scrollPosition{};
    int32_t  angle{};
    int32_t  xPos{};
    int32_t  yPos{};
    int32_t  zPos{};
    int32_t  deformationPos{};
    int32_t  deformationPosW{};
    uint8_t  type{};
    uint8_t  xSize{};
    uint8_t  ySize{};
};

}
