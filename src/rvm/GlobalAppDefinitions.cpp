#include "GlobalAppDefinitions.h"
#include "FileIO.h"
#include "ObjectSystem.h"
#include <cmath>
#include <cstring>

namespace rvm {

char    GlobalAppDefinitions::gameWindowText[32]    = "Retro-Engine";
char    GlobalAppDefinitions::gameVersion[8]        = "1.0.0";
char    GlobalAppDefinitions::gameDescriptionText[256]{};
char    GlobalAppDefinitions::gamePlatform[16]      = "Desktop";
char    GlobalAppDefinitions::gameRenderType[16]    = "HW_Rendering";
char    GlobalAppDefinitions::gameHapticsSetting[16]= "No_Haptics";
uint8_t GlobalAppDefinitions::gameMode              = 1;
uint8_t GlobalAppDefinitions::gameLanguage          = 0;
int32_t GlobalAppDefinitions::gameMessage           = 0;
uint8_t GlobalAppDefinitions::gameOnlineActive      = 0;
uint8_t GlobalAppDefinitions::gameHapticsEnabled    = 0;
uint8_t GlobalAppDefinitions::frameCounter          = 0;
int32_t GlobalAppDefinitions::frameSkipTimer        = -1;
int32_t GlobalAppDefinitions::frameSkipSetting      = 0;
int32_t GlobalAppDefinitions::gameSFXVolume         = 100;
int32_t GlobalAppDefinitions::gameBGMVolume         = 100;
uint8_t GlobalAppDefinitions::gameTrialMode         = 0;
int32_t GlobalAppDefinitions::gamePlatformID        = 0;
bool    GlobalAppDefinitions::HQ3DFloorEnabled      = false;
int32_t GlobalAppDefinitions::SCREEN_XSIZE          = 320;
int32_t GlobalAppDefinitions::SCREEN_CENTER         = 0;
int32_t GlobalAppDefinitions::SCREEN_SCROLL_LEFT    = 0;
int32_t GlobalAppDefinitions::SCREEN_SCROLL_RIGHT   = 0;
int32_t GlobalAppDefinitions::OBJECT_BORDER_X1      = 0;
int32_t GlobalAppDefinitions::OBJECT_BORDER_X2      = 0;
int32_t GlobalAppDefinitions::SinValue256[256]{};
int32_t GlobalAppDefinitions::CosValue256[256]{};
int32_t GlobalAppDefinitions::SinValue512[512]{};
int32_t GlobalAppDefinitions::CosValue512[512]{};
int32_t GlobalAppDefinitions::SinValueM7[512]{};
int32_t GlobalAppDefinitions::CosValueM7[512]{};
uint8_t GlobalAppDefinitions::ATanValue256[256][256]{};

void GlobalAppDefinitions::CalculateTrigAngles()
{
    for (int i = 0; i < 512; ++i) {
        SinValueM7[i] = (int32_t)(std::sin((double)i / 256.0 * Pi) * 4096.0);
        CosValueM7[i] = (int32_t)(std::cos((double)i / 256.0 * Pi) * 4096.0);
    }
    SinValueM7[0]   = 0;    CosValueM7[0]   = 4096;
    SinValueM7[128] = 4096; CosValueM7[128] = 0;
    SinValueM7[256] = 0;    CosValueM7[256] = -4096;
    SinValueM7[384] = -4096;CosValueM7[384] = 0;

    for (int i = 0; i < 512; ++i) {
        SinValue512[i] = (int32_t)(std::sin((double)i / 256.0 * Pi) * 512.0);
        CosValue512[i] = (int32_t)(std::cos((double)i / 256.0 * Pi) * 512.0);
    }
    SinValue512[0]   = 0;   CosValue512[0]   = 512;
    SinValue512[128] = 512; CosValue512[128] = 0;
    SinValue512[256] = 0;   CosValue512[256] = -512;
    SinValue512[384] = -512;CosValue512[384] = 0;

    for (int i = 0; i < 256; ++i) {
        SinValue256[i] = SinValue512[i << 1] >> 1;
        CosValue256[i] = CosValue512[i << 1] >> 1;
    }

    for (int y = 0; y < 256; ++y) {
        for (int x = 0; x < 256; ++x) {
            ATanValue256[x][y] = (uint8_t)(std::atan2((double)y, (double)x) * 40.74366542620519);
        }
    }
}

uint8_t GlobalAppDefinitions::ArcTanLookup(int32_t x, int32_t y)
{
    int32_t ix = x >= 0 ? x : -x;
    int32_t iy = y >= 0 ? y : -y;
    if (ix > iy) {
        while (ix > 255) { ix >>= 4; iy >>= 4; }
    } else {
        while (iy > 255) { ix >>= 4; iy >>= 4; }
    }
    if (x <= 0) {
        if (y <= 0)
            return (uint8_t)(128 + ATanValue256[ix][iy]);
        else
            return (uint8_t)(128 - ATanValue256[ix][iy]);
    } else {
        if (y <= 0)
            return (uint8_t)(256 - ATanValue256[ix][iy]);
        else
            return ATanValue256[ix][iy];
    }
}

void GlobalAppDefinitions::LoadGameConfig(const char* filePath)
{
    FileData fData{};
    char strB[32]{};
    if (!FileIO::LoadFile(filePath, fData))
        return;

    uint8_t numBytes1 = FileIO::ReadByte();
    FileIO::ReadCharArray(gameWindowText, (int32_t)numBytes1);
    gameWindowText[numBytes1] = '\0';

    uint8_t num1 = FileIO::ReadByte();
    uint8_t num2;
    for (int i = 0; i < (int)num1; ++i)
        num2 = FileIO::ReadByte();

    uint8_t numBytes2 = FileIO::ReadByte();
    FileIO::ReadCharArray(gameDescriptionText, (int32_t)numBytes2);
    gameDescriptionText[numBytes2] = '\0';

    uint8_t num3 = FileIO::ReadByte();
    for (int i = 0; i < (int)num3; ++i) {
        uint8_t num4 = FileIO::ReadByte();
        for (int j = 0; j < (int)num4; ++j)
            num2 = FileIO::ReadByte();
    }
    for (int i = 0; i < (int)num3; ++i) {
        uint8_t num5 = FileIO::ReadByte();
        for (int j = 0; j < (int)num5; ++j)
            num2 = FileIO::ReadByte();
    }

    uint8_t num6 = FileIO::ReadByte();
    ObjectSystem::NO_GLOBALVARIABLES = 0;
    for (int strPos = 0; strPos < (int)num6; ++strPos) {
        ++ObjectSystem::NO_GLOBALVARIABLES;
        uint8_t num7 = FileIO::ReadByte();
        int idx;
        for (idx = 0; idx < (int)num7; ++idx) {
            uint8_t num8 = FileIO::ReadByte();
            strB[idx] = (char)num8;
        }
        strB[idx] = '\0';
        FileIO::StrCopy(ObjectSystem::globalVariableNames[strPos], 32, strB, 32);
        uint8_t n1 = FileIO::ReadByte();
        ObjectSystem::globalVariables[strPos] = (int32_t)n1 << 24;
        uint8_t n2 = FileIO::ReadByte();
        ObjectSystem::globalVariables[strPos] += (int32_t)n2 << 16;
        uint8_t n3 = FileIO::ReadByte();
        ObjectSystem::globalVariables[strPos] += (int32_t)n3 << 8;
        uint8_t n4 = FileIO::ReadByte();
        ObjectSystem::globalVariables[strPos] += (int32_t)n4;
    }

    uint8_t num13 = FileIO::ReadByte();
    for (int i = 0; i < (int)num13; ++i) {
        uint8_t num14 = FileIO::ReadByte();
        int idx;
        for (idx = 0; idx < (int)num14; ++idx) {
            uint8_t num15 = FileIO::ReadByte();
            strB[idx] = (char)num15;
        }
        strB[idx] = '\0';
    }

    uint8_t num16 = FileIO::ReadByte();
    for (int i = 0; i < (int)num16; ++i) {
        uint8_t num17 = FileIO::ReadByte();
        for (int j = 0; j < (int)num17; ++j)
            num2 = FileIO::ReadByte();
    }

    FileIO::noPresentationStages = FileIO::ReadByte();
    uint8_t num18;
    for (int i = 0; i < (int)FileIO::noPresentationStages; ++i) {
        uint8_t num19 = FileIO::ReadByte();
        int idx1;
        for (idx1 = 0; idx1 < (int)num19; ++idx1) {
            uint8_t b = FileIO::ReadByte();
            FileIO::pStageList[i].stageFolderName[idx1] = (char)b;
        }
        FileIO::pStageList[i].stageFolderName[idx1] = '\0';
        uint8_t num21 = FileIO::ReadByte();
        int idx2;
        for (idx2 = 0; idx2 < (int)num21; ++idx2) {
            uint8_t b = FileIO::ReadByte();
            FileIO::pStageList[i].actNumber[idx2] = (char)b;
        }
        FileIO::pStageList[i].actNumber[idx2] = '\0';
        uint8_t num23 = FileIO::ReadByte();
        for (int j = 0; j < (int)num23; ++j)
            num2 = FileIO::ReadByte();
        num18 = FileIO::ReadByte();
    }

    FileIO::noZoneStages = FileIO::ReadByte();
    for (int i = 0; i < (int)FileIO::noZoneStages; ++i) {
        uint8_t num24 = FileIO::ReadByte();
        int idx1;
        for (idx1 = 0; idx1 < (int)num24; ++idx1) {
            FileIO::zStageList[i].stageFolderName[idx1] = (char)FileIO::ReadByte();
        }
        FileIO::zStageList[i].stageFolderName[idx1] = '\0';
        uint8_t num26 = FileIO::ReadByte();
        int idx2;
        for (idx2 = 0; idx2 < (int)num26; ++idx2) {
            FileIO::zStageList[i].actNumber[idx2] = (char)FileIO::ReadByte();
        }
        FileIO::zStageList[i].actNumber[idx2] = '\0';
        uint8_t num28 = FileIO::ReadByte();
        for (int j = 0; j < (int)num28; ++j)
            num2 = FileIO::ReadByte();
        num18 = FileIO::ReadByte();
    }

    FileIO::noSpecialStages = FileIO::ReadByte();
    for (int i = 0; i < (int)FileIO::noSpecialStages; ++i) {
        uint8_t n = FileIO::ReadByte();
        int idx1;
        for (idx1 = 0; idx1 < (int)n; ++idx1)
            FileIO::sStageList[i].stageFolderName[idx1] = (char)FileIO::ReadByte();
        FileIO::sStageList[i].stageFolderName[idx1] = '\0';
        uint8_t n2_ = FileIO::ReadByte();
        int idx2;
        for (idx2 = 0; idx2 < (int)n2_; ++idx2)
            FileIO::sStageList[i].actNumber[idx2] = (char)FileIO::ReadByte();
        FileIO::sStageList[i].actNumber[idx2] = '\0';
        uint8_t num33 = FileIO::ReadByte();
        for (int j = 0; j < (int)num33; ++j)
            num2 = FileIO::ReadByte();
        num18 = FileIO::ReadByte();
    }

    FileIO::noBonusStages = FileIO::ReadByte();
    for (int i = 0; i < (int)FileIO::noBonusStages; ++i) {
        uint8_t n = FileIO::ReadByte();
        int idx1;
        for (idx1 = 0; idx1 < (int)n; ++idx1)
            FileIO::bStageList[i].stageFolderName[idx1] = (char)FileIO::ReadByte();
        FileIO::bStageList[i].stageFolderName[idx1] = '\0';
        uint8_t n2_ = FileIO::ReadByte();
        int idx2;
        for (idx2 = 0; idx2 < (int)n2_; ++idx2)
            FileIO::bStageList[i].actNumber[idx2] = (char)FileIO::ReadByte();
        FileIO::bStageList[i].actNumber[idx2] = '\0';
        uint8_t num38 = FileIO::ReadByte();
        for (int j = 0; j < (int)num38; ++j)
            num2 = FileIO::ReadByte();
        num18 = FileIO::ReadByte();
    }

    (void)num18;
    FileIO::CloseFile();
}

}
