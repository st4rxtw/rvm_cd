#pragma once
#include <cstdint>

namespace rvm {

struct ObjectEntity {
    int32_t xPos{};
    int32_t yPos{};
    int32_t value[8]{};
    int32_t scale{};
    int32_t rotation{};
    int32_t animationTimer{};
    int32_t animationSpeed{};
    uint8_t type{};
    uint8_t propertyValue{};
    uint8_t state{};
    uint8_t priority{};
    uint8_t drawOrder{};
    uint8_t direction{};
    uint8_t inkEffect{};
    int32_t alpha{};
    uint8_t animation{};
    uint8_t prevAnimation{};
    uint8_t frame{};
};

}
