#include "../NativeScript.h"

namespace cd {

enum { PLAYER_SONIC_A = 0, PLAYER_TAILS_A = 1 };

void Explosion_Startup()
{
    if (NS_PlayerListPos() == PLAYER_SONIC_A)
        NS_LoadSpriteSheet("Global/Items2.gif");
    if (NS_PlayerListPos() == PLAYER_TAILS_A)
        NS_LoadSpriteSheet("Global/Items2_t.gif");

    NS_SpriteFrame(-16,  -8, 32, 16,  1,   1);
    NS_SpriteFrame(-16, -16, 32, 32,  1,  84);
    NS_SpriteFrame(-16, -16, 32, 32,  1, 117);
    NS_SpriteFrame(-24, -24, 48, 48, 34,   1);
    NS_SpriteFrame(-24, -24, 48, 48, 34,  50);
}

void Explosion_Draw(int32_t entityNo)
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
