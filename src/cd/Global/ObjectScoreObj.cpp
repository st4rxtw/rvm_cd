#include "../NativeScript.h"

namespace cd {

enum { OBJECTSCORE_SCORE = 0, OBJECTSCORE_CONTROL = 1 };
enum { PRIORITY_ACTIVE = 1 };

void ObjectScore_Startup()
{
    NS_LoadSpriteSheet("Global/Items3.gif");

    NS_SpriteFrame(-6, -4, 12, 8,  0, 246);
    NS_SpriteFrame(-7, -4, 13, 8, 20, 246);
    NS_SpriteFrame(-7, -4, 13, 8, 33, 246);
    NS_SpriteFrame(-8, -4, 16, 8,  0, 246);
}

void ObjectScore_Draw(int32_t entityNo)
{
    ObjectEntity& e = ObjectSystem::objectEntityList[entityNo];

    if (e.state == OBJECTSCORE_SCORE) {
        NS_DrawSprite(entityNo, (int32_t)e.propertyValue);

        e.yPos -= 0x20000;

        e.value[0]++;
        if (e.value[0] == 24) {
            e.type = NS_TypeID("Blank Object");

            ObjectEntity& o26 = ObjectSystem::objectEntityList[26];
            if (o26.type == NS_TypeID("Object Score")) {

                if (o26.value[0] < 3)
                    o26.value[0]++;
            } else {
                o26.type     = NS_TypeID("Object Score");
                o26.value[0] = 1;
            }

            o26.state    = OBJECTSCORE_CONTROL;
            o26.value[1] = (int32_t)NS_Player().gravity;
            o26.priority = PRIORITY_ACTIVE;
        }
    } else {

        if (e.value[1] != (int32_t)NS_Player().gravity)
            NS_ResetObjectEntity(26, NS_TypeID("Blank Object"), 0, 0, 0);
    }
}

}
