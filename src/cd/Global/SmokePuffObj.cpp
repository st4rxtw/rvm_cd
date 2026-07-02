#include "../NativeScript.h"

namespace cd {

void SmokePuff_Startup()
{
    if (NS_PlayerListPos() == 0)
        NS_LoadSpriteSheet("Global/Items2.gif");
    if (NS_PlayerListPos() == 1)
        NS_LoadSpriteSheet("Global/Items2_t.gif");

    NS_SpriteFrame(-16,  -8, 32, 16,  1,  1);
    NS_SpriteFrame(-16, -16, 32, 32,  1, 18);
    NS_SpriteFrame(-16, -16, 32, 32,  1, 51);
    NS_SpriteFrame(-24, -24, 48, 48, 34,  1);
    NS_SpriteFrame(-24, -24, 48, 48, 34, 50);
}

void SmokePuff_Draw(int32_t entityNo)
{
    ObjectEntity& e = ObjectSystem::objectEntityList[entityNo];

    NS_DrawSprite(entityNo, (int32_t)e.frame);
    e.value[0]++;
    if (e.value[0] > 3) {
        e.value[0] = 0;
        e.frame++;
        if (e.frame > 4) {
            e.type  = NS_TypeID("Blank Object");
            e.frame = 0;
        }
    }
}

}
