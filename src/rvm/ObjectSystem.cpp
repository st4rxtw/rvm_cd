#include "ObjectSystem.h"
#include "AnimationSystem.h"
#include "AudioPlayback.h"
#include "EngineCallbacks.h"
#include "FileIO.h"
#include "GlobalAppDefinitions.h"
#include "GraphicsSystem.h"
#include "Log.h"
#include "PlayerSystem.h"
#include "Scene3D.h"
#include "StageSystem.h"
#include "TextSystem.h"
#include <cmath>
#include <cstdlib>
#include <cstring>

namespace rvm {

int32_t        ObjectSystem::scriptData[262144]{};
int32_t        ObjectSystem::scriptDataPos      = 0;
int32_t        ObjectSystem::scriptDataOffset   = 0;
int32_t        ObjectSystem::scriptLineNumber   = 0;
int32_t        ObjectSystem::jumpTableData[16384]{};
int32_t        ObjectSystem::jumpTableDataPos   = 0;
int32_t        ObjectSystem::jumpTableOffset    = 0;
int32_t        ObjectSystem::jumpTableStack[1024]{};
int32_t        ObjectSystem::jumpTableStackPos  = 0;
int32_t        ObjectSystem::NUM_FUNCTIONS      = 0;
int32_t        ObjectSystem::functionStack[1024]{};
int32_t        ObjectSystem::functionStackPos   = 0;
SpriteFrame    ObjectSystem::scriptFrames[4096]{};
int32_t        ObjectSystem::scriptFramesNo     = 0;
uint8_t        ObjectSystem::NO_GLOBALVARIABLES = 0;
char           ObjectSystem::globalVariableNames[256][32]{};
int32_t        ObjectSystem::globalVariables[256]{};
int32_t        ObjectSystem::objectLoop         = 0;
ScriptEngine   ObjectSystem::scriptEng          {};
char           ObjectSystem::scriptText[256]    {};
ObjectScript   ObjectSystem::objectScriptList[256]{};
FunctionScript ObjectSystem::functionScriptList[512]{};
ObjectEntity   ObjectSystem::objectEntityList[1184]{};
ObjectDrawList ObjectSystem::objectDrawOrderList[7]{};
int32_t        ObjectSystem::playerNum          = 0;
CollisionSensor ObjectSystem::cSensor[6]        {};
char           ObjectSystem::functionNames[512][32]{};
char           ObjectSystem::typeNames[256][32] {};

static const int8_t kOpSizes[135] = {
    0,2,2,2,1,1,2,2,2,2,2,2,2,2,1,2,2,2,2,3,3,3,3,3,3,0,0,3,3,3,3,3,3,0,2,0,0,2,2,2,2,2,5,5,3,4,7,1,1,1,3,3,4,7,7,3,6,6,5,3,4,3,7,2,1,4,4,1,4,3,4,0,8,5,5,4,2,0,0,0,0,0,3,1,0,2,1,3,4,4,1,0,2,1,1,0,1,2,4,4,2,2,2,4,3,1,0,6,4,4,4,3,3,0,0,1,2,3,3,4,2,4,2,0,0,1,3,7,5,2,2,2,1,1,4
};

void ObjectSystem::Init() {}

void ObjectSystem::ClearScriptData()
{
    std::memset(scriptData,    0, sizeof(scriptData));
    std::memset(jumpTableData, 0, sizeof(jumpTableData));
    scriptDataPos    = 0;
    jumpTableDataPos = 0;
    scriptFramesNo   = 0;
    NUM_FUNCTIONS    = 0;
    AnimationSystem::ClearAnimationData();
    for (int i = 0; i < 2; ++i) {
        PlayerSystem::playerList[i].animationFile = AnimationSystem::GetDefaultAnimationRef();
        PlayerSystem::playerList[i].objectPtr     = &objectEntityList[0];
    }
    for (int i = 0; i < 256; ++i) {
        objectScriptList[i].mainScript       = 262143;
        objectScriptList[i].mainJumpTable    = 16383;
        objectScriptList[i].playerScript     = 262143;
        objectScriptList[i].playerJumpTable  = 16383;
        objectScriptList[i].drawScript       = 262143;
        objectScriptList[i].drawJumpTable    = 16383;
        objectScriptList[i].startupScript    = 262143;
        objectScriptList[i].startupJumpTable = 16383;
        objectScriptList[i].frameListOffset  = 0;
        objectScriptList[i].numFrames        = 0;
        objectScriptList[i].surfaceNum       = 0;
        objectScriptList[i].animationFile    = AnimationSystem::GetDefaultAnimationRef();
        functionScriptList[i].mainScript     = 262143;
        functionScriptList[i].mainJumpTable  = 16383;
        typeNames[i][0] = '\0';
    }
    SetObjectTypeName("BlankObject", 0);
}

void ObjectSystem::SetObjectTypeName(const char* name, int32_t scriptNum)
{
    int i = 0, j = 0;
    while (name[i] != '\0') {
        if (name[i] != ' ') typeNames[scriptNum][j++] = name[i];
        ++i;
    }
    if (j < 32) typeNames[scriptNum][j] = '\0';
}

void ObjectSystem::LoadByteCodeFile(int32_t fileType, int32_t scriptNum)
{
    FileData fData{};
    FileIO::StrCopy(scriptText, 256, "Data/Scripts/ByteCode/", 256);

    if (FileIO::bytecodeMode == 1) {
        static const char kListID[4] = { 'P', 'R', 'B', 'S' };
        char seg[8] = "GS000";
        if (fileType < 4) {
            seg[0] = kListID[fileType];
            int pos = StageSystem::stageListPosition;
            seg[2] = (char)('0' + pos / 100);
            seg[3] = (char)('0' + pos % 100 / 10);
            seg[4] = (char)('0' + pos % 10);
        }
        FileIO::StrAdd(scriptText, 256, seg, 8);
        FileIO::StrAdd(scriptText, 256, ".bin", 5);
    } else {
        switch (fileType) {
        case 0: FileIO::StrAdd(scriptText,256,FileIO::pStageList[StageSystem::stageListPosition].stageFolderName,8); FileIO::StrAdd(scriptText,256,".bin",5); break;
        case 1: FileIO::StrAdd(scriptText,256,FileIO::zStageList[StageSystem::stageListPosition].stageFolderName,8); FileIO::StrAdd(scriptText,256,".bin",5); break;
        case 2: FileIO::StrAdd(scriptText,256,FileIO::bStageList[StageSystem::stageListPosition].stageFolderName,8); FileIO::StrAdd(scriptText,256,".bin",5); break;
        case 3: FileIO::StrAdd(scriptText,256,FileIO::sStageList[StageSystem::stageListPosition].stageFolderName,8); FileIO::StrAdd(scriptText,256,".bin",5); break;
        case 4: FileIO::StrAdd(scriptText,256,"GlobalCode.bin",256); break;
        }
    }
    if (!FileIO::LoadFile(scriptText, fData)) return;
    Log::Info("Bytecode: %s", scriptText);

    auto read4 = []() -> int32_t {
        return (int32_t)FileIO::ReadByte()
             | ((int32_t)FileIO::ReadByte() <<  8)
             | ((int32_t)FileIO::ReadByte() << 16)
             | ((int32_t)FileIO::ReadByte() << 24);
    };

    auto readBlock = [&](int32_t*& arr, int32_t& pos) {
        int32_t count = read4();
        while (count > 0) {
            uint8_t b = FileIO::ReadByte();
            uint8_t n = b & 0x7F;
            if (b < 128) {
                for (; n; --n) { arr[pos++] = FileIO::ReadByte(); --count; }
            } else {
                for (; n; --n) { arr[pos++] = read4(); --count; }
            }
        }
    };

    int32_t* sd = scriptData;
    int32_t* jd = jumpTableData;
    readBlock(sd, scriptDataPos);
    readBlock(jd, jumpTableDataPos);

    int32_t nObj = (int32_t)FileIO::ReadByte() | ((int32_t)FileIO::ReadByte() << 8);
    for (int i = scriptNum; i < scriptNum + nObj; ++i) {
        objectScriptList[i].mainScript    = read4();
        objectScriptList[i].playerScript  = read4();
        objectScriptList[i].drawScript    = read4();
        objectScriptList[i].startupScript = read4();
    }
    for (int i = scriptNum; i < scriptNum + nObj; ++i) {
        objectScriptList[i].mainJumpTable    = read4();
        objectScriptList[i].playerJumpTable  = read4();
        objectScriptList[i].drawJumpTable    = read4();
        objectScriptList[i].startupJumpTable = read4();
    }

    int32_t nFn = (int32_t)FileIO::ReadByte() | ((int32_t)FileIO::ReadByte() << 8);
    for (int i = 0; i < nFn; ++i) functionScriptList[i].mainScript    = read4();
    for (int i = 0; i < nFn; ++i) functionScriptList[i].mainJumpTable = read4();
    FileIO::CloseFile();
}

#define PL  PlayerSystem::playerList[playerNum]
#define OE  objectEntityList[objectLoop]
#define OSL objectScriptList[(int32_t)OE.type]

static inline int32_t resolveIndex(int32_t& scpRef, int32_t* sd, int32_t loop)
{
    int32_t mode = sd[scpRef++];
    int32_t val;
    bool ind;
    switch (mode) {
    case 0: return loop;
    case 1: ind = sd[scpRef++] == 1; val = sd[scpRef++]; return ind ? ObjectSystem::scriptEng.arrayPosition[val] : val;
    case 2: ind = sd[scpRef++] == 1; val = sd[scpRef++]; return ind ? loop + ObjectSystem::scriptEng.arrayPosition[val] : loop + val;
    case 3: ind = sd[scpRef++] == 1; val = sd[scpRef++]; return ind ? loop - ObjectSystem::scriptEng.arrayPosition[val] : loop - val;
    }
    return 0;
}

void ObjectSystem::ProcessScript(int32_t startPtr, int32_t jtPtr, int32_t sub)
{
    int32_t* sd = scriptData;
    int32_t scp = startPtr;
    int32_t jtp = jtPtr;
    int32_t base= startPtr;
    jumpTableStackPos = 0;
    functionStackPos  = 0;

    for (;;) {
        int32_t op = sd[scp++];
        if (op == 0) break;

        int8_t  sz = kOpSizes[op];
        int32_t nb = 0;
        int32_t scpPreOps = scp;

        for (int32_t arg = 0; arg < (int32_t)sz; ++arg) {
            int32_t mode = sd[scp];
            switch (mode) {
            case 1: {
                ++scp;
                int32_t idx = resolveIndex(scp, sd, objectLoop);
                int32_t var = sd[scp++];
                int32_t v = 0;
                switch (var) {
                case 0:  v = scriptEng.tempValue[0]; break;
                case 1:  v = scriptEng.tempValue[1]; break;
                case 2:  v = scriptEng.tempValue[2]; break;
                case 3:  v = scriptEng.tempValue[3]; break;
                case 4:  v = scriptEng.tempValue[4]; break;
                case 5:  v = scriptEng.tempValue[5]; break;
                case 6:  v = scriptEng.tempValue[6]; break;
                case 7:  v = scriptEng.tempValue[7]; break;
                case 8:  v = scriptEng.checkResult; break;
                case 9:  v = scriptEng.arrayPosition[0]; break;
                case 10: v = scriptEng.arrayPosition[1]; break;
                case 11: v = globalVariables[idx]; break;
                case 12: v = idx; break;
                case 13: v = (int32_t)objectEntityList[idx].type; break;
                case 14: v = (int32_t)objectEntityList[idx].propertyValue; break;
                case 15: v = objectEntityList[idx].xPos; break;
                case 16: v = objectEntityList[idx].yPos; break;
                case 17: v = objectEntityList[idx].xPos >> 16; break;
                case 18: v = objectEntityList[idx].yPos >> 16; break;
                case 19: v = (int32_t)objectEntityList[idx].state; break;
                case 20: v = objectEntityList[idx].rotation; break;
                case 21: v = objectEntityList[idx].scale; break;
                case 22: v = (int32_t)objectEntityList[idx].priority; break;
                case 23: v = (int32_t)objectEntityList[idx].drawOrder; break;
                case 24: v = (int32_t)objectEntityList[idx].direction; break;
                case 25: v = (int32_t)objectEntityList[idx].inkEffect; break;
                case 26: v = (int32_t)objectEntityList[idx].alpha; break;
                case 27: v = (int32_t)objectEntityList[idx].frame; break;
                case 28: v = (int32_t)objectEntityList[idx].animation; break;
                case 29: v = (int32_t)objectEntityList[idx].prevAnimation; break;
                case 30: v = objectEntityList[idx].animationSpeed; break;
                case 31: v = objectEntityList[idx].animationTimer; break;
                case 32: case 33: case 34: case 35:
                case 36: case 37: case 38: case 39:
                    v = objectEntityList[idx].value[var-32]; break;
                case 40: {
                    int32_t ex = objectEntityList[objectLoop].xPos >> 16;
                    if (ex > StageSystem::xScrollOffset - GlobalAppDefinitions::OBJECT_BORDER_X1 &&
                        ex < StageSystem::xScrollOffset + GlobalAppDefinitions::OBJECT_BORDER_X2) {
                        int32_t ey = objectEntityList[objectLoop].yPos >> 16;
                        v = (ey <= StageSystem::yScrollOffset-256 || ey >= StageSystem::yScrollOffset+496) ? 1 : 0;
                    } else v = 1;
                    break;
                }
                case 41: v = (int32_t)PL.objectPtr->state; break;
                case 42: v = (int32_t)PL.controlMode; break;
                case 43: v = (int32_t)PL.controlLock; break;
                case 44: v = (int32_t)PL.collisionMode; break;
                case 45: v = (int32_t)PL.collisionPlane; break;
                case 46: v = PL.xPos; break;
                case 47: v = PL.yPos; break;
                case 48: v = PL.xPos >> 16; break;
                case 49: v = PL.yPos >> 16; break;
                case 50: v = PL.screenXPos; break;
                case 51: v = PL.screenYPos; break;
                case 52: v = PL.speed; break;
                case 53: v = PL.xVelocity; break;
                case 54: v = PL.yVelocity; break;
                case 55: v = (int32_t)PL.gravity; break;
                case 56: v = PL.angle; break;
                case 57: v = (int32_t)PL.skidding; break;
                case 58: v = (int32_t)PL.pushing; break;
                case 59: v = (int32_t)PL.trackScroll; break;
                case 60: v = (int32_t)PL.up; break;
                case 61: v = (int32_t)PL.down; break;
                case 62: v = (int32_t)PL.left; break;
                case 63: v = (int32_t)PL.right; break;
                case 64: v = (int32_t)PL.jumpPress; break;
                case 65: v = (int32_t)PL.jumpHold; break;
                case 66: v = (int32_t)PL.followPlayer1; break;
                case 67: v = PL.lookPos; break;
                case 68: v = (int32_t)PL.water; break;
                case 69: v = PL.movementStats.topSpeed; break;
                case 70: v = PL.movementStats.acceleration; break;
                case 71: v = PL.movementStats.deceleration; break;
                case 72: v = PL.movementStats.airAcceleration; break;
                case 73: v = PL.movementStats.airDeceleration; break;
                case 74: v = PL.movementStats.gravity; break;
                case 75: v = PL.movementStats.jumpStrength; break;
                case 76: v = PL.movementStats.jumpCap; break;
                case 77: v = PL.movementStats.rollingAcceleration; break;
                case 78: v = PL.movementStats.rollingDeceleration; break;
                case 79: v = PL.objectNum; break;
                case 80: { auto& af=*PL.animationFile; v=(int32_t)AnimationSystem::collisionBoxList[af.cbListOffset+(int32_t)AnimationSystem::animationFrames[AnimationSystem::animationList[af.aniListOffset+(int32_t)PL.objectPtr->animation].frameListOffset+(int32_t)PL.objectPtr->frame].collisionBox].left[0]; break; }
                case 81: { auto& af=*PL.animationFile; v=(int32_t)AnimationSystem::collisionBoxList[af.cbListOffset+(int32_t)AnimationSystem::animationFrames[AnimationSystem::animationList[af.aniListOffset+(int32_t)PL.objectPtr->animation].frameListOffset+(int32_t)PL.objectPtr->frame].collisionBox].top[0]; break; }
                case 82: { auto& af=*PL.animationFile; v=(int32_t)AnimationSystem::collisionBoxList[af.cbListOffset+(int32_t)AnimationSystem::animationFrames[AnimationSystem::animationList[af.aniListOffset+(int32_t)PL.objectPtr->animation].frameListOffset+(int32_t)PL.objectPtr->frame].collisionBox].right[0]; break; }
                case 83: { auto& af=*PL.animationFile; v=(int32_t)AnimationSystem::collisionBoxList[af.cbListOffset+(int32_t)AnimationSystem::animationFrames[AnimationSystem::animationList[af.aniListOffset+(int32_t)PL.objectPtr->animation].frameListOffset+(int32_t)PL.objectPtr->frame].collisionBox].bottom[0]; break; }
                case 84: v = (int32_t)PL.flailing[idx]; break;
                case 85: v = PL.timer; break;
                case 86: v = (int32_t)PL.tileCollisions; break;
                case 87: v = (int32_t)PL.objectInteraction; break;
                case 88: v = (int32_t)PL.visible; break;
                case 89: v = PL.objectPtr->rotation; break;
                case 90: v = PL.objectPtr->scale; break;
                case 91: v = (int32_t)PL.objectPtr->priority; break;
                case 92: v = (int32_t)PL.objectPtr->drawOrder; break;
                case 93: v = (int32_t)PL.objectPtr->direction; break;
                case 94: v = (int32_t)PL.objectPtr->inkEffect; break;
                case 95: v = (int32_t)PL.objectPtr->alpha; break;
                case 96: v = (int32_t)PL.objectPtr->frame; break;
                case 97: v = (int32_t)PL.objectPtr->animation; break;
                case 98: v = (int32_t)PL.objectPtr->prevAnimation; break;
                case 99: v = PL.objectPtr->animationSpeed; break;
                case 100: v = PL.objectPtr->animationTimer; break;
                case 101: case 102: case 103: case 104:
                case 105: case 106: case 107: case 108:
                    v = PL.objectPtr->value[var-101]; break;
                case 109: case 110: case 111: case 112:
                case 113: case 114: case 115: case 116:
                    v = PL.value[var-109]; break;
                case 117: {
                    int32_t ex=PL.objectPtr->xPos>>16;
                    if (ex>StageSystem::xScrollOffset-GlobalAppDefinitions::OBJECT_BORDER_X1&&ex<StageSystem::xScrollOffset+GlobalAppDefinitions::OBJECT_BORDER_X2) {
                        int32_t ey=PL.objectPtr->yPos>>16;
                        v=(ey<=StageSystem::yScrollOffset-256||ey>=StageSystem::yScrollOffset+496)?1:0;
                    } else v=1;
                    break;
                }
                case 118: v=(int32_t)StageSystem::stageMode; break;
                case 119: v=(int32_t)FileIO::activeStageList; break;
                case 120: v=StageSystem::stageListPosition; break;
                case 121: v=(int32_t)StageSystem::timeEnabled; break;
                case 122: v=(int32_t)StageSystem::milliSeconds; break;
                case 123: v=(int32_t)StageSystem::seconds; break;
                case 124: v=(int32_t)StageSystem::minutes; break;
                case 125: v=FileIO::actNumber; break;
                case 126: v=(int32_t)StageSystem::pauseEnabled; break;
                case 127: switch(FileIO::activeStageList){case 0:v=FileIO::noPresentationStages;break;case 1:v=FileIO::noZoneStages;break;case 2:v=FileIO::noBonusStages;break;case 3:v=FileIO::noSpecialStages;break;} break;
                case 128: v=StageSystem::newXBoundary1; break;
                case 129: v=StageSystem::newXBoundary2; break;
                case 130: v=StageSystem::newYBoundary1; break;
                case 131: v=StageSystem::newYBoundary2; break;
                case 132: v=StageSystem::xBoundary1; break;
                case 133: v=StageSystem::xBoundary2; break;
                case 134: v=StageSystem::yBoundary1; break;
                case 135: v=StageSystem::yBoundary2; break;
                case 136: v=StageSystem::bgDeformationData0[idx]; break;
                case 137: v=StageSystem::bgDeformationData1[idx]; break;
                case 138: v=StageSystem::bgDeformationData2[idx]; break;
                case 139: v=StageSystem::bgDeformationData3[idx]; break;
                case 140: v=StageSystem::waterLevel; break;
                case 141: v=(int32_t)StageSystem::activeTileLayers[idx]; break;
                case 142: v=(int32_t)StageSystem::tLayerMidPoint; break;
                case 143: v=(int32_t)PlayerSystem::playerMenuNum; break;
                case 144: v=playerNum; break;
                case 145: v=(int32_t)StageSystem::cameraEnabled; break;
                case 146: v=(int32_t)StageSystem::cameraTarget; break;
                case 147: v=(int32_t)StageSystem::cameraStyle; break;
                case 148: v=objectDrawOrderList[idx].listSize; break;
                case 149: v=GlobalAppDefinitions::SCREEN_CENTER; break;
                case 150: v=120; break;
                case 151: v=GlobalAppDefinitions::SCREEN_XSIZE; break;
                case 152: v=240; break;
                case 153: v=StageSystem::xScrollOffset; break;
                case 154: v=StageSystem::yScrollOffset; break;
                case 155: v=StageSystem::screenShakeX; break;
                case 156: v=StageSystem::screenShakeY; break;
                case 157: v=StageSystem::cameraAdjustY; break;
                case 158: v=(int32_t)StageSystem::gKeyDown.touchDown[idx]; break;
                case 159: v=StageSystem::gKeyDown.touchX[idx]; break;
                case 160: v=StageSystem::gKeyDown.touchY[idx]; break;
                case 161: v=AudioPlayback::musicVolume; break;
                case 162: v=AudioPlayback::currentMusicTrack; break;
                case 163: v=(int32_t)StageSystem::gKeyDown.up; break;
                case 164: v=(int32_t)StageSystem::gKeyDown.down; break;
                case 165: v=(int32_t)StageSystem::gKeyDown.left; break;
                case 166: v=(int32_t)StageSystem::gKeyDown.right; break;
                case 167: v=(int32_t)StageSystem::gKeyDown.buttonA; break;
                case 168: v=(int32_t)StageSystem::gKeyDown.buttonB; break;
                case 169: v=(int32_t)StageSystem::gKeyDown.buttonC; break;
                case 170: v=(int32_t)StageSystem::gKeyDown.start; break;
                case 171: v=(int32_t)StageSystem::gKeyPress.up; break;
                case 172: v=(int32_t)StageSystem::gKeyPress.down; break;
                case 173: v=(int32_t)StageSystem::gKeyPress.left; break;
                case 174: v=(int32_t)StageSystem::gKeyPress.right; break;
                case 175: v=(int32_t)StageSystem::gKeyPress.buttonA; break;
                case 176: v=(int32_t)StageSystem::gKeyPress.buttonB; break;
                case 177: v=(int32_t)StageSystem::gKeyPress.buttonC; break;
                case 178: v=(int32_t)StageSystem::gKeyPress.start; break;
                case 179: v=StageSystem::gameMenu[0].selection1; break;
                case 180: v=StageSystem::gameMenu[1].selection1; break;
                case 181: v=(int32_t)StageSystem::stageLayouts[idx].xSize; break;
                case 182: v=(int32_t)StageSystem::stageLayouts[idx].ySize; break;
                case 183: v=(int32_t)StageSystem::stageLayouts[idx].type; break;
                case 184: v=StageSystem::stageLayouts[idx].angle; break;
                case 185: v=StageSystem::stageLayouts[idx].xPos; break;
                case 186: v=StageSystem::stageLayouts[idx].yPos; break;
                case 187: v=StageSystem::stageLayouts[idx].zPos; break;
                case 188: v=StageSystem::stageLayouts[idx].parallaxFactor; break;
                case 189: v=StageSystem::stageLayouts[idx].scrollSpeed; break;
                case 190: v=StageSystem::stageLayouts[idx].scrollPosition; break;
                case 191: v=StageSystem::stageLayouts[idx].deformationPos; break;
                case 192: v=StageSystem::stageLayouts[idx].deformationPosW; break;
                case 193: v=StageSystem::hParallax.parallaxFactor[idx]; break;
                case 194: v=StageSystem::hParallax.scrollSpeed[idx]; break;
                case 195: v=StageSystem::hParallax.scrollPosition[idx]; break;
                case 196: v=StageSystem::vParallax.parallaxFactor[idx]; break;
                case 197: v=StageSystem::vParallax.scrollSpeed[idx]; break;
                case 198: v=StageSystem::vParallax.scrollPosition[idx]; break;
                case 199: v=Scene3D::numVertices; break;
                case 200: v=Scene3D::numFaces; break;
                case 201: v=Scene3D::vertexBuffer[idx].x; break;
                case 202: v=Scene3D::vertexBuffer[idx].y; break;
                case 203: v=Scene3D::vertexBuffer[idx].z; break;
                case 204: v=Scene3D::vertexBuffer[idx].u; break;
                case 205: v=Scene3D::vertexBuffer[idx].v; break;
                case 206: v=Scene3D::indexBuffer[idx].a; break;
                case 207: v=Scene3D::indexBuffer[idx].b; break;
                case 208: v=Scene3D::indexBuffer[idx].c; break;
                case 209: v=Scene3D::indexBuffer[idx].d; break;
                case 210: v=(int32_t)Scene3D::indexBuffer[idx].flag; break;
                case 211: v=Scene3D::indexBuffer[idx].color; break;
                case 212: v=Scene3D::projectionX; break;
                case 213: v=Scene3D::projectionY; break;
                case 214: v=(int32_t)GlobalAppDefinitions::gameMode; break;
                case 215: v=(int32_t)StageSystem::debugMode; break;
                case 216: v=GlobalAppDefinitions::gameMessage; break;
                case 217: v=FileIO::saveRAM[idx]; break;
                case 218: v=(int32_t)GlobalAppDefinitions::gameLanguage; break;
                case 219: v=(int32_t)objectScriptList[(int32_t)OE.type].surfaceNum; break;
                case 220: v=(int32_t)GlobalAppDefinitions::gameOnlineActive; break;
                case 221: v=GlobalAppDefinitions::frameSkipTimer; break;
                case 222: v=GlobalAppDefinitions::frameSkipSetting; break;
                case 223: v=GlobalAppDefinitions::gameSFXVolume; break;
                case 224: v=GlobalAppDefinitions::gameBGMVolume; break;
                case 225: v=GlobalAppDefinitions::gamePlatformID; break;
                case 226: v=(int32_t)GlobalAppDefinitions::gameTrialMode; break;
                case 227: v=(int32_t)StageSystem::gKeyPress.start; break;
                case 228: v=(int32_t)GlobalAppDefinitions::gameHapticsEnabled; break;
                }
                scriptEng.operands[arg] = v;
                break;
            }
            case 2: {
                ++scp;
                scriptEng.operands[arg] = sd[scp++];
                nb += 2;
                break;
            }
            case 3: {
                ++scp;
                int32_t slen = sd[scp];
                scriptText[slen] = '\0';
                int slot = 0;
                for (int ci = 0; ci < slen; ++ci) {
                    switch (slot) {
                    case 0: ++scp; scriptText[ci]=(char)(sd[scp]>>24); slot=1; break;
                    case 1: scriptText[ci]=(char)((sd[scp]>>16)&0xFF); slot=2; break;
                    case 2: scriptText[ci]=(char)((sd[scp]>>8)&0xFF);  slot=3; break;
                    case 3: scriptText[ci]=(char)(sd[scp]&0xFF);        slot=0; break;
                    }
                }
                if (slot==0) scp+=2; else ++scp;
                break;
            }
            }
        }
        nb = scp - scpPreOps;

#define OP0(x) case x: sz=0;
#define JT0 scp=base+jumpTableData[jtp+
        switch (op) {
        case 0: return;
        case 1:  scriptEng.operands[0]=scriptEng.operands[1]; break;
        case 2:  scriptEng.operands[0]+=scriptEng.operands[1]; break;
        case 3:  scriptEng.operands[0]-=scriptEng.operands[1]; break;
        case 4:  ++scriptEng.operands[0]; break;
        case 5:  --scriptEng.operands[0]; break;
        case 6:  scriptEng.operands[0]*=scriptEng.operands[1]; break;
        case 7:  if(scriptEng.operands[1]) scriptEng.operands[0]/=scriptEng.operands[1]; break;
        case 8:  scriptEng.operands[0]>>=scriptEng.operands[1]; break;
        case 9:  scriptEng.operands[0]<<=scriptEng.operands[1]; break;
        case 10: scriptEng.operands[0]&=scriptEng.operands[1]; break;
        case 11: scriptEng.operands[0]|=scriptEng.operands[1]; break;
        case 12: scriptEng.operands[0]^=scriptEng.operands[1]; break;
        case 13: if(scriptEng.operands[1]) scriptEng.operands[0]%=scriptEng.operands[1]; break;
        case 14: scriptEng.operands[0]=-scriptEng.operands[0]; break;
        case 15: scriptEng.checkResult=(scriptEng.operands[0]==scriptEng.operands[1])?1:0; sz=0; break;
        case 16: scriptEng.checkResult=(scriptEng.operands[0]> scriptEng.operands[1])?1:0; sz=0; break;
        case 17: scriptEng.checkResult=(scriptEng.operands[0]>=scriptEng.operands[1])?1:0; sz=0; break;
        case 18: scriptEng.checkResult=(scriptEng.operands[0]!=scriptEng.operands[1])?1:0; sz=0; break;
        case 19: sz=0; if(scriptEng.operands[1]==scriptEng.operands[2]){++jumpTableStackPos;jumpTableStack[jumpTableStackPos]=scriptEng.operands[0];}else{JT0 scriptEng.operands[0]];++jumpTableStackPos;jumpTableStack[jumpTableStackPos]=scriptEng.operands[0];} break;
        case 20: sz=0; if(scriptEng.operands[1]> scriptEng.operands[2]){++jumpTableStackPos;jumpTableStack[jumpTableStackPos]=scriptEng.operands[0];}else{JT0 scriptEng.operands[0]];++jumpTableStackPos;jumpTableStack[jumpTableStackPos]=scriptEng.operands[0];} break;
        case 21: sz=0; if(scriptEng.operands[1]>=scriptEng.operands[2]){++jumpTableStackPos;jumpTableStack[jumpTableStackPos]=scriptEng.operands[0];}else{JT0 scriptEng.operands[0]];++jumpTableStackPos;jumpTableStack[jumpTableStackPos]=scriptEng.operands[0];} break;
        case 22: sz=0; if(scriptEng.operands[1]< scriptEng.operands[2]){++jumpTableStackPos;jumpTableStack[jumpTableStackPos]=scriptEng.operands[0];}else{JT0 scriptEng.operands[0]];++jumpTableStackPos;jumpTableStack[jumpTableStackPos]=scriptEng.operands[0];} break;
        case 23: sz=0; if(scriptEng.operands[1]<=scriptEng.operands[2]){++jumpTableStackPos;jumpTableStack[jumpTableStackPos]=scriptEng.operands[0];}else{JT0 scriptEng.operands[0]];++jumpTableStackPos;jumpTableStack[jumpTableStackPos]=scriptEng.operands[0];} break;
        case 24: sz=0; if(scriptEng.operands[1]!=scriptEng.operands[2]){++jumpTableStackPos;jumpTableStack[jumpTableStackPos]=scriptEng.operands[0];}else{JT0 scriptEng.operands[0]];++jumpTableStackPos;jumpTableStack[jumpTableStackPos]=scriptEng.operands[0];} break;
        case 25: sz=0; JT0 jumpTableStack[jumpTableStackPos]+1]; --jumpTableStackPos; break;
        case 26: sz=0; --jumpTableStackPos; break;
        case 27: sz=0; if(scriptEng.operands[1]==scriptEng.operands[2]){++jumpTableStackPos;jumpTableStack[jumpTableStackPos]=scriptEng.operands[0];}else{JT0 scriptEng.operands[0]+1];} break;
        case 28: sz=0; if(scriptEng.operands[1]> scriptEng.operands[2]){++jumpTableStackPos;jumpTableStack[jumpTableStackPos]=scriptEng.operands[0];}else{JT0 scriptEng.operands[0]+1];} break;
        case 29: sz=0; if(scriptEng.operands[1]>=scriptEng.operands[2]){++jumpTableStackPos;jumpTableStack[jumpTableStackPos]=scriptEng.operands[0];}else{JT0 scriptEng.operands[0]+1];} break;
        case 30: sz=0; if(scriptEng.operands[1]< scriptEng.operands[2]){++jumpTableStackPos;jumpTableStack[jumpTableStackPos]=scriptEng.operands[0];}else{JT0 scriptEng.operands[0]+1];} break;
        case 31: sz=0; if(scriptEng.operands[1]<=scriptEng.operands[2]){++jumpTableStackPos;jumpTableStack[jumpTableStackPos]=scriptEng.operands[0];}else{JT0 scriptEng.operands[0]+1];} break;
        case 32: sz=0; if(scriptEng.operands[1]!=scriptEng.operands[2]){++jumpTableStackPos;jumpTableStack[jumpTableStackPos]=scriptEng.operands[0];}else{JT0 scriptEng.operands[0]+1];} break;
        case 33: sz=0; JT0 jumpTableStack[jumpTableStackPos]]; --jumpTableStackPos; break;
        case 34: sz=0; ++jumpTableStackPos; jumpTableStack[jumpTableStackPos]=scriptEng.operands[0]; if(scriptEng.operands[1]>=jumpTableData[jtp+scriptEng.operands[0]]&&scriptEng.operands[1]<=jumpTableData[jtp+scriptEng.operands[0]+1]){JT0 scriptEng.operands[0]+4+(scriptEng.operands[1]-jumpTableData[jtp+scriptEng.operands[0]])];}else{JT0 scriptEng.operands[0]+2];} break;
        case 35: sz=0; JT0 jumpTableStack[jumpTableStackPos]+3]; --jumpTableStackPos; break;
        case 36: sz=0; --jumpTableStackPos; break;
        case 37: scriptEng.operands[0]=scriptEng.operands[1]?(rand()%scriptEng.operands[1]):0; break;
        case 38: { int32_t a=((scriptEng.operands[1]<0)?(512-scriptEng.operands[1]):scriptEng.operands[1])&511; scriptEng.operands[0]=GlobalAppDefinitions::SinValue512[a]; break; }
        case 39: { int32_t a=((scriptEng.operands[1]<0)?(512-scriptEng.operands[1]):scriptEng.operands[1])&511; scriptEng.operands[0]=GlobalAppDefinitions::CosValue512[a]; break; }
        case 40: { int32_t a=((scriptEng.operands[1]<0)?(256-scriptEng.operands[1]):scriptEng.operands[1])&255; scriptEng.operands[0]=GlobalAppDefinitions::SinValue256[a]; break; }
        case 41: { int32_t a=((scriptEng.operands[1]<0)?(256-scriptEng.operands[1]):scriptEng.operands[1])&255; scriptEng.operands[0]=GlobalAppDefinitions::CosValue256[a]; break; }
        case 42: { int32_t a=((scriptEng.operands[1]<0)?(512-scriptEng.operands[1]):scriptEng.operands[1])&511; scriptEng.operands[0]=(GlobalAppDefinitions::SinValue512[a]>>scriptEng.operands[2])+scriptEng.operands[3]-scriptEng.operands[4]; break; }
        case 43: { int32_t a=((scriptEng.operands[1]<0)?(512-scriptEng.operands[1]):scriptEng.operands[1])&511; scriptEng.operands[0]=(GlobalAppDefinitions::CosValue512[a]>>scriptEng.operands[2])+scriptEng.operands[3]-scriptEng.operands[4]; break; }
        case 44: scriptEng.operands[0]=(int32_t)GlobalAppDefinitions::ArcTanLookup(scriptEng.operands[1],scriptEng.operands[2]); break;
        case 45: scriptEng.operands[0]=(scriptEng.operands[1]*scriptEng.operands[3]+scriptEng.operands[2]*(256-scriptEng.operands[3]))>>8; break;
        case 46: scriptEng.operands[0]=(scriptEng.operands[2]*scriptEng.operands[6]>>8)+(scriptEng.operands[3]*(256-scriptEng.operands[6])>>8); scriptEng.operands[1]=(scriptEng.operands[4]*scriptEng.operands[6]>>8)+(scriptEng.operands[5]*(256-scriptEng.operands[6])>>8); break;
        OP0(47) OSL.surfaceNum=GraphicsSystem::AddGraphicsFile(scriptText); break;
        OP0(48) GraphicsSystem::RemoveGraphicsFile(scriptText,-1); break;
        OP0(49) { auto& f=scriptFrames[OSL.frameListOffset+scriptEng.operands[0]]; {static int s_d49=0; if(++s_d49<=30) Log::Info("op49 type=%d surN=%d fIdx=%d xPos=%d yPos=%d xSz=%d ySz=%d l=%d t=%d texX=%d texY=%d",(int)OE.type,(int)OSL.surfaceNum,scriptEng.operands[0],(OE.xPos>>16)-StageSystem::xScrollOffset+f.xPivot,(OE.yPos>>16)-StageSystem::yScrollOffset+f.yPivot,f.xSize,f.ySize,f.left,f.top,GraphicsSystem::gfxSurface[(int)OSL.surfaceNum].texStartX,GraphicsSystem::gfxSurface[(int)OSL.surfaceNum].texStartY);} GraphicsSystem::DrawSprite((OE.xPos>>16)-StageSystem::xScrollOffset+f.xPivot,(OE.yPos>>16)-StageSystem::yScrollOffset+f.yPivot,f.xSize,f.ySize,f.left,f.top,(int32_t)OSL.surfaceNum); break; }
        OP0(50) { auto& f=scriptFrames[OSL.frameListOffset+scriptEng.operands[0]]; GraphicsSystem::DrawSprite((scriptEng.operands[1]>>16)-StageSystem::xScrollOffset+f.xPivot,(scriptEng.operands[2]>>16)-StageSystem::yScrollOffset+f.yPivot,f.xSize,f.ySize,f.left,f.top,(int32_t)OSL.surfaceNum); break; }
        OP0(51) { auto& f=scriptFrames[OSL.frameListOffset+scriptEng.operands[0]]; GraphicsSystem::DrawSprite(scriptEng.operands[1]+f.xPivot,scriptEng.operands[2]+f.yPivot,f.xSize,f.ySize,f.left,f.top,(int32_t)OSL.surfaceNum); break; }
        OP0(52) GraphicsSystem::DrawTintRectangle(scriptEng.operands[0],scriptEng.operands[1],scriptEng.operands[2],scriptEng.operands[3]); break;
        OP0(53) {
            int32_t pw=10, lim=scriptEng.operands[3]?scriptEng.operands[3]*10:10;
            if(!scriptEng.operands[6]) { while(scriptEng.operands[4]>0){if(lim>=pw){int32_t d=(scriptEng.operands[3]-scriptEng.operands[3]/pw*pw)/(pw/10)+scriptEng.operands[0]; auto& f=scriptFrames[OSL.frameListOffset+d]; GraphicsSystem::DrawSprite(scriptEng.operands[1]+f.xPivot,scriptEng.operands[2]+f.yPivot,f.xSize,f.ySize,f.left,f.top,(int32_t)OSL.surfaceNum);}scriptEng.operands[1]-=scriptEng.operands[5];pw*=10;--scriptEng.operands[4];}}
            else { while(scriptEng.operands[4]>0){int32_t d=(scriptEng.operands[3]-scriptEng.operands[3]/pw*pw)/(pw/10)+scriptEng.operands[0]; auto& f=scriptFrames[OSL.frameListOffset+d]; GraphicsSystem::DrawSprite(scriptEng.operands[1]+f.xPivot,scriptEng.operands[2]+f.yPivot,f.xSize,f.ySize,f.left,f.top,(int32_t)OSL.surfaceNum);scriptEng.operands[1]-=scriptEng.operands[5];pw*=10;--scriptEng.operands[4];}}
            break;
        }
        OP0(54) {
            auto tcDraw=[&](int32_t si){int32_t c=(int32_t)StageSystem::titleCardText[si];if(c==32||c==45)c=0;else if(c>47&&c<58)c-=22;else if(c>57&&c<102)c-=65;if(c>-1){c+=scriptEng.operands[0];auto& f=scriptFrames[OSL.frameListOffset+c];GraphicsSystem::DrawSprite(scriptEng.operands[1]+f.xPivot,scriptEng.operands[2]+f.yPivot,f.xSize,f.ySize,f.left,f.top,(int32_t)OSL.surfaceNum);scriptEng.operands[1]+=f.xSize+scriptEng.operands[6];}else scriptEng.operands[1]+=scriptEng.operands[5]+scriptEng.operands[6];};
            switch(scriptEng.operands[3]){
            case 1:{int32_t si=0;if(scriptEng.operands[4]==1&&StageSystem::titleCardText[si]!='\0'){tcDraw(si);scriptEng.operands[0]+=26;++si;}for(;StageSystem::titleCardText[si]!='\0'&&StageSystem::titleCardText[si]!='-';++si)tcDraw(si);break;}
            case 2:{int32_t si=(int32_t)StageSystem::titleCardWord2;if(scriptEng.operands[4]==1&&StageSystem::titleCardText[si]!='\0'){tcDraw(si);scriptEng.operands[0]+=26;++si;}for(;StageSystem::titleCardText[si]!='\0';++si)tcDraw(si);break;}
            }break;
        }
        OP0(55) TextSystem::textMenuSurfaceNo=(int32_t)OSL.surfaceNum; TextSystem::DrawTextMenu(StageSystem::gameMenu[scriptEng.operands[0]],scriptEng.operands[1],scriptEng.operands[2]); break;
        OP0(56) if(sub==3&&scriptFramesNo<4096){auto& f=scriptFrames[scriptFramesNo];f.xPivot=scriptEng.operands[0];f.yPivot=scriptEng.operands[1];f.xSize=scriptEng.operands[2];f.ySize=scriptEng.operands[3];f.left=scriptEng.operands[4];f.top=scriptEng.operands[5];++scriptFramesNo;} break;
        OP0(57) break;
        OP0(58) GraphicsSystem::LoadPalette(scriptText,scriptEng.operands[1],scriptEng.operands[2],scriptEng.operands[3],scriptEng.operands[4]); break;
        OP0(59) GraphicsSystem::RotatePalette((uint8_t)scriptEng.operands[0],(uint8_t)scriptEng.operands[1],(uint8_t)scriptEng.operands[2]); break;
        OP0(60) GraphicsSystem::SetFade((uint8_t)scriptEng.operands[0],(uint8_t)scriptEng.operands[1],(uint8_t)scriptEng.operands[2],(uint16_t)scriptEng.operands[3]); break;
        OP0(61) GraphicsSystem::SetActivePalette((uint8_t)scriptEng.operands[0],scriptEng.operands[1],scriptEng.operands[2]); break;
        case 62: GraphicsSystem::SetLimitedFade((uint8_t)scriptEng.operands[0],(uint8_t)scriptEng.operands[1],(uint8_t)scriptEng.operands[2],(uint8_t)scriptEng.operands[3],(uint16_t)scriptEng.operands[4],scriptEng.operands[5],scriptEng.operands[6]); break;
        OP0(63) GraphicsSystem::CopyPalette((uint8_t)scriptEng.operands[0],(uint8_t)scriptEng.operands[1]); break;
        OP0(64) GraphicsSystem::ClearScreen((uint8_t)scriptEng.operands[0]); break;
        OP0(65) {
            auto& e=OE; auto fr=[&]()->SpriteFrame&{return scriptFrames[OSL.frameListOffset+scriptEng.operands[0]];};
            switch(scriptEng.operands[1]){
            case 0:{auto&f=fr();GraphicsSystem::DrawScaledSprite(e.direction,(scriptEng.operands[2]>>16)-StageSystem::xScrollOffset,(scriptEng.operands[3]>>16)-StageSystem::yScrollOffset,-f.xPivot,-f.yPivot,e.scale,e.scale,f.xSize,f.ySize,f.left,f.top,(int32_t)OSL.surfaceNum);break;}
            case 1:{auto&f=fr();GraphicsSystem::DrawRotatedSprite(e.direction,(scriptEng.operands[2]>>16)-StageSystem::xScrollOffset,(scriptEng.operands[3]>>16)-StageSystem::yScrollOffset,-f.xPivot,-f.yPivot,f.left,f.top,f.xSize,f.ySize,e.rotation,(int32_t)OSL.surfaceNum);break;}
            case 2:{auto&f=fr();GraphicsSystem::DrawRotoZoomSprite(e.direction,(scriptEng.operands[2]>>16)-StageSystem::xScrollOffset,(scriptEng.operands[3]>>16)-StageSystem::yScrollOffset,-f.xPivot,-f.yPivot,f.left,f.top,f.xSize,f.ySize,e.rotation,e.scale,(int32_t)OSL.surfaceNum);break;}
            case 3:{auto&f=fr();int32_t bx=(scriptEng.operands[2]>>16)-StageSystem::xScrollOffset+f.xPivot,by=(scriptEng.operands[3]>>16)-StageSystem::yScrollOffset+f.yPivot;{static int s_d65=0;if(++s_d65<=20)Log::Info("op65c3 type=%d surN=%d fIdx=%d pos=(%d,%d) sz=%dx%d uv=(%d,%d) ink=%d texXY=(%d,%d)",(int)e.type,(int)OSL.surfaceNum,scriptEng.operands[0],bx,by,f.xSize,f.ySize,f.left,f.top,(int)e.inkEffect,GraphicsSystem::gfxSurface[(int)OSL.surfaceNum].texStartX,GraphicsSystem::gfxSurface[(int)OSL.surfaceNum].texStartY);}switch(e.inkEffect){case 0:GraphicsSystem::DrawSprite(bx,by,f.xSize,f.ySize,f.left,f.top,(int32_t)OSL.surfaceNum);break;case 1:GraphicsSystem::DrawBlendedSprite(bx,by,f.xSize,f.ySize,f.left,f.top,(int32_t)OSL.surfaceNum);break;case 2:GraphicsSystem::DrawAlphaBlendedSprite(bx,by,f.xSize,f.ySize,f.left,f.top,(int32_t)e.alpha,(int32_t)OSL.surfaceNum);break;case 3:GraphicsSystem::DrawAdditiveBlendedSprite(bx,by,f.xSize,f.ySize,f.left,f.top,(int32_t)e.alpha,(int32_t)OSL.surfaceNum);break;case 4:GraphicsSystem::DrawSubtractiveBlendedSprite(bx,by,f.xSize,f.ySize,f.left,f.top,(int32_t)e.alpha,(int32_t)OSL.surfaceNum);break;}break;}
            case 4:{auto&f=fr();if(e.inkEffect!=2)GraphicsSystem::DrawScaledSprite(e.direction,(scriptEng.operands[2]>>16)-StageSystem::xScrollOffset,(scriptEng.operands[3]>>16)-StageSystem::yScrollOffset,-f.xPivot,-f.yPivot,e.scale,e.scale,f.xSize,f.ySize,f.left,f.top,(int32_t)OSL.surfaceNum);else GraphicsSystem::DrawScaledTintMask(e.direction,(scriptEng.operands[2]>>16)-StageSystem::xScrollOffset,(scriptEng.operands[3]>>16)-StageSystem::yScrollOffset,-f.xPivot,-f.yPivot,e.scale,e.scale,f.xSize,f.ySize,f.left,f.top,(int32_t)OSL.surfaceNum);break;}
            case 5:{auto&f=fr();int32_t bx=(scriptEng.operands[2]>>16)-StageSystem::xScrollOffset,by=(scriptEng.operands[3]>>16)-StageSystem::yScrollOffset;switch(e.direction){case 0:GraphicsSystem::DrawSpriteFlipped(bx+f.xPivot,by+f.yPivot,f.xSize,f.ySize,f.left,f.top,0,(int32_t)OSL.surfaceNum);break;case 1:GraphicsSystem::DrawSpriteFlipped(bx-f.xSize-f.xPivot,by+f.yPivot,f.xSize,f.ySize,f.left,f.top,1,(int32_t)OSL.surfaceNum);break;case 2:GraphicsSystem::DrawSpriteFlipped(bx+f.xPivot,by-f.ySize-f.yPivot,f.xSize,f.ySize,f.left,f.top,2,(int32_t)OSL.surfaceNum);break;case 3:GraphicsSystem::DrawSpriteFlipped(bx-f.xSize-f.xPivot,by-f.ySize-f.yPivot,f.xSize,f.ySize,f.left,f.top,3,(int32_t)OSL.surfaceNum);break;}break;}
            }break;
        }
        OP0(66) {
            auto& e=OE; auto fr=[&]()->SpriteFrame&{return scriptFrames[OSL.frameListOffset+scriptEng.operands[0]];};
            switch(scriptEng.operands[1]){
            case 0:{auto&f=fr();GraphicsSystem::DrawScaledSprite(e.direction,scriptEng.operands[2],scriptEng.operands[3],-f.xPivot,-f.yPivot,e.scale,e.scale,f.xSize,f.ySize,f.left,f.top,(int32_t)OSL.surfaceNum);break;}
            case 1:{auto&f=fr();GraphicsSystem::DrawRotatedSprite(e.direction,scriptEng.operands[2],scriptEng.operands[3],-f.xPivot,-f.yPivot,f.left,f.top,f.xSize,f.ySize,e.rotation,(int32_t)OSL.surfaceNum);break;}
            case 2:{auto&f=fr();GraphicsSystem::DrawRotoZoomSprite(e.direction,scriptEng.operands[2],scriptEng.operands[3],-f.xPivot,-f.yPivot,f.left,f.top,f.xSize,f.ySize,e.rotation,e.scale,(int32_t)OSL.surfaceNum);break;}
            case 3:{auto&f=fr();int32_t bx=scriptEng.operands[2]+f.xPivot,by=scriptEng.operands[3]+f.yPivot;switch(e.inkEffect){case 0:GraphicsSystem::DrawSprite(bx,by,f.xSize,f.ySize,f.left,f.top,(int32_t)OSL.surfaceNum);break;case 1:GraphicsSystem::DrawBlendedSprite(bx,by,f.xSize,f.ySize,f.left,f.top,(int32_t)OSL.surfaceNum);break;case 2:GraphicsSystem::DrawAlphaBlendedSprite(bx,by,f.xSize,f.ySize,f.left,f.top,(int32_t)e.alpha,(int32_t)OSL.surfaceNum);break;case 3:GraphicsSystem::DrawAdditiveBlendedSprite(bx,by,f.xSize,f.ySize,f.left,f.top,(int32_t)e.alpha,(int32_t)OSL.surfaceNum);break;case 4:GraphicsSystem::DrawSubtractiveBlendedSprite(bx,by,f.xSize,f.ySize,f.left,f.top,(int32_t)e.alpha,(int32_t)OSL.surfaceNum);break;}break;}
            case 4:{auto&f=fr();if(e.inkEffect!=2)GraphicsSystem::DrawScaledSprite(e.direction,scriptEng.operands[2],scriptEng.operands[3],-f.xPivot,-f.yPivot,e.scale,e.scale,f.xSize,f.ySize,f.left,f.top,(int32_t)OSL.surfaceNum);else GraphicsSystem::DrawScaledTintMask(e.direction,scriptEng.operands[2],scriptEng.operands[3],-f.xPivot,-f.yPivot,e.scale,e.scale,f.xSize,f.ySize,f.left,f.top,(int32_t)OSL.surfaceNum);break;}
            case 5:{auto&f=fr();switch(e.direction){case 0:GraphicsSystem::DrawSpriteFlipped(scriptEng.operands[2]+f.xPivot,scriptEng.operands[3]+f.yPivot,f.xSize,f.ySize,f.left,f.top,0,(int32_t)OSL.surfaceNum);break;case 1:GraphicsSystem::DrawSpriteFlipped(scriptEng.operands[2]-f.xSize-f.xPivot,scriptEng.operands[3]+f.yPivot,f.xSize,f.ySize,f.left,f.top,1,(int32_t)OSL.surfaceNum);break;case 2:GraphicsSystem::DrawSpriteFlipped(scriptEng.operands[2]+f.xPivot,scriptEng.operands[3]-f.ySize-f.yPivot,f.xSize,f.ySize,f.left,f.top,2,(int32_t)OSL.surfaceNum);break;case 3:GraphicsSystem::DrawSpriteFlipped(scriptEng.operands[2]-f.xSize-f.xPivot,scriptEng.operands[3]-f.ySize-f.yPivot,f.xSize,f.ySize,f.left,f.top,3,(int32_t)OSL.surfaceNum);break;}break;}
            }break;
        }
        OP0(67) OSL.animationFile=AnimationSystem::AddAnimationFile(scriptText); break;
        OP0(68) TextSystem::SetupTextMenu(StageSystem::gameMenu[scriptEng.operands[0]],scriptEng.operands[1]); StageSystem::gameMenu[scriptEng.operands[0]].numSelections=(uint8_t)scriptEng.operands[2]; StageSystem::gameMenu[scriptEng.operands[0]].alignment=(uint8_t)scriptEng.operands[3]; break;
        OP0(69) StageSystem::gameMenu[scriptEng.operands[0]].entryHighlight[(int32_t)StageSystem::gameMenu[scriptEng.operands[0]].numRows]=(uint8_t)scriptEng.operands[2]; TextSystem::AddTextMenuEntry(StageSystem::gameMenu[scriptEng.operands[0]],scriptText); break;
        OP0(70) TextSystem::EditTextMenuEntry(StageSystem::gameMenu[scriptEng.operands[0]],scriptText,scriptEng.operands[2]); StageSystem::gameMenu[scriptEng.operands[0]].entryHighlight[scriptEng.operands[2]]=(uint8_t)scriptEng.operands[3]; break;
        OP0(71) StageSystem::stageMode=0; break;
        OP0(72) GraphicsSystem::DrawRectangle(scriptEng.operands[0],scriptEng.operands[1],scriptEng.operands[2],scriptEng.operands[3],scriptEng.operands[4],scriptEng.operands[5],scriptEng.operands[6],scriptEng.operands[7]); break;
        OP0(73) { auto& e=objectEntityList[scriptEng.operands[0]]; e.type=(uint8_t)scriptEng.operands[1];e.propertyValue=(uint8_t)scriptEng.operands[2];e.xPos=scriptEng.operands[3];e.yPos=scriptEng.operands[4];e.direction=0;e.frame=0;e.priority=0;e.rotation=0;e.state=0;e.drawOrder=3;e.scale=512;e.inkEffect=0;for(int v=0;v<8;++v)e.value[v]=0; break; }
        OP0(74) {
            switch(scriptEng.operands[0]){
            case 0:{int32_t ex=OE.xPos>>16,ey=OE.yPos>>16;BasicCollision(scriptEng.operands[1]+ex,scriptEng.operands[2]+ey,scriptEng.operands[3]+ex,scriptEng.operands[4]+ey);break;}
            case 1:case 2:BoxCollision((scriptEng.operands[1]<<16)+OE.xPos,(scriptEng.operands[2]<<16)+OE.yPos,(scriptEng.operands[3]<<16)+OE.xPos,(scriptEng.operands[4]<<16)+OE.yPos);break;
            case 3:PlatformCollision((scriptEng.operands[1]<<16)+OE.xPos,(scriptEng.operands[2]<<16)+OE.yPos,(scriptEng.operands[3]<<16)+OE.xPos,(scriptEng.operands[4]<<16)+OE.yPos);break;
            }break;
        }
        OP0(75) {
            if(objectEntityList[scriptEng.arrayPosition[2]].type>0){++scriptEng.arrayPosition[2];if(scriptEng.arrayPosition[2]==1184)scriptEng.arrayPosition[2]=1056;}
            auto& e=objectEntityList[scriptEng.arrayPosition[2]];
            e.type=(uint8_t)scriptEng.operands[0];e.propertyValue=(uint8_t)scriptEng.operands[1];e.xPos=scriptEng.operands[2];e.yPos=scriptEng.operands[3];
            e.direction=0;e.frame=0;e.priority=1;e.rotation=0;e.state=0;e.drawOrder=3;e.scale=512;e.inkEffect=0;e.alpha=0;e.animation=0;e.prevAnimation=0;e.animationSpeed=0;e.animationTimer=0;
            for(int v=0;v<8;++v)e.value[v]=0;
            break;
        }
        OP0(76) PlayerSystem::playerList[scriptEng.operands[0]].animationFile=objectScriptList[(int32_t)objectEntityList[scriptEng.operands[1]].type].animationFile; PlayerSystem::playerList[scriptEng.operands[0]].objectPtr=&objectEntityList[scriptEng.operands[1]]; PlayerSystem::playerList[scriptEng.operands[0]].objectNum=scriptEng.operands[1]; break;
        OP0(77) if(PL.tileCollisions==1)PlayerSystem::ProcessPlayerTileCollisions(PL);else{PL.xPos+=PL.xVelocity;PL.yPos+=PL.yVelocity;} break;
        OP0(78) PlayerSystem::ProcessPlayerControl(PL); break;
        case 79: AnimationSystem::ProcessObjectAnimation(AnimationSystem::animationList[OSL.animationFile->aniListOffset+(int32_t)OE.animation],OE); sz=0; break;
        OP0(80) AnimationSystem::DrawObjectAnimation(AnimationSystem::animationList[OSL.animationFile->aniListOffset+(int32_t)OE.animation],OE,(OE.xPos>>16)-StageSystem::xScrollOffset,(OE.yPos>>16)-StageSystem::yScrollOffset); break;
        OP0(81) if(PL.visible==1){if((int32_t)StageSystem::cameraEnabled==playerNum)AnimationSystem::DrawObjectAnimation(AnimationSystem::animationList[OSL.animationFile->aniListOffset+(int32_t)OE.animation],OE,PL.screenXPos,PL.screenYPos);else AnimationSystem::DrawObjectAnimation(AnimationSystem::animationList[OSL.animationFile->aniListOffset+(int32_t)OE.animation],OE,(PL.xPos>>16)-StageSystem::xScrollOffset,(PL.yPos>>16)-StageSystem::yScrollOffset);} break;
        OP0(82) if(scriptEng.operands[2]>1)AudioPlayback::SetMusicTrack(scriptText,scriptEng.operands[1],1,(uint32_t)scriptEng.operands[2]);else AudioPlayback::SetMusicTrack(scriptText,scriptEng.operands[1],(uint8_t)scriptEng.operands[2],0); break;
        OP0(83) AudioPlayback::PlayMusic(scriptEng.operands[0]); break;
        OP0(84) AudioPlayback::StopMusic(); break;
        OP0(85) AudioPlayback::PlaySfx(scriptEng.operands[0],(uint8_t)scriptEng.operands[1]); break;
        OP0(86) AudioPlayback::StopSfx(scriptEng.operands[0]); break;
        OP0(87) AudioPlayback::SetSfxAttributes(scriptEng.operands[0],scriptEng.operands[1],scriptEng.operands[2]); break;
        OP0(88) switch(scriptEng.operands[0]){case 0:ObjectFloorCollision(scriptEng.operands[1],scriptEng.operands[2],scriptEng.operands[3]);break;case 1:ObjectLWallCollision(scriptEng.operands[1],scriptEng.operands[2],scriptEng.operands[3]);break;case 2:ObjectRWallCollision(scriptEng.operands[1],scriptEng.operands[2],scriptEng.operands[3]);break;case 3:ObjectRoofCollision(scriptEng.operands[1],scriptEng.operands[2],scriptEng.operands[3]);break;} break;
        OP0(89) switch(scriptEng.operands[0]){case 0:ObjectFloorGrip(scriptEng.operands[1],scriptEng.operands[2],scriptEng.operands[3]);break;case 1:ObjectLWallGrip(scriptEng.operands[1],scriptEng.operands[2],scriptEng.operands[3]);break;case 2:ObjectRWallGrip(scriptEng.operands[1],scriptEng.operands[2],scriptEng.operands[3]);break;case 3:ObjectRoofGrip(scriptEng.operands[1],scriptEng.operands[2],scriptEng.operands[3]);break;} break;
        OP0(90) AudioPlayback::PauseSound(); EngineCallbacks::PlayVideoFile(scriptText); AudioPlayback::ResumeSound(); break;
        OP0(91) break;
        OP0(92) AudioPlayback::PlaySfx(scriptEng.operands[0]+AudioPlayback::numGlobalSFX,(uint8_t)scriptEng.operands[1]); break;
        OP0(93) AudioPlayback::StopSfx(scriptEng.operands[0]+AudioPlayback::numGlobalSFX); break;
        case 94: scriptEng.operands[0]=~scriptEng.operands[0]; break;
        OP0(95) Scene3D::TransformVertexBuffer(); Scene3D::Sort3DDrawList(); Scene3D::Draw3DScene((int32_t)OSL.surfaceNum); break;
        OP0(96) switch(scriptEng.operands[0]){case 0:Scene3D::SetIdentityMatrix(Scene3D::matWorld);break;case 1:Scene3D::SetIdentityMatrix(Scene3D::matView);break;case 2:Scene3D::SetIdentityMatrix(Scene3D::matTemp);break;} break;
        OP0(97) {int32_t* d=scriptEng.operands[0]==0?Scene3D::matWorld:scriptEng.operands[0]==1?Scene3D::matView:Scene3D::matTemp; const int32_t* s=scriptEng.operands[1]==0?Scene3D::matWorld:scriptEng.operands[1]==1?Scene3D::matView:Scene3D::matTemp; Scene3D::MatrixMultiply(d,s); break;}
        OP0(98) {int32_t* m=scriptEng.operands[0]==0?Scene3D::matWorld:scriptEng.operands[0]==1?Scene3D::matView:Scene3D::matTemp; Scene3D::MatrixTranslateXYZ(m,scriptEng.operands[1],scriptEng.operands[2],scriptEng.operands[3]); break;}
        OP0(99) {int32_t* m=scriptEng.operands[0]==0?Scene3D::matWorld:scriptEng.operands[0]==1?Scene3D::matView:Scene3D::matTemp; Scene3D::MatrixScaleXYZ(m,scriptEng.operands[1],scriptEng.operands[2],scriptEng.operands[3]); break;}
        OP0(100) {int32_t* m=scriptEng.operands[0]==0?Scene3D::matWorld:scriptEng.operands[0]==1?Scene3D::matView:Scene3D::matTemp; Scene3D::MatrixRotateX(m,scriptEng.operands[1]); break;}
        OP0(101) {int32_t* m=scriptEng.operands[0]==0?Scene3D::matWorld:scriptEng.operands[0]==1?Scene3D::matView:Scene3D::matTemp; Scene3D::MatrixRotateY(m,scriptEng.operands[1]); break;}
        OP0(102) {int32_t* m=scriptEng.operands[0]==0?Scene3D::matWorld:scriptEng.operands[0]==1?Scene3D::matView:Scene3D::matTemp; Scene3D::MatrixRotateZ(m,scriptEng.operands[1]); break;}
        OP0(103) {int32_t* m=scriptEng.operands[0]==0?Scene3D::matWorld:scriptEng.operands[0]==1?Scene3D::matView:Scene3D::matTemp; Scene3D::MatrixRotateXYZ(m,scriptEng.operands[1],scriptEng.operands[2],scriptEng.operands[3]); break;}
        OP0(104) {const int32_t* m=scriptEng.operands[0]==0?Scene3D::matWorld:scriptEng.operands[0]==1?Scene3D::matView:Scene3D::matTemp; Scene3D::TransformVertices(m,scriptEng.operands[1],scriptEng.operands[2]); break;}
        OP0(105) functionStack[functionStackPos++]=scp; functionStack[functionStackPos++]=jtp; functionStack[functionStackPos++]=base; scp=functionScriptList[scriptEng.operands[0]].mainScript; base=scp; jtp=functionScriptList[scriptEng.operands[0]].mainJumpTable; break;
        OP0(106) --functionStackPos; base=functionStack[functionStackPos]; --functionStackPos; jtp=functionStack[functionStackPos]; --functionStackPos; scp=functionStack[functionStackPos]; break;
        OP0(107) StageSystem::SetLayerDeformation(scriptEng.operands[0],scriptEng.operands[1],scriptEng.operands[2],scriptEng.operands[3],scriptEng.operands[4],scriptEng.operands[5]); break;
        OP0(108) {scriptEng.checkResult=-1;for(int8_t ti=0;ti<StageSystem::gKeyDown.touches;++ti)if(StageSystem::gKeyDown.touchDown[ti]==1&&StageSystem::gKeyDown.touchX[ti]>scriptEng.operands[0]&&StageSystem::gKeyDown.touchX[ti]<scriptEng.operands[2]&&StageSystem::gKeyDown.touchY[ti]>scriptEng.operands[1]&&StageSystem::gKeyDown.touchY[ti]<scriptEng.operands[3])scriptEng.checkResult=(int32_t)ti; break;}
        case 109: scriptEng.operands[0]=(scriptEng.operands[2]<=-1||scriptEng.operands[3]<=-1)?0:(int32_t)StageSystem::stageLayouts[scriptEng.operands[1]].tileMap[scriptEng.operands[2]+(scriptEng.operands[3]<<8)]; break;
        case 110: if(scriptEng.operands[2]>-1&&scriptEng.operands[3]>-1)StageSystem::stageLayouts[scriptEng.operands[1]].tileMap[scriptEng.operands[2]+(scriptEng.operands[3]<<8)]=(uint16_t)scriptEng.operands[0]; break;
        case 111: scriptEng.operands[0]=(scriptEng.operands[1]&(1<<scriptEng.operands[2]))>>scriptEng.operands[2]; break;
        case 112: if(scriptEng.operands[2]>0)scriptEng.operands[0]|=(1<<scriptEng.operands[1]);else scriptEng.operands[0]&=~(1<<scriptEng.operands[1]); break;
        OP0(113) AudioPlayback::PauseSound(); break;
        OP0(114) AudioPlayback::ResumeSound(); break;
        OP0(115) objectDrawOrderList[scriptEng.operands[0]].listSize=0; break;
        OP0(116) objectDrawOrderList[scriptEng.operands[0]].entityRef[objectDrawOrderList[scriptEng.operands[0]].listSize]=scriptEng.operands[1]; ++objectDrawOrderList[scriptEng.operands[0]].listSize; break;
        case 117: scriptEng.operands[0]=objectDrawOrderList[scriptEng.operands[1]].entityRef[scriptEng.operands[2]]; break;
        case 118: objectDrawOrderList[scriptEng.operands[1]].entityRef[scriptEng.operands[2]]=scriptEng.operands[0]; break;
        case 119: {int32_t tx=scriptEng.operands[1]>>7,tz=scriptEng.operands[2]>>7;int32_t ti=(tx<=-1||tz<=-1)?0:((int32_t)StageSystem::stageLayouts[0].tileMap[tx+(tz<<8)]<<6);ti+=((scriptEng.operands[1]&127)>>4)+((scriptEng.operands[2]&127)>>4<<3);switch(scriptEng.operands[3]){case 0:scriptEng.operands[0]=(int32_t)StageSystem::tile128x128.tile16x16[ti];break;case 1:scriptEng.operands[0]=(int32_t)StageSystem::tile128x128.direction[ti];break;case 2:scriptEng.operands[0]=(int32_t)StageSystem::tile128x128.visualPlane[ti];break;case 3:scriptEng.operands[0]=(int32_t)StageSystem::tile128x128.collisionFlag[0][ti];break;case 4:scriptEng.operands[0]=(int32_t)StageSystem::tile128x128.collisionFlag[1][ti];break;case 5:scriptEng.operands[0]=(int32_t)StageSystem::tileCollisions[0].flags[(int32_t)StageSystem::tile128x128.tile16x16[ti]];break;case 6:scriptEng.operands[0]=(int32_t)StageSystem::tileCollisions[0].angle[(int32_t)StageSystem::tile128x128.tile16x16[ti]];break;case 7:scriptEng.operands[0]=(int32_t)StageSystem::tileCollisions[1].flags[(int32_t)StageSystem::tile128x128.tile16x16[ti]];break;case 8:scriptEng.operands[0]=(int32_t)StageSystem::tileCollisions[1].angle[(int32_t)StageSystem::tile128x128.tile16x16[ti]];break;}break;}
        OP0(120) GraphicsSystem::Copy16x16Tile(scriptEng.operands[0],scriptEng.operands[1]); break;
        case 121: {int32_t tx=scriptEng.operands[1]>>7,tz=scriptEng.operands[2]>>7;int32_t ti=(tx<=-1||tz<=-1)?0:((int32_t)StageSystem::stageLayouts[0].tileMap[tx+(tz<<8)]<<6);ti+=((scriptEng.operands[1]&127)>>4)+((scriptEng.operands[2]&127)>>4<<3);switch(scriptEng.operands[3]){case 0:StageSystem::tile128x128.tile16x16[ti]=(uint16_t)scriptEng.operands[0];StageSystem::tile128x128.gfxDataPos[ti]=(int32_t)StageSystem::tile128x128.tile16x16[ti]<<2;break;case 1:StageSystem::tile128x128.direction[ti]=(uint8_t)scriptEng.operands[0];break;case 2:StageSystem::tile128x128.visualPlane[ti]=(uint8_t)scriptEng.operands[0];break;case 3:StageSystem::tile128x128.collisionFlag[0][ti]=(uint8_t)scriptEng.operands[0];break;case 4:StageSystem::tile128x128.collisionFlag[1][ti]=(uint8_t)scriptEng.operands[0];break;case 5:StageSystem::tileCollisions[0].flags[(int32_t)StageSystem::tile128x128.tile16x16[ti]]=(uint8_t)scriptEng.operands[0];break;case 6:StageSystem::tileCollisions[0].angle[(int32_t)StageSystem::tile128x128.tile16x16[ti]]=(uint32_t)(uint8_t)scriptEng.operands[0];break;}break;}
        case 122: {scriptEng.operands[0]=-1;int32_t si=0;auto& af=*PL.animationFile;while(scriptEng.operands[0]==-1){if(FileIO::StringComp(scriptText,AnimationSystem::animationList[af.aniListOffset+si].name))scriptEng.operands[0]=si;else{++si;if(si==af.numAnimations)scriptEng.operands[0]=0;}}break;}
        OP0(123) scriptEng.checkResult=(int32_t)FileIO::ReadSaveRAMData(); break;
        OP0(124) scriptEng.checkResult=(int32_t)FileIO::WriteSaveRAMData(); break;
        OP0(125) TextSystem::LoadFontFile(scriptText); break;
        OP0(126) if(scriptEng.operands[2]==0)TextSystem::LoadTextFile(StageSystem::gameMenu[scriptEng.operands[0]],scriptText,0);else TextSystem::LoadTextFile(StageSystem::gameMenu[scriptEng.operands[0]],scriptText,1); break;
        OP0(127) TextSystem::textMenuSurfaceNo=(int32_t)OSL.surfaceNum; TextSystem::DrawBitmapText(StageSystem::gameMenu[scriptEng.operands[0]],scriptEng.operands[1],scriptEng.operands[2],scriptEng.operands[3],scriptEng.operands[4],scriptEng.operands[5],scriptEng.operands[6]); break;
        case 128: switch(scriptEng.operands[2]){case 0:scriptEng.operands[0]=(int32_t)StageSystem::gameMenu[scriptEng.operands[1]].textData[StageSystem::gameMenu[scriptEng.operands[1]].entryStart[scriptEng.operands[3]]+scriptEng.operands[4]];break;case 1:scriptEng.operands[0]=StageSystem::gameMenu[scriptEng.operands[1]].entrySize[scriptEng.operands[3]];break;case 2:scriptEng.operands[0]=(int32_t)StageSystem::gameMenu[scriptEng.operands[1]].numRows;break;}break;
        OP0(129) StageSystem::gameMenu[scriptEng.operands[0]].entryHighlight[(int32_t)StageSystem::gameMenu[scriptEng.operands[0]].numRows]=(uint8_t)scriptEng.operands[1]; TextSystem::AddTextMenuEntry(StageSystem::gameMenu[scriptEng.operands[0]],GlobalAppDefinitions::gameVersion); break;
        OP0(130) EngineCallbacks::OnlineSetAchievement(scriptEng.operands[0],scriptEng.operands[1]); break;
        OP0(131) EngineCallbacks::OnlineSetLeaderboard(scriptEng.operands[0],scriptEng.operands[1]); break;
        OP0(132) switch(scriptEng.operands[0]){case 0:EngineCallbacks::OnlineLoadAchievementsMenu();break;case 1:EngineCallbacks::OnlineLoadLeaderboardsMenu();break;} break;
        OP0(133) EngineCallbacks::RetroEngineCallback(scriptEng.operands[0]); break;
        OP0(134) break;
        }
