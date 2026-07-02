#pragma once
#include "../rvm/ObjectSystem.h"
#include "../rvm/GraphicsSystem.h"
#include "../rvm/AudioPlayback.h"
#include "../rvm/StageSystem.h"
#include "../rvm/GlobalAppDefinitions.h"
#include "../rvm/Scene3D.h"
#include "../rvm/FileIO.h"
#include "../rvm/EngineCallbacks.h"
#include "../rvm/PlayerSystem.h"
#include "../rvm/AnimationSystem.h"
#include <cstring>

namespace cd {

using rvm::ObjectSystem;
using rvm::ObjectEntity;
using rvm::PlayerObject;
using rvm::SpriteFrame;
using rvm::GraphicsSystem;
using rvm::AudioPlayback;
using rvm::StageSystem;
using rvm::GlobalAppDefinitions;
using rvm::Scene3D;
using rvm::FileIO;
using rvm::EngineCallbacks;
using rvm::PlayerSystem;

static constexpr int32_t FX_SCALE    = 0;
static constexpr int32_t FX_ROTATE   = 1;
static constexpr int32_t FX_ROTOZOOM = 2;
static constexpr int32_t FX_INK      = 3;
static constexpr int32_t FX_SCALE_INK= 4;
static constexpr int32_t FX_FLIP     = 5;

static constexpr int32_t INK_NONE    = 0;
static constexpr int32_t INK_BLEND   = 1;
static constexpr int32_t INK_ALPHA   = 2;
static constexpr int32_t INK_ADD     = 3;
static constexpr int32_t INK_SUB     = 4;

static constexpr int32_t MAT_WORLD = 0;
static constexpr int32_t MAT_VIEW  = 1;
static constexpr int32_t MAT_TEMP  = 2;
static constexpr int32_t FACE_FLAG_TEXTURED_3D = 0;

inline int32_t NS_GetGlobalVar(const char* name)
{
    for (int i = 0; i < 256; ++i)
        if (strcmp(ObjectSystem::globalVariableNames[i], name) == 0)
            return ObjectSystem::globalVariables[i];
    return 0;
}

inline void NS_SetGlobalVar(const char* name, int32_t val)
{
    for (int i = 0; i < 256; ++i)
        if (strcmp(ObjectSystem::globalVariableNames[i], name) == 0) {
            ObjectSystem::globalVariables[i] = val;
            return;
        }
}

inline uint8_t NS_TypeID(const char* name) { return ObjectSystem::GetTypeID(name); }

inline void NS_LoadSpriteSheet(const char* fileName)
{
    int32_t t = (int32_t)ObjectSystem::objectEntityList[ObjectSystem::objectLoop].type;
    ObjectSystem::objectScriptList[t].surfaceNum = GraphicsSystem::AddGraphicsFile(fileName);
}

inline void NS_SpriteFrame(int32_t xPivot, int32_t yPivot, int32_t xSize, int32_t ySize, int32_t left, int32_t top)
{
    if (ObjectSystem::scriptFramesNo < 4096) {
        SpriteFrame& f = ObjectSystem::scriptFrames[ObjectSystem::scriptFramesNo++];
        f.xPivot = xPivot;
        f.yPivot = yPivot;
        f.xSize  = xSize;
        f.ySize  = ySize;
        f.left   = left;
        f.top    = top;
    }
}

inline SpriteFrame& NS_Frame(int32_t entityNo, int32_t frameIdx)
{
    int32_t t = (int32_t)ObjectSystem::objectEntityList[entityNo].type;
    return ObjectSystem::scriptFrames[ObjectSystem::objectScriptList[t].frameListOffset + frameIdx];
}

inline int32_t NS_Surface(int32_t entityNo)
{
    int32_t t = (int32_t)ObjectSystem::objectEntityList[entityNo].type;
    return (int32_t)ObjectSystem::objectScriptList[t].surfaceNum;
}

inline void NS_DrawSprite(int32_t entityNo, int32_t frameIdx)
{
    ObjectEntity& e = ObjectSystem::objectEntityList[entityNo];
    SpriteFrame& f = NS_Frame(entityNo, frameIdx);
    GraphicsSystem::DrawSprite(
        (e.xPos >> 16) - StageSystem::xScrollOffset + f.xPivot,
        (e.yPos >> 16) - StageSystem::yScrollOffset + f.yPivot,
        f.xSize, f.ySize, f.left, f.top, NS_Surface(entityNo));
}

inline void NS_DrawSpriteXY(int32_t entityNo, int32_t frameIdx, int32_t xPos16, int32_t yPos16)
{
    SpriteFrame& f = NS_Frame(entityNo, frameIdx);
    GraphicsSystem::DrawSprite(
        (xPos16 >> 16) - StageSystem::xScrollOffset + f.xPivot,
        (yPos16 >> 16) - StageSystem::yScrollOffset + f.yPivot,
        f.xSize, f.ySize, f.left, f.top, NS_Surface(entityNo));
}

inline void NS_DrawSpriteScreenXY(int32_t entityNo, int32_t frameIdx, int32_t x, int32_t y)
{
    SpriteFrame& f = NS_Frame(entityNo, frameIdx);
    GraphicsSystem::DrawSprite(x + f.xPivot, y + f.yPivot,
        f.xSize, f.ySize, f.left, f.top, NS_Surface(entityNo));
}

inline void NS_DrawSpriteFX(int32_t entityNo, int32_t frameIdx, int32_t fx, int32_t xPos16, int32_t yPos16)
{
    ObjectEntity& e = ObjectSystem::objectEntityList[entityNo];
    SpriteFrame& f  = NS_Frame(entityNo, frameIdx);
    int32_t sn = NS_Surface(entityNo);
    int32_t bx = (xPos16 >> 16) - StageSystem::xScrollOffset;
    int32_t by = (yPos16 >> 16) - StageSystem::yScrollOffset;
    switch (fx) {
    case FX_SCALE:
        GraphicsSystem::DrawScaledSprite(e.direction, bx, by, -f.xPivot, -f.yPivot,
            e.scale, e.scale, f.xSize, f.ySize, f.left, f.top, sn);
        break;
    case FX_ROTATE:
        GraphicsSystem::DrawRotatedSprite(e.direction, bx, by, -f.xPivot, -f.yPivot,
            f.left, f.top, f.xSize, f.ySize, e.rotation, sn);
        break;
    case FX_ROTOZOOM:
        GraphicsSystem::DrawRotoZoomSprite(e.direction, bx, by, -f.xPivot, -f.yPivot,
            f.left, f.top, f.xSize, f.ySize, e.rotation, e.scale, sn);
        break;
    case FX_INK: {
        int32_t ix = bx + f.xPivot, iy = by + f.yPivot;
        switch (e.inkEffect) {
        case INK_BLEND: GraphicsSystem::DrawBlendedSprite(ix, iy, f.xSize, f.ySize, f.left, f.top, sn); break;
        case INK_ALPHA: GraphicsSystem::DrawAlphaBlendedSprite(ix, iy, f.xSize, f.ySize, f.left, f.top, (int32_t)e.alpha, sn); break;
        case INK_ADD:   GraphicsSystem::DrawAdditiveBlendedSprite(ix, iy, f.xSize, f.ySize, f.left, f.top, (int32_t)e.alpha, sn); break;
        case INK_SUB:   GraphicsSystem::DrawSubtractiveBlendedSprite(ix, iy, f.xSize, f.ySize, f.left, f.top, (int32_t)e.alpha, sn); break;
        default:        GraphicsSystem::DrawSprite(ix, iy, f.xSize, f.ySize, f.left, f.top, sn); break;
        }
        break;
    }
    case FX_SCALE_INK:
        GraphicsSystem::DrawScaledSprite(e.direction, bx, by, -f.xPivot, -f.yPivot,
            e.scale, e.scale, f.xSize, f.ySize, f.left, f.top, sn);
        break;
    case FX_FLIP:
        switch (e.direction) {
        case 0: GraphicsSystem::DrawSpriteFlipped(bx + f.xPivot, by + f.yPivot, f.xSize, f.ySize, f.left, f.top, 0, sn); break;
        case 1: GraphicsSystem::DrawSpriteFlipped(bx - f.xSize - f.xPivot, by + f.yPivot, f.xSize, f.ySize, f.left, f.top, 1, sn); break;
        case 2: GraphicsSystem::DrawSpriteFlipped(bx + f.xPivot, by - f.ySize - f.yPivot, f.xSize, f.ySize, f.left, f.top, 2, sn); break;
        case 3: GraphicsSystem::DrawSpriteFlipped(bx - f.xSize - f.xPivot, by - f.ySize - f.yPivot, f.xSize, f.ySize, f.left, f.top, 3, sn); break;
        }
        break;
    }
}

inline void NS_DrawSpriteScreenFX(int32_t entityNo, int32_t frameIdx, int32_t fx, int32_t x, int32_t y)
{
    ObjectEntity& e = ObjectSystem::objectEntityList[entityNo];
    SpriteFrame& f  = NS_Frame(entityNo, frameIdx);
    int32_t sn = NS_Surface(entityNo);
    switch (fx) {
    case FX_SCALE:
        GraphicsSystem::DrawScaledSprite(e.direction, x, y, -f.xPivot, -f.yPivot,
            e.scale, e.scale, f.xSize, f.ySize, f.left, f.top, sn);
        break;
    case FX_ROTATE:
        GraphicsSystem::DrawRotatedSprite(e.direction, x, y, -f.xPivot, -f.yPivot,
            f.left, f.top, f.xSize, f.ySize, e.rotation, sn);
        break;
    case FX_ROTOZOOM:
        GraphicsSystem::DrawRotoZoomSprite(e.direction, x, y, -f.xPivot, -f.yPivot,
            f.left, f.top, f.xSize, f.ySize, e.rotation, e.scale, sn);
        break;
    case FX_INK: {
        int32_t ix = x + f.xPivot, iy = y + f.yPivot;
        switch (e.inkEffect) {
        case INK_BLEND: GraphicsSystem::DrawBlendedSprite(ix, iy, f.xSize, f.ySize, f.left, f.top, sn); break;
        case INK_ALPHA: GraphicsSystem::DrawAlphaBlendedSprite(ix, iy, f.xSize, f.ySize, f.left, f.top, (int32_t)e.alpha, sn); break;
        case INK_ADD:   GraphicsSystem::DrawAdditiveBlendedSprite(ix, iy, f.xSize, f.ySize, f.left, f.top, (int32_t)e.alpha, sn); break;
        case INK_SUB:   GraphicsSystem::DrawSubtractiveBlendedSprite(ix, iy, f.xSize, f.ySize, f.left, f.top, (int32_t)e.alpha, sn); break;
        default:        GraphicsSystem::DrawSprite(ix, iy, f.xSize, f.ySize, f.left, f.top, sn); break;
        }
        break;
    }
    case FX_FLIP:
        switch (e.direction) {
        case 0: GraphicsSystem::DrawSpriteFlipped(x + f.xPivot, y + f.yPivot, f.xSize, f.ySize, f.left, f.top, 0, sn); break;
        case 1: GraphicsSystem::DrawSpriteFlipped(x - f.xSize - f.xPivot, y + f.yPivot, f.xSize, f.ySize, f.left, f.top, 1, sn); break;
        case 2: GraphicsSystem::DrawSpriteFlipped(x + f.xPivot, y - f.ySize - f.yPivot, f.xSize, f.ySize, f.left, f.top, 2, sn); break;
        case 3: GraphicsSystem::DrawSpriteFlipped(x - f.xSize - f.xPivot, y - f.ySize - f.yPivot, f.xSize, f.ySize, f.left, f.top, 3, sn); break;
        }
        break;
    }
}

inline void NS_SetScreenFade(int32_t r, int32_t g, int32_t b, int32_t a)
{
    GraphicsSystem::SetFade((uint8_t)r, (uint8_t)g, (uint8_t)b, (uint16_t)a);
}

inline void NS_DrawRect(int32_t x, int32_t y, int32_t w, int32_t h, int32_t r, int32_t g, int32_t b, int32_t a)
{
    GraphicsSystem::DrawRectangle(x, y, w, h, r, g, b, a);
}

inline void NS_ResetObjectEntity(int32_t entityNo, uint8_t typeID, uint8_t propVal, int32_t xPos16, int32_t yPos16)
{
    ObjectEntity& e = ObjectSystem::objectEntityList[entityNo];
    e.type         = typeID;
    e.propertyValue= propVal;
    e.xPos         = xPos16;
    e.yPos         = yPos16;
    e.direction    = 0;
    e.frame        = 0;
    e.priority     = 0;
    e.rotation     = 0;
    e.state        = 0;
    e.drawOrder    = 3;
    e.scale        = 512;
    e.inkEffect    = 0;
    for (int v = 0; v < 8; ++v) e.value[v] = 0;
}

inline int32_t NS_CreateTempObject(uint8_t typeID, uint8_t propVal, int32_t xPos16, int32_t yPos16)
{
    int32_t& tp = ObjectSystem::scriptEng.arrayPosition[2];
    if (ObjectSystem::objectEntityList[tp].type > 0) {
        ++tp;
        if (tp == 1184) tp = 1056;
    }
    ObjectEntity& e = ObjectSystem::objectEntityList[tp];
    e.type = typeID; e.propertyValue = propVal; e.xPos = xPos16; e.yPos = yPos16;
    e.direction = 0; e.frame = 0; e.priority = 1; e.rotation = 0; e.state = 0;
    e.drawOrder = 3; e.scale = 512; e.inkEffect = 0; e.alpha = 0;
    e.animation = 0; e.prevAnimation = 0; e.animationSpeed = 0; e.animationTimer = 0;
    for (int v = 0; v < 8; ++v) e.value[v] = 0;
    return tp;
}

inline int32_t NS_TempObjectPos() { return ObjectSystem::scriptEng.arrayPosition[2]; }

inline int32_t NS_PlayerListPos() { return (int32_t)PlayerSystem::playerMenuNum; }

inline PlayerObject& NS_Player() { return PlayerSystem::playerList[0]; }

inline void NS_SetMusicTrack(const char* fileName, int32_t trackNo, int32_t loopParam)
{
    if (loopParam > 1)
        AudioPlayback::SetMusicTrack(fileName, trackNo, 1, (uint32_t)loopParam);
    else
        AudioPlayback::SetMusicTrack(fileName, trackNo, (uint8_t)loopParam, 0);
}

inline int32_t NS_MusicVolume()              { return AudioPlayback::musicVolume; }
inline void    NS_SetMusicVolume(int32_t v)  { AudioPlayback::SetMusicVolume(v); }

inline void NS_PlayMusic(int32_t trackNo) { AudioPlayback::PlayMusic(trackNo); }
inline void NS_StopMusic()                { AudioPlayback::StopMusic(); }
inline void NS_PlaySfx(int32_t sfxNo, bool loop)      { AudioPlayback::PlaySfx(sfxNo, (uint8_t)loop); }
inline void NS_StopSfx(int32_t sfxNo)                 { AudioPlayback::StopSfx(sfxNo); }
inline void NS_PlayStageSfx(int32_t sfxNo, bool loop) { AudioPlayback::PlaySfx(sfxNo + AudioPlayback::numGlobalSFX, (uint8_t)loop); }

inline void NS_LoadStage() { StageSystem::stageMode = StageSystem::LOADSTAGE; }

inline int32_t NS_CheckTouchRect(int32_t x1, int32_t y1, int32_t x2, int32_t y2)
{
    int32_t result = -1;
    for (int8_t ti = 0; ti < StageSystem::gKeyDown.touches; ++ti)
        if (StageSystem::gKeyDown.touchDown[ti] == 1 &&
            StageSystem::gKeyDown.touchX[ti] > x1 &&
            StageSystem::gKeyDown.touchX[ti] < x2 &&
            StageSystem::gKeyDown.touchY[ti] > y1 &&
            StageSystem::gKeyDown.touchY[ti] < y2)
            result = (int32_t)ti;
    return result;
}

inline void NS_Draw3DScene(int32_t entityNo)
{
    int32_t sn = NS_Surface(entityNo);
    Scene3D::TransformVertexBuffer();
    Scene3D::Sort3DDrawList();
    Scene3D::Draw3DScene(sn);
}

inline int32_t* NS_Mat(int32_t which)
{
    switch (which) {
    case MAT_VIEW: return Scene3D::matView;
    case MAT_TEMP: return Scene3D::matTemp;
    default:       return Scene3D::matWorld;
    }
}

inline void NS_MatrixTranslateXYZ(int32_t mat, int32_t x, int32_t y, int32_t z)
{
    Scene3D::MatrixTranslateXYZ(NS_Mat(mat), x, y, z);
}

inline void NS_MatrixRotateXYZ(int32_t mat, int32_t ax, int32_t ay, int32_t az)
{
    Scene3D::MatrixRotateXYZ(NS_Mat(mat), ax, ay, az);
}

inline int32_t NS_Sin512(int32_t angle)
{
    int32_t a = ((angle < 0) ? (512 - angle) : angle) & 511;
    return GlobalAppDefinitions::SinValue512[a];
}

inline int32_t NS_Cos512(int32_t angle)
{
    int32_t a = ((angle < 0) ? (512 - angle) : angle) & 511;
    return GlobalAppDefinitions::CosValue512[a];
}

inline int32_t NS_Sin256(int32_t angle)
{
    int32_t a = ((angle < 0) ? (256 - angle) : angle) & 255;
    return GlobalAppDefinitions::SinValue256[a];
}

inline int32_t NS_Cos256(int32_t angle)
{
    int32_t a = ((angle < 0) ? (256 - angle) : angle) & 255;
    return GlobalAppDefinitions::CosValue256[a];
}

inline rvm::CollisionBox& NS_PlayerBox()
{
    auto& af = *PlayerSystem::playerList[0].animationFile;
    auto& e  = *PlayerSystem::playerList[0].objectPtr;
    auto& fr = rvm::AnimationSystem::animationFrames[
        rvm::AnimationSystem::animationList[af.aniListOffset + (int32_t)e.animation].frameListOffset + (int32_t)e.frame];
    return rvm::AnimationSystem::collisionBoxList[af.cbListOffset + (int32_t)fr.collisionBox];
}
inline int32_t NS_PlayerCollTop()    { return (int32_t)NS_PlayerBox().top[0]; }
inline int32_t NS_PlayerCollBottom() { return (int32_t)NS_PlayerBox().bottom[0]; }
inline int32_t NS_PlayerCollLeft()   { return (int32_t)NS_PlayerBox().left[0]; }
inline int32_t NS_PlayerCollRight()  { return (int32_t)NS_PlayerBox().right[0]; }

enum { CSIDE_FLOOR = 0, CSIDE_LWALL = 1, CSIDE_RWALL = 2, CSIDE_ROOF = 3, CSIDE_ENTITY = 4 };
inline void NS_ObjectTileCollision(int32_t side, int32_t xOff, int32_t yOff, int32_t plane)
{
    switch (side) {
    case CSIDE_FLOOR: ObjectSystem::ObjectFloorCollision(xOff, yOff, plane); break;
    case CSIDE_LWALL: ObjectSystem::ObjectLWallCollision(xOff, yOff, plane); break;
    case CSIDE_RWALL: ObjectSystem::ObjectRWallCollision(xOff, yOff, plane); break;
    case CSIDE_ROOF:  ObjectSystem::ObjectRoofCollision(xOff, yOff, plane);  break;
    }
}

inline bool NS_PlayerTileCollision()
{
    PlayerObject& p = PlayerSystem::playerList[0];
    if (p.tileCollisions == 1)
        PlayerSystem::ProcessPlayerTileCollisions(p);
    else { p.xPos += p.xVelocity; p.yPos += p.yVelocity; }
    return ObjectSystem::scriptEng.checkResult != 0;
}

inline void NS_ProcessPlayerAnimation()
{
    auto& af = *PlayerSystem::playerList[0].animationFile;
    auto& e  = *PlayerSystem::playerList[0].objectPtr;
    rvm::AnimationSystem::ProcessObjectAnimation(
        rvm::AnimationSystem::animationList[af.aniListOffset + (int32_t)e.animation], e);
}

inline void NS_DrawPlayerAnimation()
{
    PlayerObject& p = PlayerSystem::playerList[0];
    if (p.visible != 1) return;
    auto& af = *p.animationFile;
    auto& e  = *p.objectPtr;
    auto& anim = rvm::AnimationSystem::animationList[af.aniListOffset + (int32_t)e.animation];
    if ((int32_t)StageSystem::cameraEnabled == 0)
        rvm::AnimationSystem::DrawObjectAnimation(anim, e, p.screenXPos, p.screenYPos);
    else
        rvm::AnimationSystem::DrawObjectAnimation(anim, e,
            (p.xPos >> 16) - StageSystem::xScrollOffset, (p.yPos >> 16) - StageSystem::yScrollOffset);
}

inline void NS_LoadAnimation(const char* fileName)
{
    int32_t t = (int32_t)ObjectSystem::objectEntityList[ObjectSystem::objectLoop].type;
    ObjectSystem::objectScriptList[t].animationFile = rvm::AnimationSystem::AddAnimationFile(fileName);
}

inline void NS_BindPlayerToObject(int32_t playerNo, int32_t entityNo)
{
    PlayerObject& p = PlayerSystem::playerList[playerNo];
    p.animationFile = ObjectSystem::objectScriptList[(int32_t)ObjectSystem::objectEntityList[entityNo].type].animationFile;
    p.objectPtr     = &ObjectSystem::objectEntityList[entityNo];
    p.objectNum     = entityNo;
}

inline int32_t NS_GetAnimationByName(const char* name)
{
    auto& af = *PlayerSystem::playerList[0].animationFile;
    for (int32_t i = 0; i < af.numAnimations; ++i)
        if (strcmp(rvm::AnimationSystem::animationList[af.aniListOffset + i].name, name) == 0)
            return i;
    return 0;
}

}
