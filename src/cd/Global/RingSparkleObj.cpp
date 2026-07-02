#include "../NativeScript.h"

namespace cd {

void RingSparkle_Startup()
{
    NS_LoadSpriteSheet("Global/Items.gif");

    NS_SpriteFrame(-8, -8, 16, 16,  1, 202);
    NS_SpriteFrame(-8, -8, 16, 16, 18, 202);
    NS_SpriteFrame(-8, -8, 16, 16,  1, 219);
    NS_SpriteFrame(-8, -8, 16, 16, 18, 219);
}

void RingSparkle_Main(int32_t entityNo)
{
    ObjectEntity& e = ObjectSystem::objectEntityList[entityNo];

    e.value[0]++;
    if (e.value[0] == 6) {
        e.value[0] = 0;
        e.frame++;
        if (e.frame == 4) {
            e.type  = NS_TypeID("Blank Object");
            e.frame = 0;
        }
    }
}

void RingSparkle_Draw(int32_t entityNo)
{
    ObjectEntity& e = ObjectSystem::objectEntityList[entityNo];
    NS_DrawSprite(entityNo, (int32_t)e.frame);
}

}
