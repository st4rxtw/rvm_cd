#pragma once
#include <cstdint>
#include "SpriteFrame.h"
#include "ScriptEngine.h"
#include "ObjectScript.h"
#include "FunctionScript.h"
#include "ObjectEntity.h"
#include "ObjectDrawList.h"
#include "CollisionSensor.h"

namespace rvm {

struct ObjectSystem {
    static constexpr int32_t SUB_MAIN              = 0;
    static constexpr int32_t SUB_PLAYER            = 1;
    static constexpr int32_t SUB_DRAW              = 2;
    static constexpr int32_t SUB_STARTUP           = 3;
    static constexpr int32_t NUM_ARITHMETIC_TOKENS = 13;
    static constexpr int32_t NUM_EVALUATION_TOKENS = 6;
    static constexpr int32_t NUM_OPCODES           = 135;
    static constexpr int32_t NUM_VARIABLE_NAMES    = 229;
    static constexpr int32_t NUM_CONSTANTS         = 31;
    static constexpr int32_t SCRIPT_DATA_SIZE      = 262144;
    static constexpr int32_t JUMP_TABLE_SIZE       = 16384;

    static int32_t  scriptData[262144];
    static int32_t  scriptDataPos;
    static int32_t  scriptDataOffset;
    static int32_t  scriptLineNumber;
    static int32_t  jumpTableData[16384];
    static int32_t  jumpTableDataPos;
    static int32_t  jumpTableOffset;
    static int32_t  jumpTableStack[1024];
    static int32_t  jumpTableStackPos;
    static int32_t  NUM_FUNCTIONS;
    static int32_t  functionStack[1024];
    static int32_t  functionStackPos;
    static SpriteFrame scriptFrames[4096];
    static int32_t  scriptFramesNo;
    static uint8_t  NO_GLOBALVARIABLES;
    static char     globalVariableNames[256][32];
    static int32_t  globalVariables[256];
    static int32_t  objectLoop;
    static ScriptEngine scriptEng;
    static char     scriptText[256];
    static ObjectScript    objectScriptList[256];
    static FunctionScript  functionScriptList[512];
    static ObjectEntity    objectEntityList[1184];
    static ObjectDrawList  objectDrawOrderList[7];
    static int32_t  playerNum;
    static CollisionSensor cSensor[6];

    static void Init();
    static void ClearScriptData();
    static void SetObjectTypeName(const char* typeName, int32_t scriptNum);
    static void LoadByteCodeFile(int32_t fileType, int32_t scriptNum);
    static void ProcessScript(int32_t scriptCodePtr, int32_t jumpTablePtr, int32_t scriptSub);
    static void ProcessStartupScripts();
    static void ProcessObjects();
    static void ProcessPausedObjects();
    static void DrawObjectList(int32_t drawListNo);
    static void BasicCollision(int32_t cLeft, int32_t cTop, int32_t cRight, int32_t cBottom);
    static void BoxCollision(int32_t cLeft, int32_t cTop, int32_t cRight, int32_t cBottom);
    static void PlatformCollision(int32_t cLeft, int32_t cTop, int32_t cRight, int32_t cBottom);
    static void ObjectFloorCollision(int32_t xOffset, int32_t yOffset, int32_t cPlane);
    static void ObjectLWallCollision(int32_t xOffset, int32_t yOffset, int32_t cPlane);
    static void ObjectRWallCollision(int32_t xOffset, int32_t yOffset, int32_t cPlane);
    static void ObjectRoofCollision(int32_t xOffset, int32_t yOffset, int32_t cPlane);
    static void ObjectFloorGrip(int32_t xOffset, int32_t yOffset, int32_t cPlane);
    static void ObjectLWallGrip(int32_t xOffset, int32_t yOffset, int32_t cPlane);
    static void ObjectRWallGrip(int32_t xOffset, int32_t yOffset, int32_t cPlane);
    static void ObjectRoofGrip(int32_t xOffset, int32_t yOffset, int32_t cPlane);

private:
    static char     functionNames[512][32];
    static char     typeNames[256][32];
    static int8_t   scriptOpcodeSizes[135];
};

}
