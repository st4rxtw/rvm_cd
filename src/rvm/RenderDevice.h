#pragma once
#include <cstdint>

namespace rvm {

struct RenderDevice {
    static constexpr int32_t NUM_TEXTURES = 6;

    static int32_t orthWidth;
    static int32_t viewWidth;
    static int32_t viewHeight;
    static float   viewAspect;
    static int32_t bufferWidth;
    static int32_t bufferHeight;
    static int32_t highResMode;
    static bool    useFBTexture;

    static void*   gfxTexture[6];

    static void InitRenderDevice();
    static void UpdateHardwareTextures();
    static void SetScreenDimensions(int32_t width, int32_t height);
    static void FlipScreen();
    static void FlipScreenHRes();
};

}
