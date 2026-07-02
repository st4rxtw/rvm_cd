#include "../NativeScript.h"

namespace cd {

enum {
    FADEMUSIC_TO_BOSS   = 0,
    FADEMUSIC_TO_LEVEL  = 1,
    FADEMUSIC_SSZEGGMAN = 2,
    FADEMUSIC_SSZAMY    = 3
};
enum { PRIORITY_ACTIVE = 1 };

void FadeMusic_Main(int32_t entityNo)
{
    ObjectEntity& e = ObjectSystem::objectEntityList[entityNo];

    switch (e.propertyValue) {
    case FADEMUSIC_TO_BOSS:
        if (e.value[0] < 100) {
            e.value[0]++;
            NS_SetMusicVolume(NS_MusicVolume() - 1);
        } else {
            NS_PlayMusic(4);
            e.type = NS_TypeID("Blank Object");
        }
        break;

    case FADEMUSIC_TO_LEVEL:
        if (e.value[0] < 100) {
            e.value[0]++;
            NS_SetMusicVolume(NS_MusicVolume() - 1);
        } else {
            NS_PlayMusic(0);
            e.type = NS_TypeID("Blank Object");
        }
        break;

    case FADEMUSIC_SSZEGGMAN:
        if (e.value[0] < 50) {
            e.value[0]++;
            NS_SetMusicVolume(NS_MusicVolume() - 2);
        } else {
            NS_PlayMusic(0);
            e.type = NS_TypeID("Blank Object");
        }
        break;

    case FADEMUSIC_SSZAMY:
        if (e.value[0] < 100) {
            e.value[0]++;
            NS_SetMusicVolume(NS_MusicVolume() - 1);
        } else {
            NS_PlayMusic(1);
            e.type = NS_TypeID("Blank Object");

            ObjectEntity& o30 = ObjectSystem::objectEntityList[30];
            o30.type      = NS_TypeID("ActFinish");
            o30.drawOrder = 6;
            o30.priority  = PRIORITY_ACTIVE;
        }
        break;
    }
}

}
