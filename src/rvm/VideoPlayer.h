#pragma once
#include <cstdint>

namespace rvm {

struct VideoPlayer {
    static int      width;
    static int      height;
    static uint8_t* frameRGB;
    static bool     hasNewFrame;

    static bool Open(const char* path);
    static void DecodeFrame();
    static bool IsFinished();
    static void Close();
};

}
