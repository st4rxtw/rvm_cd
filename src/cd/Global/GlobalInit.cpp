#include "GlobalInit.h"
#include "../Title/TitleInit.h"
#include "../../rvm/ObjectSystem.h"

namespace cd {

void Explosion_Startup();
void Explosion_Draw(int32_t);

void SmokePuff_Startup();
void SmokePuff_Draw(int32_t);

void RingSparkle_Startup();
void RingSparkle_Main(int32_t);
void RingSparkle_Draw(int32_t);

void ObjectScore_Startup();
void ObjectScore_Draw(int32_t);

void FadeMusic_Main(int32_t);

void Invincibility_Startup();
void Invincibility_Draw(int32_t);

void RegisterGlobalScripts()
{
    using OSys = rvm::ObjectSystem;
    OSys::RegisterNative("Explosion",    Explosion_Startup,   nullptr,           Explosion_Draw);
    OSys::RegisterNative("Smoke Puff",   SmokePuff_Startup,   nullptr,           SmokePuff_Draw);
    OSys::RegisterNative("Ring Sparkle", RingSparkle_Startup, RingSparkle_Main,  RingSparkle_Draw);
    OSys::RegisterNative("Object Score", ObjectScore_Startup, nullptr,           ObjectScore_Draw);
    OSys::RegisterNative("FadeMusic",    nullptr,             FadeMusic_Main,    nullptr);
    OSys::RegisterNative("Invincibility", Invincibility_Startup, nullptr,        Invincibility_Draw);
}

void RegisterAllNativeScripts()
{
    RegisterGlobalScripts();
    RegisterTitleScripts();
}

}
