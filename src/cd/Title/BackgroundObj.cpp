#include "../NativeScript.h"

namespace cd {

void Background_Startup()
{
    NS_LoadSpriteSheet("Title/Title.gif");

    NS_SpriteFrame(   0,   0, 176,  68, 256,   4);
    NS_SpriteFrame(-144,   0, 144,  68, 257, 251);
    NS_SpriteFrame(   0,   0, 120, 104, 257, 146);

    int32_t uv    = 447;
    int32_t yPivot= 168;
    while (uv < 511) {
        NS_SpriteFrame(    0, yPivot, 512, 4, 0, uv);
        NS_SpriteFrame( -512, yPivot, 512, 4, 0, uv);
        uv     += 4;
        yPivot += 4;
    }

    uv -= 16;
    while (yPivot < 240) {
        NS_SpriteFrame(    0, yPivot, 512, 4, 0, uv);
        NS_SpriteFrame( -512, yPivot, 512, 4, 0, uv);
        uv     += 4;
        yPivot += 4;
    }
}

void Background_Draw(int32_t entityNo)
{
    ObjectEntity& e = ObjectSystem::objectEntityList[entityNo];

    int32_t bgWidth = GlobalAppDefinitions::SCREEN_CENTER << 1;
    NS_DrawRect(0, 0, bgWidth, 100, 0, 0, 96, 255);

    int32_t lpSin = NS_Sin512(e.value[0]);
    lpSin >>= 7;
    lpSin -= 16;
    NS_DrawSpriteScreenXY(entityNo, 2, 280, lpSin);
    e.value[0] = (e.value[0] + 1) & 511;

    int32_t spriteIdx = 3;
    int32_t speed     = 16;
    int32_t ap        = 40;

    while (spriteIdx < 39) {
        ObjectEntity& we = ObjectSystem::objectEntityList[ap];
        we.xPos += speed;
        if (we.xPos > 0x2000000)
            we.xPos -= 0x2000000;

        int32_t screenX = we.xPos >> 16;
        ap++;

        NS_DrawSpriteScreenXY(entityNo, spriteIdx,     screenX, 0);
        NS_DrawSpriteScreenXY(entityNo, spriteIdx + 1, screenX, 0);
        spriteIdx += 2;

        speed += 0x2000;
    }

    e.value[1] = (e.value[1] + 1) & 511;
}

}
