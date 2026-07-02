#pragma once
#include <cstdint>
#include "AnimationFileList.h"

namespace rvm {

struct ObjectScript {
    int32_t         numFrames{};
    uint8_t         surfaceNum{};
    int32_t         mainScript{};
    int32_t         playerScript{};
    int32_t         drawScript{};
    int32_t         startupScript{};
    int32_t         mainJumpTable{};
    int32_t         playerJumpTable{};
    int32_t         drawJumpTable{};
    int32_t         startupJumpTable{};
    int32_t         frameListOffset{};
    AnimationFileList* animationFile{};
    void (*nativeStartup)(){};
    void (*nativeMain)(int32_t entityNo){};
    void (*nativeDraw)(int32_t entityNo){};
};

}
