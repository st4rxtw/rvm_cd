#pragma once
#include <cstdint>

namespace rvm {

struct GifDecoder {
    int32_t  depth{};
    int32_t  clearCode{};
    int32_t  eofCode{};
    int32_t  runningCode{};
    int32_t  runningBits{};
    int32_t  prevCode{};
    int32_t  currentCode{};
    int32_t  maxCodePlusOne{};
    int32_t  stackPtr{};
    int32_t  shiftState{};
    int32_t  fileState{};
    int32_t  position{};
    int32_t  bufferSize{};
    uint32_t shiftData{};
    uint32_t pixelCount{};
    uint8_t  buffer[256]{};
    uint8_t  stack[4096]{};
    uint8_t  suffix[4096]{};
    uint32_t prefix[4096]{};
};

}
