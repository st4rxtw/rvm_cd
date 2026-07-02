#include "../NativeScript.h"

namespace cd {

enum CWLogoState {
    CWLOGO_SETUP=0,
    CWLOGO_BGSCALE_INCREASE_1=1,
    CWLOGO_BGSCALE_INCREASE_2=2,
    CWLOGO_LOGO_FADEIN=3,
    CWLOGO_BGSCALE_DECREASE=4,
    CWLOGO_LOGO_FADEOUT=5
};

void CWLogo_Startup()
{
    NS_LoadSpriteSheet("Title/CWLogo.gif");
    NS_SpriteFrame(-255, -117, 510, 235, 1, 1);
    NS_SpriteFrame(-32,  -32,   64,  64, 1, 1);
}

void CWLogo_Main(int32_t entityNo)
{
    ObjectEntity& e = ObjectSystem::objectEntityList[entityNo];

    switch (e.state) {
    case CWLOGO_SETUP:
        e.rotation  = 0;
        e.value[2]  = 0;
        e.state++;
        break;

    case CWLOGO_BGSCALE_INCREASE_1:
        if (e.value[2] < 2048) {
            e.value[2] += 64;
        } else {
            e.value[1] = 256;
            e.state++;
        }
        e.rotation  = (e.rotation + 8) & 511;
        break;

    case CWLOGO_BGSCALE_INCREASE_2:
        if (e.value[2] < 4096) {
            e.value[2] += (e.value[2] < 2048) ? 64 : 128;
            e.rotation   = (e.rotation + 8) & 511;
        }
        if (e.value[1] > 0) {
            e.value[1] -= 8;
        } else {
            e.state++;
        }
        break;

    case CWLOGO_LOGO_FADEIN:
        if (e.value[0] < 144) {
            e.value[0]++;
        } else {
            e.value[0] = 0;
            e.value[3] = 0;
            e.state++;
        }
        break;

    case CWLOGO_BGSCALE_DECREASE:
        if (e.value[2] > 0) {
            e.value[2] -= (e.value[2] < 2048) ? 64 : 128;
            e.rotation  -= 8;
            if (e.rotation < 0) e.rotation += 512;
        }
        if (e.value[1] < 256) {
            e.value[1] += 10;
        } else {
            e.state++;
            e.value[1] = 256;
        }
        break;

    case CWLOGO_LOGO_FADEOUT:
        if (e.value[2] > 0) {
            e.value[2] -= (e.value[2] < 2048) ? 64 : 128;
            e.rotation  -= 8;
            if (e.rotation < 0) e.rotation += 512;
        } else {
            if (e.value[0] == 16) {
                NS_ResetObjectEntity(entityNo, NS_TypeID("Sonic"), 0, e.xPos, e.yPos);
                ObjectSystem::objectEntityList[entityNo].inkEffect = (uint8_t)INK_ALPHA;
                ObjectSystem::objectEntityList[entityNo].alpha     = 0;
                ObjectSystem::objectEntityList[entityNo].value[0]  = 0;
                GlobalAppDefinitions::gameMode = GlobalAppDefinitions::EXIT_HIRESMODE;
                ObjectSystem::objectEntityList[entityNo].value[1] = 256;
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

void CWLogo_Draw(int32_t entityNo)
{
    ObjectEntity& e = ObjectSystem::objectEntityList[entityNo];

    NS_DrawRect(0, 0, GlobalAppDefinitions::SCREEN_XSIZE, 240, 0, 0, 96, 255);

    e.scale = e.value[2];
    NS_DrawSpriteFX(entityNo, 1, FX_ROTOZOOM, e.xPos, e.yPos);
    e.rotation += 64;
    NS_DrawSpriteFX(entityNo, 1, FX_ROTOZOOM, e.xPos, e.yPos);
    e.rotation -= 64;

    switch (e.state) {
    case CWLOGO_BGSCALE_INCREASE_2:
    case CWLOGO_LOGO_FADEIN:
    case CWLOGO_BGSCALE_DECREASE: {
        e.scale = 192;
        NS_DrawSpriteFX(entityNo, 0, FX_SCALE, e.xPos, e.yPos);
        int32_t tx = GlobalAppDefinitions::SCREEN_CENTER - 96;
        int32_t ty = 120 - 48;
        NS_DrawRect(tx, ty, 192, 96, 255, 149, 18, e.value[1]);
        break;
    }
    default:
        break;
    }
}

}
