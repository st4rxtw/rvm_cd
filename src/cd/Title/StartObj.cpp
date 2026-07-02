#include "../NativeScript.h"

namespace cd {

enum StartState { START_SETUP=0, START_GOTO_MENU=1, START_GOTO_STAGESELECT=2, START_GOTO_SOUNDTEST=3, START_GOTO_DEMO=4 };

static constexpr int32_t STAGE_P_MENU        = 1;
static constexpr int32_t STAGE_P_STAGESELECT = 3;
static constexpr int32_t STAGE_P_SOUNDTEST   = 4;

static constexpr int32_t REGULAR_STAGE = 1;
static constexpr int32_t SPECIAL_STAGE = 2;

void Start_Startup()
{
    NS_LoadSpriteSheet("Title/Title.gif");

    switch (GlobalAppDefinitions::gameLanguage) {
    case GlobalAppDefinitions::RETRO_FR:
        NS_SpriteFrame(-60,  0, 120, 11, 391, 387);
        NS_SpriteFrame(-94,  0, 189, 11, 322, 399);
        break;
    case GlobalAppDefinitions::RETRO_IT:
        NS_SpriteFrame(-44,  0,  88, 11, 270, 411);
        NS_SpriteFrame(-76,  0, 152, 11, 359, 411);
        break;
    case GlobalAppDefinitions::RETRO_DE:
        NS_SpriteFrame(-52, -2, 106, 13, 405, 373);
        NS_SpriteFrame(-87,  0, 175, 11, 336, 423);
        break;
    case GlobalAppDefinitions::RETRO_ES:
        NS_SpriteFrame(-43,  0,  87, 11, 274, 435);
        NS_SpriteFrame(-74,  0, 149, 11, 362, 435);
        break;
    default:
        NS_SpriteFrame(-43,  0,  87, 11, 106, 103);
        NS_SpriteFrame(-56,  0, 117, 11, 106,  91);
        break;
    }
    NS_SpriteFrame(-48,  0,  97, 11, 414, 361);
    NS_SpriteFrame(-88, -8,  88,  8, 423, 311);
}

void Start_Main(int32_t entityNo)
{
    ObjectEntity& e = ObjectSystem::objectEntityList[entityNo];

    switch (e.state) {
    case START_SETUP: {
        int32_t touched = NS_CheckTouchRect(0, 0, GlobalAppDefinitions::SCREEN_XSIZE, 240);
        if (StageSystem::gKeyPress.start)
            touched = 0;

        if (touched > -1) {
            e.state = START_GOTO_MENU;
            NS_StopMusic();
            NS_PlaySfx(27, false);
        } else {
            e.value[3]++;
            if (e.value[3] == 1550)
                e.state = START_GOTO_DEMO;
        }

        if (GlobalAppDefinitions::gamePlatformID == GlobalAppDefinitions::RETRO_WP7)
            if (StageSystem::gKeyPress.buttonB)
                EngineCallbacks::RetroEngineCallback(EngineCallbacks::EXIT_GAME_SELECTED);
        break;
    }

    case START_GOTO_MENU:
        e.value[0] += 4;
        if (e.value[0] == 384) {
            StageSystem::stageListPosition = STAGE_P_MENU;
            NS_LoadStage();
        }
        NS_SetScreenFade(0, 0, 0, e.value[0]);
        break;

    case START_GOTO_STAGESELECT:
        e.value[0] += 4;
        if (e.value[0] == 384) {
            NS_SetGlobalVar("Options.GameMode", 0);
            StageSystem::stageListPosition = STAGE_P_STAGESELECT;
            NS_LoadStage();
        }
        NS_SetScreenFade(0, 0, 0, e.value[0]);
        break;

    case START_GOTO_SOUNDTEST:
        e.value[0] += 4;
        if (e.value[0] == 384) {
            NS_SetGlobalVar("Options.GameMode", 0);
            StageSystem::stageListPosition = STAGE_P_SOUNDTEST;
            NS_LoadStage();
        }
        NS_SetScreenFade(0, 0, 0, e.value[0]);
        break;

    case START_GOTO_DEMO:
        e.value[0] += 4;
        if (e.value[0] == 384) {
            GlobalAppDefinitions::gameHapticsEnabled = 0;

            NS_SetGlobalVar("Fade_Colour", 0x000000);
            NS_SetGlobalVar("Player.Score",       0);
            NS_SetGlobalVar("Player.Lives",       3);
            NS_SetGlobalVar("Player.ScoreBonus",  50000);
            NS_SetGlobalVar("Transporter_Destroyed", 0);
            NS_SetGlobalVar("MetalSonic_Destroyed",  0);
            NS_SetGlobalVar("Good_Future",           0);
            NS_SetGlobalVar("Options.GameMode",      0);
            NS_SetGlobalVar("Options.AttractMode",   1);

            int32_t demoNum = NS_GetGlobalVar("Options.DemoNumber");

            switch (demoNum) {
            case 0:
                FileIO::activeStageList    = (uint8_t)REGULAR_STAGE;
                StageSystem::stageListPosition = 0;
                break;
            case 1:
                FileIO::activeStageList    = (uint8_t)SPECIAL_STAGE;
                StageSystem::stageListPosition = 0;
                break;
            case 2:
                FileIO::activeStageList    = (uint8_t)REGULAR_STAGE;
                StageSystem::stageListPosition = 28;
                break;
            case 3:
                FileIO::activeStageList    = (uint8_t)SPECIAL_STAGE;
                StageSystem::stageListPosition = 5;
                break;
            case 4:
                FileIO::activeStageList    = (uint8_t)REGULAR_STAGE;
                StageSystem::stageListPosition = 64;
                break;
            }

            demoNum = (demoNum + 1) % 5;
            NS_SetGlobalVar("Options.DemoNumber", demoNum);

            NS_LoadStage();
        }
        NS_SetScreenFade(0, 0, 0, e.value[0]);
        break;
    }
}

void Start_Draw(int32_t entityNo)
{
    ObjectEntity& e = ObjectSystem::objectEntityList[entityNo];

    e.animationTimer++;
    if (e.animationTimer > 59)
        e.animationTimer = 0;

    if (e.animationTimer > 29)
        NS_DrawSprite(entityNo, 1);
}

}
