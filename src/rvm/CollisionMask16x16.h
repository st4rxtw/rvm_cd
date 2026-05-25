#pragma once
#include <cstdint>

namespace rvm {

struct CollisionMask16x16 {
    int8_t   floorMask[16384]{};
    int8_t   leftWallMask[16384]{};
    int8_t   rightWallMask[16384]{};
    int8_t   roofMask[16384]{};
    uint32_t angle[1024]{};
    uint8_t  flags[1024]{};
};

}
