#pragma once
#include <cstdint>
#include <string>

namespace rvm {

struct EngineCallbacks {
    static constexpr int32_t DISPLAY_LOGOS          = 0;
    static constexpr int32_t TITLE_SCREEN_PRESS_START= 1;
    static constexpr int32_t ENTER_TIMEATTACK_NOTIFY= 2;
    static constexpr int32_t EXIT_TIMEATTACK_NOTIFY = 3;
    static constexpr int32_t FINISH_GAME_NOTIFY     = 4;
    static constexpr int32_t RETURN_TO_ARCADE_SELECTED=5;
    static constexpr int32_t RESTART_GAME_SELECTED  = 6;
    static constexpr int32_t EXIT_GAME_SELECTED     = 7;
    static constexpr int32_t UNLOCK_FULL_GAME_SELECTED=8;
    static constexpr int32_t TERMS_SELECTED         = 9;
    static constexpr int32_t PRIVACY_SELECTED       = 10;
    static constexpr int32_t TRIAL_ENDED            = 11;
    static constexpr int32_t SETTINGS_SELECTED      = 12;
    static constexpr int32_t FULL_VERSION_ONLY      = 14;

    static const char* restartTitle[6];
    static const char* exitTitle[6];
    static const char* restartMessage[6];
    static const char* exitMessage[6];
    static const char* upsellTitle[6];
    static const char* upsellMessage[6];
    static const char* updateTitle[6];
    static const char* updateMessage[6];
    static const char* liveErrorMessage[6];
    static const char* yesMessage[6];
    static const char* noMessage[6];
    static const char* achievementText[6];
    static const char* gamerscoreText[6];

    static int32_t achievementEarned[12];
    static int32_t achievementGamerScore[12];
    static int32_t achievementID[12];
    static int32_t earnedGamerScore;
    static std::string achievementName[12];
    static std::string achievementDesc[12];

    static void PlayVideoFile(const char* fileName);
    static void OnlineSetAchievement(int32_t achievementID, int32_t achievementDone);
    static void OnlineSetLeaderboard(int32_t leaderboardID, int32_t result);
    static void OnlineLoadAchievementsMenu();
    static void OnlineLoadLeaderboardsMenu();
    static void RetroEngineCallback(int32_t callbackID);
    static void StartupRetroEngine();
    static void ProcessMainLoop();

private:
    static int32_t prevMessage;
    static bool    engineInit;
    static int32_t waitValue;
};

}
