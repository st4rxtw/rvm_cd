#include "InputSystem.h"
#include "GlobalAppDefinitions.h"
#include "ObjectSystem.h"
#ifndef __SWITCH__
#include <GLFW/glfw3.h>
#endif

namespace rvm {

int32_t     InputSystem::touchWidth    = 640;
int32_t     InputSystem::touchHeight   = 480;
bool        InputSystem::touchControls = false;
InputResult InputSystem::inputPress{};
InputResult InputSystem::touchData{};

#ifndef __SWITCH__
static GLFWwindow* s_window = nullptr;
void InputSystem::SetWindow(GLFWwindow* window) { s_window = window; }
#else
void InputSystem::SetWindow(GLFWwindow*) {}
#endif

void InputSystem::AddTouch(float touchX, float touchY, int32_t pointerID)
{
    for (int i = 0; i < 4; ++i) {
        if (touchData.touchDown[i] == 0) {
            touchData.touchDown[i] = 1;
            touchData.touchY[i]   = (int32_t)touchY;
            touchData.touchX[i]   = (int32_t)touchX;
            touchData.touchID[i]  = pointerID;
            return;
        }
    }
}

void InputSystem::SetTouch(float touchX, float touchY, int32_t pointerID)
{
    for (int i = 0; i < 4; ++i) {
        if (touchData.touchID[i] == pointerID && touchData.touchDown[i] == 1) {
            touchData.touchY[i] = (int32_t)touchY;
            touchData.touchX[i] = (int32_t)touchX;
            return;
        }
    }
}

void InputSystem::RemoveTouch(int32_t pointerID)
{
    for (int i = 0; i < 4; ++i)
        if (touchData.touchID[i] == pointerID)
            touchData.touchDown[i] = 0;
}

void InputSystem::ClearTouchData()
{
    touchData.touches      = 0;
    touchData.touchDown[0] = 0;
    touchData.touchDown[1] = 0;
    touchData.touchDown[2] = 0;
    touchData.touchDown[3] = 0;
}

void InputSystem::CheckKeyboardInput()
{
#ifndef __SWITCH__
    if (!s_window) return;
    auto key = [](GLFWwindow* w, int k) {
        return glfwGetKey(w, k) == GLFW_PRESS ? (uint8_t)1 : (uint8_t)0;
    };
    touchData.up      = key(s_window, GLFW_KEY_UP)    | key(s_window, GLFW_KEY_W);
    touchData.down    = key(s_window, GLFW_KEY_DOWN)   | key(s_window, GLFW_KEY_S);
    touchData.left    = key(s_window, GLFW_KEY_LEFT)   | key(s_window, GLFW_KEY_A);
    touchData.right   = key(s_window, GLFW_KEY_RIGHT)  | key(s_window, GLFW_KEY_D);
    touchData.buttonA = key(s_window, GLFW_KEY_KP_1)   | key(s_window, GLFW_KEY_J);
    touchData.buttonB = key(s_window, GLFW_KEY_KP_2)   | key(s_window, GLFW_KEY_K);
    touchData.buttonC = key(s_window, GLFW_KEY_KP_3)   | key(s_window, GLFW_KEY_L);
    touchData.start   = key(s_window, GLFW_KEY_ENTER)  | key(s_window, GLFW_KEY_V);
    if (key(s_window, GLFW_KEY_SPACE)) {
        touchControls = true;
    } else {
        if (touchControls)
            ObjectSystem::globalVariables[110] = ObjectSystem::globalVariables[110] + 1 & 1;
        touchControls = false;
    }
#else
    // nx input is handled in main.cpp
#endif
}

void InputSystem::CheckKeyDown(InputResult& gameInput, uint8_t keyFlags)
{
    gameInput.touches = 0;
    for (int i = 0; i < 4; ++i) {
        if (touchData.touchDown[i] == 1) {
            int t = gameInput.touches;
            gameInput.touchDown[t] = touchData.touchDown[i];
            gameInput.touchX[t]   = touchData.touchX[i] * GlobalAppDefinitions::SCREEN_XSIZE / touchWidth;
            gameInput.touchY[t]   = touchData.touchY[i] * 240 / touchHeight;
            ++gameInput.touches;
        }
    }
    if (keyFlags & 1)   gameInput.up      = touchData.up;
    if (keyFlags & 2)   gameInput.down    = touchData.down;
    if (keyFlags & 4)   gameInput.left    = touchData.left;
    if (keyFlags & 8)   gameInput.right   = touchData.right;
    if (keyFlags & 16)  gameInput.buttonA = touchData.buttonA;
    if (keyFlags & 32)  gameInput.buttonB = touchData.buttonB;
    if (keyFlags & 64)  gameInput.buttonC = touchData.buttonC;
    if (keyFlags & 128) gameInput.start   = touchData.start;
}

void InputSystem::MenuKeyDown(InputResult& gameInput, uint8_t /*keyFlags*/)
{
    gameInput.touches = 0;
    for (int i = 0; i < 4; ++i) {
        if (touchData.touchDown[i] == 1) {
            int t = gameInput.touches;
            gameInput.touchDown[t] = touchData.touchDown[i];
            gameInput.touchX[t]   = touchData.touchX[i] * GlobalAppDefinitions::SCREEN_XSIZE / touchWidth;
            gameInput.touchY[t]   = touchData.touchY[i] * 240 / touchHeight;
            ++gameInput.touches;
        }
    }
}

void InputSystem::CheckKeyPress(InputResult& gameInput, uint8_t keyFlags)
{
#define CHECK_KEY(flag, field)                          \
    if ((keyFlags & (flag)) == (flag)) {                \
        if (touchData.field == 1) {                     \
            if (inputPress.field == 0) {                \
                inputPress.field = 1;                   \
                gameInput.field  = 1;                   \
            } else gameInput.field = 0;                 \
        } else { gameInput.field = 0; inputPress.field = 0; } \
    }
    CHECK_KEY(1,   up)
    CHECK_KEY(2,   down)
    CHECK_KEY(4,   left)
    CHECK_KEY(8,   right)
    CHECK_KEY(16,  buttonA)
    CHECK_KEY(32,  buttonB)
    CHECK_KEY(64,  buttonC)
    CHECK_KEY(128, start)
#undef CHECK_KEY
}

}
