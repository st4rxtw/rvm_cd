#pragma once
#include <cstdint>

namespace rvm {

struct Mappings128x128 {
    int32_t  gfxDataPos[32768]{};
    uint16_t tile16x16[32768]{};
    uint8_t  direction[32768]{};
    uint8_t  visualPlane[32768]{};

    uint8_t  collisionFlag[2][32768]{};
};

}
