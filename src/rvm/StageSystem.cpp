#include "StageSystem.h"
#include "ObjectSystem.h"
#include "PlayerSystem.h"
#include "GraphicsSystem.h"
#include "AudioPlayback.h"
#include "AnimationSystem.h"
#include "TextSystem.h"
#include "InputSystem.h"
#include "RenderDevice.h"
#include "EngineCallbacks.h"
#include "FileIO.h"
#include "GlobalAppDefinitions.h"
#include "Scene3D.h"
#include "Log.h"
#include <cstring>
#include <cmath>

namespace rvm {

InputResult  StageSystem::gKeyDown       {};
InputResult  StageSystem::gKeyPress      {};
uint8_t      StageSystem::stageMode      = 0;
uint8_t      StageSystem::pauseEnabled   = 0;
int32_t      StageSystem::stageListPosition = 0;
Mappings128x128  StageSystem::tile128x128   {};
LayoutMap    StageSystem::stageLayouts[9] {};
uint8_t      StageSystem::tLayerMidPoint = 0;
uint8_t      StageSystem::activeTileLayers[4]{};
CollisionMask16x16 StageSystem::tileCollisions[2]{};
LineScrollParallax StageSystem::hParallax{};
LineScrollParallax StageSystem::vParallax{};
int32_t      StageSystem::lastXSize      = 0;
int32_t      StageSystem::lastYSize      = 0;
int32_t      StageSystem::bgDeformationData0[576]{};
int32_t      StageSystem::bgDeformationData1[576]{};
int32_t      StageSystem::bgDeformationData2[576]{};
int32_t      StageSystem::bgDeformationData3[576]{};
int32_t      StageSystem::xBoundary1     = 0;
int32_t      StageSystem::xBoundary2     = 0;
int32_t      StageSystem::yBoundary1     = 0;
int32_t      StageSystem::yBoundary2     = 0;
int32_t      StageSystem::newXBoundary1  = 0;
int32_t      StageSystem::newXBoundary2  = 0;
int32_t      StageSystem::newYBoundary1  = 0;
int32_t      StageSystem::newYBoundary2  = 0;
uint8_t      StageSystem::cameraEnabled  = 0;
int8_t       StageSystem::cameraTarget   = 0;
uint8_t      StageSystem::cameraShift    = 0;
uint8_t      StageSystem::cameraStyle    = 0;
int32_t      StageSystem::cameraAdjustY  = 0;
int32_t      StageSystem::xScrollOffset  = 0;
int32_t      StageSystem::yScrollOffset  = 0;
int32_t      StageSystem::yScrollA       = 0;
int32_t      StageSystem::yScrollB       = 240;
int32_t      StageSystem::xScrollA       = 0;
int32_t      StageSystem::xScrollB       = 320;
int32_t      StageSystem::xScrollMove    = 0;
int32_t      StageSystem::yScrollMove    = 0;
int32_t      StageSystem::screenShakeX   = 0;
int32_t      StageSystem::screenShakeY   = 0;
int32_t      StageSystem::waterLevel     = 0;
char         StageSystem::titleCardText[24]{};
char         StageSystem::titleCardWord2 = 0;
TextMenu     StageSystem::gameMenu[2]    {};
uint8_t      StageSystem::timeEnabled    = 0;
uint8_t      StageSystem::milliSeconds   = 0;
uint8_t      StageSystem::seconds        = 0;
uint8_t      StageSystem::minutes        = 0;
uint8_t      StageSystem::debugMode      = 0;

void StageSystem::ProcessStage()
{
    switch (stageMode) {
    case 0: {
        AudioPlayback::StopMusic();
        GraphicsSystem::fadeMode   = 0;
        GraphicsSystem::paletteMode = 0;
        GraphicsSystem::SetActivePalette(0, 0, 256);
        cameraEnabled = 1; cameraTarget = -1; cameraAdjustY = 0;
        xScrollOffset = yScrollOffset = yScrollA = xScrollA = xScrollMove = yScrollMove = 0;
        yScrollB = 240; xScrollB = 320;
        screenShakeX = screenShakeY = 0;
        Scene3D::numVertices = Scene3D::numFaces = 0;
        for (int i = 0; i < 2; ++i) {
            auto& p = PlayerSystem::playerList[i];
            p.xPos=p.yPos=p.xVelocity=p.yVelocity=p.angle=p.speed=0;
            p.visible=1; p.collisionPlane=0; p.collisionMode=0; p.gravity=1; p.tileCollisions=1; p.objectInteraction=1;
            for(int v=0;v<8;++v)p.value[v]=0;
        }
        pauseEnabled=0; timeEnabled=0; milliSeconds=0; seconds=0; minutes=0;
        GlobalAppDefinitions::frameCounter=0;
        ResetBackgroundSettings();
        LoadStageFiles();
        GraphicsSystem::texBufferMode=0;
        for(int i=0;i<9;++i) if(stageLayouts[i].type==4) GraphicsSystem::texBufferMode=1;
        for(int i=0;i<(int32_t)hParallax.numEntries;++i) if(hParallax.deformationEnabled[i]==1) GraphicsSystem::texBufferMode=1;
        if(GraphicsSystem::tileGfx[204802]>0) GraphicsSystem::texBufferMode=0;
        if(GraphicsSystem::texBufferMode==0){
            for(int i=0;i<4096;i+=4){
                GraphicsSystem::tileUVArray[i]   =(float)((i>>2&31)*16)*0.0009765625f;
                GraphicsSystem::tileUVArray[i+1] =(float)((i>>2>>5)*16)*0.0009765625f;
                GraphicsSystem::tileUVArray[i+2] =GraphicsSystem::tileUVArray[i]+1.f/64.f;
                GraphicsSystem::tileUVArray[i+3] =GraphicsSystem::tileUVArray[i+1]+1.f/64.f;
            }
        } else {
            for(int i=0;i<4096;i+=4){
                GraphicsSystem::tileUVArray[i]   =(float)((i>>2)%28*18+1)*0.0009765625f;
                GraphicsSystem::tileUVArray[i+1] =(float)((i>>2)/28*18+1)*0.0009765625f;
                GraphicsSystem::tileUVArray[i+2] =GraphicsSystem::tileUVArray[i]+1.f/64.f;
                GraphicsSystem::tileUVArray[i+3] =GraphicsSystem::tileUVArray[i+1]+1.f/64.f;
            }
            GraphicsSystem::tileUVArray[4092]=0.475585938f;
            GraphicsSystem::tileUVArray[4093]=0.475585938f;
            GraphicsSystem::tileUVArray[4094]=0.491210938f;
            GraphicsSystem::tileUVArray[4095]=0.491210938f;
        }
        RenderDevice::UpdateHardwareTextures();
        stageMode=1;
        GraphicsSystem::gfxIndexSize=GraphicsSystem::gfxVertexSize=0;
        GraphicsSystem::gfxIndexSizeOpaque=GraphicsSystem::gfxVertexSizeOpaque=0;
        break;
    }
    case 1: {
        if(GraphicsSystem::fadeMode>0) --GraphicsSystem::fadeMode;
        if(GraphicsSystem::paletteMode>0){GraphicsSystem::paletteMode=0;GraphicsSystem::texPaletteNum=0;}
        lastXSize=lastYSize=-1;
        InputSystem::CheckKeyDown(gKeyDown,255);
        InputSystem::CheckKeyPress(gKeyPress,255);
        if(pauseEnabled==1&&gKeyPress.start==1){stageMode=2;AudioPlayback::PauseSound();}
        if(timeEnabled==1){
            ++GlobalAppDefinitions::frameCounter;
            if(GlobalAppDefinitions::frameCounter==60){
                GlobalAppDefinitions::frameCounter=0;
                ++seconds;
                if(seconds>59){seconds=0;++minutes;if(minutes>59)minutes=0;}
            }
            milliSeconds=(uint8_t)((int32_t)GlobalAppDefinitions::frameCounter*100/60);
        }
        int32_t _preY0 = PlayerSystem::playerList[0].yPos;
        int32_t _preGrav0 = (int32_t)PlayerSystem::playerList[0].gravity;
        int32_t _preSpd0 = PlayerSystem::playerList[0].speed;
        ObjectSystem::ProcessObjects();
        {
            int32_t _dy = (PlayerSystem::playerList[0].yPos - _preY0) >> 16;
            if((_dy < -3 || _dy > 3) && _preGrav0 == 0) {
                auto& _p = PlayerSystem::playerList[0];
                Log::Info("YPOS_JUMP_GRND: dy=%d prevY=%d newY=%d grav=%d angle=%d spd=%d xV=%d yV=%d",
                    _dy, _preY0>>16, _p.yPos>>16,
                    (int)_p.gravity, _p.angle, _p.speed, _p.xVelocity, _p.yVelocity);
            }
        }
        {
            auto& _p = PlayerSystem::playerList[0];
            int32_t _ds = _p.speed - _preSpd0;
            if((_ds > 131072 || _ds < -131072) && _preGrav0 == 0) {
                Log::Info("SPEED_BURST: ds=%d prevSpd=%d newSpd=%d grav=%d angle=%d xV=%d yV=%d anim=%d",
                    _ds>>16, _preSpd0>>16, _p.speed>>16,
                    (int)_p.gravity, _p.angle, _p.xVelocity, _p.yVelocity,
                    (int)_p.objectPtr->animation);
            }
        }
        {
            static int s_pDiagFrame=0;
            if(++s_pDiagFrame>=60){s_pDiagFrame=0;
                auto& _p=PlayerSystem::playerList[0];
                Log::Info("PL0: xPos=%d yPos=%d spd=%d xV=%d yV=%d grav=%d angle=%d mode=%d",
                    _p.xPos>>16, _p.yPos>>16, _p.speed, _p.xVelocity, _p.yVelocity,
                    (int)_p.gravity, _p.angle, (int)_p.collisionMode);
                Log::Info("     left=%d right=%d up=%d down=%d jPress=%d jHold=%d tileCol=%d",
                    (int)_p.left,(int)_p.right,(int)_p.up,(int)_p.down,
                    (int)_p.jumpPress,(int)_p.jumpHold,(int)_p.tileCollisions);
            }
        }
        if(cameraTarget>(int8_t)-1){
            if(cameraEnabled==1){
                switch(cameraStyle){
                case 0: PlayerSystem::SetPlayerScreenPosition(PlayerSystem::playerList[(int32_t)cameraTarget]); break;
                default: PlayerSystem::SetPlayerScreenPositionCDStyle(PlayerSystem::playerList[(int32_t)cameraTarget]); break;
                case 4: PlayerSystem::SetPlayerHLockedScreenPosition(PlayerSystem::playerList[(int32_t)cameraTarget]); break;
                }
            } else PlayerSystem::SetPlayerLockedScreenPosition(PlayerSystem::playerList[(int32_t)cameraTarget]);
        }
        DrawStageGfx();
        if(GraphicsSystem::fadeMode>0) GraphicsSystem::DrawRectangle(0,0,GlobalAppDefinitions::SCREEN_XSIZE,240,(int32_t)GraphicsSystem::fadeR,(int32_t)GraphicsSystem::fadeG,(int32_t)GraphicsSystem::fadeB,(int32_t)GraphicsSystem::fadeA);
        if(stageMode==2) GlobalAppDefinitions::gameMode=8;
        break;
    }
    case 2: {
        if(GraphicsSystem::fadeMode>0) --GraphicsSystem::fadeMode;
        if(GraphicsSystem::paletteMode>0){GraphicsSystem::paletteMode=0;GraphicsSystem::texPaletteNum=0;}
        lastXSize=lastYSize=-1;
        InputSystem::CheckKeyDown(gKeyDown,255);
        InputSystem::CheckKeyPress(gKeyPress,255);
        GraphicsSystem::gfxIndexSize=GraphicsSystem::gfxVertexSize=0;
        GraphicsSystem::gfxIndexSizeOpaque=GraphicsSystem::gfxVertexSizeOpaque=0;
        ObjectSystem::ProcessPausedObjects();
        for(int i=0;i<7;++i) ObjectSystem::DrawObjectList(i);
        if(pauseEnabled==1&&gKeyPress.start==1){stageMode=1;AudioPlayback::ResumeSound();}
        break;
    }
    }
}

void StageSystem::LoadStageFiles()
{
    FileData fData{};
    uint8_t byteP[3]{};
    char charP[64]{};
    int32_t scriptNum=1;
    AudioPlayback::StopAllSFX();
    if(!FileIO::CheckCurrentStageFolder(stageListPosition)){
        AudioPlayback::ReleaseStageSFX();
        GraphicsSystem::LoadPalette("MasterPalette.act",0,0,0,256);
        ObjectSystem::ClearScriptData();
        for(int i=16;i>0;--i) GraphicsSystem::RemoveGraphicsFile("",i-1);
        if(FileIO::LoadStageFile("StageConfig.bin",stageListPosition,fData)){
            byteP[0]=FileIO::ReadByte();
            FileIO::CloseFile();
        }
        if(byteP[0]==1&&FileIO::LoadFile("Data/Game/GameConfig.bin",fData)){
            byteP[0]=FileIO::ReadByte();
            for(int i=0;i<(int32_t)byteP[0];++i) byteP[1]=FileIO::ReadByte();
            byteP[0]=FileIO::ReadByte();
            for(int i=0;i<(int32_t)byteP[0];++i) byteP[1]=FileIO::ReadByte();
            byteP[0]=FileIO::ReadByte();
            for(int i=0;i<(int32_t)byteP[0];++i) byteP[1]=FileIO::ReadByte();
            byteP[0]=FileIO::ReadByte();
            for(int i=0;i<(int32_t)byteP[0];++i){
                byteP[1]=FileIO::ReadByte();
                FileIO::ReadCharArray(charP,(int32_t)byteP[1]);
                charP[(int32_t)byteP[1]]='\0';
                ObjectSystem::SetObjectTypeName(charP,scriptNum+i);
            }
            if(FileIO::useByteCode){
                FileIO::GetFileInfo(fData); FileIO::CloseFile();
                ObjectSystem::LoadByteCodeFile(4,scriptNum);
                scriptNum+=(int32_t)byteP[0];
                FileIO::SetFileInfo(fData);
            }
            FileIO::CloseFile();
        }
        if(FileIO::LoadStageFile("StageConfig.bin",stageListPosition,fData)){
            byteP[0]=FileIO::ReadByte();
            for(int ep=96;ep<128;++ep){
                FileIO::ReadByteArray(byteP,3);
                GraphicsSystem::SetPaletteEntry(0,(uint8_t)ep,byteP[0],byteP[1],byteP[2]);
            }
            byteP[0]=FileIO::ReadByte();
            for(int i=0;i<(int32_t)byteP[0];++i){
                byteP[1]=FileIO::ReadByte();
                FileIO::ReadCharArray(charP,(int32_t)byteP[1]);
                charP[(int32_t)byteP[1]]='\0';
                ObjectSystem::SetObjectTypeName(charP,i+scriptNum);
            }
            if(FileIO::useByteCode){
                for(int i=0;i<(int32_t)byteP[0];++i){
                    byteP[1]=FileIO::ReadByte();
                    FileIO::ReadCharArray(charP,(int32_t)byteP[1]);
                    charP[(int32_t)byteP[1]]='\0';
                }
                FileIO::GetFileInfo(fData); FileIO::CloseFile();
                ObjectSystem::LoadByteCodeFile((int32_t)FileIO::activeStageList,scriptNum);
                FileIO::SetFileInfo(fData);
            }
            byteP[0]=FileIO::ReadByte();
            AudioPlayback::numStageSFX=(int32_t)byteP[0];
            for(int i=0;i<(int32_t)byteP[0];++i){
                byteP[1]=FileIO::ReadByte();
                FileIO::ReadCharArray(charP,(int32_t)byteP[1]);
                charP[(int32_t)byteP[1]]='\0';
                FileIO::GetFileInfo(fData); FileIO::CloseFile();
                AudioPlayback::LoadSfx(charP,i+AudioPlayback::numGlobalSFX);
                FileIO::SetFileInfo(fData);
            }
            FileIO::CloseFile();
        }
        GraphicsSystem::LoadStageGIFFile(stageListPosition);
        LoadStageCollisions();
        LoadStageBackground();
    }
    Load128x128Mappings();
    for(int t=0;t<16;++t) AudioPlayback::SetMusicTrack("",t,0,0);
    for(int i=0;i<1184;++i){
        auto& e=ObjectSystem::objectEntityList[i];
        e.type=0;e.direction=0;e.animation=0;e.prevAnimation=0;e.animationSpeed=0;e.animationTimer=0;
        e.frame=0;e.priority=0;e.rotation=0;e.state=0;e.propertyValue=0;e.xPos=0;e.yPos=0;
        e.drawOrder=3;e.scale=512;e.inkEffect=0;
        for(int v=0;v<8;++v)e.value[v]=0;
    }
    LoadActLayout();
    ObjectSystem::ProcessStartupScripts();
    {
        Log::Info("=== gfxSurface after ProcessStartupScripts ===");
        for(int _i=0;_i<24;++_i){
            if(FileIO::StringLength(GraphicsSystem::gfxSurface[_i].fileName)>0)
                Log::Info("  [%d] '%s' %dx%d dataStart=%u",
                    _i, GraphicsSystem::gfxSurface[_i].fileName,
                    GraphicsSystem::gfxSurface[_i].width, GraphicsSystem::gfxSurface[_i].height,
                    (unsigned)GraphicsSystem::gfxSurface[_i].dataStart);
        }
        Log::Info("=== scriptFramesNo=%d, objectScriptList (non-zero frames) ===",
            (int)ObjectSystem::scriptFramesNo);
        for(int _i=0;_i<256;++_i){
            auto& osl=ObjectSystem::objectScriptList[_i];
            if(osl.numFrames>0)
                Log::Info("  type[%d] surfaceNum=%d frameListOffset=%d numFrames=%d",
                    _i,(int)osl.surfaceNum,(int)osl.frameListOffset,(int)osl.numFrames);
        }
        Log::Info("=== First scriptFrames (up to 10) ===");
        int _fLimit=(int)ObjectSystem::scriptFramesNo;
        if(_fLimit>10)_fLimit=10;
        for(int _i=0;_i<_fLimit;++_i){
            auto& _f=ObjectSystem::scriptFrames[_i];
            Log::Info("  frame[%d] xPivot=%d yPivot=%d xSize=%d ySize=%d left=%d top=%d",
                _i,_f.xPivot,_f.yPivot,_f.xSize,_f.ySize,_f.left,_f.top);
        }
        Log::Info("=== graphicData pixel samples per surface ===");
        for(int _si=0;_si<24;++_si){
            if(FileIO::StringLength(GraphicsSystem::gfxSurface[_si].fileName)==0) continue;
            uint32_t _ds=GraphicsSystem::gfxSurface[_si].dataStart;
            int _w=GraphicsSystem::gfxSurface[_si].width;
            int _h=GraphicsSystem::gfxSurface[_si].height;
            Log::Info("  surf[%d] '%s' dataStart=%u w=%d h=%d, first 16 px:",
                _si,GraphicsSystem::gfxSurface[_si].fileName,(unsigned)_ds,_w,_h);
            int _nonzero=0;
            for(int _p=0;_p<16;++_p){
                int _v=(int)GraphicsSystem::graphicData[(int)_ds+_p];
                if(_v) ++_nonzero;
            }
            Log::Info("    px[0..15] nonzero count=%d (first=%d second=%d)",
                _nonzero,
                (int)GraphicsSystem::graphicData[(int)_ds+0],
                (int)GraphicsSystem::graphicData[(int)_ds+1]);
            if(_w>0 && _h>0){
                int _mid=(int)_ds+(_h/2)*_w+(_w/4);
                int _midv=(int)GraphicsSystem::graphicData[_mid];
                Log::Info("    midpoint px[%d]=%d", _mid, _midv);
            }
        }
        if(FileIO::StringLength(GraphicsSystem::gfxSurface[1].fileName)>0){
            uint32_t _ds=GraphicsSystem::gfxSurface[1].dataStart;
            int _w=GraphicsSystem::gfxSurface[1].width;
            Log::Info("=== Title.gif (surf[1]) row y=203, x=1..16 ===");
            int _base=(int)_ds+203*_w+1;
            int _nz=0;
            for(int _p=0;_p<16;++_p){ if(GraphicsSystem::graphicData[_base+_p]) ++_nz; }
            Log::Info("  nonzero=%d first=%d last=%d",
                _nz,
                (int)GraphicsSystem::graphicData[_base],
                (int)GraphicsSystem::graphicData[_base+15]);
        }
    }
    xScrollA=(PlayerSystem::playerList[0].xPos>>16)-160;
    xScrollB=xScrollA+320;
    yScrollA=(PlayerSystem::playerList[0].yPos>>16)-104;
    yScrollB=yScrollA+240;
    Log::Info("Stage loaded.");
}

void StageSystem::Load128x128Mappings()
{
    FileData fData{};
    uint8_t byteP[2]{};
    if(!FileIO::LoadStageFile("128x128Tiles.bin",stageListPosition,fData)) return;
    for(int i=0;i<32768;++i){
        FileIO::ReadByteArray(byteP,2);
        uint8_t b=byteP[0]-(byteP[0]>>6<<6);
        tile128x128.visualPlane[i]  =b>>4;
        b=b-(b>>4<<4);
        tile128x128.direction[i]    =b>>2;
        b=b-(b>>2<<2);
        tile128x128.tile16x16[i]    =(uint16_t)(((uint32_t)b<<8)+(uint32_t)byteP[1]);
        tile128x128.gfxDataPos[i]   =(int32_t)tile128x128.tile16x16[i]<<2;
        uint8_t cf=FileIO::ReadByte();
        tile128x128.collisionFlag[0][i]=cf>>4;
        tile128x128.collisionFlag[1][i]=cf-(cf>>4<<4);
    }
    FileIO::CloseFile();
}

void StageSystem::LoadStageCollisions()
{
    FileData fData{};
    if(!FileIO::LoadStageFile("CollisionMasks.bin",stageListPosition,fData)) return;
    int32_t base=0;
    for(int t=0;t<1024;++t){
        for(int cp=0;cp<2;++cp){
            uint8_t b0=FileIO::ReadByte();
            int32_t type=b0>>4;
            tileCollisions[cp].flags[t]=b0&15;
            uint8_t a0=FileIO::ReadByte(),a1=FileIO::ReadByte(),a2=FileIO::ReadByte(),a3=FileIO::ReadByte();
            tileCollisions[cp].angle[t]=(uint32_t)a0|((uint32_t)a1<<8)|((uint32_t)a2<<16)|((uint32_t)a3<<24);
            if(type==0){
                for(int j=0;j<16;j+=2){
                    uint8_t b=FileIO::ReadByte();
                    tileCollisions[cp].floorMask[base+j]  =(int8_t)(b>>4);
                    tileCollisions[cp].floorMask[base+j+1]=(int8_t)(b&15);
                }
                uint8_t hi=FileIO::ReadByte(),lo=FileIO::ReadByte();
                uint8_t m=1;
                for(int j=0;j<8;++j,m<<=1){
                    if(!(hi&m)){tileCollisions[cp].floorMask[base+j+8]=(int8_t)64;tileCollisions[cp].roofMask[base+j+8]=(int8_t)-64;}
                    else tileCollisions[cp].roofMask[base+j+8]=(int8_t)15;
                }
                m=1;
                for(int j=0;j<8;++j,m<<=1){
                    if(!(lo&m)){tileCollisions[cp].floorMask[base+j]=(int8_t)64;tileCollisions[cp].roofMask[base+j]=(int8_t)-64;}
                    else tileCollisions[cp].roofMask[base+j]=(int8_t)15;
                }
                for(uint8_t row=0;row<16;++row){
                    int32_t col=0;
                    while(col>-1){
                        if(col==16){tileCollisions[cp].leftWallMask[base+(int32_t)row]=(int8_t)64;col=-1;}
                        else if((int32_t)row>=(int32_t)tileCollisions[cp].floorMask[base+col]){tileCollisions[cp].leftWallMask[base+(int32_t)row]=(int8_t)col;col=-1;}
                        else ++col;
                    }
                }
                for(uint8_t row=0;row<16;++row){
                    int32_t col=15;
                    while(col<16){
                        if(col==-1){tileCollisions[cp].rightWallMask[base+(int32_t)row]=(int8_t)-64;col=16;}
                        else if((int32_t)row>=(int32_t)tileCollisions[cp].floorMask[base+col]){tileCollisions[cp].rightWallMask[base+(int32_t)row]=(int8_t)col;col=16;}
                        else --col;
                    }
                }
            } else {
                for(int j=0;j<16;j+=2){
                    uint8_t b=FileIO::ReadByte();
                    tileCollisions[cp].roofMask[base+j]  =(int8_t)(b>>4);
                    tileCollisions[cp].roofMask[base+j+1]=(int8_t)(b&15);
                }
                uint8_t hi=FileIO::ReadByte(),lo=FileIO::ReadByte();
                uint8_t m=1;
                for(int j=0;j<8;++j,m<<=1){
                    if(!(hi&m)){tileCollisions[cp].floorMask[base+j+8]=(int8_t)64;tileCollisions[cp].roofMask[base+j+8]=(int8_t)-64;}
                    else tileCollisions[cp].floorMask[base+j+8]=(int8_t)0;
                }
                m=1;
                for(int j=0;j<8;++j,m<<=1){
                    if(!(lo&m)){tileCollisions[cp].floorMask[base+j]=(int8_t)64;tileCollisions[cp].roofMask[base+j]=(int8_t)-64;}
                    else tileCollisions[cp].floorMask[base+j]=(int8_t)0;
                }
                for(uint8_t row=0;row<16;++row){
                    int32_t col=0;
                    while(col>-1){
                        if(col==16){tileCollisions[cp].leftWallMask[base+(int32_t)row]=(int8_t)64;col=-1;}
                        else if((int32_t)row<=(int32_t)tileCollisions[cp].roofMask[base+col]){tileCollisions[cp].leftWallMask[base+(int32_t)row]=(int8_t)col;col=-1;}
                        else ++col;
                    }
                }
                for(uint8_t row=0;row<16;++row){
                    int32_t col=15;
                    while(col<16){
                        if(col==-1){tileCollisions[cp].rightWallMask[base+(int32_t)row]=(int8_t)-64;col=16;}
                        else if((int32_t)row<=(int32_t)tileCollisions[cp].roofMask[base+col]){tileCollisions[cp].rightWallMask[base+(int32_t)row]=(int8_t)col;col=16;}
                        else --col;
                    }
                }
            }
        }
        base+=16;
    }
    FileIO::CloseFile();
}

void StageSystem::LoadActLayout()
{
    FileData fData{};
    if(!FileIO::LoadActFile(".bin",stageListPosition,fData)) return;
    uint8_t tcLen=FileIO::ReadByte();
    int32_t n=(int32_t)tcLen;
    titleCardWord2=(char)tcLen;
    int32_t idx=0;
    for(;idx<n;++idx){
        titleCardText[idx]=(char)FileIO::ReadByte();
        if(titleCardText[idx]=='-') titleCardWord2=(char)(idx+1);
    }
    titleCardText[idx]='\0';
    for(int i=0;i<4;++i) activeTileLayers[i]=FileIO::ReadByte();
    tLayerMidPoint=FileIO::ReadByte();
    stageLayouts[0].xSize=FileIO::ReadByte();
    stageLayouts[0].ySize=FileIO::ReadByte();
    xBoundary1=newXBoundary1=yBoundary1=newYBoundary1=0;
    xBoundary2=newXBoundary2=(int32_t)stageLayouts[0].xSize<<7;
    yBoundary2=newYBoundary2=(int32_t)stageLayouts[0].ySize<<7;
    waterLevel=yBoundary2+128;
    for(int i=0;i<65536;++i) stageLayouts[0].tileMap[i]=0;
    for(int y=0;y<(int32_t)stageLayouts[0].ySize;++y)
        for(int x=0;x<(int32_t)stageLayouts[0].xSize;++x){
            uint8_t hi=FileIO::ReadByte(),lo=FileIO::ReadByte();
            stageLayouts[0].tileMap[(y<<8)+x]=(uint16_t)((uint32_t)hi<<8|(uint32_t)lo);
        }
    int32_t nGroups=(int32_t)FileIO::ReadByte();
    for(int g=0;g<nGroups;++g){
        int32_t gc=(int32_t)FileIO::ReadByte();
        for(int i=0;i<gc;++i) FileIO::ReadByte();
    }
    int32_t nObjs=((int32_t)FileIO::ReadByte()<<8)+(int32_t)FileIO::ReadByte();
    int32_t ei=32;
    for(int i=0;i<nObjs;++i){
        auto& e=ObjectSystem::objectEntityList[ei];
        e.type=FileIO::ReadByte();
        e.propertyValue=FileIO::ReadByte();
        uint8_t xh=FileIO::ReadByte(),xl=FileIO::ReadByte();
        e.xPos=((int32_t)xh<<8|(int32_t)xl)<<16;
        uint8_t yh=FileIO::ReadByte(),yl=FileIO::ReadByte();
        e.yPos=((int32_t)yh<<8|(int32_t)yl)<<16;
        ++ei;
    }
    stageLayouts[0].type=1;
    FileIO::CloseFile();
}

void StageSystem::LoadStageBackground()
{
    FileData fData{};
    uint8_t na1[3]{},na2[2]{};
    for(int i=0;i<9;++i){stageLayouts[i].type=0;stageLayouts[i].deformationPos=0;stageLayouts[i].deformationPosW=0;}
    for(int i=0;i<256;++i){hParallax.scrollPosition[i]=0;vParallax.scrollPosition[i]=0;}
    for(int i=0;i<32768;++i) stageLayouts[0].lineScrollRef[i]=0;
    if(!FileIO::LoadStageFile("Backgrounds.bin",stageListPosition,fData)) return;
    uint8_t nBG=FileIO::ReadByte();
    hParallax.numEntries=FileIO::ReadByte();
    for(int i=0;i<(int32_t)hParallax.numEntries;++i){
        uint8_t pf0=FileIO::ReadByte(),pf1=FileIO::ReadByte();
        hParallax.parallaxFactor[i]=((int32_t)pf0<<8)+(int32_t)pf1;
        hParallax.scrollSpeed[i]=(int32_t)FileIO::ReadByte()<<10;
        hParallax.scrollPosition[i]=0;
        hParallax.deformationEnabled[i]=FileIO::ReadByte();
    }
    vParallax.numEntries=FileIO::ReadByte();
    for(int i=0;i<(int32_t)vParallax.numEntries;++i){
        uint8_t pf0=FileIO::ReadByte(),pf1=FileIO::ReadByte();
        vParallax.parallaxFactor[i]=((int32_t)pf0<<8)+(int32_t)pf1;
        vParallax.scrollSpeed[i]=(int32_t)FileIO::ReadByte()<<10;
        vParallax.scrollPosition[i]=0;
        vParallax.deformationEnabled[i]=FileIO::ReadByte();
    }
    for(int li=1;li<=(int32_t)nBG;++li){
        stageLayouts[li].xSize=FileIO::ReadByte();
        stageLayouts[li].ySize=FileIO::ReadByte();
        stageLayouts[li].type=FileIO::ReadByte();
        uint8_t pf0=FileIO::ReadByte(),pf1=FileIO::ReadByte();
        stageLayouts[li].parallaxFactor=((int32_t)pf0<<8)+(int32_t)pf1;
        stageLayouts[li].scrollSpeed=(int32_t)FileIO::ReadByte()<<10;
        stageLayouts[li].scrollPosition=0;
        for(int i=0;i<65536;++i) stageLayouts[li].tileMap[i]=0;
        for(int i=0;i<32768;++i) stageLayouts[li].lineScrollRef[i]=0;
        int32_t si=0;
        bool done=false;
        while(!done){
            uint8_t b=FileIO::ReadByte();
            if(b==255){
                uint8_t b2=FileIO::ReadByte();
                if(b2==255){done=true;}
                else{uint8_t cnt=(uint8_t)(FileIO::ReadByte()-1); for(int c=0;c<(int32_t)cnt;++c)stageLayouts[li].lineScrollRef[si++]=b2;}
            } else {
                stageLayouts[li].lineScrollRef[si++]=b;
            }
        }
        for(int y=0;y<(int32_t)stageLayouts[li].ySize;++y)
            for(int x=0;x<(int32_t)stageLayouts[li].xSize;++x){
                uint8_t hi=FileIO::ReadByte(),lo=FileIO::ReadByte();
                stageLayouts[li].tileMap[(y<<8)+x]=(uint16_t)((uint32_t)hi<<8|(uint32_t)lo);
            }
    }
    FileIO::CloseFile();
}

void StageSystem::ResetBackgroundSettings()
{
    for(int i=0;i<9;++i){stageLayouts[i].deformationPos=0;stageLayouts[i].deformationPosW=0;stageLayouts[i].scrollPosition=0;}
    for(int i=0;i<256;++i){hParallax.scrollPosition[i]=0;vParallax.scrollPosition[i]=0;}
    for(int i=0;i<576;++i){bgDeformationData0[i]=0;bgDeformationData1[i]=0;bgDeformationData2[i]=0;bgDeformationData3[i]=0;}
}

void StageSystem::SetLayerDeformation(int32_t sel,int32_t wLen,int32_t wWid,int32_t wType,int32_t yPos,int32_t wSize)
{
    int32_t* arr=sel==0?bgDeformationData0:sel==1?bgDeformationData1:sel==2?bgDeformationData2:bgDeformationData3;
    int32_t idx=0;
    switch(wType){
    case 0: for(int i=0;i<131072;i+=512){arr[idx++]=GlobalAppDefinitions::SinValue512[i/wLen&511]*wWid>>5;} break;
    case 1: idx=yPos; for(int i=0;i<wSize;++i){arr[idx++]=GlobalAppDefinitions::SinValue512[(i<<9)/wLen&511]*wWid>>5;} break;
    }
    for(int i=256;i<576;++i) arr[i]=arr[i-256];
}

static int s_gfxFrameCount = 0;
void StageSystem::DrawStageGfx()
{
    ++s_gfxFrameCount;
    GraphicsSystem::gfxVertexSize=0;
    GraphicsSystem::gfxIndexSize=0;
    GraphicsSystem::render3DEnabled=false;
    GraphicsSystem::vertexSize3D=0;
    GraphicsSystem::indexSize3D=0;
    GraphicsSystem::waterDrawPos=waterLevel-yScrollOffset;
    if(GraphicsSystem::waterDrawPos<-16) GraphicsSystem::waterDrawPos=-16;
    if(GraphicsSystem::waterDrawPos>=240) GraphicsSystem::waterDrawPos=256;
    if(s_gfxFrameCount<=3){
        Log::Info("DrawStageGfx: activeTileLayers=[%d,%d,%d,%d] tLayerMidPoint=%d",
            (int)activeTileLayers[0],(int)activeTileLayers[1],(int)activeTileLayers[2],(int)activeTileLayers[3],
            (int)tLayerMidPoint);
        Log::Info("  stageLayouts[0]: type=%d xSize=%d ySize=%d",
            (int)stageLayouts[0].type,(int)stageLayouts[0].xSize,(int)stageLayouts[0].ySize);
        int nonzero=0;
        for(int i=0;i<(int)stageLayouts[0].xSize*(int)stageLayouts[0].ySize;++i)
            if(stageLayouts[0].tileMap[i]) ++nonzero;
        Log::Info("  tileMap nonzero entries: %d",nonzero);
        int nzGfx=0;
        for(int i=0;i<32768;++i) if(tile128x128.gfxDataPos[i]>0) ++nzGfx;
        Log::Info("  tile128x128 gfxDataPos>0: %d",nzGfx);
    }
    ObjectSystem::DrawObjectList(0);
    if(activeTileLayers[0]<9) switch(stageLayouts[(int32_t)activeTileLayers[0]].type){case 1:DrawHLineScrollLayer8(0);break;case 3:case 4:Draw3DFloorLayer(0);break;}
    GraphicsSystem::gfxIndexSizeOpaque=GraphicsSystem::gfxIndexSize;
    GraphicsSystem::gfxVertexSizeOpaque=GraphicsSystem::gfxVertexSize;
    ObjectSystem::DrawObjectList(1);
    if(activeTileLayers[1]<9) switch(stageLayouts[(int32_t)activeTileLayers[1]].type){case 1:DrawHLineScrollLayer8(1);break;case 3:case 4:Draw3DFloorLayer(1);break;}
    ObjectSystem::DrawObjectList(2);
    if(activeTileLayers[2]<9) switch(stageLayouts[(int32_t)activeTileLayers[2]].type){case 1:DrawHLineScrollLayer8(2);break;case 3:case 4:Draw3DFloorLayer(2);break;}
    ObjectSystem::DrawObjectList(3);
    ObjectSystem::DrawObjectList(4);
    if(activeTileLayers[3]<9) switch(stageLayouts[(int32_t)activeTileLayers[3]].type){case 1:DrawHLineScrollLayer8(3);break;case 3:case 4:Draw3DFloorLayer(3);break;}
    ObjectSystem::DrawObjectList(5);
    ObjectSystem::DrawObjectList(6);
}

static inline void emitQuad(int32_t x0,int32_t y0,int32_t x1,int32_t y1,int32_t x2,int32_t y2,int32_t x3,int32_t y3,float u0,float v0,float u1,float v1,float u2,float v2,float u3,float v3)
{
    auto* g=GraphicsSystem::gfxPolyList;
    uint16_t& vs=GraphicsSystem::gfxVertexSize;
    uint16_t& is=GraphicsSystem::gfxIndexSize;
    g[vs].x=(float)x0;g[vs].y=(float)y0;g[vs].u=u0;g[vs].v=v0;g[vs].color.r=g[vs].color.g=g[vs].color.b=g[vs].color.a=255;++vs;
    g[vs].x=(float)x1;g[vs].y=(float)y1;g[vs].u=u1;g[vs].v=v1;g[vs].color.r=g[vs].color.g=g[vs].color.b=g[vs].color.a=255;++vs;
    g[vs].x=(float)x2;g[vs].y=(float)y2;g[vs].u=u2;g[vs].v=v2;g[vs].color.r=g[vs].color.g=g[vs].color.b=g[vs].color.a=255;++vs;
    g[vs].x=(float)x3;g[vs].y=(float)y3;g[vs].u=u3;g[vs].v=v3;g[vs].color.r=g[vs].color.g=g[vs].color.b=g[vs].color.a=255;++vs;
    is+=2;
}

static inline void emitQuad3D(int32_t x0,int32_t z0,int32_t x1,int32_t z1,int32_t x2,int32_t z2,int32_t x3,int32_t z3,float u0,float v0,float u1,float v1,float u2,float v2,float u3,float v3)
{
    auto* g=GraphicsSystem::polyList3D;
    uint16_t& vs=GraphicsSystem::vertexSize3D;
    uint16_t& is=GraphicsSystem::indexSize3D;
    g[vs].x=(float)x0;g[vs].y=0;g[vs].z=(float)z0;g[vs].u=u0;g[vs].v=v0;g[vs].color.r=g[vs].color.g=g[vs].color.b=g[vs].color.a=255;++vs;
    g[vs].x=(float)x1;g[vs].y=0;g[vs].z=(float)z1;g[vs].u=u1;g[vs].v=v1;g[vs].color.r=g[vs].color.g=g[vs].color.b=g[vs].color.a=255;++vs;
    g[vs].x=(float)x2;g[vs].y=0;g[vs].z=(float)z2;g[vs].u=u2;g[vs].v=v2;g[vs].color.r=g[vs].color.g=g[vs].color.b=g[vs].color.a=255;++vs;
    g[vs].x=(float)x3;g[vs].y=0;g[vs].z=(float)z3;g[vs].u=u3;g[vs].v=v3;g[vs].color.r=g[vs].color.g=g[vs].color.b=g[vs].color.a=255;++vs;
    is+=2;
}

void StageSystem::DrawHLineScrollLayer8(uint8_t layerNum)
{
    uint8_t layIdx=(uint8_t)activeTileLayers[(int32_t)layerNum];
    auto& lay=stageLayouts[(int32_t)layIdx];
    int32_t mapW=(int32_t)lay.xSize;
    int32_t mapH=(int32_t)lay.ySize;
    int32_t tilesPerRow=(GlobalAppDefinitions::SCREEN_XSIZE>>4)+3;
    uint8_t vPlane=(layerNum<tLayerMidPoint)?0:1;

    ushort* tileMap;
    uint8_t* lineRef;
    int32_t defPos0,defPos1;
    int32_t* defArr0; int32_t* defArr1;
    int32_t scrollY;

    if(layIdx==0){
        tileMap=stageLayouts[0].tileMap;
        lineRef=stageLayouts[0].lineScrollRef;
        lastXSize=mapW;
        hParallax.linePos[0]=xScrollOffset;
        defPos0=(stageLayouts[0].deformationPos+yScrollOffset)&255;
        defPos1=(stageLayouts[0].deformationPosW+yScrollOffset)&255;
        defArr0=bgDeformationData0; defArr1=bgDeformationData1;
        scrollY=yScrollOffset%(mapH<<7);
    } else {
        tileMap=lay.tileMap;
        int32_t vscroll=lay.parallaxFactor*yScrollOffset>>8;
        int32_t totH=mapH<<7;
        lay.scrollPosition+=lay.scrollSpeed;
        if(lay.scrollPosition>totH<<16) lay.scrollPosition-=totH<<16;
        scrollY=(vscroll+(lay.scrollPosition>>16))%totH;
        mapH=totH>>7;
        lineRef=lay.lineScrollRef;
        defPos0=(lay.deformationPos+scrollY)&255;
        defPos1=(lay.deformationPosW+scrollY)&255;
        defArr0=bgDeformationData2; defArr1=bgDeformationData3;
    }

    if(lay.type==1&&lastXSize!=mapW){
        int32_t totW=mapW<<7;
        for(int i=0;i<(int32_t)hParallax.numEntries;++i){
            hParallax.linePos[i]=hParallax.parallaxFactor[i]*xScrollOffset>>8;
            hParallax.scrollPosition[i]+=hParallax.scrollSpeed[i];
            if(hParallax.scrollPosition[i]>totW<<16) hParallax.scrollPosition[i]-=totW<<16;
            hParallax.linePos[i]+=hParallax.scrollPosition[i]>>16;
            hParallax.linePos[i]%=totW;
        }
        mapW=totW>>7;
    }
    lastXSize=mapW;

    if(scrollY<0) scrollY+=mapH<<7;
    int32_t sy16=scrollY>>4<<4;
    int32_t lrIdx=sy16;
    int32_t d0Idx=defPos0+(sy16-scrollY);
    int32_t d1Idx=defPos1+(sy16-scrollY);
    if(d0Idx<0)d0Idx+=256; if(d1Idx<0)d1Idx+=256;
    int32_t screenY=-(scrollY&15);
    int32_t tileRow=scrollY>>7;
    int32_t tileSubRow=(scrollY&127)>>4;
    int32_t nRows=(screenY!=0)?272:256;
    GraphicsSystem::waterDrawPos<<=4;
    int32_t sy=screenY<<4;

    for(int32_t rows=nRows;rows>0;rows-=16){
        int32_t linePos0=hParallax.linePos[(int32_t)lineRef[lrIdx]]-16;
        int32_t midRef=lrIdx+8;
        bool splitRow;
        if(linePos0==hParallax.linePos[(int32_t)lineRef[midRef]]-16){
            if(hParallax.deformationEnabled[(int32_t)lineRef[midRef]]==1){
                int32_t da=sy<GraphicsSystem::waterDrawPos?defArr0[d0Idx]:defArr1[d1Idx];
                int32_t db=(sy+64<=GraphicsSystem::waterDrawPos)?defArr0[d0Idx+8]:defArr1[d1Idx+8];
                splitRow=(da!=db);
                d0Idx-=8; d1Idx-=8;
            } else splitRow=false;
        } else splitRow=true;

        int32_t rowRef=lrIdx;
        if(splitRow){
            int32_t rowW=mapW<<7;
            if(linePos0<0)linePos0+=rowW; if(linePos0>=rowW)linePos0-=rowW;
            int32_t col=linePos0>>7;
            int32_t colSub=(linePos0&127)>>4;
            int32_t px0=-((linePos0&15)<<4)-256;
            int32_t px1=px0;
            if(hParallax.deformationEnabled[(int32_t)lineRef[rowRef]]==1){
                if(sy>=GraphicsSystem::waterDrawPos)px0-=defArr1[d1Idx];
                else px0-=defArr0[d0Idx];
                if(sy+64>GraphicsSystem::waterDrawPos)px1-=defArr1[d1Idx+8];
                else px1-=defArr0[d0Idx+8];
            }
            d0Idx+=8; d1Idx+=8;
            lrIdx=midRef+8;
            int32_t tIdx=((col<=-1||tileRow<=-1)?0:((int32_t)tileMap[col+(tileRow<<8)]<<6))+(colSub+(tileSubRow<<3));
            for(int32_t tx=tilesPerRow;tx>0;--tx){
                if((int32_t)tile128x128.visualPlane[tIdx]==(int32_t)vPlane&&tile128x128.gfxDataPos[tIdx]>0){
                    int32_t gp=tile128x128.gfxDataPos[tIdx];
                    float* uv=GraphicsSystem::tileUVArray;
                    switch(tile128x128.direction[tIdx]){
                    case 0: emitQuad(px0,sy,px0+256,sy,px1,sy+128,px1+256,sy+128,uv[gp],uv[gp+1],uv[gp+2],uv[gp+1],uv[gp],uv[gp+3]-1.f/128.f,uv[gp+2],uv[gp+3]-1.f/128.f); break;
                    case 1: emitQuad(px0+256,sy,px0,sy,px1+256,sy+128,px1,sy+128,uv[gp],uv[gp+1],uv[gp+2],uv[gp+1],uv[gp],uv[gp+3]-1.f/128.f,uv[gp+2],uv[gp+3]-1.f/128.f); break;
                    case 2: emitQuad(px1,sy+128,px1+256,sy+128,px0,sy,px0+256,sy,uv[gp],uv[gp+1]+1.f/128.f,uv[gp+2],uv[gp+1]+1.f/128.f,uv[gp],uv[gp+3],uv[gp+2],uv[gp+3]); break;
                    case 3: emitQuad(px1+256,sy+128,px1,sy+128,px0+256,sy,px0,sy,uv[gp],uv[gp+1]+1.f/128.f,uv[gp+2],uv[gp+1]+1.f/128.f,uv[gp],uv[gp+3],uv[gp+2],uv[gp+3]); break;
                    }
                }
                px0+=256; px1+=256; ++colSub;
                if(colSub>7){++col;if(col==mapW)col=0;colSub=0;tIdx=((int32_t)tileMap[col+(tileRow<<8)]<<6)+(colSub+(tileSubRow<<3));}
                else ++tIdx;
            }
            int32_t nextSy=sy+128;
            int32_t nextRef=lrIdx-8;
            int32_t linePos1=hParallax.linePos[(int32_t)lineRef[nextRef]]-16;
            rowW=mapW<<7;
            if(linePos1<0)linePos1+=rowW; if(linePos1>=rowW)linePos1-=rowW;
            int32_t col2=linePos1>>7; int32_t colSub2=(linePos1&127)>>4;
            int32_t px2=-((linePos1&15)<<4)-256; int32_t px3=px2;
            if(hParallax.deformationEnabled[(int32_t)lineRef[nextRef]]==1){
                if(nextSy>=GraphicsSystem::waterDrawPos)px2-=defArr1[d1Idx];
                else px2-=defArr0[d0Idx];
                d0Idx+=8; d1Idx+=8;
                if(nextSy+64>GraphicsSystem::waterDrawPos)px3-=defArr1[d1Idx];
                else px3-=defArr0[d0Idx];
            } else {d0Idx+=8;d1Idx+=8;}
            int32_t tIdx2=((col2<=-1||tileRow<=-1)?0:((int32_t)tileMap[col2+(tileRow<<8)]<<6))+(colSub2+(tileSubRow<<3));
            for(int32_t tx=tilesPerRow;tx>0;--tx){
                if((int32_t)tile128x128.visualPlane[tIdx2]==(int32_t)vPlane&&tile128x128.gfxDataPos[tIdx2]>0){
                    int32_t gp=tile128x128.gfxDataPos[tIdx2];
                    float* uv=GraphicsSystem::tileUVArray;
                    switch(tile128x128.direction[tIdx2]){
                    case 0: emitQuad(px2,nextSy,px2+256,nextSy,px3,nextSy+128,px3+256,nextSy+128,uv[gp],uv[gp+1]+1.f/128.f,uv[gp+2],uv[gp+1]+1.f/128.f,uv[gp],uv[gp+3],uv[gp+2],uv[gp+3]); break;
                    case 1: emitQuad(px2+256,nextSy,px2,nextSy,px3+256,nextSy+128,px3,nextSy+128,uv[gp],uv[gp+1]+1.f/128.f,uv[gp+2],uv[gp+1]+1.f/128.f,uv[gp],uv[gp+3],uv[gp+2],uv[gp+3]); break;
                    case 2: emitQuad(px3,nextSy+128,px3+256,nextSy+128,px2,nextSy,px2+256,nextSy,uv[gp],uv[gp+1]-1.f/128.f,uv[gp+2],uv[gp+1]-1.f/128.f,uv[gp],uv[gp+3],uv[gp+2],uv[gp+3]); break;
                    case 3: emitQuad(px3+256,nextSy+128,px3,nextSy+128,px2+256,nextSy,px2,nextSy,uv[gp],uv[gp+1]-1.f/128.f,uv[gp+2],uv[gp+1]-1.f/128.f,uv[gp],uv[gp+3],uv[gp+2],uv[gp+3]); break;
                    }
                }
                px2+=256;px3+=256;++colSub2;
                if(colSub2>7){++col2;if(col2==mapW)col2=0;colSub2=0;tIdx2=((int32_t)tileMap[col2+(tileRow<<8)]<<6)+(colSub2+(tileSubRow<<3));}
                else ++tIdx2;
            }
            sy+=256;
        } else {
            int32_t rowW=mapW<<7;
            if(linePos0<0)linePos0+=rowW; if(linePos0>=rowW)linePos0-=rowW;
            int32_t col=linePos0>>7; int32_t colSub=(linePos0&127)>>4;
            int32_t px=-((linePos0&15)<<4)-256; int32_t px2=px;
            if(hParallax.deformationEnabled[(int32_t)lineRef[rowRef]]==1){
                if(sy>=GraphicsSystem::waterDrawPos)px-=defArr1[d1Idx];
                else px-=defArr0[d0Idx];
                d0Idx+=16; d1Idx+=16;
                if(sy+128>GraphicsSystem::waterDrawPos)px2-=defArr1[d1Idx];
                else px2-=defArr0[d0Idx];
            } else {d0Idx+=16;d1Idx+=16;}
            lrIdx=rowRef+16;
            int32_t tIdx=((col<=-1||tileRow<=-1)?0:((int32_t)tileMap[col+(tileRow<<8)]<<6))+(colSub+(tileSubRow<<3));
            for(int32_t tx=tilesPerRow;tx>0;--tx){
                if((int32_t)tile128x128.visualPlane[tIdx]==(int32_t)vPlane&&tile128x128.gfxDataPos[tIdx]>0){
                    int32_t gp=tile128x128.gfxDataPos[tIdx];
                    float* uv=GraphicsSystem::tileUVArray;
                    switch(tile128x128.direction[tIdx]){
                    case 0: emitQuad(px,sy,px+256,sy,px2,sy+256,px2+256,sy+256,uv[gp],uv[gp+1],uv[gp+2],uv[gp+1],uv[gp],uv[gp+3],uv[gp+2],uv[gp+3]); break;
                    case 1: emitQuad(px+256,sy,px,sy,px2+256,sy+256,px2,sy+256,uv[gp],uv[gp+1],uv[gp+2],uv[gp+1],uv[gp],uv[gp+3],uv[gp+2],uv[gp+3]); break;
                    case 2: emitQuad(px2,sy+256,px2+256,sy+256,px,sy,px+256,sy,uv[gp],uv[gp+1],uv[gp+2],uv[gp+1],uv[gp],uv[gp+3],uv[gp+2],uv[gp+3]); break;
                    case 3: emitQuad(px2+256,sy+256,px2,sy+256,px+256,sy,px,sy,uv[gp],uv[gp+1],uv[gp+2],uv[gp+1],uv[gp],uv[gp+3],uv[gp+2],uv[gp+3]); break;
                    }
                }
                px+=256;px2+=256;++colSub;
                if(colSub>7){++col;if(col==mapW)col=0;colSub=0;tIdx=((int32_t)tileMap[col+(tileRow<<8)]<<6)+(colSub+(tileSubRow<<3));}
                else ++tIdx;
            }
            sy+=256;
        }
        ++tileSubRow;
        if(tileSubRow>7){++tileRow;if(tileRow==mapH){tileRow=0;lrIdx-=mapH<<7;} tileSubRow=0;}
    }
    GraphicsSystem::waterDrawPos>>=4;
}

void StageSystem::Draw3DFloorLayer(uint8_t layerNum)
{
    int32_t layIdx=(int32_t)activeTileLayers[(int32_t)layerNum];
    auto& lay=stageLayouts[layIdx];
    int32_t mapW=(int32_t)lay.xSize<<7;
    int32_t mapH=(int32_t)lay.ySize<<7;
    auto* tileMap=lay.tileMap;

    GraphicsSystem::vertexSize3D=0; GraphicsSystem::indexSize3D=0;
    {
        auto& g=GraphicsSystem::polyList3D; uint16_t& vs=GraphicsSystem::vertexSize3D; uint16_t& is=GraphicsSystem::indexSize3D;
        g[vs].x=0;g[vs].y=0;g[vs].z=0;g[vs].u=0.5f;g[vs].v=0;g[vs].color.r=g[vs].color.g=g[vs].color.b=g[vs].color.a=255;++vs;
        g[vs].x=4096;g[vs].y=0;g[vs].z=0;g[vs].u=1;g[vs].v=0;g[vs].color.r=g[vs].color.g=g[vs].color.b=g[vs].color.a=255;++vs;
        g[vs].x=0;g[vs].y=0;g[vs].z=4096;g[vs].u=0.5f;g[vs].v=0.5f;g[vs].color.r=g[vs].color.g=g[vs].color.b=g[vs].color.a=255;++vs;
        g[vs].x=4096;g[vs].y=0;g[vs].z=4096;g[vs].u=1;g[vs].v=0.5f;g[vs].color.r=g[vs].color.g=g[vs].color.b=g[vs].color.a=255;++vs;
        is+=2;
    }

    bool hq=GlobalAppDefinitions::HQ3DFloorEnabled;
    int32_t startX,startZ,gridSize,gridCount;
    if(!hq){
        startX=(lay.xPos>>16)-160+(GlobalAppDefinitions::SinValue512[lay.angle]/3>>4<<4);
        startZ=(lay.zPos>>16)-160+(GlobalAppDefinitions::CosValue512[lay.angle]/3>>4<<4);
        gridSize=16; gridCount=20;
    } else {
        startX=(lay.xPos>>16)-256+(GlobalAppDefinitions::SinValue512[lay.angle]>>1>>4<<4);
        startZ=(lay.zPos>>16)-256+(GlobalAppDefinitions::CosValue512[lay.angle]>>1>>4<<4);
        gridSize=16; gridCount=32;
    }

    for(int32_t row=gridCount;row>0;--row){
        int32_t tx=startX;
        for(int32_t col=gridCount;col>0;--col){
            if(tx>-1&&tx<mapW&&startZ>-1&&startZ<mapH){
                int32_t mx=tx>>7,mz=startZ>>7;
                int32_t sx=(tx&127)>>4,sz=(startZ&127)>>4;
                int32_t tidx=((int32_t)tileMap[mx+(mz<<8)]<<6)+(sx+(sz<<3));
                if(tile128x128.gfxDataPos[tidx]>0){
                    int32_t gp=tile128x128.gfxDataPos[tidx];
                    float* uv=GraphicsSystem::tileUVArray;
                    switch(tile128x128.direction[tidx]){
                    case 0: emitQuad3D(tx,startZ,tx+gridSize,startZ,tx,startZ+gridSize,tx+gridSize,startZ+gridSize,uv[gp],uv[gp+1],uv[gp+2],uv[gp+1],uv[gp],uv[gp+3],uv[gp+2],uv[gp+3]); break;
                    case 1: emitQuad3D(tx+gridSize,startZ,tx,startZ,tx+gridSize,startZ+gridSize,tx,startZ+gridSize,uv[gp],uv[gp+1],uv[gp+2],uv[gp+1],uv[gp],uv[gp+3],uv[gp+2],uv[gp+3]); break;
                    case 2: emitQuad3D(tx,startZ+gridSize,tx+gridSize,startZ+gridSize,tx,startZ,tx+gridSize,startZ,uv[gp],uv[gp+1],uv[gp+2],uv[gp+1],uv[gp],uv[gp+3],uv[gp+2],uv[gp+3]); break;
                    case 3: emitQuad3D(tx+gridSize,startZ+gridSize,tx,startZ+gridSize,tx+gridSize,startZ,tx,startZ,uv[gp],uv[gp+1],uv[gp+2],uv[gp+1],uv[gp],uv[gp+3],uv[gp+2],uv[gp+3]); break;
                    }
                }
            }
            tx+=gridSize;
        }
        startX-=hq?512:320;
        startZ+=gridSize;
    }

    GraphicsSystem::floor3DPosX=(float)(lay.xPos>>8)*(-1.f/256.f);
    GraphicsSystem::floor3DPosY=(float)(lay.yPos>>8)*(1.f/256.f);
    GraphicsSystem::floor3DPosZ=(float)(lay.zPos>>8)*(-1.f/256.f);
    GraphicsSystem::floor3DAngle=(float)((double)lay.angle/512.0*-360.0);
    GraphicsSystem::render3DEnabled=true;
}

void StageSystem::InitFirstStage()
{
    xScrollOffset=yScrollOffset=0;
    AudioPlayback::StopMusic(); AudioPlayback::StopAllSFX(); AudioPlayback::ReleaseStageSFX();
    GraphicsSystem::fadeMode=0; PlayerSystem::playerMenuNum=0;
    GraphicsSystem::ClearGraphicsData(); AnimationSystem::ClearAnimationData();
    GraphicsSystem::LoadPalette("MasterPalette.act",0,0,0,256);
    FileIO::activeStageList=0; stageMode=0; GlobalAppDefinitions::gameMode=1; stageListPosition=0;
}

void StageSystem::InitStageSelectMenu()
{
    xScrollOffset=yScrollOffset=0;
    AudioPlayback::StopMusic(); AudioPlayback::StopAllSFX(); AudioPlayback::ReleaseStageSFX();
    GraphicsSystem::fadeMode=0; PlayerSystem::playerMenuNum=0; GlobalAppDefinitions::gameMode=0;
    GraphicsSystem::ClearGraphicsData(); AnimationSystem::ClearAnimationData();
    GraphicsSystem::LoadPalette("MasterPalette.act",0,0,0,256);
    TextSystem::textMenuSurfaceNo=0;
    GraphicsSystem::LoadGIFFile("Data/Game/SystemText.gif",0);
    stageMode=0;
    TextSystem::SetupTextMenu(gameMenu[0],0);
    TextSystem::AddTextMenuEntry(gameMenu[0],"RETRO ENGINE DEV MENU");
    TextSystem::AddTextMenuEntry(gameMenu[0]," ");
    TextSystem::AddTextMenuEntry(gameMenu[0],"SONIC CD Version");
    TextSystem::AddTextMenuEntry(gameMenu[0],GlobalAppDefinitions::gameVersion);
    TextSystem::AddTextMenuEntry(gameMenu[0]," ");
    TextSystem::AddTextMenuEntry(gameMenu[0]," ");
    TextSystem::AddTextMenuEntry(gameMenu[0]," ");
    TextSystem::AddTextMenuEntry(gameMenu[0],"PLAY GAME");
    TextSystem::AddTextMenuEntry(gameMenu[0]," ");
    TextSystem::AddTextMenuEntry(gameMenu[0],"STAGE SELECT");
    gameMenu[0].alignment=2; gameMenu[0].numSelections=2; gameMenu[0].selection1=0; gameMenu[0].selection2=7;
    gameMenu[1].numVisibleRows=0; gameMenu[1].visibleRowOffset=0;
    RenderDevice::UpdateHardwareTextures();
}

void StageSystem::InitErrorMessage()
{
    xScrollOffset=yScrollOffset=0;
    AudioPlayback::StopMusic(); AudioPlayback::StopAllSFX(); AudioPlayback::ReleaseStageSFX();
    GraphicsSystem::fadeMode=0; PlayerSystem::playerMenuNum=0; GlobalAppDefinitions::gameMode=0;
    GraphicsSystem::ClearGraphicsData(); AnimationSystem::ClearAnimationData();
    GraphicsSystem::LoadPalette("MasterPalette.act",0,0,0,256);
    TextSystem::textMenuSurfaceNo=0;
    GraphicsSystem::LoadGIFFile("Data/Game/SystemText.gif",0);
    gameMenu[0].alignment=2; gameMenu[0].numSelections=1; gameMenu[0].selection1=0;
    gameMenu[1].numVisibleRows=0; gameMenu[1].visibleRowOffset=0;
    RenderDevice::UpdateHardwareTextures();
    stageMode=4;
}

void StageSystem::ProcessStageSelectMenu()
{
    GraphicsSystem::gfxVertexSize=0; GraphicsSystem::gfxIndexSize=0;
    GraphicsSystem::ClearScreen(240);
    InputSystem::MenuKeyDown(gKeyDown,131);
    GraphicsSystem::DrawSprite(32,66,16,16,78,240,0);
    GraphicsSystem::DrawSprite(32,178,16,16,95,240,0);
    GraphicsSystem::DrawSprite(GlobalAppDefinitions::SCREEN_XSIZE-32,208,16,16,112,240,0);
    gKeyPress.start=gKeyPress.up=gKeyPress.down=0;
    if(gKeyDown.touches>0){
        if(gKeyDown.touchX[0]<120){
            if(gKeyDown.touchY[0]<120){if(!gKeyDown.up)gKeyPress.up=1;gKeyDown.up=1;}
            else{if(!gKeyDown.down)gKeyPress.down=1;gKeyDown.down=1;}
        }
        if(gKeyDown.touchX[0]>200){if(!gKeyDown.start)gKeyPress.start=1;gKeyDown.start=1;}
    } else {gKeyDown.start=0;gKeyDown.up=0;gKeyDown.down=0;}

    switch(stageMode){
    case 0:
        if(gKeyPress.down==1)gameMenu[0].selection2+=2;
        if(gKeyPress.up==1)gameMenu[0].selection2-=2;
        if(gameMenu[0].selection2>9)gameMenu[0].selection2=7;
        if(gameMenu[0].selection2<7)gameMenu[0].selection2=9;
        TextSystem::DrawTextMenu(gameMenu[0],GlobalAppDefinitions::SCREEN_CENTER,72);
        if(gKeyPress.start==1){
            if(gameMenu[0].selection2==7){stageMode=0;GlobalAppDefinitions::gameMode=1;FileIO::activeStageList=0;stageListPosition=0;break;}
            TextSystem::SetupTextMenu(gameMenu[0],0); TextSystem::AddTextMenuEntry(gameMenu[0],"CHOOSE A PLAYER");
            TextSystem::SetupTextMenu(gameMenu[1],0); TextSystem::LoadConfigListText(gameMenu[1],0);
            gameMenu[1].alignment=0;gameMenu[1].numSelections=1;gameMenu[1].selection1=0;stageMode=1;
        }
        break;
    case 1:
        if(gKeyPress.down==1)++gameMenu[1].selection1;
        if(gKeyPress.up==1)--gameMenu[1].selection1;
        if(gameMenu[1].selection1==(int32_t)gameMenu[1].numRows)gameMenu[1].selection1=0;
        if(gameMenu[1].selection1<0)gameMenu[1].selection1=(int32_t)gameMenu[1].numRows-1;
        TextSystem::DrawTextMenu(gameMenu[0],GlobalAppDefinitions::SCREEN_CENTER-4,72);
        TextSystem::DrawTextMenu(gameMenu[1],GlobalAppDefinitions::SCREEN_CENTER-40,96);
        if(gKeyPress.start==1){
            PlayerSystem::playerMenuNum=(uint8_t)gameMenu[1].selection1;
            TextSystem::SetupTextMenu(gameMenu[0],0);
            TextSystem::AddTextMenuEntry(gameMenu[0],"SELECT A STAGE LIST");
            TextSystem::AddTextMenuEntry(gameMenu[0]," ");TextSystem::AddTextMenuEntry(gameMenu[0]," ");
            TextSystem::AddTextMenuEntry(gameMenu[0],"   PRESENTATION");TextSystem::AddTextMenuEntry(gameMenu[0]," ");
            TextSystem::AddTextMenuEntry(gameMenu[0],"   REGULAR");TextSystem::AddTextMenuEntry(gameMenu[0]," ");
            TextSystem::AddTextMenuEntry(gameMenu[0],"   SPECIAL");TextSystem::AddTextMenuEntry(gameMenu[0]," ");
            TextSystem::AddTextMenuEntry(gameMenu[0],"   BONUS");
            gameMenu[0].alignment=0;gameMenu[0].selection2=3;stageMode=2;
        }
        break;
    case 2: {
        if(gKeyPress.down==1)gameMenu[0].selection2+=2;
        if(gKeyPress.up==1)gameMenu[0].selection2-=2;
        if(gameMenu[0].selection2>9)gameMenu[0].selection2=3;
        if(gameMenu[0].selection2<3)gameMenu[0].selection2=9;
        TextSystem::DrawTextMenu(gameMenu[0],GlobalAppDefinitions::SCREEN_CENTER-80,72);
        int32_t ok=0;
        switch(gameMenu[0].selection2){
        case 3: if(FileIO::noPresentationStages>0)ok=1; FileIO::activeStageList=0; break;
        case 5: if(FileIO::noZoneStages>0)ok=1; FileIO::activeStageList=1; break;
        case 7: if(FileIO::noSpecialStages>0)ok=1; FileIO::activeStageList=3; break;
        case 9: if(FileIO::noBonusStages>0)ok=1; FileIO::activeStageList=2; break;
        }
        if(gKeyPress.start==1&&ok==1){
            TextSystem::SetupTextMenu(gameMenu[0],0); TextSystem::AddTextMenuEntry(gameMenu[0],"SELECT A STAGE");
            TextSystem::SetupTextMenu(gameMenu[1],0); TextSystem::LoadConfigListText(gameMenu[1],1+(gameMenu[0].selection2-3>>1));
            gameMenu[1].alignment=1;gameMenu[1].numSelections=3;gameMenu[1].selection1=0;
            if(gameMenu[1].numRows>18)gameMenu[1].numVisibleRows=18;
            gameMenu[0].alignment=2;gameMenu[0].numSelections=1;gameMenu[1].timer=0;stageMode=3;
        }
        break;
    }
    case 3: {
        if(gKeyDown.down==1){++gameMenu[1].timer;if(gameMenu[1].timer>4){gameMenu[1].timer=0;gKeyPress.down=1;}}
        else if(gKeyDown.up==1){--gameMenu[1].timer;if(gameMenu[1].timer<-4){gameMenu[1].timer=0;gKeyPress.up=1;}}
        else gameMenu[1].timer=0;
        if(gKeyPress.down==1){++gameMenu[1].selection1;if(gameMenu[1].selection1-(int32_t)gameMenu[1].visibleRowOffset>=(int32_t)gameMenu[1].numVisibleRows)++gameMenu[1].visibleRowOffset;}
        if(gKeyPress.up==1){--gameMenu[1].selection1;if(gameMenu[1].selection1-(int32_t)gameMenu[1].visibleRowOffset<0)--gameMenu[1].visibleRowOffset;}
        if(gameMenu[1].selection1==(int32_t)gameMenu[1].numRows){gameMenu[1].selection1=0;gameMenu[1].visibleRowOffset=0;}
        if(gameMenu[1].selection1<0){gameMenu[1].selection1=(int32_t)gameMenu[1].numRows-1;gameMenu[1].visibleRowOffset=(uint16_t)((uint32_t)gameMenu[1].numRows-(uint32_t)gameMenu[1].numVisibleRows);}
        TextSystem::DrawTextMenu(gameMenu[0],GlobalAppDefinitions::SCREEN_CENTER-4,40);
        TextSystem::DrawTextMenu(gameMenu[1],GlobalAppDefinitions::SCREEN_CENTER+100,64);
        if(gKeyPress.start==1){debugMode=gKeyDown.touches<=1?0:1;stageMode=0;GlobalAppDefinitions::gameMode=1;stageListPosition=gameMenu[1].selection1;}
        break;
    }
    case 4:
        TextSystem::DrawTextMenu(gameMenu[0],GlobalAppDefinitions::SCREEN_CENTER,72);
        if(gKeyPress.start==1){
            stageMode=0;
            TextSystem::SetupTextMenu(gameMenu[0],0);
            TextSystem::AddTextMenuEntry(gameMenu[0],"RETRO ENGINE DEV MENU");
            for(int i=0;i<6;++i)TextSystem::AddTextMenuEntry(gameMenu[0]," ");
            TextSystem::AddTextMenuEntry(gameMenu[0],"PLAY GAME");
            TextSystem::AddTextMenuEntry(gameMenu[0]," ");
            TextSystem::AddTextMenuEntry(gameMenu[0],"STAGE SELECT");
            gameMenu[0].alignment=2;gameMenu[0].numSelections=2;gameMenu[0].selection1=0;gameMenu[0].selection2=7;
            gameMenu[1].numVisibleRows=0;gameMenu[1].visibleRowOffset=0;
        }
        break;
    }
    GraphicsSystem::gfxIndexSizeOpaque=GraphicsSystem::gfxIndexSize;
    GraphicsSystem::gfxVertexSizeOpaque=GraphicsSystem::gfxVertexSize;
}

}