#undef OP0
#undef JT0

        if (sz <= 0) continue;

        scp -= nb;
        for (int32_t wi = 0; wi < (int32_t)sz; ++wi) {
            int32_t wmode = sd[scp];
            switch (wmode) {
            case 1: {
                ++scp;
                int32_t idx = resolveIndex(scp, sd, objectLoop);
                int32_t dst = sd[scp++];
                int32_t v = scriptEng.operands[wi];
                switch (dst) {
                case 0: scriptEng.tempValue[0]=v; break;
                case 1: scriptEng.tempValue[1]=v; break;
                case 2: scriptEng.tempValue[2]=v; break;
                case 3: scriptEng.tempValue[3]=v; break;
                case 4: scriptEng.tempValue[4]=v; break;
                case 5: scriptEng.tempValue[5]=v; break;
                case 6: scriptEng.tempValue[6]=v; break;
                case 7: scriptEng.tempValue[7]=v; break;
                case 8: scriptEng.checkResult=v; break;
                case 9: scriptEng.arrayPosition[0]=v; break;
                case 10: scriptEng.arrayPosition[1]=v; break;
                case 11: globalVariables[idx]=v; break;
                case 13: objectEntityList[idx].type=(uint8_t)v; break;
                case 14: objectEntityList[idx].propertyValue=(uint8_t)v; break;
                case 15: objectEntityList[idx].xPos=v; break;
                case 16: objectEntityList[idx].yPos=v; break;
                case 17: objectEntityList[idx].xPos=v<<16; break;
                case 18: objectEntityList[idx].yPos=v<<16; break;
                case 19: objectEntityList[idx].state=(uint8_t)v; break;
                case 20: objectEntityList[idx].rotation=v; break;
                case 21: objectEntityList[idx].scale=v; break;
                case 22: objectEntityList[idx].priority=(uint8_t)v; break;
                case 23: objectEntityList[idx].drawOrder=(uint8_t)v; break;
                case 24: objectEntityList[idx].direction=(uint8_t)v; break;
                case 25: objectEntityList[idx].inkEffect=(uint8_t)v; break;
                case 26: objectEntityList[idx].alpha=(uint8_t)v; break;
                case 27: objectEntityList[idx].frame=(uint8_t)v; break;
                case 28: objectEntityList[idx].animation=(uint8_t)v; break;
                case 29: objectEntityList[idx].prevAnimation=(uint8_t)v; break;
                case 30: objectEntityList[idx].animationSpeed=v; break;
                case 31: objectEntityList[idx].animationTimer=v; break;
                case 32: case 33: case 34: case 35:
                case 36: case 37: case 38: case 39:
                    objectEntityList[idx].value[dst-32]=v; break;
                case 41: PL.objectPtr->state=(uint8_t)v; break;
                case 42: PL.controlMode=(int8_t)v; break;
                case 43: PL.controlLock=(uint8_t)v; break;
                case 44: PL.collisionMode=(uint8_t)v; break;
                case 45: PL.collisionPlane=(uint8_t)v; break;
                case 46: PL.xPos=v; break;
                case 47: PL.yPos=v; break;
                case 48: PL.xPos=v<<16; break;
                case 49: PL.yPos=v<<16; break;
                case 50: PL.screenXPos=v; break;
                case 51: PL.screenYPos=v; break;
                case 52: PL.speed=v; break;
                case 53: PL.xVelocity=v; break;
                case 54: PL.yVelocity=v; break;
                case 55: PL.gravity=(uint8_t)v; break;
                case 56: PL.angle=v; break;
                case 57: PL.skidding=(uint8_t)v; break;
                case 58: PL.pushing=(uint8_t)v; break;
                case 59: PL.trackScroll=(uint8_t)v; break;
                case 60: PL.up=(uint8_t)v; break;
                case 61: PL.down=(uint8_t)v; break;
                case 62: PL.left=(uint8_t)v; break;
                case 63: PL.right=(uint8_t)v; break;
                case 64: PL.jumpPress=(uint8_t)v; break;
                case 65: PL.jumpHold=(uint8_t)v; break;
                case 66: PL.followPlayer1=(uint8_t)v; break;
                case 67: PL.lookPos=v; break;
                case 68: PL.water=(uint8_t)v; break;
                case 69: PL.movementStats.topSpeed=v; break;
                case 70: PL.movementStats.acceleration=v; break;
                case 71: PL.movementStats.deceleration=v; break;
                case 72: PL.movementStats.airAcceleration=v; break;
                case 73: PL.movementStats.airDeceleration=v; break;
                case 74: PL.movementStats.gravity=v; break;
                case 75: PL.movementStats.jumpStrength=v; break;
                case 76: PL.movementStats.jumpCap=v; break;
                case 77: PL.movementStats.rollingAcceleration=v; break;
                case 78: PL.movementStats.rollingDeceleration=v; break;
                case 84: PL.flailing[idx]=(uint8_t)v; break;
                case 85: PL.timer=v; break;
                case 86: PL.tileCollisions=(uint8_t)v; break;
                case 87: PL.objectInteraction=(uint8_t)v; break;
                case 88: PL.visible=(uint8_t)v; break;
                case 89: PL.objectPtr->rotation=v; break;
                case 90: PL.objectPtr->scale=v; break;
                case 91: PL.objectPtr->priority=(uint8_t)v; break;
                case 92: PL.objectPtr->drawOrder=(uint8_t)v; break;
                case 93: PL.objectPtr->direction=(uint8_t)v; break;
                case 94: PL.objectPtr->inkEffect=(uint8_t)v; break;
                case 95: PL.objectPtr->alpha=(uint8_t)v; break;
                case 96: PL.objectPtr->frame=(uint8_t)v; break;
                case 97: PL.objectPtr->animation=(uint8_t)v; break;
                case 98: PL.objectPtr->prevAnimation=(uint8_t)v; break;
                case 99: PL.objectPtr->animationSpeed=v; break;
                case 100: PL.objectPtr->animationTimer=v; break;
                case 101: case 102: case 103: case 104:
                case 105: case 106: case 107: case 108:
                    PL.objectPtr->value[dst-101]=v; break;
                case 109: case 110: case 111: case 112:
                case 113: case 114: case 115: case 116:
                    PL.value[dst-109]=v; break;
                case 118: StageSystem::stageMode=(uint8_t)v; break;
                case 119: FileIO::activeStageList=(uint8_t)v; break;
                case 120: StageSystem::stageListPosition=v; break;
                case 121: StageSystem::timeEnabled=(uint8_t)v; break;
                case 122: StageSystem::milliSeconds=(uint8_t)v; break;
                case 123: StageSystem::seconds=(uint8_t)v; break;
                case 124: StageSystem::minutes=(uint8_t)v; break;
                case 125: FileIO::actNumber=v; break;
                case 126: StageSystem::pauseEnabled=(uint8_t)v; break;
                case 128: StageSystem::newXBoundary1=v; break;
                case 129: StageSystem::newXBoundary2=v; break;
                case 130: StageSystem::newYBoundary1=v; break;
                case 131: StageSystem::newYBoundary2=v; break;
                case 132: if(StageSystem::xBoundary1!=v){StageSystem::xBoundary1=v;StageSystem::newXBoundary1=v;} break;
                case 133: if(StageSystem::xBoundary2!=v){StageSystem::xBoundary2=v;StageSystem::newXBoundary2=v;} break;
                case 134: if(StageSystem::yBoundary1!=v){StageSystem::yBoundary1=v;StageSystem::newYBoundary1=v;} break;
                case 135: if(StageSystem::yBoundary2!=v){StageSystem::yBoundary2=v;StageSystem::newYBoundary2=v;} break;
                case 136: StageSystem::bgDeformationData0[idx]=v; break;
                case 137: StageSystem::bgDeformationData1[idx]=v; break;
                case 138: StageSystem::bgDeformationData2[idx]=v; break;
                case 139: StageSystem::bgDeformationData3[idx]=v; break;
                case 140: StageSystem::waterLevel=v; break;
                case 141: StageSystem::activeTileLayers[idx]=(uint8_t)v; break;
                case 142: StageSystem::tLayerMidPoint=(uint8_t)v; break;
                case 143: PlayerSystem::playerMenuNum=(uint8_t)v; break;
                case 144: playerNum=v; break;
                case 145: StageSystem::cameraEnabled=(uint8_t)v; break;
                case 146: StageSystem::cameraTarget=(int8_t)v; break;
                case 147: StageSystem::cameraStyle=(uint8_t)v; break;
                case 148: objectDrawOrderList[idx].listSize=v; break;
                case 153: StageSystem::xScrollOffset=v; StageSystem::xScrollA=v; StageSystem::xScrollB=v+GlobalAppDefinitions::SCREEN_XSIZE; break;
                case 154: StageSystem::yScrollOffset=v; StageSystem::yScrollA=v; StageSystem::yScrollB=v+240; break;
                case 155: StageSystem::screenShakeX=v; break;
                case 156: StageSystem::screenShakeY=v; break;
                case 157: StageSystem::cameraAdjustY=v; break;
                case 161: AudioPlayback::SetMusicVolume(v); break;
                case 163: StageSystem::gKeyDown.up=(uint8_t)v; break;
                case 164: StageSystem::gKeyDown.down=(uint8_t)v; break;
                case 165: StageSystem::gKeyDown.left=(uint8_t)v; break;
                case 166: StageSystem::gKeyDown.right=(uint8_t)v; break;
                case 167: StageSystem::gKeyDown.buttonA=(uint8_t)v; break;
                case 168: StageSystem::gKeyDown.buttonB=(uint8_t)v; break;
                case 169: StageSystem::gKeyDown.buttonC=(uint8_t)v; break;
                case 170: StageSystem::gKeyDown.start=(uint8_t)v; break;
                case 171: StageSystem::gKeyPress.up=(uint8_t)v; break;
                case 172: StageSystem::gKeyPress.down=(uint8_t)v; break;
                case 173: StageSystem::gKeyPress.left=(uint8_t)v; break;
                case 174: StageSystem::gKeyPress.right=(uint8_t)v; break;
                case 175: StageSystem::gKeyPress.buttonA=(uint8_t)v; break;
                case 176: StageSystem::gKeyPress.buttonB=(uint8_t)v; break;
                case 177: StageSystem::gKeyPress.buttonC=(uint8_t)v; break;
                case 178: StageSystem::gKeyPress.start=(uint8_t)v; break;
                case 179: StageSystem::gameMenu[0].selection1=v; break;
                case 180: StageSystem::gameMenu[1].selection1=v; break;
                case 181: StageSystem::stageLayouts[idx].xSize=(uint8_t)v; break;
                case 182: StageSystem::stageLayouts[idx].ySize=(uint8_t)v; break;
                case 183: StageSystem::stageLayouts[idx].type=(uint8_t)v; break;
                case 184: StageSystem::stageLayouts[idx].angle=v; if(v<0)StageSystem::stageLayouts[idx].angle+=512; StageSystem::stageLayouts[idx].angle&=511; break;
                case 185: StageSystem::stageLayouts[idx].xPos=v; break;
                case 186: StageSystem::stageLayouts[idx].yPos=v; break;
                case 187: StageSystem::stageLayouts[idx].zPos=v; break;
                case 188: StageSystem::stageLayouts[idx].parallaxFactor=v; break;
                case 189: StageSystem::stageLayouts[idx].scrollSpeed=v; break;
                case 190: StageSystem::stageLayouts[idx].scrollPosition=v; break;
                case 191: StageSystem::stageLayouts[idx].deformationPos=v&255; break;
                case 192: StageSystem::stageLayouts[idx].deformationPosW=v&255; break;
                case 193: StageSystem::hParallax.parallaxFactor[idx]=v; break;
                case 194: StageSystem::hParallax.scrollSpeed[idx]=v; break;
                case 195: StageSystem::hParallax.scrollPosition[idx]=v; break;
                case 196: StageSystem::vParallax.parallaxFactor[idx]=v; break;
                case 197: StageSystem::vParallax.scrollSpeed[idx]=v; break;
                case 198: StageSystem::vParallax.scrollPosition[idx]=v; break;
                case 199: Scene3D::numVertices=v; break;
                case 200: Scene3D::numFaces=v; break;
                case 201: Scene3D::vertexBuffer[idx].x=v; break;
                case 202: Scene3D::vertexBuffer[idx].y=v; break;
                case 203: Scene3D::vertexBuffer[idx].z=v; break;
                case 204: Scene3D::vertexBuffer[idx].u=v; break;
                case 205: Scene3D::vertexBuffer[idx].v=v; break;
                case 206: Scene3D::indexBuffer[idx].a=v; break;
                case 207: Scene3D::indexBuffer[idx].b=v; break;
                case 208: Scene3D::indexBuffer[idx].c=v; break;
                case 209: Scene3D::indexBuffer[idx].d=v; break;
                case 210: Scene3D::indexBuffer[idx].flag=(uint8_t)v; break;
                case 211: Scene3D::indexBuffer[idx].color=v; break;
                case 212: Scene3D::projectionX=v; break;
                case 213: Scene3D::projectionY=v; break;
                case 214: GlobalAppDefinitions::gameMode=(uint8_t)v; break;
                case 215: StageSystem::debugMode=(uint8_t)v; break;
                case 217: FileIO::saveRAM[idx]=v; break;
                case 218: GlobalAppDefinitions::gameLanguage=(uint8_t)v; break;
                case 219: objectScriptList[(int32_t)OE.type].surfaceNum=(uint8_t)v; break;
                case 221: GlobalAppDefinitions::frameSkipTimer=v; break;
                case 222: GlobalAppDefinitions::frameSkipSetting=v; break;
                case 223: GlobalAppDefinitions::gameSFXVolume=v; AudioPlayback::SetGameVolumes(GlobalAppDefinitions::gameBGMVolume,GlobalAppDefinitions::gameSFXVolume); break;
                case 224: GlobalAppDefinitions::gameBGMVolume=v; AudioPlayback::SetGameVolumes(GlobalAppDefinitions::gameBGMVolume,GlobalAppDefinitions::gameSFXVolume); break;
                case 227: StageSystem::gKeyPress.start=(uint8_t)v; break;
                case 228: GlobalAppDefinitions::gameHapticsEnabled=(uint8_t)v; break;
                }
                break;
            }
            case 2: scp+=2; break;
            case 3: {
                ++scp;
                int32_t slen=sd[scp]; int slot=0;
                for(int ci=0;ci<slen;++ci){switch(slot){case 0:++scp;slot=1;break;case 1:slot=2;break;case 2:slot=3;break;case 3:slot=0;break;}}
                if(slot==0)scp+=2;else ++scp;
                break;
            }
            }
        }
    }
}

