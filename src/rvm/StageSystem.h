#pragma once
#include <cstdint>
#include "InputResult.h"
#include "Mappings128x128.h"
#include "LayoutMap.h"
#include "CollisionMask16x16.h"
#include "LineScrollParallax.h"
#include "TextMenu.h"

namespace rvm {

struct StageSystem {
    static constexpr int32_t ACTLAYOUT  = 0;
    static constexpr int32_t LOADSTAGE  = 0;
    static constexpr int32_t PLAYSTAGE  = 1;
    static constexpr int32_t STAGEPAUSED= 2;

    static InputResult       gKeyDown;
    static InputResult       gKeyPress;
    static uint8_t           stageMode;
    static uint8_t           pauseEnabled;
    static int32_t           stageListPosition;
    static Mappings128x128   tile128x128;
    static LayoutMap         stageLayouts[9];
    static uint8_t           tLayerMidPoint;
    static uint8_t           activeTileLayers[4];
    static CollisionMask16x16 tileCollisions[2];
    static LineScrollParallax hParallax;
    static LineScrollParallax vParallax;
    static int32_t           lastXSize;
    static int32_t           lastYSize;
    static int32_t           bgDeformationData0[576];
    static int32_t           bgDeformationData1[576];
    static int32_t           bgDeformationData2[576];
    static int32_t           bgDeformationData3[576];
    static int32_t           xBoundary1;
    static int32_t           xBoundary2;
    static int32_t           yBoundary1;
    static int32_t           yBoundary2;
    static int32_t           newXBoundary1;
    static int32_t           newXBoundary2;
    static int32_t           newYBoundary1;
    static int32_t           newYBoundary2;
    static uint8_t           cameraEnabled;
    static int8_t            cameraTarget;
    static uint8_t           cameraShift;
    static uint8_t           cameraStyle;
    static int32_t           cameraAdjustY;
    static int32_t           xScrollOffset;
    static int32_t           yScrollOffset;
    static int32_t           yScrollA;
    static int32_t           yScrollB;
    static int32_t           xScrollA;
    static int32_t           xScrollB;
    static int32_t           xScrollMove;
    static int32_t           yScrollMove;
    static int32_t           screenShakeX;
    static int32_t           screenShakeY;
    static int32_t           waterLevel;
    static char              titleCardText[24];
    static char              titleCardWord2;
    static TextMenu          gameMenu[2];
    static uint8_t           timeEnabled;
    static uint8_t           milliSeconds;
    static uint8_t           seconds;
    static uint8_t           minutes;
    static uint8_t           debugMode;

    static void (*onBeforeStartupScripts)();

    static void ProcessStage();
    static void LoadStageFiles();
    static void Load128x128Mappings();
    static void LoadStageCollisions();
    static void LoadActLayout();
    static void LoadStageBackground();
    static void ResetBackgroundSettings();
    static void SetLayerDeformation(int32_t deformationData, int32_t sineWave,
                                    int32_t cosineWave, int32_t expandableWave,
                                    int32_t layerNum, int32_t notUsed = 0);
    static void DrawStageGfx();
    static void DrawHLineScrollLayer8(uint8_t layerNum);
    static void Draw3DFloorLayer(uint8_t layerNum);
    static void InitFirstStage();
    static void InitStageSelectMenu();
    static void InitErrorMessage();
    static void ProcessStageSelectMenu();
};

}
