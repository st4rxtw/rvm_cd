#pragma once
#include <cstdint>

namespace rvm {

struct PlayerStatistics {
    int32_t topSpeed{};
    int32_t acceleration{};
    int32_t deceleration{};
    int32_t airAcceleration{};
    int32_t airDeceleration{};
    int32_t gravity{};
    int32_t jumpStrength{};
    int32_t jumpCap{};
    int32_t rollingAcceleration{};
    int32_t rollingDeceleration{};
};

}
