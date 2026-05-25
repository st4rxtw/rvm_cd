#pragma once
#include <cstdint>

namespace rvm {

struct LineScrollParallax {
    int32_t parallaxFactor[256]{};
    int32_t scrollSpeed[256]{};
    int32_t scrollPosition[256]{};
    int32_t linePos[256]{};
    uint8_t deformationEnabled[256]{};
    uint8_t numEntries{};
};

}
