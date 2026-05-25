#pragma once
#include <cstdint>

namespace rvm {

struct GlobalAppDefinitions {
    static constexpr int32_t RETRO_EN       = 0;
    static constexpr int32_t RETRO_FR       = 1;
    static constexpr int32_t RETRO_IT       = 2;
    static constexpr int32_t RETRO_DE       = 3;
    static constexpr int32_t RETRO_ES       = 4;
    static constexpr int32_t RETRO_JP       = 5;
    static constexpr int32_t DEFAULTSCREEN  = 0;
    static constexpr int32_t MAINGAME       = 1;
    static constexpr int32_t RESETGAME      = 2;
    static constexpr int32_t EXITGAME       = 3;
    static constexpr int32_t SCRIPTERROR    = 4;
    static constexpr int32_t ENTER_HIRESMODE= 5;
    static constexpr int32_t EXIT_HIRESMODE = 6;
    static constexpr int32_t PAUSE_ENGINE   = 7;
    static constexpr int32_t PAUSE_WAIT     = 8;
    static constexpr int32_t VIDEO_WAIT     = 9;
    static constexpr int32_t RETRO_WIN      = 0;
    static constexpr int32_t RETRO_OSX      = 1;
    static constexpr int32_t RETRO_XBOX_360 = 2;
    static constexpr int32_t RETRO_PS3      = 3;
    static constexpr int32_t RETRO_iOS      = 4;
    static constexpr int32_t RETRO_ANDROID  = 5;
    static constexpr int32_t RETRO_WP7      = 6;
    static constexpr int32_t MAX_PLAYERS    = 2;
    static constexpr int32_t FACING_LEFT    = 1;
    static constexpr int32_t FACING_RIGHT   = 0;
    static constexpr int32_t GAME_FULL      = 0;
    static constexpr int32_t GAME_TRIAL     = 1;
    static constexpr int32_t OBJECT_BORDER_Y1 = 256;
    static constexpr int32_t OBJECT_BORDER_Y2 = 496;
    static constexpr double  Pi             = 3.141592654;

    static char    gameWindowText[32];
    static char    gameVersion[8];
    static char    gameDescriptionText[256];
    static char    gamePlatform[16];
    static char    gameRenderType[16];
    static char    gameHapticsSetting[16];
    static uint8_t gameMode;
    static uint8_t gameLanguage;
    static int32_t gameMessage;
    static uint8_t gameOnlineActive;
    static uint8_t gameHapticsEnabled;
    static uint8_t frameCounter;
    static int32_t frameSkipTimer;
    static int32_t frameSkipSetting;
    static int32_t gameSFXVolume;
    static int32_t gameBGMVolume;
    static uint8_t gameTrialMode;
    static int32_t gamePlatformID;
    static bool    HQ3DFloorEnabled;
    static int32_t SCREEN_XSIZE;
    static int32_t SCREEN_CENTER;
    static int32_t SCREEN_SCROLL_LEFT;
    static int32_t SCREEN_SCROLL_RIGHT;
    static int32_t OBJECT_BORDER_X1;
    static int32_t OBJECT_BORDER_X2;
    static int32_t SinValue256[256];
    static int32_t CosValue256[256];
    static int32_t SinValue512[512];
    static int32_t CosValue512[512];
    static int32_t SinValueM7[512];
    static int32_t CosValueM7[512];
    static uint8_t ATanValue256[256][256];

    static void CalculateTrigAngles();
    static uint8_t ArcTanLookup(int32_t x, int32_t y);
    static void LoadGameConfig(const char* filePath);
};

}
