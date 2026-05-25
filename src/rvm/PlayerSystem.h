#pragma once
#include <cstdint>
#include "PlayerObject.h"
#include "CollisionSensor.h"

namespace rvm {

struct PlayerSystem {
    static constexpr int32_t MAX_PLAYERS = 2;

    static uint16_t      delayLeft;
    static uint16_t      delayRight;
    static uint16_t      delayUp;
    static uint16_t      delayDown;
    static uint16_t      delayJumpPress;
    static uint16_t      delayJumpHold;
    static uint8_t       jumpWait;
    static uint8_t       numActivePlayers;
    static uint8_t       playerMenuNum;
    static PlayerObject  playerList[2];
    static CollisionSensor cSensor[6];
    static int32_t       collisionLeft;
    static int32_t       collisionTop;
    static int32_t       collisionRight;
    static int32_t       collisionBottom;

    static void SetPlayerScreenPosition(PlayerObject& playerO);
    static void SetPlayerScreenPositionCDStyle(PlayerObject& playerO);
    static void SetPlayerLockedScreenPosition(PlayerObject& playerO);
    static void SetPlayerHLockedScreenPosition(PlayerObject& playerO);
    static void ProcessPlayerControl(PlayerObject& playerO);
    static void ProcessPlayerTileCollisions(PlayerObject& playerO);
    static void ProcessAirCollision(PlayerObject& playerO);
    static void SetPathGripSensors(PlayerObject& playerO);
    static void ProcessPathGrip(PlayerObject& playerO);
    static void FloorCollision(PlayerObject& playerO, CollisionSensor& cSensorRef);
    static void FindFloorPosition(PlayerObject& playerO, CollisionSensor& cSensorRef, int32_t prevCollisionPos);
    static void LWallCollision(PlayerObject& playerO, CollisionSensor& cSensorRef);
    static void FindLWallPosition(PlayerObject& playerO, CollisionSensor& cSensorRef, int32_t prevCollisionPos);
    static void RWallCollision(PlayerObject& playerO, CollisionSensor& cSensorRef);
    static void FindRWallPosition(PlayerObject& playerO, CollisionSensor& cSensorRef, int32_t prevCollisionPos);
    static void RoofCollision(PlayerObject& playerO, CollisionSensor& cSensorRef);
    static void FindRoofPosition(PlayerObject& playerO, CollisionSensor& cSensorRef, int32_t prevCollisionPos);
};

}