#undef PL
#undef OE
#undef OSL

void ObjectSystem::ProcessStartupScripts()
{
    objectEntityList[1057].type = objectEntityList[0].type;
    scriptFramesNo=0; playerNum=0;
    scriptEng.arrayPosition[2]=1056;
    for(int32_t i=0;i<256;++i){
        objectLoop=1056;
        objectEntityList[1056].type=(uint8_t)i;
        objectScriptList[i].numFrames=0; objectScriptList[i].surfaceNum=0;
        objectScriptList[i].frameListOffset=scriptFramesNo;
        objectScriptList[i].numFrames=scriptFramesNo;
        if(scriptData[objectScriptList[i].startupScript]>0)
            ProcessScript(objectScriptList[i].startupScript,objectScriptList[i].startupJumpTable,3);
        objectScriptList[i].numFrames=scriptFramesNo-objectScriptList[i].numFrames;
    }
    objectEntityList[1056].type=objectEntityList[1057].type;
    objectEntityList[1056].type=0;
}

void ObjectSystem::ProcessObjects()
{
    for(int i=0;i<7;++i) objectDrawOrderList[i].listSize=0;
    for(objectLoop=0;objectLoop<1184;++objectLoop){
        bool act=false;
        auto& e=objectEntityList[objectLoop];
        int32_t ex=e.xPos>>16, ey=e.yPos>>16;
        switch(e.priority){
        case 0: act=(ex>StageSystem::xScrollOffset-GlobalAppDefinitions::OBJECT_BORDER_X1&&ex<StageSystem::xScrollOffset+GlobalAppDefinitions::OBJECT_BORDER_X2&&ey>StageSystem::yScrollOffset-256&&ey<StageSystem::yScrollOffset+496); break;
        case 1: case 2: act=true; break;
        case 3: act=(ex>StageSystem::xScrollOffset-GlobalAppDefinitions::OBJECT_BORDER_X1&&ex<StageSystem::xScrollOffset+GlobalAppDefinitions::OBJECT_BORDER_X2); break;
        case 4: if(ex>StageSystem::xScrollOffset-GlobalAppDefinitions::OBJECT_BORDER_X1&&ex<StageSystem::xScrollOffset+GlobalAppDefinitions::OBJECT_BORDER_X2&&ey>StageSystem::yScrollOffset-256&&ey<StageSystem::yScrollOffset+496)act=true;else{act=false;e.type=0;} break;
        case 5: act=false; break;
        }
        if(act&&e.type>0){
            int32_t t=(int32_t)e.type; playerNum=0;
            if(scriptData[objectScriptList[t].mainScript]>0) ProcessScript(objectScriptList[t].mainScript,objectScriptList[t].mainJumpTable,0);
            if(scriptData[objectScriptList[t].playerScript]>0){
                for(;playerNum<(int32_t)PlayerSystem::numActivePlayers;++playerNum)
                    if(PlayerSystem::playerList[playerNum].objectInteraction==1)
                        ProcessScript(objectScriptList[t].playerScript,objectScriptList[t].playerJumpTable,1);
            }
            int32_t d=(int32_t)e.drawOrder;
            if(d<7){objectDrawOrderList[d].entityRef[objectDrawOrderList[d].listSize]=objectLoop;++objectDrawOrderList[d].listSize;}
        }
    }
}

