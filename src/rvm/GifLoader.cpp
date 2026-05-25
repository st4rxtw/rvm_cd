#include "GifLoader.h"
#include "GifDecoder.h"
#include "FileIO.h"

namespace rvm {

static GifDecoder gifDecoder{};

int32_t GifLoader::codeMasks[13] = {
    0, 1, 3, 7, 15, 31, 63, 127, 255, 511, 1023, 2047, 4095
};

void GifLoader::InitGifDecoder()
{
    int num = (int32_t)FileIO::ReadByte();
    gifDecoder.fileState      = 0;
    gifDecoder.position       = 0;
    gifDecoder.bufferSize     = 0;
    gifDecoder.buffer[0]      = 0;
    gifDecoder.depth          = num;
    gifDecoder.clearCode      = 1 << num;
    gifDecoder.eofCode        = gifDecoder.clearCode + 1;
    gifDecoder.runningCode    = gifDecoder.eofCode + 1;
    gifDecoder.runningBits    = num + 1;
    gifDecoder.maxCodePlusOne = 1 << gifDecoder.runningBits;
    gifDecoder.stackPtr       = 0;
    gifDecoder.prevCode       = 4098;
    gifDecoder.shiftState     = 0;
    gifDecoder.shiftData      = 0;
    for (int i = 0; i <= 4095; ++i)
        gifDecoder.prefix[i] = 4098;
}

void GifLoader::ReadGifPictureData(int32_t width, int32_t height, bool interlaced,
                                    uint8_t* gfxData, int32_t offset)
{
    static const int passStart[4] = { 0, 4, 2, 1 };
    static const int passStep[4]  = { 8, 8, 4, 2 };
    InitGifDecoder();
    if (interlaced) {
        for (int p = 0; p < 4; ++p)
            for (int row = passStart[p]; row < height; row += passStep[p])
                ReadGifLine(gfxData, width, row * width + offset);
    } else {
        for (int row = 0; row < height; ++row)
            ReadGifLine(gfxData, width, row * width + offset);
    }
}

void GifLoader::ReadGifLine(uint8_t* line, int32_t length, int32_t offset)
{
    int num1     = 0;
    int stackPtr = gifDecoder.stackPtr;
    int eofCode  = gifDecoder.eofCode;
    int clearCode= gifDecoder.clearCode;
    int code1    = gifDecoder.prevCode;

    while (stackPtr != 0 && num1 < length)
        line[offset++] = gifDecoder.stack[--stackPtr], ++num1;

    while (num1 < length) {
        int code2 = ReadGifCode();
        if (code2 == eofCode) {
            if (num1 != length - 1 || gifDecoder.pixelCount != 0) return;
            ++num1;
        } else if (code2 == clearCode) {
            for (int i = 0; i <= 4095; ++i) gifDecoder.prefix[i] = 4098;
            gifDecoder.runningCode    = gifDecoder.eofCode + 1;
            gifDecoder.runningBits    = gifDecoder.depth + 1;
            gifDecoder.maxCodePlusOne = 1 << gifDecoder.runningBits;
            code1 = gifDecoder.prevCode = 4098;
        } else {
            if (code2 < clearCode) {
                line[offset++] = (uint8_t)code2;
                ++num1;
            } else {
                if (code2 < 0 || code2 > 4095) return;
                int index;
                if (gifDecoder.prefix[code2] == 4098) {
                    if (code2 != gifDecoder.runningCode - 2) return;
                    index = code1;
                    gifDecoder.suffix[gifDecoder.runningCode - 2] =
                        gifDecoder.stack[stackPtr++] = TracePrefix(gifDecoder.prefix, code1, clearCode);
                } else {
                    index = code2;
                }
                int num2 = 0;
                while (++num2 <= 4095 && index > clearCode && index <= 4095) {
                    gifDecoder.stack[stackPtr++] = gifDecoder.suffix[index];
                    index = (int32_t)gifDecoder.prefix[index];
                }
                if (num2 >= 4095 || index > 4095) return;
                gifDecoder.stack[stackPtr++] = (uint8_t)index;
                while (stackPtr != 0 && num1 < length)
                    line[offset++] = gifDecoder.stack[--stackPtr], ++num1;
            }
            if (code1 != 4098) {
                if (gifDecoder.runningCode < 2 || gifDecoder.runningCode > 4097) return;
                gifDecoder.prefix[gifDecoder.runningCode - 2] = (uint32_t)code1;
                gifDecoder.suffix[gifDecoder.runningCode - 2] =
                    (code2 != gifDecoder.runningCode - 2)
                        ? TracePrefix(gifDecoder.prefix, code2, clearCode)
                        : TracePrefix(gifDecoder.prefix, code1, clearCode);
            }
            code1 = code2;
        }
    }
    gifDecoder.prevCode = code1;
    gifDecoder.stackPtr = stackPtr;
}

int32_t GifLoader::ReadGifCode()
{
    while (gifDecoder.shiftState < gifDecoder.runningBits) {
        uint8_t b = ReadGifByte();
        gifDecoder.shiftData  |= (uint32_t)b << gifDecoder.shiftState;
        gifDecoder.shiftState += 8;
    }
    int num1 = (int32_t)((int64_t)gifDecoder.shiftData & (int64_t)codeMasks[gifDecoder.runningBits]);
    gifDecoder.shiftData  >>= gifDecoder.runningBits;
    gifDecoder.shiftState  -= gifDecoder.runningBits;
    if (++gifDecoder.runningCode > gifDecoder.maxCodePlusOne && gifDecoder.runningBits < 12) {
        gifDecoder.maxCodePlusOne <<= 1;
        ++gifDecoder.runningBits;
    }
    return num1;
}

uint8_t GifLoader::ReadGifByte()
{
    if (gifDecoder.fileState == 1) return 0;
    uint8_t out;
    if (gifDecoder.position == gifDecoder.bufferSize) {
        uint8_t sz = FileIO::ReadByte();
        gifDecoder.bufferSize = (int32_t)sz;
        if (gifDecoder.bufferSize == 0) { gifDecoder.fileState = 1; return 0; }
        FileIO::ReadByteArray(gifDecoder.buffer, gifDecoder.bufferSize);
        out = gifDecoder.buffer[0];
        gifDecoder.position = 1;
    } else {
        out = gifDecoder.buffer[gifDecoder.position++];
    }
    return out;
}

uint8_t GifLoader::TracePrefix(uint32_t* prefix, int32_t code, int32_t clearCode)
{
    int num = 0;
    while (code > clearCode && num++ <= 4095)
        code = (int32_t)prefix[code];
    return (uint8_t)code;
}

}
