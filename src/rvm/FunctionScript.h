#pragma once
#include <cstdint>

namespace rvm {

struct FunctionScript {
    int32_t mainScript{};
    int32_t mainJumpTable{};
};

}