void ObjectSystem::ProcessPausedObjects()
{
    for(int i=0;i<7;++i) objectDrawOrderList[i].listSize=0;
    for(objectLoop=0;objectLoop<1184;++objectLoop){
        auto& e=objectEntityList[objectLoop];
        if(e.priority==2&&e.type>0){
            int32_t t=(int32_t)e.type; playerNum=0;
            if(scriptData[objectScriptList[t].mainScript]>0) ProcessScript(objectScriptList[t].mainScript,objectScriptList[t].mainJumpTable,0);
            if(scriptData[objectScriptList[t].playerScript]>0)
                for(;playerNum<(int32_t)PlayerSystem::numActivePlayers;++playerNum)
                    if(PlayerSystem::playerList[playerNum].objectInteraction==1)
                        ProcessScript(objectScriptList[t].playerScript,objectScriptList[t].playerJumpTable,1);
            int32_t d=(int32_t)e.drawOrder;
            if(d<7){objectDrawOrderList[d].entityRef[objectDrawOrderList[d].listSize]=objectLoop;++objectDrawOrderList[d].listSize;}
        }
    }
}

void ObjectSystem::DrawObjectList(int32_t n)
{
    for(int32_t i=0;i<objectDrawOrderList[n].listSize;++i){
        objectLoop=objectDrawOrderList[n].entityRef[i];
        if(objectEntityList[objectLoop].type>0){
            playerNum=0;
            int32_t t=(int32_t)objectEntityList[objectLoop].type;
            if(scriptData[objectScriptList[t].drawScript]>0)
                ProcessScript(objectScriptList[t].drawScript,objectScriptList[t].drawJumpTable,2);
        }
    }
}

