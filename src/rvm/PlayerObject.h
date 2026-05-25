#pragma once
#include <cstdint>
#include "PlayerStatistics.h"
#include "AnimationFileList.h"
#include "ObjectEntity.h"

namespace rvm {

struct PlayerObject {
    int32_t          objectNum{};
    int32_t          xPos{};
    int32_t          yPos{};
    int32_t          xVelocity{};
    int32_t          yVelocity{};
    int32_t          speed{};
    int32_t          screenXPos{};
    int32_t          screenYPos{};
    int32_t          angle{};
    int32_t          timer{};
    int32_t          lookPos{};
    int32_t          value[8]{};
    uint8_t          collisionMode{};
    uint8_t          skidding{};
    uint8_t          pushing{};
    uint8_t          collisionPlane{};
    int8_t           controlMode{};
    uint8_t          controlLock{};
    PlayerStatistics movementStats{};
    uint8_t          visible{};
    uint8_t          tileCollisions{};
    uint8_t          objectInteraction{};
    uint8_t          left{};
    uint8_t          right{};
    uint8_t          up{};
    uint8_t          down{};
    uint8_t          jumpPress{};
    uint8_t          jumpHold{};
    uint8_t          followPlayer1{};
    uint8_t          trackScroll{};
    uint8_t          gravity{};
    uint8_t          water{};
    uint8_t          flailing[3]{};
    AnimationFileList* animationFile{};
    ObjectEntity*    objectPtr{};
};

}
