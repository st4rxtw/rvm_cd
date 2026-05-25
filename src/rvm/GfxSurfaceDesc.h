#pragma once
#include <cstdint>

namespace rvm {

struct GfxSurfaceDesc {
    char     fileName[64]{};
    int32_t  width{};
    int32_t  height{};
    int32_t  texStartX{};
    int32_t  texStartY{};
    uint32_t dataStart{};
};

}