static CollisionBox& playerCB()
{
    auto& p=PlayerSystem::playerList[ObjectSystem::playerNum];
    return AnimationSystem::collisionBoxList[p.animationFile->cbListOffset+(int32_t)AnimationSystem::animationFrames[AnimationSystem::animationList[p.animationFile->aniListOffset+(int32_t)p.objectPtr->animation].frameListOffset+(int32_t)p.objectPtr->frame].collisionBox];
}

void ObjectSystem::BasicCollision(int32_t cL,int32_t cT,int32_t cR,int32_t cB)
{
    auto& p=PlayerSystem::playerList[playerNum]; auto& cb=playerCB();
    int32_t pl=(p.xPos>>16)+(int32_t)cb.left[0],pt=(p.yPos>>16)+(int32_t)cb.top[0];
    int32_t pr=(p.xPos>>16)+(int32_t)cb.right[0],pb=(p.yPos>>16)+(int32_t)cb.bottom[0];
    scriptEng.checkResult=(pr>cL&&pl<cR&&pb>cT&&pt<cB)?1:0;
}

void ObjectSystem::BoxCollision(int32_t cL,int32_t cT,int32_t cR,int32_t cB)
{
    auto& p=PlayerSystem::playerList[playerNum]; auto& cb=playerCB();
    PlayerSystem::collisionLeft=(int32_t)cb.left[0]; PlayerSystem::collisionTop=(int32_t)cb.top[0];
    PlayerSystem::collisionRight=(int32_t)cb.right[0]; PlayerSystem::collisionBottom=(int32_t)cb.bottom[0];
    scriptEng.checkResult=0;
    int32_t sp=0;
    switch(p.collisionMode){case 0:case 2:sp=p.xVelocity==0?std::abs(p.speed):std::abs(p.xVelocity);break;case 1:case 3:sp=std::abs(p.xVelocity);break;}
    auto& s0=cSensor[0]; auto& s1=cSensor[1]; auto& s2=cSensor[2]; auto& s3=cSensor[3]; auto& s4=cSensor[4];
    auto topBot=[&](){
        s0.collided=s1.collided=s2.collided=0;
        s0.xPos=p.xPos+((PlayerSystem::collisionLeft+2)<<16); s1.xPos=p.xPos; s2.xPos=p.xPos+((PlayerSystem::collisionRight-2)<<16);
        s0.yPos=s1.yPos=s2.yPos=p.yPos+(PlayerSystem::collisionBottom<<16);
        if(p.yVelocity>-1)for(int i=0;i<3;++i)if(cSensor[i].xPos>cL&&cSensor[i].xPos<cR&&cSensor[i].yPos>=cT&&p.yPos-p.yVelocity<cT){cSensor[i].collided=1;p.flailing[i]=1;}
        if(s0.collided||s1.collided||s2.collided){if(p.gravity==0&&(p.collisionMode==1||p.collisionMode==3)){p.xVelocity=0;p.speed=0;}p.yPos=cT-(PlayerSystem::collisionBottom<<16);p.gravity=0;p.yVelocity=0;p.angle=0;p.objectPtr->rotation=0;p.controlLock=0;scriptEng.checkResult=1;return true;}
        s0.collided=s1.collided=0; s0.xPos=p.xPos+((PlayerSystem::collisionLeft+2)<<16); s1.xPos=p.xPos+((PlayerSystem::collisionRight-2)<<16);
        s0.yPos=s1.yPos=p.yPos+(PlayerSystem::collisionTop<<16);
        for(int i=0;i<2;++i)if(cSensor[i].xPos>cL&&cSensor[i].xPos<cR&&cSensor[i].yPos<=cB&&p.yPos-p.yVelocity>cB)cSensor[i].collided=1;
        if(s0.collided||s1.collided){if(p.gravity==1)p.yPos=cB-(PlayerSystem::collisionTop<<16);if(p.yVelocity<1)p.yVelocity=0;scriptEng.checkResult=4;return true;}
        return false;
    };
    auto leftRight=[&]()->bool{
        s0.collided=s1.collided=0;
        s0.xPos=s1.xPos=p.xPos+(PlayerSystem::collisionRight<<16); s0.yPos=p.yPos-131072; s1.yPos=p.yPos+524288;
        for(int i=0;i<2;++i)if(cSensor[i].xPos>=cL&&p.xPos-p.xVelocity<cL&&s1.yPos>cT&&s0.yPos<cB)cSensor[i].collided=1;
        if(s0.collided||s1.collided){p.xPos=cL-(PlayerSystem::collisionRight<<16);if(p.xVelocity>0){if(p.objectPtr->direction==0)p.pushing=2;p.xVelocity=0;p.speed=0;}scriptEng.checkResult=2;return true;}
        s0.collided=s1.collided=0;
        s0.xPos=s1.xPos=p.xPos+(PlayerSystem::collisionLeft<<16); s0.yPos=p.yPos-131072; s1.yPos=p.yPos+524288;
        for(int i=0;i<2;++i)if(cSensor[i].xPos<=cR&&p.xPos-p.xVelocity>cR&&s1.yPos>cT&&s0.yPos<cB)cSensor[i].collided=1;
        if(s0.collided||s1.collided){p.xPos=cR-(PlayerSystem::collisionLeft<<16);if(p.xVelocity<0){if(p.objectPtr->direction==1)p.pushing=2;p.xVelocity=0;p.speed=0;}scriptEng.checkResult=3;return true;}
        return false;
    };
    if(sp>std::abs(p.yVelocity)){if(!leftRight())topBot();}else{if(!topBot())leftRight();}
}

