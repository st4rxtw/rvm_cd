#include "EngineCallbacks.h"
#include "GlobalAppDefinitions.h"
#include "GraphicsSystem.h"
#include "AudioPlayback.h"
#include "FileIO.h"
#include "ObjectSystem.h"
#include "StageSystem.h"
#include "TextSystem.h"
#include "RenderDevice.h"
#include <cstdio>
#include <cstring>

namespace rvm {

const char* EngineCallbacks::restartTitle[6] = {
    "Restart","Recommencer","Ricominciare","Neustart","Reiniciar","Restart"
};
const char* EngineCallbacks::exitTitle[6] = {
    "Exit","Quitter","Abbandonare","Verlassen","Abandonar","Exit"
};
const char* EngineCallbacks::restartMessage[6] = {
    "Are you sure you want to restart? Any unsaved progress will be lost.",
    "Veux-tu vraiment recommencer la partie? Toute progression non sauvegard\xE9""e sera perdue.",
    "Sei sicuro di voler ricominciare la partita? I progressi non salvati andranno perduti.",
    "M\xF6""chtest du das rennen wirklich verlassen? Ungespeicherter Fortschritt geht verloren.",
    "\xBF""Seguro que quieres reiniciar? Todo progreso no guardado se perder\xE1.",
    "\xe3\x83\xaa\xe3\x82\xb9\xe3\x82\xbf\xe3\x83\xbc\xe3\x83\x88\xe3\x81\x97\xe3\x81\xbe\xe3\x81\x99\xe3\x81\x8b\xef\xbc\x9f"
};
const char* EngineCallbacks::exitMessage[6] = {
    "Are you sure you want to exit? Any unsaved progress will be lost.",
    "Veux-tu vraiment quitter la partie? Toute progression non sauvegard\xE9""e sera perdue.",
    "Sei sicuro di voler abbandonare la partita? I progressi non salvati andranno perduti.",
    "M\xF6""chtest du das spiel wirklich verlassen? Ungespeicherter Fortschritt geht verloren.",
    "\xBF""Seguro que quieres abandonar? Todo progreso no guardado se perder\xE1.",
    "\xe3\x82\xb2\xe3\x83\xbc\xe3\x83\xa0\xe3\x82\x92\xe7\xb5\x82\xe4\xba\x86\xe3\x81\x97\xe3\x81\xbe\xe3\x81\x99\xe3\x81\x8b\xef\xbc\x9f"
};
const char* EngineCallbacks::upsellTitle[6] = {
    "Buy Full Game","Acheter le jeu complet","Acquista gioco completo",
    "Spiele-Vollversion kaufen","Comprar juego completo","\xe5\xae\x8c\xe5\x85\xa8\xe7\x89\x88\xe3\x81\xae\xe8\xb3\xbc\xe5\x85\xa5"
};
const char* EngineCallbacks::upsellMessage[6] = {
    "This feature is not available in the trial version. Buy Full Game?",
    "Cette option n'est pas disponible avec la version d'essai. Acheter le jeu complet ?",
    "Questa opzione non \xE8 disponibile nella versione di prova. Acquistare il gioco completo?",
    "Diese Funktion ist in der Demo-Version nicht verf\xFC""gbar. Spiele-Vollversion kaufen?",
    "Esta funci\xF3""n no est\xE1 disponible en la prueba. \xBF""Comprar juego completo?",
    "\xe5\xae\x8c\xe5\x85\xa8\xe7\x89\x88\xe3\x82\x92\xe3\x82\xa2\xe3\x83\xb3\xe3\x83\xad\xe3\x83\x83\xe3\x82\xaf"
};
const char* EngineCallbacks::updateTitle[6] = {
    "Title Update Available","Mise \xE0 jour du titre disponible",
    "\xC8 disponibile un aggiornamento del gioco","Titel-Aktualisierung verf\xFC""gbar",
    "Actualizaci\xF3""n del t\xED""tulo disponible",
    "\xe3\x82\xa2\xe3\x83\x83\xe3\x83\x97\xe3\x83\x87\xe3\x83\xbc\xe3\x83\x88\xe3\x81\x8c\xe5\x8f\xaf\xe8\x83\xbd\xe3\x81\xa7\xe3\x81\x99"
};
const char* EngineCallbacks::updateMessage[6] = {
    "An update is available! Update now?",
    "Une mise \xE0 jour est disponible ! Mettre \xE0 jour maintenant ?",
    "\xC8 disponibile un aggiornamento! Aggiornare ora?",
    "Eine Aktualisierung ist verf\xFC""gbar! Jetzt aktualisieren?",
    "\xA1""Hay una actualizaci\xF3""n disponible! \xBF""Actualizar ahora?",
    "\xe3\x82\xa2\xe3\x83\x83\xe3\x83\x97\xe3\x83\x87\xe3\x83\xbc\xe3\x83\x88\xe3\x82\x92\xe8\xa1\x8c\xe3\x81\x84\xe3\x81\xbe\xe3\x81\x99\xe3\x81\x8b\xef\xbc\x9f"
};
const char* EngineCallbacks::liveErrorMessage[6] = {
    "Unable to connect at this time. Please check your connection.",
    "Connexion impossible. V\xE9""rifiez votre connexion.",
    "Impossibile connettersi. Verifica la connessione.",
    "Es konnte keine Verbindung hergestellt werden. Bitte \xFC""berpr\xFC""fen Sie Ihre Verbindung.",
    "No se puede conectar. Comprueba tu conexi\xF3""n.",
    "\xe6\x8e\xa5\xe7\xb6\x9a\xe3\x81\xa7\xe3\x81\x8d\xe3\x81\xbe\xe3\x81\x9b\xe3\x82\x93\xe3\x81\xa7\xe3\x81\x97\xe3\x81\x9f\xe3\x80\x82"
};
const char* EngineCallbacks::yesMessage[6]  = {"Yes","Oui","S\xED","Ya","S\xED","Yes"};
const char* EngineCallbacks::noMessage[6]   = {"No","Non","No","Nein","No","No"};
const char* EngineCallbacks::achievementText[6] = {
    "Achievements ","Succ\xE8""s ","Obiettivi ","Erfolge ","Logros ","Achievements "
};
const char* EngineCallbacks::gamerscoreText[6] = {
    "Gamerscore ","Score ","Punteggio ","Punkte ","Puntuaci\xF3""n ","Gamerscore "
};

int32_t     EngineCallbacks::achievementEarned[12]    {};
int32_t     EngineCallbacks::achievementGamerScore[12]{};
int32_t     EngineCallbacks::achievementID[12]        {};
int32_t     EngineCallbacks::earnedGamerScore          = 0;
std::string EngineCallbacks::achievementName[12]      {};
std::string EngineCallbacks::achievementDesc[12]      {};

int32_t EngineCallbacks::prevMessage = 0;
bool    EngineCallbacks::engineInit  = false;
int32_t EngineCallbacks::waitValue   = 0;

void EngineCallbacks::PlayVideoFile(const char* /*fileName*/)
{
    AudioPlayback::StopMusic();
    waitValue = 0;
    GlobalAppDefinitions::gameMode = 9;
}

void EngineCallbacks::OnlineSetAchievement(int32_t achievementId, int32_t achievementDone)
{
    if (achievementDone <= 99 ||
        GlobalAppDefinitions::gameOnlineActive != 1 ||
        GlobalAppDefinitions::gameTrialMode != 0)
        return;

    static const char* names[12] = {
        "88 Miles Per Hour","Just One Hug is Enough","Paradise Found",
        "Take the High Road","King of the Rings","Statue Saviour",
        "Heavy Metal","All Stages Clear","Treasure Hunter",
        "Dr Eggman Got Served","Just In Time","Saviour of the Planet"
    };
    if (achievementId >= 0 && achievementId < 12) {
        achievementEarned[achievementId] = 1;
        (void)names[achievementId];
    }
}

void EngineCallbacks::OnlineSetLeaderboard(int32_t /*leaderboardID*/, int32_t /*result*/) {}

void EngineCallbacks::OnlineLoadAchievementsMenu()
{
    int32_t num = 0;
    for (int i = 0; i < 12; ++i) num += achievementEarned[i];

    TextSystem::SetupTextMenu(StageSystem::gameMenu[0], 0);
    TextSystem::SetupTextMenu(StageSystem::gameMenu[1], 0);

    int lang = (int)GlobalAppDefinitions::gameLanguage;
    if (lang < 0 || lang > 5) lang = 0;

    char buf[128];
    std::snprintf(buf, sizeof(buf), "%s    (%d/12)", achievementText[lang], num);
    TextSystem::AddTextMenuEntryMapped(StageSystem::gameMenu[0], buf);
    std::snprintf(buf, sizeof(buf), "%s    (%d/200)", gamerscoreText[lang], earnedGamerScore);
    TextSystem::AddTextMenuEntryMapped(StageSystem::gameMenu[0], buf);

    for (int i = 0; i < 12; ++i) {
        ObjectSystem::objectEntityList[34 + i].value[1] = achievementEarned[i];
        ObjectSystem::objectEntityList[34 + i].frame    = (uint8_t)achievementID[i];
        const char* aname = achievementName[i].empty() ? "Achievement Name" : achievementName[i].c_str();
        std::snprintf(buf, sizeof(buf), "%s    (%d G)", aname, achievementGamerScore[i]);
        TextSystem::AddTextMenuEntryMapped(StageSystem::gameMenu[1], buf);
        const char* adesc = achievementDesc[i].empty() ? "Achievement Description" : achievementDesc[i].c_str();
        TextSystem::AddTextMenuEntryMapped(StageSystem::gameMenu[1], adesc);
    }
}

void EngineCallbacks::OnlineLoadLeaderboardsMenu() {}

void EngineCallbacks::RetroEngineCallback(int32_t callbackID)
{

    switch (callbackID) {
        case RESTART_GAME_SELECTED:
            GlobalAppDefinitions::gameMode    = 1;
            GlobalAppDefinitions::gameMessage = 3;
            break;
        case EXIT_GAME_SELECTED:
            AudioPlayback::ResumeSound();
            GlobalAppDefinitions::gameMode    = 2;
            GlobalAppDefinitions::gameMessage = 0;
            break;
        case UNLOCK_FULL_GAME_SELECTED:
            GlobalAppDefinitions::gameMode    = 1;
            GlobalAppDefinitions::gameMessage = 3;
            break;
        case FULL_VERSION_ONLY:
            GlobalAppDefinitions::gameMode    = 1;
            GlobalAppDefinitions::gameMessage = 3;
            break;
        default:
            break;
    }
}

void EngineCallbacks::StartupRetroEngine()
{
    if (!engineInit) {
        GlobalAppDefinitions::CalculateTrigAngles();
        GraphicsSystem::GenerateBlendLookupTable();
        if (FileIO::CheckRSDKFile())
            GlobalAppDefinitions::LoadGameConfig("Data/Game/GameConfig.bin");
        AudioPlayback::InitAudioPlayback();
        StageSystem::InitFirstStage();
        ObjectSystem::ClearScriptData();
        engineInit = true;
    } else {
        RenderDevice::UpdateHardwareTextures();
    }
}

void EngineCallbacks::ProcessMainLoop()
{
    switch (GlobalAppDefinitions::gameMode) {
        case 0:
            GraphicsSystem::gfxIndexSize        = 0;
            GraphicsSystem::gfxVertexSize       = 0;
            GraphicsSystem::gfxIndexSizeOpaque  = 0;
            GraphicsSystem::gfxVertexSizeOpaque = 0;
            StageSystem::ProcessStageSelectMenu();
            break;
        case 1:
            GraphicsSystem::gfxIndexSize        = 0;
            GraphicsSystem::gfxVertexSize       = 0;
            GraphicsSystem::gfxIndexSizeOpaque  = 0;
            GraphicsSystem::gfxVertexSizeOpaque = 0;
            GraphicsSystem::vertexSize3D        = 0;
            GraphicsSystem::indexSize3D         = 0;
            GraphicsSystem::render3DEnabled     = false;
            StageSystem::ProcessStage();
            if (prevMessage == GlobalAppDefinitions::gameMessage) {
                GlobalAppDefinitions::gameMessage = 0;
                prevMessage = 0;
            } else {
                prevMessage = GlobalAppDefinitions::gameMessage;
            }
            break;
        case 2:
            GlobalAppDefinitions::LoadGameConfig("Data/Game/GameConfig.bin");
            StageSystem::InitFirstStage();
            FileIO::ResetCurrentStageFolder();
            break;
        case 4:
            GlobalAppDefinitions::LoadGameConfig("Data/Game/GameConfig.bin");
            StageSystem::InitErrorMessage();
            FileIO::ResetCurrentStageFolder();
            break;
        case 5:
            GlobalAppDefinitions::gameMode = 1;
            RenderDevice::highResMode = 1;
            break;
        case 6:
            GlobalAppDefinitions::gameMode = 1;
            RenderDevice::highResMode = 0;
            break;
        case 8:
            if (waitValue < 8) { ++waitValue; break; }
            GlobalAppDefinitions::gameMode = 1;
            break;
        case 9:
            if (waitValue < 60) { ++waitValue; break; }
            GlobalAppDefinitions::gameMode = 1;
            break;
    }
}

}
