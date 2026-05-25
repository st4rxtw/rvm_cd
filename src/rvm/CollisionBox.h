#pragma once
#include <cstdint>

namespace rvm {

struct CollisionBox {
    int8_t left[8]{};
    int8_t top[8]{};
    int8_t right[8]{};
    int8_t bottom[8]{};
};

}
