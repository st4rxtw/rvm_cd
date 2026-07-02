#include "../NativeScript.h"

namespace cd {

enum { INVINCIBILITY_STATE = 0, INVINCIBILITY_AFTERIMAGE = 1 };

void Invincibility_Startup()
{
    NS_LoadSpriteSheet("Global/Items3.gif");

    NS_SpriteFrame(-24, -24, 48, 48, 1,   1);
    NS_SpriteFrame(-24, -24, 48, 48, 1,  50);
    NS_SpriteFrame(-24, -24, 48, 48, 1,  99);
    NS_SpriteFrame(-24, -24, 48, 48, 1, 148);
}

void Invincibility_Draw(int32_t entityNo)
{
    ObjectEntity& e = ObjectSystem::objectEntityList[entityNo];

    switch (e.state) {
    case INVINCIBILITY_STATE:
        e.value[0]++;
        if (e.value[0] > 3) {
            e.value[0] = 0;

            int32_t tp = NS_CreateTempObject(NS_TypeID("Invincibility"), 0,
                                             NS_Player().xPos, NS_Player().yPos);
            ObjectEntity& t = ObjectSystem::objectEntityList[tp];
            t.state     = INVINCIBILITY_AFTERIMAGE;
            t.inkEffect = INK_ALPHA;
            t.alpha     = 256;
            t.drawOrder = 4;

            e.frame = (uint8_t)(NS_GetGlobalVar("Ring.Frame") & 3);

            if (NS_GetGlobalVar("Warp.Timer") == 0)
                NS_DrawSpriteXY(entityNo, (int32_t)e.frame, NS_Player().xPos, NS_Player().yPos);
        }
        break;

    case INVINCIBILITY_AFTERIMAGE:
        e.frame = (uint8_t)(NS_GetGlobalVar("Ring.Frame") & 3);

        if (NS_GetGlobalVar("Warp.Timer") == 0)
            NS_DrawSpriteFX(entityNo, (int32_t)e.frame, FX_INK, e.xPos, e.yPos);

        e.alpha -= 8;
        if (e.alpha == 128)
            e.type = NS_TypeID("Blank Object");
        break;
    }
}

}
