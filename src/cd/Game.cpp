
#include "Game.h"
#include "Title/TitleInit.h"
#include "Global/GlobalInit.h"
#include <rvm/GlobalAppDefinitions.h>
#include <rvm/RenderDevice.h>
#include <rvm/EngineCallbacks.h>
#include <rvm/InputSystem.h>
#include <rvm/AudioPlayback.h>
#include <rvm/StageSystem.h>
#include <rvm/ObjectSystem.h>
#include <rvm/FileIO.h>
#include <rvm/VideoPlayer.h>

#ifndef __SWITCH__
#include <GLFW/glfw3.h>
#endif

namespace cd {

void Game::Init(void* window, int32_t windowWidth, int32_t windowHeight)
{
    using namespace rvm;

#ifndef __SWITCH__
    InputSystem::SetWindow(static_cast<GLFWwindow*>(window));
#endif

    GlobalAppDefinitions::CalculateTrigAngles();

    RenderDevice::InitRenderDevice();
    RenderDevice::SetScreenDimensions(windowWidth, windowHeight);

    rvm::StageSystem::onBeforeStartupScripts = RegisterAllNativeScripts;

    EngineCallbacks::StartupRetroEngine();

    GlobalAppDefinitions::gameLanguage      = 0;
    GlobalAppDefinitions::gameTrialMode     = 0;
    GlobalAppDefinitions::gameOnlineActive  = 0;
}

void Game::Update(void* window)
{
    using namespace rvm;

    InputSystem::CheckKeyboardInput();
    InputSystem::ClearTouchData();

#ifndef __SWITCH__
    if (glfwGetKey(static_cast<GLFWwindow*>(window), GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        if (FileIO::activeStageList == 0) {
            if (StageSystem::stageListPosition == 4 ||
                StageSystem::stageListPosition == 5)
                InputSystem::touchData.start   = 1;
            else
                InputSystem::touchData.buttonB = 1;
        } else if (StageSystem::stageMode == 2) {
            ObjectEntity& pauseObj = ObjectSystem::objectEntityList[9];
            if (pauseObj.state == 3 && GlobalAppDefinitions::gameMode == 1) {
                pauseObj.state    = 4;
                pauseObj.value[0] = 0;
                pauseObj.value[1] = 0;
                pauseObj.alpha    = 248;
                AudioPlayback::PlaySfx(27, 0);
            }
        } else {
            GlobalAppDefinitions::gameMessage = 2;
        }
    }
#endif

    if (StageSystem::stageMode != 2)
        EngineCallbacks::ProcessMainLoop();
}

void Game::Draw()
{
    using namespace rvm;

    if (StageSystem::stageMode == 2)
        EngineCallbacks::ProcessMainLoop();

    if (GlobalAppDefinitions::gameMode == 9) {
        if (VideoPlayer::frameRGB)
            RenderDevice::PresentVideoFrame(VideoPlayer::width, VideoPlayer::height,
                                            VideoPlayer::frameRGB);
    } else if (RenderDevice::highResMode == 0) {
        RenderDevice::FlipScreen();
    } else {
        RenderDevice::FlipScreenHRes();
    }
}

void Game::OnFocusGained()
{
    using namespace rvm;
    if (StageSystem::stageMode == 2) {
        if (GlobalAppDefinitions::gameMode == 7) {
            GlobalAppDefinitions::gameMode    = 1;
            GlobalAppDefinitions::gameMessage = 4;
        }
    } else {
        if (GlobalAppDefinitions::gameMode == 7)
            GlobalAppDefinitions::gameMode = 1;
        GlobalAppDefinitions::gameMessage = 2;
        AudioPlayback::ResumeSound();
    }
}

void Game::OnFocusLost()
{
    using namespace rvm;
    GlobalAppDefinitions::gameMessage = 2;
    if (StageSystem::stageMode != 2 && GlobalAppDefinitions::gameMode != 7)
        AudioPlayback::PauseSound();
    GlobalAppDefinitions::gameMode = 7;
}

}
