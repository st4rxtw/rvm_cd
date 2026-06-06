#pragma once
#include <cstdint>

namespace cd {

struct Game {
    static void Init(void* window, int32_t windowWidth, int32_t windowHeight);
    static void Update(void* window);
    static void Draw();
    static void OnFocusGained();
    static void OnFocusLost();
};

}
