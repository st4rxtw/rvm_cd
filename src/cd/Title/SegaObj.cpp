#include "../NativeScript.h"

namespace cd {

enum SegaState { SEGA_SETUP=0, SEGA_FADEIN=1, SEGA_JINGLE=2, SEGA_FADEOUT=3, SEGA_END=4 };

void Sega_Startup()
{
    NS_LoadSpriteSheet("Title/Sega.gif");
    NS_SpriteFrame(-255, -87, 510, 174, 1, 1);
}

void Sega_Main(int32_t entityNo)
{
    ObjectEntity& e = ObjectSystem::objectEntityList[entityNo];

    switch (e.state) {
    case SEGA_SETUP:
        FileIO::ReadSaveRAMData();
        if (FileIO::saveRAM[32]) {
            GlobalAppDefinitions::gameBGMVolume = FileIO::saveRAM[33];
            GlobalAppDefinitions::gameSFXVolume = FileIO::saveRAM[34];
        }
        NS_SetGlobalVar("Options.Soundtrack", FileIO::saveRAM[38]);
        e.state = SEGA_FADEIN;
        e.value[0] = 384;
        GlobalAppDefinitions::gameMode = GlobalAppDefinitions::ENTER_HIRESMODE;
        e.alpha     = 255;
        e.inkEffect = INK_ALPHA;
        NS_SetScreenFade(0, 0, 0, 255);
        e.xPos = (int32_t)GlobalAppDefinitions::SCREEN_CENTER << 16;
        break;

    case SEGA_FADEIN:
        if (e.value[0] > 0) {
            e.value[0] -= 8;
        } else {
            if (GlobalAppDefinitions::gameOnlineActive < 2)
                e.state = SEGA_JINGLE;
        }
        NS_SetScreenFade(0, 0, 0, e.value[0]);
        break;

    case SEGA_JINGLE:
        e.value[0]++;
        if (e.value[0] == 160) {
            e.value[0] = 0;
            e.state    = SEGA_FADEOUT;
        }
        if (e.value[0] == 2)
            NS_PlayStageSfx(0, false);
        break;

    case SEGA_FADEOUT:
        if (e.value[1] < 256)
            e.value[1] += 8;

        if (e.alpha > 7) {
            e.alpha -= 8;
        } else {
            if (e.value[0] == 16) {
                NS_ResetObjectEntity(entityNo, NS_TypeID("CWLogo"), 0, e.xPos, e.yPos);
                ObjectSystem::objectEntityList[entityNo].value[3] = 96;
                ObjectSystem::objectEntityList[entityNo].inkEffect = (uint8_t)INK_ALPHA;
                ObjectSystem::objectEntityList[entityNo].alpha     = 0;
                ObjectSystem::objectEntityList[entityNo].value[0]  = 0;
            } else {
                e.value[0]++;
            }
        }
        break;
    }

    if (GlobalAppDefinitions::gamePlatformID == GlobalAppDefinitions::RETRO_WP7)
        if (StageSystem::gKeyPress.buttonB)
            EngineCallbacks::RetroEngineCallback(EngineCallbacks::EXIT_GAME_SELECTED);
}

void Sega_Draw(int32_t entityNo)
{
    ObjectEntity& e = ObjectSystem::objectEntityList[entityNo];
    NS_DrawRect(0, 0, GlobalAppDefinitions::SCREEN_XSIZE, 240, 0, 0, 96, 255);
    e.scale = 128;
    NS_DrawSpriteFX(entityNo, 0, FX_SCALE, e.xPos, e.yPos);
    NS_DrawRect(0, 0, GlobalAppDefinitions::SCREEN_XSIZE, 240, 0, 0, 96, e.value[1]);
}

}
