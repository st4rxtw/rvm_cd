#pragma once
#include <cstdint>

struct GLFWwindow;

namespace cd {

struct Game {

    static void Init(GLFWwindow* window, int32_t windowWidth, int32_t windowHeight);

    static void Update(GLFWwindow* window);

    static void Draw();

    static void OnFocusGained();
    static void OnFocusLost();
};

}
