#pragma once
#include <cstdint>

namespace rvm {

struct GifLoader {
    static constexpr int32_t LOADING_IMAGE = 0;
    static constexpr int32_t LOAD_COMPLETE = 1;
    static constexpr int32_t LZ_MAX_CODE   = 4095;
    static constexpr int32_t LZ_BITS       = 12;
    static constexpr int32_t FLUSH_OUTPUT  = 4096;
    static constexpr int32_t FIRST_CODE    = 4097;
    static constexpr int32_t NO_SUCH_CODE  = 4098;
    static constexpr int32_t HT_SIZE       = 8192;
    static constexpr int32_t HT_KEY_MASK   = 8191;

    static void InitGifDecoder();
    static void ReadGifPictureData(int32_t width, int32_t height, bool interlaced,
                                   uint8_t* gfxData, int32_t offset);
    static void ReadGifLine(uint8_t* line, int32_t length, int32_t offset);

private:
    static int32_t ReadGifCode();
    static uint8_t ReadGifByte();
    static uint8_t TracePrefix(uint32_t* prefix, int32_t code, int32_t clearCode);

    static int32_t codeMasks[13];
};

}
