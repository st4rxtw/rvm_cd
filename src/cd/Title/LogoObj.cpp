#include "../NativeScript.h"

namespace cd {

enum LogoState { LOGO_WHITE_FADEIN=0, LOGO_SONIC_ANIMATE=1, LOGO_SONIC_RESET=2 };

void Logo_Startup()
{
    NS_LoadSpriteSheet("Title/Title.gif");
    NS_SpriteFrame(-130,   0, 130, 152, 381,  73);
    NS_SpriteFrame(   0,   0, 220,  80,   1, 122);
    NS_SpriteFrame( -47, -96, 104, 120,   1,   1);
    NS_SpriteFrame(   0,   0,  56,  48, 106,  33);
    NS_SpriteFrame(   0, -38,  45,  55, 210, 203);
    NS_SpriteFrame(   0, -44,  32,  61, 223, 125);
    NS_SpriteFrame(  -1, -44,  25,  61, 230,  63);
    NS_SpriteFrame( -10, -44,  30,  61, 225,   1);
    NS_SpriteFrame(  -1, -44,  25,  61, 230,  63);
    NS_SpriteFrame(   0, -44,  32,  61, 223, 125);
    NS_SpriteFrame(   0, -38,  45,  55, 210, 203);
    NS_SpriteFrame(   0, -44,  32,  61, 223, 125);
    NS_SpriteFrame(  -1, -44,  25,  61, 230,  63);
    NS_SpriteFrame( -10, -44,  30,  61, 225,   1);
    NS_SpriteFrame(  -1, -44,  25,  61, 230,  63);
    NS_SpriteFrame(   0, -44,  32,  61, 223, 125);
    NS_SpriteFrame(   0,   0, 176,  68, 256,   4);

    NS_SpriteFrame(-174,   0, 174,  68, 246, 267);
}

void Logo_Draw(int32_t entityNo)
{
    ObjectEntity& e = ObjectSystem::objectEntityList[entityNo];
    int32_t cx = GlobalAppDefinitions::SCREEN_CENTER;

    NS_DrawSpriteScreenXY(entityNo, 16, 0,     100);
    NS_DrawSpriteScreenXY(entityNo, 17, cx * 2, 100);

    switch (e.state) {
    case LOGO_WHITE_FADEIN:
        NS_SetScreenFade(255, 255, 255, e.value[0]);

        e.direction = GlobalAppDefinitions::FACING_RIGHT;
        NS_DrawSpriteScreenFX(entityNo, 0, FX_FLIP, cx, 40);
        e.direction = GlobalAppDefinitions::FACING_LEFT;
        NS_DrawSpriteScreenFX(entityNo, 0, FX_FLIP, cx, 40);

        NS_DrawSpriteScreenXY(entityNo, 2, cx, 120);

        NS_DrawSpriteScreenXY(entityNo, 1, cx - 110, 135);
        NS_DrawSpriteScreenXY(entityNo, 4, cx - 110 + 121, 110);

        if (e.value[0] > 0) {
            e.value[0] -= 8;
        } else {
            e.state = LOGO_SONIC_ANIMATE;
            e.frame = 4;
        }
        break;

    case LOGO_SONIC_ANIMATE:
        e.direction = GlobalAppDefinitions::FACING_RIGHT;
        NS_DrawSpriteScreenFX(entityNo, 0, FX_FLIP, cx, 40);
        e.direction = GlobalAppDefinitions::FACING_LEFT;
        NS_DrawSpriteScreenFX(entityNo, 0, FX_FLIP, cx, 40);

        NS_DrawSpriteScreenXY(entityNo, 2, cx, 120);

        if (e.frame > 7 && e.frame < 13)
            NS_DrawSpriteScreenXY(entityNo, 3, cx - 43, 48);

        NS_DrawSpriteScreenXY(entityNo, 1, cx - 110, 135);
        NS_DrawSpriteScreenXY(entityNo, (int32_t)e.frame, cx - 110 + 121, 110);

        e.value[0]++;
        if (e.value[0] > 1) {
            e.value[0] = 0;
            e.frame++;
            if (e.frame > 15) {
                e.state = LOGO_SONIC_RESET;

                int32_t next = entityNo + 1;
                ObjectSystem::objectEntityList[next].type = NS_TypeID("Touch Start");
                ObjectSystem::objectEntityList[next].xPos = (int32_t)GlobalAppDefinitions::SCREEN_CENTER << 16;
                ObjectSystem::objectEntityList[next].yPos = 200 << 16;
            }
        }
        break;

    case LOGO_SONIC_RESET:
        e.direction = GlobalAppDefinitions::FACING_RIGHT;
        NS_DrawSpriteScreenFX(entityNo, 0, FX_FLIP, cx, 40);
        e.direction = GlobalAppDefinitions::FACING_LEFT;
        NS_DrawSpriteScreenFX(entityNo, 0, FX_FLIP, cx, 40);

        NS_DrawSpriteScreenXY(entityNo, 2, cx, 120);

        NS_DrawSpriteScreenXY(entityNo, 1, cx - 110, 135);
        NS_DrawSpriteScreenXY(entityNo, 4, cx - 110 + 121, 110);
        break;
    }

    if (GlobalAppDefinitions::gamePlatformID == GlobalAppDefinitions::RETRO_WP7)
        if (StageSystem::gKeyPress.buttonB)
            EngineCallbacks::RetroEngineCallback(EngineCallbacks::EXIT_GAME_SELECTED);
}

}