void ObjectSystem::PlatformCollision(int32_t cL,int32_t cT,int32_t cR,int32_t cB)
{
    auto& p=PlayerSystem::playerList[playerNum]; auto& cb=playerCB();
    PlayerSystem::collisionLeft=(int32_t)cb.left[0]; PlayerSystem::collisionTop=(int32_t)cb.top[0];
    PlayerSystem::collisionRight=(int32_t)cb.right[0]; PlayerSystem::collisionBottom=(int32_t)cb.bottom[0];
    cSensor[0].collided=cSensor[1].collided=cSensor[2].collided=0;
    cSensor[0].xPos=p.xPos+((PlayerSystem::collisionLeft+1)<<16); cSensor[1].xPos=p.xPos; cSensor[2].xPos=p.xPos+(PlayerSystem::collisionRight<<16);
    cSensor[0].yPos=cSensor[1].yPos=cSensor[2].yPos=p.yPos+(PlayerSystem::collisionBottom<<16);
    scriptEng.checkResult=0;
    for(int i=0;i<3;++i)if(cSensor[i].xPos>cL&&cSensor[i].xPos<cR&&cSensor[i].yPos>cT-2&&cSensor[i].yPos<cB&&p.yVelocity>=0){cSensor[i].collided=1;p.flailing[i]=1;}
    if(!cSensor[0].collided&&!cSensor[1].collided&&!cSensor[2].collided)return;
    if(p.gravity==0&&(p.collisionMode==1||p.collisionMode==3)){p.xVelocity=0;p.speed=0;}
    p.yPos=cT-(PlayerSystem::collisionBottom<<16); p.gravity=0;p.yVelocity=0;p.angle=0;p.objectPtr->rotation=0;p.controlLock=0;
    scriptEng.checkResult=1;
}

