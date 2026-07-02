#include "../NativeScript.h"

namespace cd {

enum SonicState { SONIC_SETUP=0, SONIC_TURNING=1 };

void Sonic_Startup()
{
    NS_SetMusicTrack("JP/TitleScreen.ogg", 0, 0);
    NS_SetMusicTrack("US/TitleScreen.ogg", 1, 0);

    NS_LoadSpriteSheet("Title/Title.gif");
    NS_SpriteFrame(-47, -89,  96, 112,   1, 203);
    NS_SpriteFrame(-47, -89, 104, 120,  98, 203);
    NS_SpriteFrame(-47, -96,  96, 120,   1, 316);
    NS_SpriteFrame(-47, -96, 104, 120,  98, 324);
    NS_SpriteFrame(-47, -96, 104, 120,   1,   1);
    NS_SpriteFrame(  0,   0,  45,  55, 210, 203);
}

void Sonic_Draw(int32_t entityNo)
{
    ObjectEntity& e = ObjectSystem::objectEntityList[entityNo];

    NS_DrawRect(0, 0, GlobalAppDefinitions::SCREEN_XSIZE, 240, 0, 0, 96, 255);

    switch (e.state) {
    case SONIC_SETUP:
        if (e.alpha > 8)
            NS_DrawSpriteFX(entityNo, 0, FX_INK, e.xPos, e.yPos);

        if (e.value[0] < 256) {
            e.value[0] += 8;
            e.alpha = (uint8_t)(e.value[0] < 256 ? e.value[0] : 255);
        } else {
            if (e.value[0] < 272) {
                e.value[0]++;
            } else {
                e.value[0] = 0;
                e.state    = SONIC_TURNING;

                NS_SetGlobalVar("LampPost.Check", 0);
                NS_SetGlobalVar("Player.ControlMode", 0);
                PlayerSystem::playerMenuNum = 0;
                NS_PlayMusic(NS_GetGlobalVar("Options.Soundtrack"));
            }
        }
        break;

    case SONIC_TURNING: {
        int32_t frame = e.value[0] >> 2;
        NS_DrawSprite(entityNo, frame);

        int32_t armX = 11 + GlobalAppDefinitions::SCREEN_CENTER;
        if (e.value[0] > 15)
            NS_DrawSpriteScreenXY(entityNo, 5, armX, 72);

        e.value[0]++;
        if (e.value[0] > 19) {
            int32_t savedEntityNo = entityNo;
            NS_ResetObjectEntity(entityNo, NS_TypeID("Logo"), 0, e.xPos, e.yPos);
            ObjectSystem::objectEntityList[entityNo].value[0] = 256;

            int32_t ap = savedEntityNo - 2;
            ObjectSystem::objectEntityList[ap].type      = NS_TypeID("Background");
            ObjectSystem::objectEntityList[ap].drawOrder = 0;

            StageSystem::activeTileLayers[2] = 9;

            ap++;
            ObjectSystem::objectEntityList[ap].type = NS_TypeID("Clouds");
        }
        break;
    }
    }
}

}
