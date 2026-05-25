#pragma once
#include <cstdint>

namespace rvm {

struct AnimationFileList {
    char    fileName[32]{};
    int32_t numAnimations{};
    int32_t aniListOffset{};
    int32_t cbListOffset{};
};

}
