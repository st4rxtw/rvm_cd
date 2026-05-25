#pragma once
#include <cstdint>

namespace rvm {

struct ScriptEngine {
    int32_t operands[10]{};
    int32_t tempValue[8]{};
    int32_t arrayPosition[3]{};
    int32_t checkResult{};
    int32_t sRegister{};
};

}