static bool tileFloor(int32_t x,int32_t& y,int32_t cp){
    using SS=StageSystem;
    if(x<=0||x>=(int32_t)SS::stageLayouts[0].xSize<<7||y<=0||y>=(int32_t)SS::stageLayouts[0].ySize<<7)return false;
    int32_t tx=x>>7,tc=(x&127)>>4,ty=y>>7,tr=(y&127)>>4;
    int32_t idx=((int32_t)SS::stageLayouts[0].tileMap[tx+(ty<<8)]<<6)+(tc+(tr<<3));
    int32_t t=(int32_t)SS::tile128x128.tile16x16[idx];
    if(SS::tile128x128.collisionFlag[cp][idx]==2||SS::tile128x128.collisionFlag[cp][idx]==3)return false;
    switch(SS::tile128x128.direction[idx]){
    case 0:{int32_t m=(x&15)+(t<<4);if((y&15)>(int32_t)SS::tileCollisions[cp].floorMask[m]){y=(int32_t)SS::tileCollisions[cp].floorMask[m]+(ty<<7)+(tr<<4);return true;}break;}
    case 1:{int32_t m=15-(x&15)+(t<<4);if((y&15)>(int32_t)SS::tileCollisions[cp].floorMask[m]){y=(int32_t)SS::tileCollisions[cp].floorMask[m]+(ty<<7)+(tr<<4);return true;}break;}
    case 2:{int32_t m=(x&15)+(t<<4);if((y&15)>15-(int32_t)SS::tileCollisions[cp].roofMask[m]){y=15-(int32_t)SS::tileCollisions[cp].roofMask[m]+(ty<<7)+(tr<<4);return true;}break;}
    case 3:{int32_t m=15-(x&15)+(t<<4);if((y&15)>15-(int32_t)SS::tileCollisions[cp].roofMask[m]){y=15-(int32_t)SS::tileCollisions[cp].roofMask[m]+(ty<<7)+(tr<<4);return true;}break;}
    }
    return false;
}
static bool tileRoof(int32_t x,int32_t& y,int32_t cp){
    using SS=StageSystem;
    if(x<=0||x>=(int32_t)SS::stageLayouts[0].xSize<<7||y<=0||y>=(int32_t)SS::stageLayouts[0].ySize<<7)return false;
    int32_t tx=x>>7,tc=(x&127)>>4,ty=y>>7,tr=(y&127)>>4;
    int32_t idx=((int32_t)SS::stageLayouts[0].tileMap[tx+(ty<<8)]<<6)+(tc+(tr<<3));
    int32_t t=(int32_t)SS::tile128x128.tile16x16[idx];
    if(SS::tile128x128.collisionFlag[cp][idx]==1||SS::tile128x128.collisionFlag[cp][idx]>=3)return false;
    switch(SS::tile128x128.direction[idx]){
    case 0:{int32_t m=(x&15)+(t<<4);if((y&15)<(int32_t)SS::tileCollisions[cp].roofMask[m]){y=(int32_t)SS::tileCollisions[cp].roofMask[m]+(ty<<7)+(tr<<4);return true;}break;}
    case 1:{int32_t m=15-(x&15)+(t<<4);if((y&15)<(int32_t)SS::tileCollisions[cp].roofMask[m]){y=(int32_t)SS::tileCollisions[cp].roofMask[m]+(ty<<7)+(tr<<4);return true;}break;}
    case 2:{int32_t m=(x&15)+(t<<4);if((y&15)<15-(int32_t)SS::tileCollisions[cp].floorMask[m]){y=15-(int32_t)SS::tileCollisions[cp].floorMask[m]+(ty<<7)+(tr<<4);return true;}break;}
    case 3:{int32_t m=15-(x&15)+(t<<4);if((y&15)<15-(int32_t)SS::tileCollisions[cp].floorMask[m]){y=15-(int32_t)SS::tileCollisions[cp].floorMask[m]+(ty<<7)+(tr<<4);return true;}break;}
    }
    return false;
}
static bool tileLWall(int32_t& x,int32_t y,int32_t cp){
    using SS=StageSystem;
    if(x<=0||x>=(int32_t)SS::stageLayouts[0].xSize<<7||y<=0||y>=(int32_t)SS::stageLayouts[0].ySize<<7)return false;
    int32_t tx=x>>7,tc=(x&127)>>4,ty=y>>7,tr=(y&127)>>4;
    int32_t idx=((int32_t)SS::stageLayouts[0].tileMap[tx+(ty<<8)]<<6)+(tc+(tr<<3));
    int32_t t=(int32_t)SS::tile128x128.tile16x16[idx];
    if(SS::tile128x128.collisionFlag[cp][idx]==1||SS::tile128x128.collisionFlag[cp][idx]>=3)return false;
    switch(SS::tile128x128.direction[idx]){
    case 0:{int32_t m=(y&15)+(t<<4);if((x&15)>(int32_t)SS::tileCollisions[cp].leftWallMask[m]){x=(int32_t)SS::tileCollisions[cp].leftWallMask[m]+(tx<<7)+(tc<<4);return true;}break;}
    case 1:{int32_t m=(y&15)+(t<<4);if((x&15)>15-(int32_t)SS::tileCollisions[cp].rightWallMask[m]){x=15-(int32_t)SS::tileCollisions[cp].rightWallMask[m]+(tx<<7)+(tc<<4);return true;}break;}
    case 2:{int32_t m=15-(y&15)+(t<<4);if((x&15)>(int32_t)SS::tileCollisions[cp].leftWallMask[m]){x=(int32_t)SS::tileCollisions[cp].leftWallMask[m]+(tx<<7)+(tc<<4);return true;}break;}
    case 3:{int32_t m=15-(y&15)+(t<<4);if((x&15)>15-(int32_t)SS::tileCollisions[cp].rightWallMask[m]){x=15-(int32_t)SS::tileCollisions[cp].rightWallMask[m]+(tx<<7)+(tc<<4);return true;}break;}
    }
    return false;
}
static bool tileRWall(int32_t& x,int32_t y,int32_t cp){
    using SS=StageSystem;
    if(x<=0||x>=(int32_t)SS::stageLayouts[0].xSize<<7||y<=0||y>=(int32_t)SS::stageLayouts[0].ySize<<7)return false;
    int32_t tx=x>>7,tc=(x&127)>>4,ty=y>>7,tr=(y&127)>>4;
    int32_t idx=((int32_t)SS::stageLayouts[0].tileMap[tx+(ty<<8)]<<6)+(tc+(tr<<3));
    int32_t t=(int32_t)SS::tile128x128.tile16x16[idx];
    if(SS::tile128x128.collisionFlag[cp][idx]==1||SS::tile128x128.collisionFlag[cp][idx]>=3)return false;
    switch(SS::tile128x128.direction[idx]){
    case 0:{int32_t m=(y&15)+(t<<4);if((x&15)<(int32_t)SS::tileCollisions[cp].rightWallMask[m]){x=(int32_t)SS::tileCollisions[cp].rightWallMask[m]+(tx<<7)+(tc<<4);return true;}break;}
    case 1:{int32_t m=(y&15)+(t<<4);if((x&15)<15-(int32_t)SS::tileCollisions[cp].leftWallMask[m]){x=15-(int32_t)SS::tileCollisions[cp].leftWallMask[m]+(tx<<7)+(tc<<4);return true;}break;}
    case 2:{int32_t m=15-(y&15)+(t<<4);if((x&15)<(int32_t)SS::tileCollisions[cp].rightWallMask[m]){x=(int32_t)SS::tileCollisions[cp].rightWallMask[m]+(tx<<7)+(tc<<4);return true;}break;}
    case 3:{int32_t m=15-(y&15)+(t<<4);if((x&15)<15-(int32_t)SS::tileCollisions[cp].leftWallMask[m]){x=15-(int32_t)SS::tileCollisions[cp].leftWallMask[m]+(tx<<7)+(tc<<4);return true;}break;}
    }
    return false;
}

