#pragma once
#include <cstdint>

namespace rvm {

struct FileData {
    char     fileName[64]{};
    uint32_t fileSize{};
    uint32_t position{};
    uint32_t bufferPos{};
    uint32_t virtualFileOffset{};
    uint8_t  eStringPosA{};
    uint8_t  eStringPosB{};
    uint8_t  eStringNo{};
    bool     eNybbleSwap{};
};

}
