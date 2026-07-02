#include "TitleInit.h"
#include "../NativeScript.h"
#include <rvm/Log.h>

namespace cd {

void Sega_Startup();
void Sega_Main(int32_t);
void Sega_Draw(int32_t);

void CWLogo_Startup();
void CWLogo_Main(int32_t);
void CWLogo_Draw(int32_t);

void Sonic_Startup();
void Sonic_Draw(int32_t);

void Logo_Startup();
void Logo_Draw(int32_t);

void Background_Startup();
void Background_Draw(int32_t);

void Clouds_Startup();
void Clouds_Draw(int32_t);

void LiteRibbon_Startup();
void LiteRibbon_Draw(int32_t);

void Start_Startup();
void Start_Main(int32_t);
void Start_Draw(int32_t);

void RegisterTitleScripts()
{
    using OSys = rvm::ObjectSystem;
    OSys::RegisterNative("Sega",        Sega_Startup,        Sega_Main,       Sega_Draw);
    OSys::RegisterNative("CWLogo",      CWLogo_Startup,      CWLogo_Main,     CWLogo_Draw);
    OSys::RegisterNative("Sonic",       Sonic_Startup,       nullptr,         Sonic_Draw);
    OSys::RegisterNative("Logo",        Logo_Startup,        nullptr,         Logo_Draw);
    OSys::RegisterNative("Background",  Background_Startup,  nullptr,         Background_Draw);
    OSys::RegisterNative("Clouds",      Clouds_Startup,      nullptr,         Clouds_Draw);
    OSys::RegisterNative("Lite Ribbon", LiteRibbon_Startup,  nullptr,         LiteRibbon_Draw);
    OSys::RegisterNative("Touch Start", Start_Startup,       Start_Main,      Start_Draw);
    rvm::Log::Info("If you see this, uh we're running the Title from a C++ script and NOT the RetroEngine script!");
}

}
