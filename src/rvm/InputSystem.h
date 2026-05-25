#pragma once
#include <cstdint>
#include "InputResult.h"

struct GLFWwindow;

namespace rvm {

struct InputSystem {
    static constexpr int32_t BUTTON_UP    = 1;
    static constexpr int32_t BUTTON_DOWN  = 2;
    static constexpr int32_t BUTTON_LEFT  = 4;
    static constexpr int32_t BUTTON_RIGHT = 8;
    static constexpr int32_t BUTTON_A     = 16;
    static constexpr int32_t BUTTON_B     = 32;
    static constexpr int32_t BUTTON_C     = 64;
    static constexpr int32_t BUTTON_START = 128;
    static constexpr int32_t ALL_BUTTONS  = 255;
    static constexpr int32_t NO_BUTTONS   = 0;

    static int32_t     touchWidth;
    static int32_t     touchHeight;
    static bool        touchControls;
    static InputResult inputPress;
    static InputResult touchData;

    static void SetWindow(GLFWwindow* window);
    static void AddTouch(float touchX, float touchY, int32_t pointerID);
    static void SetTouch(float touchX, float touchY, int32_t pointerID);
    static void RemoveTouch(int32_t pointerID);
    static void ClearTouchData();
    static void CheckKeyboardInput();
    static void CheckKeyDown(InputResult& gameInput, uint8_t keyFlags);
    static void MenuKeyDown(InputResult& gameInput, uint8_t keyFlags);
    static void CheckKeyPress(InputResult& gameInput, uint8_t keyFlags);
};

}
