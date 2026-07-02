#include "../NativeScript.h"

namespace cd {

static inline PlayerObject& P()   { return PlayerSystem::playerList[0]; }
static inline ObjectEntity& Pe()  { return *PlayerSystem::playerList[0].objectPtr; }
static inline ObjectEntity& Obj() { return ObjectSystem::objectEntityList[ObjectSystem::objectLoop]; }
static inline int32_t  g(const char* n)            { return NS_GetGlobalVar(n); }
static inline void     sg(const char* n, int32_t v){ NS_SetGlobalVar(n, v); }

enum { FACING_RIGHT = 0, FACING_LEFT = 1 };
enum { GRAVITY_GROUND = 0, GRAVITY_AIR = 1 };
enum { CMODE_FLOOR = 0 };
enum { CONTROLMODE_NONE = -1, CONTROLMODE_NORMAL = 0 };
enum { COLLISION_PLANE_A = 0 };
enum { CAMERASTYLE_FOLLOW = 0, CAMERASTYLE_EXTENDED = 1, CAMERASTYLE_EXTENDED_OFFSET_L = 2,
       CAMERASTYLE_EXTENDED_OFFSET_R = 3, CAMERASTYLE_HLOCKED = 4 };
enum { DAMAGE_SHIELDED = 1, DAMAGE_HURT = 2, DAMAGE_DEATH = 3 };
enum { NO_SHIELD = 0, ACTIVE_SHIELD = 1 };
enum { MESSAGE_LOSTFOCUS = 2, STAGE_PAUSED = 2 };
enum { PRIORITY_ACTIVE = 1, PRIORITY_ALWAYS = 2 };
enum { MODE_TIMEATTACK = 2, WARPDEST_NONE = 0 };
enum { DEATHEVENT_GAMEOVER = 0, DEATHEVENT_TIMEOVER = 1, DEATHEVENT_FADEOUT = 2, DEATHEVENT_TIMEATTACK = 3 };
enum { SFX_G_JUMP = 0, SFX_G_SKIDDING = 3, SFX_G_LOSERINGS = 4, SFX_G_HURT = 5, SFX_G_CHARGE = 6,
       SFX_G_RELEASE = 7, SFX_G_DESTROY = 8, SFX_G_FLYING = 24, SFX_G_TIRED = 25,
       SFX_G_OUTTAHERE = 26, SFX_G_SELECT = 27 };
enum { PLAYER_SONIC_A = 0, PLAYER_TAILS_A = 1 };

static int32_t aniStopped, aniWaiting, aniBored, aniLookingUp, aniLookingDown, aniWalking,
    aniRunning, aniSkidding, aniPeelout, aniSpindash, aniJumping, aniBouncing, aniHurt,
    aniDying, aniDrowning, aniPushing, aniFlailingL, aniFlailingR, aniHanging, aniFlying,
    aniFlyingTired, aniSwimming, aniSwimmingTired, aniSpinningTop, aniSizeChange;
static int32_t aniRamp[7];

enum PlayerFn {
    PF_Static = 0, PF_Ground, PF_Air, PF_Roll, PF_LookUp, PF_Crouch,
    PF_Spindash_S2, PF_Spindash_CD, PF_Peelout_S2, PF_Peelout_CD, PF_Fly,
    PF_GotHit, PF_Hurt, PF_OuttaHere, PF_Death, PF_Drown,
    PF_HangBar, PF_TubeRoll, PF_TubeAirRoll, PF_SpinningTop, PF_Hugged,
    PF_Ramp3D, PF_WaterCurrent, PF_SizeChange,
    PF_Action_Jump, PF_Action_Spindash_S2, PF_Action_Spindash_CD,
    PF_Action_Peelout_S2, PF_Action_Peelout_CD, PF_Action_DblJumpTails,
    PF_COUNT
};

static void CallFn(int32_t id);

static void Player_Static() { ObjectSystem::scriptEng.checkResult = 0; }

static void Player_HandleRollAnimSpeed()
{
    if (NS_PlayerListPos() == PLAYER_TAILS_A) {
        P().value[2] = 120;
    } else {
        int32_t s = P().speed;
        if (s < 0) s = -s;
        s *= 240; s /= 0x60000; s += 48;
        P().value[2] = s;
    }
}

static void Player_HandleWalkAnimSpeed()
{
    int32_t s = P().speed;
    if (s < 0) s = -s;
    s *= 60; s /= 0x60000; s += 20;
    Pe().animationSpeed = s;
}

static void Player_HandleRunAnimSpeed()
{
    int32_t s = P().speed;
    if (s < 0) s = -s;
    s *= 80; s /= 0x60000;
    Pe().animationSpeed = s;
}

static void Player_HandleGroundMovement()
{
    if (P().controlLock > 0) {
        P().controlLock--;
        int32_t t = NS_Sin256(P().angle); t *= 0x2000; t >>= 8;
        P().speed += t;
    } else {
        if (P().left) {
            int32_t top = -P().movementStats.topSpeed;
            if (P().speed > top) {
                if (P().speed > 0) {
                    if (P().collisionMode == CMODE_FLOOR)
                        if (P().speed > 0x40000) P().skidding = 16;
                    if (P().speed < 0x8000) { P().speed = -0x8000; P().skidding = 0; }
                    else P().speed -= 0x8000;
                } else {
                    P().speed -= P().movementStats.acceleration; P().skidding = 0;
                }
            }
            if (P().speed <= 0) Pe().direction = FACING_LEFT;
        }
        if (P().right) {
            if (P().speed < P().movementStats.topSpeed) {
                if (P().speed < 0) {
                    if (P().collisionMode == CMODE_FLOOR)
                        if (P().speed < -0x40000) P().skidding = 16;
                    if (P().speed > -0x8000) { P().speed = 0x8000; P().skidding = 0; }
                    else P().speed += 0x8000;
                } else {
                    P().speed += P().movementStats.acceleration; P().skidding = 0;
                }
            }
            if (P().speed >= 0) Pe().direction = FACING_RIGHT;
        }

        if (!(P().left | P().right)) {
            if (P().speed > 0) { P().speed -= P().movementStats.deceleration; if (P().speed < 0) P().speed = 0; }
            else               { P().speed += P().movementStats.deceleration; if (P().speed > 0) P().speed = 0; }

            if (P().speed > 0x2000)  { int32_t t = NS_Sin256(P().angle); t *= 0x2000; t >>= 8; P().speed += t; }
            if (P().speed < -0x2000) { int32_t t = NS_Sin256(P().angle); t *= 0x2000; t >>= 8; P().speed += t; }

            if (P().angle > 192 && P().angle < 228)
                if (P().speed > -0x10000 && P().speed < 0x10000) P().controlLock = 30;
            if (P().angle > 28 && P().angle < 64)
                if (P().speed > -0x10000 && P().speed < 0x10000) P().controlLock = 30;
        } else {
            int32_t t = NS_Sin256(P().angle); t *= 0x2000; t >>= 8; P().speed += t;
            if (P().right && !P().left) {
                if (P().angle > 192 && P().angle < 228)
                    if (P().speed < 0x28000 && P().speed > -0x20000) P().controlLock = 30;
            } else if (P().left) {
                if (P().angle > 28 && P().angle < 64)
                    if (P().speed > -0x28000 && P().speed < 0x20000) P().controlLock = 30;
            }
        }
    }
}

static void Player_HandleAirFriction()
{
    if (P().yVelocity > -0x40000 && P().yVelocity < 0) {
        int32_t t = P().speed >> 5; P().speed -= t;
    }
    int32_t top = -P().movementStats.topSpeed;
    if (P().speed > top) {
        if (P().left) { P().speed -= P().movementStats.airAcceleration; Pe().direction = FACING_LEFT; }
    } else {
        if (P().left) Pe().direction = FACING_LEFT;
    }
    if (P().speed < P().movementStats.topSpeed) {
        if (P().right) { P().speed += P().movementStats.airAcceleration; Pe().direction = FACING_RIGHT; }
    } else {
        if (P().right) Pe().direction = FACING_RIGHT;
    }
    if (g("Options.OriginalControls")) {
        if (P().left)  { int32_t tp = -P().movementStats.topSpeed; if (P().speed < tp) P().speed = tp; }
        if (P().right) { if (P().speed > P().movementStats.topSpeed) P().speed = P().movementStats.topSpeed; }
    }
}

static void Player_HandleRollDeceleration()
{
    if (P().right && P().speed < 0) P().speed += P().movementStats.rollingDeceleration;
    if (P().left  && P().speed > 0) P().speed -= P().movementStats.rollingDeceleration;

    if (P().speed > 0) {
        P().speed -= P().movementStats.airDeceleration;
        if (P().speed < 0) P().speed = 0;
        if (P().speed == 0) { if (P().angle > 224) Pe().state = PF_Ground; if (P().angle < 32) Pe().state = PF_Ground; }
        int32_t s = NS_Sin256(P().angle);
        int32_t t = (s > 0) ? NS_Sin256(P().angle) * 0x5000 : NS_Sin256(P().angle) * 0x1E00;
        t >>= 8; P().speed += t;
    } else {
        P().speed += P().movementStats.airDeceleration;
        if (P().speed > 0) P().speed = 0;
        if (P().speed == 0) { if (P().angle > 224) Pe().state = PF_Ground; if (P().angle < 32) Pe().state = PF_Ground; }
        int32_t s = NS_Sin256(P().angle);
        int32_t t = (s < 0) ? NS_Sin256(P().angle) * 0x5000 : NS_Sin256(P().angle) * 0x1E00;
        t >>= 8; P().speed += t;
    }
    if (P().speed > 0x180000)  P().speed = 0x180000;
    if (P().speed < -0x180000) P().speed = -0x180000;
}

static void Player_HandleAirMovement()
{
    P().trackScroll = 1;
    P().yVelocity += P().movementStats.gravity;
    if (P().yVelocity < P().movementStats.jumpCap) {
        if (!P().jumpHold) {
            if (P().timer > 0) {
                P().yVelocity = P().movementStats.jumpCap;
                int32_t t = P().speed >> 5; P().speed -= t;
            }
        }
    }
    P().xVelocity = P().speed;
    if (Pe().rotation < 256) { if (Pe().rotation > 0) Pe().rotation -= 4; else Pe().rotation = 0; }
    else                     { if (Pe().rotation < 512) Pe().rotation += 4; else Pe().rotation = 0; }
    P().collisionMode = CMODE_FLOOR;
    if (Pe().animation == aniJumping) Pe().animationSpeed = P().value[2];
}

static void Player_HandleOnGround()
{
    P().trackScroll = 0;
    int32_t c = NS_Cos256(P().angle); c *= P().speed; c >>= 8; P().xVelocity = c;
    int32_t s = NS_Sin256(P().angle); s *= P().speed; s >>= 8; P().yVelocity = s;
}

static void Player_Action_Jump()
{
    ObjectSystem::scriptEng.checkResult = 0;
    if (P().collisionMode == CMODE_FLOOR) {
        int32_t sx = Obj().xPos, sy = Obj().yPos;
        Obj().xPos = P().xPos; Obj().yPos = P().yPos;
        int32_t t = NS_PlayerCollTop() - 2;
        NS_ObjectTileCollision(CSIDE_RWALL, 0, t, 0);
        Obj().xPos = sx; Obj().yPos = sy;
    }
    if (ObjectSystem::scriptEng.checkResult == 0) {
        P().controlLock = 0;
        P().gravity     = GRAVITY_AIR;
        P().value[1]    = 8;

        P().xVelocity = NS_Sin256(P().angle) * P().movementStats.jumpStrength;
        P().xVelocity += NS_Cos256(P().angle) * P().speed;
        P().xVelocity >>= 8;

        P().yVelocity = NS_Sin256(P().angle) * P().speed;
        P().yVelocity -= NS_Cos256(P().angle) * P().movementStats.jumpStrength;
        P().yVelocity >>= 8;

        P().speed = P().xVelocity;
        P().trackScroll = 1;
        Pe().animation = aniJumping;
        P().angle = 0;
        P().collisionMode = CMODE_FLOOR;
        P().timer = 1;
        Player_HandleRollAnimSpeed();
        Pe().state = PF_Air;
        NS_PlaySfx(SFX_G_JUMP, false);
    }
}

static void Player_Action_Spindash_S2()
{
    Pe().state = PF_Spindash_S2;
    Pe().animation = aniSpindash;
    P().value[1] = 0;
    NS_PlaySfx(SFX_G_CHARGE, false);
    int32_t tp = NS_CreateTempObject(NS_TypeID("Dust Puff"), (uint8_t)P().objectNum, P().xPos, P().yPos);
    ObjectEntity& d = ObjectSystem::objectEntityList[tp];
    d.yPos = (NS_PlayerCollBottom() << 16);
    d.yPos += P().yPos;
    d.frame = 4;
    d.drawOrder = 4;
    d.direction = Pe().direction;
}

static void Player_Action_Spindash_CD()
{
    Pe().state = PF_Spindash_CD;
    Pe().animation = aniJumping;
    P().yPos += 0x50000;
    P().value[1] = 0;
    NS_PlaySfx(SFX_G_CHARGE, false);
}

static void Player_Action_Peelout_S2()
{
    Pe().state = PF_Peelout_S2;
    P().value[1] = 0;
    NS_PlaySfx(SFX_G_CHARGE, false);
}

static void Player_Action_Peelout_CD()
{
    Pe().state = PF_Peelout_CD;
    P().value[1] = 0;
    NS_PlaySfx(SFX_G_CHARGE, false);
}

static void Player_Action_DblJumpTails()
{
    if (P().value[1] > 0) {
        P().value[1]--;
    } else if (P().jumpPress) {
        P().timer = 0;
        Pe().state = PF_Fly;
        P().value[15] = 0x800;
        if (P().movementStats.gravity == 0x3800) {
            NS_PlaySfx(SFX_G_FLYING, true);
            Pe().animation = aniFlying;
        } else {
            Pe().animation = aniSwimming;
        }
    }
}

static void Player_State_Ground()
{
    bool wasNotSkid = (Pe().animation != aniSkidding);

    Player_HandleGroundMovement();

    if (P().gravity == GRAVITY_AIR) {
        Pe().state = PF_Air;
        Player_HandleAirMovement();
    } else {
        Player_HandleOnGround();
        if (P().speed == 0) {
            if (g("Warp.Destination") > WARPDEST_NONE) {
                if (g("Warp.Timer") > 99 && g("Warp.Timer") < 220) sg("Warp.Destination", WARPDEST_NONE);
                sg("Warp.Timer", 0);
            }
            if (P().collisionMode == CMODE_FLOOR) {
                if (P().timer < 240) { Pe().animation = aniStopped; P().timer++; }
                else {
                    Pe().animation = aniWaiting;
                    if (NS_PlayerListPos() == PLAYER_SONIC_A) {
                        P().timer++;
                        if (P().timer == 10620) {
                            P().timer = 0;
                            NS_PlaySfx(SFX_G_OUTTAHERE, false);
                            Pe().state = PF_OuttaHere;
                            Pe().animation = aniBored;
                        }
                    }
                }
                if (!P().flailing[1] && !P().flailing[2]) {
                    P().timer = 0;
                    Pe().animation = (Pe().direction == FACING_LEFT) ? aniFlailingL : aniFlailingR;
                }
                if (!P().flailing[1] && !P().flailing[0]) {
                }
                if (!P().flailing[0]) {
                    P().timer = 0;
                    Pe().animation = (Pe().direction == FACING_RIGHT) ? aniFlailingL : aniFlailingR;
                }
            }
        } else {
            P().timer = 0;
            int32_t sp = P().speed; bool pos = sp > 0; int32_t asp = pos ? sp : -sp;
            if (asp < 0x5F5C2) {
                Pe().animation = aniWalking;
                Player_HandleWalkAnimSpeed();
                if (g("Warp.Destination") > WARPDEST_NONE) {
                    if (g("Warp.Timer") > 99 && g("Warp.Timer") < 204) sg("Warp.Destination", WARPDEST_NONE);
                    sg("Warp.Timer", 0);
                    ObjectSystem::objectEntityList[3].type = NS_TypeID("Blank Object");
                }
            } else {
                if (g("Warp.Destination") > WARPDEST_NONE) {
                    if (g("Warp.Timer") == 0) {
                        sg("Warp.Timer", 1);
                        NS_ResetObjectEntity(3, NS_TypeID("Warp Star"), 0, P().xPos, P().yPos);
                        ObjectSystem::objectEntityList[3].value[0] = 7;
                        ObjectSystem::objectEntityList[3].drawOrder = 4;
                    }
                }
                Pe().animation = (asp > 0x9FFFF) ? aniPeelout : aniRunning;
                Player_HandleRunAnimSpeed();
            }
        }

        if (P().skidding > 0) {
            if (wasNotSkid) NS_PlaySfx(SFX_G_SKIDDING, false);
            Pe().animation = aniSkidding;
            Pe().animationSpeed = 0;
            P().skidding--;
            if (g("Ring.AniCount") == 0) {
                int32_t tp = NS_CreateTempObject(NS_TypeID("Dust Puff"), 0, P().xPos, P().yPos);
                ObjectEntity& d = ObjectSystem::objectEntityList[tp];
                d.yPos = (((d.yPos >> 16) + NS_PlayerCollBottom()) << 16);
                d.drawOrder = Pe().drawOrder;
            }
            Pe().direction = (P().speed > 0) ? FACING_RIGHT : FACING_LEFT;
        }

        if (P().collisionMode == CMODE_FLOOR)
            if (P().pushing == 2) { Pe().animation = aniPushing; Pe().animationSpeed = 0; }

        if (P().jumpPress) {
            Player_Action_Jump();
        } else {
            if (P().up && P().speed == 0) {
                if (Pe().animation != aniFlailingL && Pe().animation != aniFlailingR) {
                    Pe().state = PF_LookUp; Pe().animation = aniLookingUp; P().timer = 0;
                }
            }
            if (P().down) {
                if (P().speed == 0) {
                    if (Pe().animation != aniFlailingL && Pe().animation != aniFlailingR) {
                        Pe().state = PF_Crouch; Pe().animation = aniLookingDown; P().timer = 0;
                    }
                } else if (!P().left && !P().right) {
                    bool roll = (P().speed > 0) ? (P().speed > 0x8800) : (P().speed < -0x8800);
                    if (roll) {
                        Pe().state = PF_Roll; Pe().animation = aniJumping;
                        P().yPos = (((P().yPos >> 16) - P().value[9]) << 16);
                        P().value[1] = 1024;
                    }
                }
            }
        }
    }
}

static void Player_State_Air()
{
    Player_HandleAirFriction();
    if (P().gravity == GRAVITY_AIR) {
        Player_HandleAirMovement();

        if (g("Warp.Destination") > WARPDEST_NONE) {
            int32_t yv = P().yVelocity; if (yv < 0) yv = -yv;
            if (yv < 0x60000) {
                int32_t xv = P().xVelocity; if (xv < 0) xv = -xv;
                if (xv < 0x60000) {
                    int32_t d = P().xVelocity - g("Warp.SpeedCompare"); if (d < 0) d = -d;
                    if (d > 0x40000) {
                        if (g("Warp.Timer") > 99 && g("Warp.Timer") < 204) sg("Warp.Destination", WARPDEST_NONE);
                        sg("Warp.Timer", 0);
                    }
                }
            }
            sg("Warp.SpeedCompare", P().xVelocity);
        }

        if (P().yVelocity > 0x20000) {
            if (Pe().animation == aniFlailingL) Pe().animation = aniWalking;
            if (Pe().animation == aniFlailingR) Pe().animation = aniWalking;
        }
        if (Pe().animation == aniBouncing) {
            if (P().yVelocity >= 0) {
                if (P().value[7] == aniStopped) P().value[7] = aniWalking;
                Pe().animation = P().value[7];
            }
        }
        if (Pe().animation == aniHurt) {
            if (P().yVelocity >= 0) {
                if (P().value[7] == aniStopped) P().value[7] = aniWalking;
                Pe().animation = P().value[7];
            }
        }
        if (Pe().animation == aniJumping) CallFn(P().value[10]);
    } else {
        Pe().state = PF_Ground;
        Player_HandleOnGround();
        P().skidding = 0;
    }
}

static void Player_State_Roll()
{
    Player_HandleRollDeceleration();
    if (P().gravity == GRAVITY_AIR) {
        Pe().state = PF_Air;
        P().timer = 0;
        Player_HandleAirMovement();
    } else {
        Player_HandleRollAnimSpeed();
        Pe().animationSpeed = P().value[2];
        int32_t asp = P().speed; if (asp < 0) asp = -asp;
        if (asp < 0x5F5C2) {
            if (g("Warp.Destination") > WARPDEST_NONE) {
                if (g("Warp.Timer") > 99 && g("Warp.Timer") < 204) sg("Warp.Destination", WARPDEST_NONE);
                sg("Warp.Timer", 0);
            }
        } else {
            if (g("Warp.Destination") > WARPDEST_NONE) {
                if (g("Warp.Timer") == 0) {
                    sg("Warp.Timer", 1);
                    NS_ResetObjectEntity(3, NS_TypeID("Warp Star"), 0, P().xPos, P().yPos);
                    ObjectSystem::objectEntityList[3].value[0] = 7;
                    ObjectSystem::objectEntityList[3].drawOrder = 4;
                }
            }
        }
        Player_HandleOnGround();
        if (P().jumpPress) Player_Action_Jump();
    }
}

static void Player_State_LookUp()
{
    if (!P().up) { Pe().state = PF_Ground; P().timer = 0; }
    else {
        if (P().timer < 60) P().timer++;
        else if (P().lookPos > -112) P().lookPos -= 2;
        if (P().gravity == GRAVITY_AIR) { Pe().state = PF_Air; P().timer = 0; }
        else if (P().jumpPress) CallFn(P().value[11]);
    }
}

static void Player_State_Crouch()
{
    if (!P().down) { Pe().state = PF_Ground; P().timer = 0; }
    else {
        if (P().timer < 60) P().timer++;
        else if (P().lookPos < 96) P().lookPos += 2;
        if (P().gravity == GRAVITY_AIR) { Pe().state = PF_Air; P().timer = 0; }
        else if (P().jumpPress) CallFn(P().value[12]);
    }
}

static void Player_State_Spindash_S2()
{
    if (P().gravity == GRAVITY_AIR) { Pe().state = PF_Air; P().speed = 0; }
    if (P().jumpPress) {
        if (P().value[1] < 512) P().value[1] += 64;
        Pe().frame = 0;
        NS_PlaySfx(SFX_G_CHARGE, false);
    } else {
        if (P().value[1] > 0) P().value[1]--;
    }
    if (!P().down) {
        P().timer = 0;
        Pe().state = PF_Roll;
        Pe().animation = aniJumping;
        P().yPos = (((P().yPos >> 16) - P().value[9]) << 16);
        P().value[8] = 15;
        StageSystem::cameraStyle = CAMERASTYLE_HLOCKED;
        int32_t t = P().value[1]; t <<= 9; t += 0x80000;
        P().speed = (Pe().direction == FACING_RIGHT) ? t : -t;
        NS_PlaySfx(SFX_G_RELEASE, false);
        Player_HandleOnGround();
    }
}

static void Player_State_Spindash_CD()
{
    StageSystem::cameraStyle = (Pe().direction == FACING_RIGHT) ? CAMERASTYLE_EXTENDED_OFFSET_L : CAMERASTYLE_EXTENDED_OFFSET_R;
    if (P().gravity == GRAVITY_AIR) { Pe().state = PF_Air; P().speed = 0; StageSystem::cameraStyle = CAMERASTYLE_EXTENDED; }
    if (P().movementStats.gravity == 0x1000) { if (P().value[1] < 0x80000) P().value[1] += 0x6000; }
    else                                     { if (P().value[1] < 0xC0000) P().value[1] += 0x6000; }
    if (!P().down) {
        StageSystem::cameraStyle = CAMERASTYLE_EXTENDED;
        P().timer = 0;
        if (P().value[1] < 0x2FAE1) { P().speed = 0; Pe().state = PF_Ground; }
        else {
            Pe().state = PF_Roll; Pe().animation = aniJumping; P().speed = P().value[1];
            if (Pe().direction == FACING_LEFT) P().speed = -P().speed;
            NS_PlaySfx(SFX_G_RELEASE, false);
        }
        Player_HandleOnGround();
    }
}

static void Player_State_Peelout_S2()
{
    int32_t tv;
    if (P().gravity == GRAVITY_AIR) { Pe().state = PF_Air; P().speed = 0; }
    if (P().movementStats.gravity == 0x1000) { if (P().value[1] < 0x60000) P().value[1] += 0x6000; }
    else                                     { if (P().value[1] < 0xC0000) P().value[1] += 0x6000; }
    if (P().value[1] < 0x5F5C2) {
        Pe().animation = aniWalking;
        tv = P().value[1]; tv >>= 16; tv *= 80; tv /= 6; tv += 20;
    } else {
        tv = P().value[1]; tv >>= 16; tv *= 80; tv /= 6;
        Pe().animation = (P().value[1] > 0x9FFFF) ? aniPeelout : aniRunning;
    }
    if (!P().up) {
        P().value[8] = 15;
        StageSystem::cameraStyle = CAMERASTYLE_HLOCKED;
        Pe().state = PF_Ground;
        if (P().value[1] < 0x5F5C2) P().speed = 0;
        else {
            P().speed = P().value[1];
            if (Pe().direction == FACING_LEFT) P().speed = -P().speed;
            NS_PlaySfx(SFX_G_RELEASE, false);
        }
        Player_HandleOnGround();
    }
    Pe().animationSpeed = tv;
}

static void Player_State_Peelout_CD()
{
    int32_t tv;
    StageSystem::cameraStyle = (Pe().direction == FACING_RIGHT) ? CAMERASTYLE_EXTENDED_OFFSET_L : CAMERASTYLE_EXTENDED_OFFSET_R;
    if (P().gravity == GRAVITY_AIR) { Pe().state = PF_Air; P().speed = 0; StageSystem::cameraStyle = CAMERASTYLE_EXTENDED; }
    if (P().movementStats.gravity == 0x1000) { if (P().value[1] < 0x60000) P().value[1] += 0x6000; }
    else                                     { if (P().value[1] < 0xC0000) P().value[1] += 0x6000; }
    if (P().value[1] < 0x5F5C2) {
        Pe().animation = aniWalking;
        tv = P().value[1]; tv >>= 16; tv *= 80; tv /= 6; tv += 20;
    } else {
        tv = P().value[1]; tv >>= 16; tv *= 80; tv /= 6;
        Pe().animation = (P().value[1] > 0x9FFFF) ? aniPeelout : aniRunning;
    }
    if (!P().up) {
        StageSystem::cameraStyle = CAMERASTYLE_EXTENDED;
        Pe().state = PF_Ground;
        if (P().value[1] < 0x5F5C2) P().speed = 0;
        else {
            P().speed = P().value[1];
            if (Pe().direction == FACING_LEFT) P().speed = -P().speed;
            NS_PlaySfx(SFX_G_RELEASE, false);
        }
        Player_HandleOnGround();
    }
    Pe().animationSpeed = tv;
}

static void Player_State_Fly()
{
    Player_HandleAirFriction();
    if (P().gravity == GRAVITY_AIR) {
        P().xVelocity = P().speed;
        if (g("Warp.Destination") > WARPDEST_NONE) {
            int32_t xv = P().xVelocity; if (xv < 0) xv = -xv;
            int32_t yv = P().yVelocity; if (yv < 0) yv = -yv;
            if (xv + yv < 0x40000) {
                if (g("Warp.Timer") > 99 && g("Warp.Timer") < 220) sg("Warp.Destination", WARPDEST_NONE);
                sg("Warp.Timer", 0);
            }
        }
        if (P().yVelocity < -0x10000) P().value[15] = 0x800;
        else if (P().yVelocity < 1) { if (P().value[1] < 60) P().value[1]++; else P().value[15] = 0x800; }
        P().yVelocity += P().value[15];
        if (P().timer < 480) {
            Pe().animation = (P().movementStats.gravity == 0x3800) ? aniFlying : aniSwimming;
            P().timer++;
            if (P().timer == 480) {
                if (P().movementStats.gravity == 0x3800) {
                    Pe().animation = aniFlyingTired; NS_StopSfx(SFX_G_FLYING); NS_PlaySfx(SFX_G_TIRED, true);
                } else Pe().animation = aniSwimmingTired;
            } else if (P().jumpPress) { P().value[15] = -0x2000; P().value[1] = 0; }
        } else {
            Pe().animation = (P().movementStats.gravity == 0x3800) ? aniFlyingTired : aniSwimmingTired;
        }
    } else {
        Pe().state = PF_Ground;
        Player_HandleOnGround();
    }
    if (g("Player.RoofBarrier")) {
        int32_t t = P().yPos >> 16;
        if (t < NS_PlayerCollBottom()) { P().yPos = NS_PlayerCollBottom(); P().yPos <<= 16; }
    }
}

static void Player_State_GotHit()
{
    int32_t a2 = P().objectNum + 2;
    int32_t dmg;
    if (ObjectSystem::objectEntityList[a2].propertyValue > 0) {
        dmg = DAMAGE_SHIELDED;
        NS_ResetObjectEntity(a2, NS_TypeID("Blank Object"), 0, 0, 0);
        NS_PlaySfx(SFX_G_HURT, false);
    } else if (P().value[0] == 0) {
        NS_PlaySfx(SFX_G_HURT, false);
        dmg = DAMAGE_DEATH;
    } else {
        NS_PlaySfx(SFX_G_LOSERINGS, false);
        dmg = DAMAGE_HURT;
    }

    switch (dmg) {
    case DAMAGE_SHIELDED:
        Pe().state = PF_Hurt; Pe().animation = aniHurt; P().yVelocity = -0x40000;
        P().gravity = GRAVITY_AIR; P().trackScroll = 1; P().value[4] = 8000;
        if (P().movementStats.gravity == 0x1000) { P().speed >>= 1; P().yVelocity >>= 1; }
        break;
    case DAMAGE_HURT: {
        int32_t drawO = (P().collisionPlane == COLLISION_PLANE_A) ? 3 : 1;
        Pe().state = PF_Hurt; Pe().animation = aniHurt; P().yVelocity = -0x40000;
        P().gravity = GRAVITY_AIR; P().trackScroll = 1; P().value[4] = 8000;
        if (P().movementStats.gravity == 0x1000) { P().speed >>= 1; P().yVelocity >>= 1; }
        int32_t rings = P().value[0], rem;
        if (rings > 16) { rem = rings - 16; rings = 16; } else rem = 0;
        if (rem > 16) rem = 16;
        int32_t ang = 384 - ((rem >> 1) << 5);
        if (((rem >> 1) << 5) >> 4 == rem) ang += 16; else ang -= 16;
        for (int32_t i = 0; i < rem; ++i) {
            int32_t tp = NS_CreateTempObject(NS_TypeID("Lose Ring"), (uint8_t)P().collisionPlane, P().xPos, P().yPos);
            ObjectEntity& r = ObjectSystem::objectEntityList[tp];
            r.value[0] = NS_Cos512(ang) << 8; r.value[1] = NS_Sin512(ang) << 8;
            r.drawOrder = drawO; r.animationSpeed = 256; ang += 32;
        }
        int32_t ang2 = 384 - ((rings >> 1) << 5);
        if (((rings >> 1) << 5) >> 4 == rings) ang2 += 16; else ang2 -= 16;
        for (int32_t i = 0; i < rings; ++i) {
            int32_t tp = NS_CreateTempObject(NS_TypeID("Lose Ring"), (uint8_t)P().collisionPlane, P().xPos, P().yPos);
            ObjectEntity& r = ObjectSystem::objectEntityList[tp];
            r.value[0] = NS_Cos512(ang2) << 9; r.value[1] = NS_Sin512(ang2) << 9;
            r.drawOrder = drawO; r.animationSpeed = 256; ang2 += 32;
        }
        P().value[0] = 0;
        sg("Ring.ExtraLife", 100);
        break;
    }
    case DAMAGE_DEATH:
        Pe().drawOrder = 5; P().speed = 0; P().yVelocity = -0x70000; P().xVelocity = 0;
        Pe().state = PF_Death; Pe().animation = aniDying;
        P().tileCollisions = 0; P().objectInteraction = 0;
        if (P().objectNum == 0) StageSystem::cameraEnabled = 0;
        break;
    }
    if (g("Warp.Destination") > WARPDEST_NONE) {
        if (g("Warp.Timer") > 99 && g("Warp.Timer") < 204) sg("Warp.Destination", WARPDEST_NONE);
        sg("Warp.Timer", 0);
    }
}

static void Player_State_Hurt()
{
    if (P().gravity == GRAVITY_AIR) {
        P().trackScroll = 1;
        if (P().movementStats.gravity == 0x3800) P().yVelocity += 0x3000; else P().yVelocity += 0xF00;
        P().xVelocity = P().speed;
    } else {
        Pe().state = PF_Ground; P().value[4] = 120; P().value[5] = 3;
        P().speed = 0; P().xVelocity = 0; Player_HandleOnGround();
    }
}

static void Player_State_OuttaHere()
{
    if (P().timer < 140) P().timer++;
    else {
        P().timer = 0; Pe().drawOrder = 5;
        if (Pe().direction == FACING_RIGHT) { P().speed = 0x10000; P().xVelocity = 0x10000; }
        else { P().speed = -0x10000; P().xVelocity = -0x10000; }
        P().yVelocity = -0x58000; Pe().state = PF_Death;
        P().tileCollisions = 0; P().objectInteraction = 0; StageSystem::cameraEnabled = 0;
    }
}

static void Player_State_Death()
{
    P().controlMode = CONTROLMODE_NONE;
    P().yVelocity += 0x3800;
    if (Pe().animation != aniBored) Pe().animation = aniDying;
    if (P().yVelocity > 0x100000) {
        if (g("Player.Lives") > 0) {
            if (Pe().animation == aniBored) sg("Player.Lives", 0);
            else sg("Player.Lives", g("Player.Lives") - 1);
        }
        StageSystem::timeEnabled = 0;

        Obj().type = NS_TypeID("Death Event");
        ObjectEntity& de = Obj();
        de.drawOrder = 7;
        de.value[1] = GlobalAppDefinitions::SCREEN_CENTER - 232;
        de.value[2] = GlobalAppDefinitions::SCREEN_CENTER + 232;

        if (g("Options.GameMode") == MODE_TIMEATTACK) {
            de.value[3] = 0; de.state = DEATHEVENT_TIMEATTACK;
        } else {
            if (g("Player.Lives") == 0) {
                de.value[3] = -2880; de.state = DEATHEVENT_GAMEOVER;
                NS_PlayMusic(5); StageSystem::pauseEnabled = 0;
            } else {
                de.value[3] = 0; de.state = DEATHEVENT_FADEOUT;
                if (StageSystem::minutes == 9 && StageSystem::seconds == 59) {
                    de.value[3] = -2880; de.state = DEATHEVENT_TIMEOVER;
                    NS_PlayMusic(5); StageSystem::pauseEnabled = 0;
                }
            }
        }
    }
}

static void Player_State_Drown()
{
    P().controlMode = CONTROLMODE_NONE;
    P().yVelocity += P().movementStats.gravity;
    Pe().animation = aniDrowning;
    if (P().yVelocity > 0x80000) {
        if (g("Player.Lives") > 0) sg("Player.Lives", g("Player.Lives") - 1);
        StageSystem::timeEnabled = 0;

        Obj().type = NS_TypeID("Death Event");
        ObjectEntity& de = Obj();
        de.drawOrder = 7;
        de.value[1] = GlobalAppDefinitions::SCREEN_CENTER - 232;
        de.value[2] = GlobalAppDefinitions::SCREEN_CENTER + 232;
        if (g("Options.GameMode") == MODE_TIMEATTACK) { de.value[3] = 0; de.state = DEATHEVENT_TIMEATTACK; }
        else if (g("Player.Lives") == 0) {
            de.value[3] = -2880; de.state = DEATHEVENT_GAMEOVER; NS_PlayMusic(5); StageSystem::pauseEnabled = 0;
        } else { de.value[3] = 0; de.state = DEATHEVENT_FADEOUT; }
    }
}

static void Player_State_HangBar()     {}
static void Player_State_TubeRoll()    {}
static void Player_State_TubeAirRoll() {}
static void Player_State_SpinningTop() {}
static void Player_State_Hugged()      {}
static void Player_State_Ramp3D()      {}
static void Player_State_WaterCurrent(){}
static void Player_State_SizeChange()  {}

static void Player_BadnikBreak()
{
    bool kill = (Pe().animation == aniJumping) || (Pe().animation == aniSpindash);
    int32_t a2 = P().objectNum + 2;
    if (ObjectSystem::objectEntityList[a2].type == NS_TypeID("Invincibility")) kill = true;
    if (g("Warp.Timer") > 0) kill = true;
    if (Pe().animation == aniFlying) if (P().yPos > Obj().yPos) kill = true;

    if (kill) {
        NS_ResetObjectEntity(ObjectSystem::objectLoop, (uint8_t)g("Flower_TypeNo"), 0, Obj().xPos, Obj().yPos);
        Obj().drawOrder = 4;
        int32_t bonus = ObjectSystem::objectEntityList[26].value[0];
        int32_t tp = NS_CreateTempObject(NS_TypeID("Smoke Puff"), 0, Obj().xPos, Obj().yPos);
        ObjectSystem::objectEntityList[tp].drawOrder = 4;
        tp = NS_CreateTempObject(NS_TypeID("Object Score"), (uint8_t)bonus, Obj().xPos, Obj().yPos);
        ObjectSystem::objectEntityList[tp].drawOrder = 4;
        NS_PlaySfx(SFX_G_DESTROY, false);
        if (P().yVelocity > 0) P().yVelocity = -P().yVelocity; else P().yVelocity += 0xC000;
        int32_t add = (bonus == 0) ? 100 : (bonus == 1) ? 200 : (bonus == 2) ? 500 : 1000;
        sg("Player.Score", g("Player.Score") + add);
    } else {
        if (P().value[4] == 0) {
            Pe().state = PF_GotHit;
            P().speed = (P().xPos > Obj().xPos) ? 0x20000 : -0x20000;
        }
    }
}

static void Player_Hit()
{
    int32_t a2 = P().objectNum + 2;
    if (ObjectSystem::objectEntityList[a2].type != NS_TypeID("Invincibility")) {
        if (P().value[4] == 0) {
            Pe().state = PF_GotHit;
            P().speed = (P().xPos > Obj().xPos) ? 0x20000 : -0x20000;
        }
    }
}

static void Player_Kill()
{
    NS_PlaySfx(SFX_G_HURT, false);
    Pe().drawOrder = 5;
    P().speed = 0; P().xVelocity = 0; P().yVelocity = -0x68000;
    Pe().state = PF_Death; Pe().animation = aniDying;
    P().tileCollisions = 0; P().objectInteraction = 0;
    StageSystem::cameraEnabled = 0;
}

static void Player_ProcessUpdate()
{
    if (!g("Options.AttractMode")) {
        if (P().controlMode == CONTROLMODE_NORMAL) {
            if (ObjectSystem::objectEntityList[9].type == NS_TypeID("Blank Object")) {
                bool pause = false;
                if (StageSystem::gKeyPress.start) { StageSystem::gKeyPress.start = 0; pause = true; }
                if (GlobalAppDefinitions::gameMessage == MESSAGE_LOSTFOCUS) pause = true;
                if (pause) {
                    StageSystem::stageMode = STAGE_PAUSED;
                    NS_StopMusic();
                    NS_PlaySfx(SFX_G_SELECT, false);
                    NS_StopSfx(SFX_G_FLYING); NS_StopSfx(SFX_G_TIRED);
                    ObjectSystem::objectEntityList[9].type = NS_TypeID("Pause Menu");
                    ObjectSystem::objectEntityList[9].drawOrder = 7;
                    ObjectSystem::objectEntityList[9].priority = PRIORITY_ALWAYS;
                }
            }
        }
        PlayerSystem::ProcessPlayerControl(P());
    }

    if (P().value[3] > 0) {
        P().value[3]--;
        if (P().value[3] == 0) {
            P().movementStats.acceleration = 0xC00;
            P().movementStats.airAcceleration = 0x1800;
            P().movementStats.topSpeed = 0x60000;
            if (AudioPlayback::currentMusicTrack == 3) NS_PlayMusic(0);
        }
    }

    if (P().value[4] > 0) {
        if (Pe().state != PF_Hurt) {
            if (P().value[4] > 2000) { P().value[4] = 120; P().value[5] = 3; }
        }
        if (P().value[5] > 0) {
            P().value[5]++;
            if (P().value[5] > 8) P().value[5] = 1;
            P().visible = (P().value[5] > 4) ? 0 : 1;
        }
        P().value[4]--;
        if (P().value[4] == 0) {
            P().value[5] = 0; P().visible = 1;
            if (AudioPlayback::currentMusicTrack == 2) NS_PlayMusic(0);
            int32_t a2 = P().objectNum + 2;
            if (ObjectSystem::objectEntityList[a2].type == NS_TypeID("Invincibility")) {
                ObjectEntity& s = ObjectSystem::objectEntityList[a2];
                if (s.propertyValue == NO_SHIELD) {
                    NS_ResetObjectEntity(a2, NS_TypeID("Blank Object"), 0, 0, 0);
                } else if (s.propertyValue == ACTIVE_SHIELD) {
                    NS_ResetObjectEntity(a2, NS_TypeID("Blank Object"), 0, 0, 0);
                    s.type = NS_TypeID("Blue Shield"); s.propertyValue = ACTIVE_SHIELD;
                    s.priority = PRIORITY_ACTIVE; s.drawOrder = 4; s.inkEffect = INK_ALPHA;
                    s.alpha = 160; s.xPos = P().xPos; s.yPos = P().yPos;
                }
            }
        }
    }

    if (Pe().state != PF_LookUp && Pe().state != PF_Crouch) {
        if (P().lookPos > 0) P().lookPos -= 2;
        if (P().lookPos < 0) P().lookPos += 2;
    }

    if (g("Warp.Timer") > 0) {
        sg("Warp.Timer", g("Warp.Timer") + 1);
        if (g("Warp.Timer") == 204) {
            StageSystem::cameraEnabled = 0;
            int32_t tp = NS_CreateTempObject(NS_TypeID("Time Warp"), 0, 0, 0);
            ObjectSystem::objectEntityList[tp].drawOrder = 6;
        }
    }

    if (P().value[8] > 0) {
        P().value[8]--;
        if (P().value[8] == 0) StageSystem::cameraStyle = CAMERASTYLE_FOLLOW;
    }

    if (Pe().state != PF_Fly) {
        if (P().value[15] != 0) {
            NS_StopSfx(SFX_G_FLYING); NS_StopSfx(SFX_G_TIRED); P().value[15] = 0;
        }
    }
}

static void (*g_fns[PF_COUNT])() = {
    Player_Static, Player_State_Ground, Player_State_Air, Player_State_Roll,
    Player_State_LookUp, Player_State_Crouch, Player_State_Spindash_S2, Player_State_Spindash_CD,
    Player_State_Peelout_S2, Player_State_Peelout_CD, Player_State_Fly, Player_State_GotHit,
    Player_State_Hurt, Player_State_OuttaHere, Player_State_Death, Player_State_Drown,
    Player_State_HangBar, Player_State_TubeRoll, Player_State_TubeAirRoll, Player_State_SpinningTop,
    Player_State_Hugged, Player_State_Ramp3D, Player_State_WaterCurrent, Player_State_SizeChange,
    Player_Action_Jump, Player_Action_Spindash_S2, Player_Action_Spindash_CD,
    Player_Action_Peelout_S2, Player_Action_Peelout_CD, Player_Action_DblJumpTails,
};
static void CallFn(int32_t id) { if (id >= 0 && id < PF_COUNT && g_fns[id]) g_fns[id](); }

void Player_Startup()
{
    NS_SetMusicTrack("JP/TitleScreen.ogg", 0, 0);

    for (int32_t ap = 32; ap < 1056; ++ap) {
        ObjectEntity& m = ObjectSystem::objectEntityList[ap];
        if (m.type != NS_TypeID("Player Object")) continue;

        int32_t list = NS_PlayerListPos();
        NS_ResetObjectEntity(0, NS_TypeID("Player Object"), 0, m.xPos, m.yPos);
        P().xPos = m.xPos; P().yPos = m.yPos;

        if (list == PLAYER_TAILS_A) NS_LoadAnimation("Tails.Ani");
        else                        NS_LoadAnimation("Sonic.Ani");
        NS_BindPlayerToObject(0, 0);

        Pe().state    = PF_Air;
        Pe().priority = PRIORITY_ACTIVE;
        Pe().drawOrder = 4;

        P().movementStats.topSpeed            = 0x60000;
        P().movementStats.acceleration        = 0xC00;
        P().movementStats.deceleration        = 0xC00;
        P().movementStats.airAcceleration     = 0x1800;
        P().movementStats.airDeceleration     = 0x600;
        P().movementStats.gravity             = 0x3800;
        P().movementStats.jumpStrength        = 0x68000;
        P().movementStats.jumpCap             = -0x40000;
        P().movementStats.rollingDeceleration = 0x2000;

        if (list == PLAYER_TAILS_A) {
            P().value[9]  = -1;
            P().value[10] = PF_Action_DblJumpTails;
            P().value[11] = PF_Action_Jump;
            P().value[12] = g("Options.OriginalControls") ? PF_Action_Spindash_CD : PF_Action_Spindash_S2;
        } else {
            P().value[9]  = -5;
            P().value[10] = PF_Static;
            if (!g("Options.OriginalControls")) {
                P().value[11] = PF_Action_Peelout_S2;
                P().value[12] = PF_Action_Spindash_S2;
            } else {
                P().value[11] = PF_Action_Peelout_CD;
                P().value[12] = PF_Action_Spindash_CD;
            }
        }

        aniStopped = NS_GetAnimationByName("Stopped");
        aniWaiting = NS_GetAnimationByName("Waiting");
        aniBored = NS_GetAnimationByName("Bored!");
        aniLookingUp = NS_GetAnimationByName("Looking Up");
        aniLookingDown = NS_GetAnimationByName("Looking Down");
        aniWalking = NS_GetAnimationByName("Walking");
        aniRunning = NS_GetAnimationByName("Running");
        aniSkidding = NS_GetAnimationByName("Skidding");
        aniPeelout = NS_GetAnimationByName("Super Peel Out");
        aniSpindash = NS_GetAnimationByName("Spin Dash");
        aniJumping = NS_GetAnimationByName("Jumping");
        aniBouncing = NS_GetAnimationByName("Bouncing");
        aniHurt = NS_GetAnimationByName("Hurt");
        aniDying = NS_GetAnimationByName("Dying");
        aniDrowning = NS_GetAnimationByName("Drowning");
        aniPushing = NS_GetAnimationByName("Pushing");
        aniFlailingL = NS_GetAnimationByName("Flailing Left");
        aniFlailingR = NS_GetAnimationByName("Flailing Right");
        aniHanging = NS_GetAnimationByName("Hanging");
        aniFlying = NS_GetAnimationByName("Flying");
        aniFlyingTired = NS_GetAnimationByName("Flying Tired");
        aniSwimming = NS_GetAnimationByName("Swimming");
        aniSwimmingTired = NS_GetAnimationByName("Swimming Tired");
        aniSpinningTop = NS_GetAnimationByName("Spinning Top");
        aniSizeChange = NS_GetAnimationByName("Size Change");
        aniRamp[0] = NS_GetAnimationByName("3D Ramp 1");
        aniRamp[1] = NS_GetAnimationByName("3D Ramp 2");
        aniRamp[2] = NS_GetAnimationByName("3D Ramp 3");
        aniRamp[3] = NS_GetAnimationByName("3D Ramp 4");
        aniRamp[4] = NS_GetAnimationByName("3D Ramp 5");
        aniRamp[5] = NS_GetAnimationByName("3D Ramp 6");
        aniRamp[6] = NS_GetAnimationByName("3D Ramp 7");

        NS_ResetObjectEntity(ap, NS_TypeID("Blank Object"), 0, 0, 0);
    }
}

void Player_Main(int32_t  )
{
    Player_ProcessUpdate();
    CallFn(Pe().state);
    NS_ProcessPlayerAnimation();

    if (Pe().animation == aniJumping) {
        StageSystem::cameraAdjustY = P().value[9];
    } else {
        if (StageSystem::cameraAdjustY == P().value[9]) {
            StageSystem::cameraAdjustY = 0;
            P().yPos = (((P().yPos >> 16) + P().value[9]) << 16);
        }
    }

    if (P().yVelocity > 0x100000) P().yVelocity = 0x100000;

    if (NS_PlayerTileCollision()) {
        if (Pe().animation == aniJumping) {
            if (!P().down) {
                Pe().animation = aniWalking;
                StageSystem::cameraAdjustY = 0;
                P().yPos = (((P().yPos >> 16) + P().value[9]) << 16);
            }
        }
    }
}

void Player_Draw(int32_t  )
{
    if (Pe().animation != Pe().prevAnimation) {
        Pe().prevAnimation = Pe().animation;
        Pe().frame = 0; Pe().animationTimer = 0; Pe().animationSpeed = 0;
    }
    NS_DrawPlayerAnimation();
}

}
