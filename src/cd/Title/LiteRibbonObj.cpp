#include "../NativeScript.h"

namespace cd {

void LiteRibbon_Startup()
{
    NS_LoadSpriteSheet("Title/Title.gif");
    NS_SpriteFrame(0, 0, 84, 84, 427, 226);
}

void LiteRibbon_Draw(int32_t entityNo)
{
    NS_DrawSprite(entityNo, 0);
}

}