void ObjectSystem::ObjectFloorCollision(int32_t xo,int32_t yo,int32_t cp){
    scriptEng.checkResult=0;
    int32_t x=(objectEntityList[objectLoop].xPos>>16)+xo,y=(objectEntityList[objectLoop].yPos>>16)+yo;
    if(tileFloor(x,y,cp)){scriptEng.checkResult=1;objectEntityList[objectLoop].yPos=(y-yo)<<16;}
}
void ObjectSystem::ObjectLWallCollision(int32_t xo,int32_t yo,int32_t cp){
    scriptEng.checkResult=0;
    int32_t x=(objectEntityList[objectLoop].xPos>>16)+xo,y=(objectEntityList[objectLoop].yPos>>16)+yo;
    if(tileLWall(x,y,cp)){scriptEng.checkResult=1;objectEntityList[objectLoop].xPos=(x-xo)<<16;}
}
void ObjectSystem::ObjectRWallCollision(int32_t xo,int32_t yo,int32_t cp){
    scriptEng.checkResult=0;
    int32_t x=(objectEntityList[objectLoop].xPos>>16)+xo,y=(objectEntityList[objectLoop].yPos>>16)+yo;
    if(tileRWall(x,y,cp)){scriptEng.checkResult=1;objectEntityList[objectLoop].xPos=(x-xo)<<16;}
}
void ObjectSystem::ObjectRoofCollision(int32_t xo,int32_t yo,int32_t cp){
    scriptEng.checkResult=0;
    int32_t x=(objectEntityList[objectLoop].xPos>>16)+xo,y=(objectEntityList[objectLoop].yPos>>16)+yo;
    if(tileRoof(x,y,cp)){scriptEng.checkResult=1;objectEntityList[objectLoop].yPos=(y-yo)<<16;}
}


void ObjectSystem::ObjectFloorGrip(int32_t xo,int32_t yo,int32_t cp){
    scriptEng.checkResult=0;
    int32_t x=(objectEntityList[objectLoop].xPos>>16)+xo,y=(objectEntityList[objectLoop].yPos>>16)+yo,orig=y;
    for(int k=3,scan=y-16;k>0;--k,scan+=16){
        using SS=StageSystem;
        if(x>0&&x<(int32_t)SS::stageLayouts[0].xSize<<7&&scan>0&&scan<(int32_t)SS::stageLayouts[0].ySize<<7&&scriptEng.checkResult==0){
            int32_t tx=x>>7,tc=(x&127)>>4,ty=scan>>7,tr=(scan&127)>>4;
            int32_t idx=((int32_t)SS::stageLayouts[0].tileMap[tx+(ty<<8)]<<6)+(tc+(tr<<3));
            int32_t t=(int32_t)SS::tile128x128.tile16x16[idx];
            if(SS::tile128x128.collisionFlag[cp][idx]!=2&&SS::tile128x128.collisionFlag[cp][idx]!=3){
                switch(SS::tile128x128.direction[idx]){
                case 0:{int32_t m=(x&15)+(t<<4);if(SS::tileCollisions[cp].floorMask[m]<64){objectEntityList[objectLoop].yPos=(int32_t)SS::tileCollisions[cp].floorMask[m]+(ty<<7)+(tr<<4);scriptEng.checkResult=1;}break;}
                case 1:{int32_t m=15-(x&15)+(t<<4);if(SS::tileCollisions[cp].floorMask[m]<64){objectEntityList[objectLoop].yPos=(int32_t)SS::tileCollisions[cp].floorMask[m]+(ty<<7)+(tr<<4);scriptEng.checkResult=1;}break;}
                case 2:{int32_t m=(x&15)+(t<<4);if(SS::tileCollisions[cp].roofMask[m]>(int8_t)-64){objectEntityList[objectLoop].yPos=15-(int32_t)SS::tileCollisions[cp].roofMask[m]+(ty<<7)+(tr<<4);scriptEng.checkResult=1;}break;}
                case 3:{int32_t m=15-(x&15)+(t<<4);if(SS::tileCollisions[cp].roofMask[m]>(int8_t)-64){objectEntityList[objectLoop].yPos=15-(int32_t)SS::tileCollisions[cp].roofMask[m]+(ty<<7)+(tr<<4);scriptEng.checkResult=1;}break;}
                }
            }
        }
    }
    if(scriptEng.checkResult==1){
        if(std::abs(objectEntityList[objectLoop].yPos-orig)<16)objectEntityList[objectLoop].yPos=(objectEntityList[objectLoop].yPos-yo)<<16;
        else{objectEntityList[objectLoop].yPos=(orig-yo)<<16;scriptEng.checkResult=0;}
    }
}

void ObjectSystem::ObjectLWallGrip(int32_t xo,int32_t yo,int32_t cp){
    scriptEng.checkResult=0;
    int32_t x=(objectEntityList[objectLoop].xPos>>16)+xo,y=(objectEntityList[objectLoop].yPos>>16)+yo,orig=x;
    using SS=StageSystem;
    for(int k=3,scan=x-16;k>0;--k,scan+=16){
        if(scan>0&&scan<(int32_t)SS::stageLayouts[0].xSize<<7&&y>0&&y<(int32_t)SS::stageLayouts[0].ySize<<7&&scriptEng.checkResult==0){
            int32_t tx=scan>>7,tc=(scan&127)>>4,ty=y>>7,tr=(y&127)>>4;
            int32_t idx=((int32_t)SS::stageLayouts[0].tileMap[tx+(ty<<8)]<<6)+(tc+(tr<<3));
            int32_t t=(int32_t)SS::tile128x128.tile16x16[idx];
            if(SS::tile128x128.collisionFlag[cp][idx]<3){
                switch(SS::tile128x128.direction[idx]){
                case 0:{int32_t m=(y&15)+(t<<4);if(SS::tileCollisions[cp].leftWallMask[m]<64){objectEntityList[objectLoop].xPos=(int32_t)SS::tileCollisions[cp].leftWallMask[m]+(tx<<7)+(tc<<4);scriptEng.checkResult=1;}break;}
                case 1:{int32_t m=(y&15)+(t<<4);if(SS::tileCollisions[cp].rightWallMask[m]>(int8_t)-64){objectEntityList[objectLoop].xPos=15-(int32_t)SS::tileCollisions[cp].rightWallMask[m]+(tx<<7)+(tc<<4);scriptEng.checkResult=1;}break;}
                case 2:{int32_t m=15-(y&15)+(t<<4);if(SS::tileCollisions[cp].leftWallMask[m]<64){objectEntityList[objectLoop].xPos=(int32_t)SS::tileCollisions[cp].leftWallMask[m]+(tx<<7)+(tc<<4);scriptEng.checkResult=1;}break;}
                case 3:{int32_t m=15-(y&15)+(t<<4);if(SS::tileCollisions[cp].rightWallMask[m]>(int8_t)-64){objectEntityList[objectLoop].xPos=15-(int32_t)SS::tileCollisions[cp].rightWallMask[m]+(tx<<7)+(tc<<4);scriptEng.checkResult=1;}break;}
                }
            }
        }
    }
    if(scriptEng.checkResult==1){
        if(std::abs(objectEntityList[objectLoop].xPos-orig)<16)objectEntityList[objectLoop].xPos=(objectEntityList[objectLoop].xPos-xo)<<16;
        else{objectEntityList[objectLoop].xPos=(orig-xo)<<16;scriptEng.checkResult=0;}
    }
}

void ObjectSystem::ObjectRWallGrip(int32_t xo,int32_t yo,int32_t cp){
    scriptEng.checkResult=0;
    int32_t x=(objectEntityList[objectLoop].xPos>>16)+xo,y=(objectEntityList[objectLoop].yPos>>16)+yo,orig=x;
    using SS=StageSystem;
    for(int k=3,scan=x+16;k>0;--k,scan-=16){
        if(scan>0&&scan<(int32_t)SS::stageLayouts[0].xSize<<7&&y>0&&y<(int32_t)SS::stageLayouts[0].ySize<<7&&scriptEng.checkResult==0){
            int32_t tx=scan>>7,tc=(scan&127)>>4,ty=y>>7,tr=(y&127)>>4;
            int32_t idx=((int32_t)SS::stageLayouts[0].tileMap[tx+(ty<<8)]<<6)+(tc+(tr<<3));
            int32_t t=(int32_t)SS::tile128x128.tile16x16[idx];
            if(SS::tile128x128.collisionFlag[cp][idx]<3){
                switch(SS::tile128x128.direction[idx]){
                case 0:{int32_t m=(y&15)+(t<<4);if(SS::tileCollisions[cp].rightWallMask[m]>(int8_t)-64){objectEntityList[objectLoop].xPos=(int32_t)SS::tileCollisions[cp].rightWallMask[m]+(tx<<7)+(tc<<4);scriptEng.checkResult=1;}break;}
                case 1:{int32_t m=(y&15)+(t<<4);if(SS::tileCollisions[cp].leftWallMask[m]<64){objectEntityList[objectLoop].xPos=15-(int32_t)SS::tileCollisions[cp].leftWallMask[m]+(tx<<7)+(tc<<4);scriptEng.checkResult=1;}break;}
                case 2:{int32_t m=15-(y&15)+(t<<4);if(SS::tileCollisions[cp].rightWallMask[m]>(int8_t)-64){objectEntityList[objectLoop].xPos=(int32_t)SS::tileCollisions[cp].rightWallMask[m]+(tx<<7)+(tc<<4);scriptEng.checkResult=1;}break;}
                case 3:{int32_t m=15-(y&15)+(t<<4);if(SS::tileCollisions[cp].leftWallMask[m]<64){objectEntityList[objectLoop].xPos=15-(int32_t)SS::tileCollisions[cp].leftWallMask[m]+(tx<<7)+(tc<<4);scriptEng.checkResult=1;}break;}
                }
            }
        }
    }
    if(scriptEng.checkResult==1){
        if(std::abs(objectEntityList[objectLoop].xPos-orig)<16)objectEntityList[objectLoop].xPos=(objectEntityList[objectLoop].xPos-xo)<<16;
        else{objectEntityList[objectLoop].xPos=(orig-xo)<<16;scriptEng.checkResult=0;}
    }
}

void ObjectSystem::ObjectRoofGrip(int32_t xo,int32_t yo,int32_t cp){
    scriptEng.checkResult=0;
    int32_t x=(objectEntityList[objectLoop].xPos>>16)+xo,y=(objectEntityList[objectLoop].yPos>>16)+yo,orig=y;
    using SS=StageSystem;
    for(int k=3,scan=y+16;k>0;--k,scan-=16){
        if(x>0&&x<(int32_t)SS::stageLayouts[0].xSize<<7&&scan>0&&scan<(int32_t)SS::stageLayouts[0].ySize<<7&&scriptEng.checkResult==0){
            int32_t tx=x>>7,tc=(x&127)>>4,ty=scan>>7,tr=(scan&127)>>4;
            int32_t idx=((int32_t)SS::stageLayouts[0].tileMap[tx+(ty<<8)]<<6)+(tc+(tr<<3));
            int32_t t=(int32_t)SS::tile128x128.tile16x16[idx];
            if(SS::tile128x128.collisionFlag[cp][idx]<3){
                switch(SS::tile128x128.direction[idx]){
                case 0:{int32_t m=(x&15)+(t<<4);if(SS::tileCollisions[cp].roofMask[m]>(int8_t)-64){objectEntityList[objectLoop].yPos=(int32_t)SS::tileCollisions[cp].roofMask[m]+(ty<<7)+(tr<<4);scriptEng.checkResult=1;}break;}
                case 1:{int32_t m=15-(x&15)+(t<<4);if(SS::tileCollisions[cp].roofMask[m]>(int8_t)-64){objectEntityList[objectLoop].yPos=(int32_t)SS::tileCollisions[cp].roofMask[m]+(ty<<7)+(tr<<4);scriptEng.checkResult=1;}break;}
                case 2:{int32_t m=(x&15)+(t<<4);if(SS::tileCollisions[cp].floorMask[m]<64){objectEntityList[objectLoop].yPos=15-(int32_t)SS::tileCollisions[cp].floorMask[m]+(ty<<7)+(tr<<4);scriptEng.checkResult=1;}break;}
                case 3:{int32_t m=15-(x&15)+(t<<4);if(SS::tileCollisions[cp].floorMask[m]<64){objectEntityList[objectLoop].yPos=15-(int32_t)SS::tileCollisions[cp].floorMask[m]+(ty<<7)+(tr<<4);scriptEng.checkResult=1;}break;}
                }
            }
        }
    }
    if(scriptEng.checkResult==1){
        if(std::abs(objectEntityList[objectLoop].yPos-orig)<16)objectEntityList[objectLoop].yPos=(objectEntityList[objectLoop].yPos-yo)<<16;
        else{objectEntityList[objectLoop].yPos=(orig-yo)<<16;scriptEng.checkResult=0;}
    }
}

}
