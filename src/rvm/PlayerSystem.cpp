#include "PlayerSystem.h"
#include "StageSystem.h"
#include "ObjectSystem.h"
#include "AnimationSystem.h"
#include "GlobalAppDefinitions.h"
#include "Log.h"
#include <cmath>

namespace rvm {

uint16_t     PlayerSystem::delayLeft      = 0;
uint16_t     PlayerSystem::delayRight     = 0;
uint16_t     PlayerSystem::delayUp        = 0;
uint16_t     PlayerSystem::delayDown      = 0;
uint16_t     PlayerSystem::delayJumpPress = 0;
uint16_t     PlayerSystem::delayJumpHold  = 0;
uint8_t      PlayerSystem::jumpWait       = 0;
uint8_t      PlayerSystem::numActivePlayers = 1;
uint8_t      PlayerSystem::playerMenuNum  = 0;
PlayerObject PlayerSystem::playerList[2]  {};
CollisionSensor PlayerSystem::cSensor[6]  {};
int32_t      PlayerSystem::collisionLeft  = 0;
int32_t      PlayerSystem::collisionTop   = 0;
int32_t      PlayerSystem::collisionRight = 0;
int32_t      PlayerSystem::collisionBottom= 0;

static CollisionBox& getCBox(PlayerObject& p)
{
    return AnimationSystem::collisionBoxList[
        p.animationFile->cbListOffset +
        (int32_t)AnimationSystem::animationFrames[
            AnimationSystem::animationList[
                p.animationFile->aniListOffset + (int32_t)p.objectPtr->animation
            ].frameListOffset + (int32_t)p.objectPtr->frame
        ].collisionBox
    ];
}

static void shakeUpdate()
{
    if(StageSystem::screenShakeX!=0){
        if(StageSystem::screenShakeX>0)StageSystem::screenShakeX=-StageSystem::screenShakeX;
        else{StageSystem::screenShakeX=-StageSystem::screenShakeX;--StageSystem::screenShakeX;}
    }
    if(StageSystem::screenShakeY!=0){
        if(StageSystem::screenShakeY>0)StageSystem::screenShakeY=-StageSystem::screenShakeY;
        else{StageSystem::screenShakeY=-StageSystem::screenShakeY;--StageSystem::screenShakeY;}
    }
}

static void updateYBoundaries()
{
    using SS=StageSystem;
    if(SS::newYBoundary1>SS::yBoundary1)SS::yBoundary1=SS::yScrollOffset<=SS::newYBoundary1?SS::yScrollOffset:SS::newYBoundary1;
    if(SS::newYBoundary1<SS::yBoundary1){if(SS::yScrollOffset>SS::yBoundary1)SS::yBoundary1=SS::newYBoundary1;else--SS::yBoundary1;}
    if(SS::newYBoundary2<SS::yBoundary2){if(SS::yScrollOffset+240<SS::yBoundary2&&SS::yScrollOffset+240>SS::newYBoundary2)SS::yBoundary2=SS::yScrollOffset+240;else--SS::yBoundary2;}
    if(SS::newYBoundary2>SS::yBoundary2){if(SS::yScrollOffset+240<SS::yBoundary2)SS::yBoundary2=SS::newYBoundary2;else++SS::yBoundary2;}
}

static void updateXBoundaries(PlayerObject& p)
{
    using SS=StageSystem;
    if(SS::newXBoundary1>SS::xBoundary1)SS::xBoundary1=SS::xScrollOffset<=SS::newXBoundary1?SS::xScrollOffset:SS::newXBoundary1;
    if(SS::newXBoundary1<SS::xBoundary1){
        if(SS::xScrollOffset>SS::xBoundary1)SS::xBoundary1=SS::newXBoundary1;
        else{--SS::xBoundary1;if(p.xVelocity<0){SS::xBoundary1+=p.xVelocity>>16;if(SS::xBoundary1<SS::newXBoundary1)SS::xBoundary1=SS::newXBoundary1;}}
    }
    if(SS::newXBoundary2<SS::xBoundary2)SS::xBoundary2=SS::xScrollOffset+GlobalAppDefinitions::SCREEN_XSIZE>=SS::xBoundary2?SS::xScrollOffset+GlobalAppDefinitions::SCREEN_XSIZE:SS::newXBoundary2;
    if(SS::newXBoundary2>SS::xBoundary2){
        if(SS::xScrollOffset+GlobalAppDefinitions::SCREEN_XSIZE<SS::xBoundary2)SS::xBoundary2=SS::newXBoundary2;
        else{++SS::xBoundary2;if(p.xVelocity>0){SS::xBoundary2+=p.xVelocity>>16;if(SS::xBoundary2>SS::newXBoundary2)SS::xBoundary2=SS::newXBoundary2;}}
    }
}

static void applyYScroll(PlayerObject& p, int32_t yA, int32_t yB)
{
    using SS=StageSystem;
    int32_t py=(p.yPos>>16)+SS::cameraAdjustY;
    int32_t lookY=py+p.lookPos;
    SS::yScrollA=yA; SS::yScrollB=yB;
    if(lookY>yA+104){
        SS::yScrollOffset=py-104+p.lookPos+SS::screenShakeY;
        p.screenYPos=104-p.lookPos-SS::screenShakeY;
        if(lookY>yB-136){p.screenYPos=104+(py-(yB-136))+SS::screenShakeY; SS::yScrollOffset=yB-240-SS::screenShakeY;}
    } else {
        p.screenYPos=py-yA-SS::screenShakeY;
        SS::yScrollOffset=yA+SS::screenShakeY;
    }
    p.screenYPos-=SS::cameraAdjustY;
}

static void computeYScroll(PlayerObject& p, int32_t& yA, int32_t& yB)
{
    using SS=StageSystem;
    int32_t py=(p.yPos>>16)+SS::cameraAdjustY;
    int32_t lookY=py+p.lookPos;
    int32_t dy=lookY-(yA+104);
    if(p.trackScroll==1){
        SS::yScrollMove=32;
    } else {
        if(SS::yScrollMove==32){
            SS::yScrollMove=(104-p.screenYPos-p.lookPos>>1<<1);
            if(SS::yScrollMove>32)SS::yScrollMove=32;
            if(SS::yScrollMove<-32)SS::yScrollMove=-32;
        }
        if(SS::yScrollMove>0){SS::yScrollMove-=6;if(SS::yScrollMove<0)SS::yScrollMove=0;}
        if(SS::yScrollMove<0){SS::yScrollMove+=6;if(SS::yScrollMove>0)SS::yScrollMove=0;}
    }
    if(std::abs(dy)<std::abs(SS::yScrollMove)+17){
        if(SS::yScrollMove==32){
            if(lookY>yA+104+SS::yScrollMove){yA+=lookY-(yA+104+SS::yScrollMove);yB=yA+240;}
            if(lookY<yA+104-SS::yScrollMove){yA-=yA+104-SS::yScrollMove-lookY;yB=yA+240;}
        } else {yA=py-104+SS::yScrollMove+p.lookPos;yB=yA+240;}
    } else {if(dy>0)yA+=16;else yA-=16;yB=yA+240;}
    if(yA<SS::yBoundary1){yA=SS::yBoundary1;yB=SS::yBoundary1+240;}
    if(yB>SS::yBoundary2){yB=SS::yBoundary2;yA=SS::yBoundary2-240;}
}

void PlayerSystem::SetPlayerScreenPosition(PlayerObject& p)
{
    using SS=StageSystem;
    updateYBoundaries(); updateXBoundaries(p);
    int32_t px=p.xPos>>16;
    int32_t xA=SS::xScrollA, xB=SS::xScrollB;
    int32_t dx=px-(xA+GlobalAppDefinitions::SCREEN_CENTER);
    if(std::abs(dx)<25){
        if(px>xA+GlobalAppDefinitions::SCREEN_SCROLL_RIGHT){xA+=px-(xA+GlobalAppDefinitions::SCREEN_SCROLL_RIGHT);xB=xA+GlobalAppDefinitions::SCREEN_XSIZE;}
        if(px<xA+GlobalAppDefinitions::SCREEN_SCROLL_LEFT){xA-=xA+GlobalAppDefinitions::SCREEN_SCROLL_LEFT-px;xB=xA+GlobalAppDefinitions::SCREEN_XSIZE;}
    } else {if(dx>0)xA+=16;else xA-=16;xB=xA+GlobalAppDefinitions::SCREEN_XSIZE;}
    if(xA<SS::xBoundary1){xA=SS::xBoundary1;xB=SS::xBoundary1+GlobalAppDefinitions::SCREEN_XSIZE;}
    if(xB>SS::xBoundary2){xB=SS::xBoundary2;xA=SS::xBoundary2-GlobalAppDefinitions::SCREEN_XSIZE;}
    SS::xScrollA=xA; SS::xScrollB=xB;
    if(px>xA+GlobalAppDefinitions::SCREEN_CENTER){
        SS::xScrollOffset=px-GlobalAppDefinitions::SCREEN_CENTER+SS::screenShakeX;
        p.screenXPos=GlobalAppDefinitions::SCREEN_CENTER-SS::screenShakeX;
        if(px>xB-GlobalAppDefinitions::SCREEN_CENTER){p.screenXPos=GlobalAppDefinitions::SCREEN_CENTER+(px-(xB-GlobalAppDefinitions::SCREEN_CENTER))+SS::screenShakeX;SS::xScrollOffset=xB-GlobalAppDefinitions::SCREEN_XSIZE-SS::screenShakeX;}
    } else {p.screenXPos=px-xA+SS::screenShakeX;SS::xScrollOffset=xA-SS::screenShakeX;}
    int32_t yA=SS::yScrollA, yB=SS::yScrollB;
    computeYScroll(p,yA,yB);
    applyYScroll(p,yA,yB);
    shakeUpdate();
}

void PlayerSystem::SetPlayerScreenPositionCDStyle(PlayerObject& p)
{
    using SS=StageSystem;
    updateYBoundaries(); updateXBoundaries(p);
    int32_t px=p.xPos>>16;
    if(p.gravity==0)
        SS::cameraShift=p.objectPtr->direction!=0?(p.speed<-390594||SS::cameraStyle==3)?2:0:(p.speed>390594||SS::cameraStyle==2)?1:0;
    switch(SS::cameraShift){
    case 0: if(SS::xScrollMove<0)SS::xScrollMove+=2; if(SS::xScrollMove>0)SS::xScrollMove-=2; break;
    case 1: if(SS::xScrollMove>-64)SS::xScrollMove-=2; break;
    case 2: if(SS::xScrollMove<64)SS::xScrollMove+=2; break;
    }
    if(px>SS::xBoundary1+GlobalAppDefinitions::SCREEN_CENTER+SS::xScrollMove){
        SS::xScrollOffset=px-GlobalAppDefinitions::SCREEN_CENTER+SS::screenShakeX-SS::xScrollMove;
        p.screenXPos=GlobalAppDefinitions::SCREEN_CENTER-SS::screenShakeX+SS::xScrollMove;
        if(px-SS::xScrollMove>SS::xBoundary2-GlobalAppDefinitions::SCREEN_CENTER){p.screenXPos=GlobalAppDefinitions::SCREEN_CENTER+(px-(SS::xBoundary2-GlobalAppDefinitions::SCREEN_CENTER))+SS::screenShakeX;SS::xScrollOffset=SS::xBoundary2-GlobalAppDefinitions::SCREEN_XSIZE-SS::screenShakeX;}
    } else {p.screenXPos=px-SS::xBoundary1+SS::screenShakeX;SS::xScrollOffset=SS::xBoundary1-SS::screenShakeX;}
    SS::xScrollA=SS::xScrollOffset; SS::xScrollB=SS::xScrollA+GlobalAppDefinitions::SCREEN_XSIZE;
    int32_t yA=SS::yScrollA, yB=SS::yScrollB;
    computeYScroll(p,yA,yB);
    applyYScroll(p,yA,yB);
    shakeUpdate();
}

void PlayerSystem::SetPlayerLockedScreenPosition(PlayerObject& p)
{
    using SS=StageSystem;
    int32_t px=p.xPos>>16;
    int32_t xA=SS::xScrollA, xB=SS::xScrollB;
    if(px>xA+GlobalAppDefinitions::SCREEN_CENTER){
        SS::xScrollOffset=px-GlobalAppDefinitions::SCREEN_CENTER+SS::screenShakeX;
        p.screenXPos=GlobalAppDefinitions::SCREEN_CENTER-SS::screenShakeX;
        if(px>xB-GlobalAppDefinitions::SCREEN_CENTER){p.screenXPos=GlobalAppDefinitions::SCREEN_CENTER+(px-(xB-GlobalAppDefinitions::SCREEN_CENTER))+SS::screenShakeX;SS::xScrollOffset=xB-GlobalAppDefinitions::SCREEN_XSIZE-SS::screenShakeX;}
    } else {p.screenXPos=px-xA+SS::screenShakeX;SS::xScrollOffset=xA-SS::screenShakeX;}
    int32_t yA=SS::yScrollA, yB=SS::yScrollB;
    applyYScroll(p,yA,yB);
    shakeUpdate();
}

void PlayerSystem::SetPlayerHLockedScreenPosition(PlayerObject& p)
{
    using SS=StageSystem;
    updateYBoundaries();
    int32_t px=p.xPos>>16;
    int32_t xA=SS::xScrollA, xB=SS::xScrollB;
    if(px>xA+GlobalAppDefinitions::SCREEN_CENTER){
        SS::xScrollOffset=px-GlobalAppDefinitions::SCREEN_CENTER+SS::screenShakeX;
        p.screenXPos=GlobalAppDefinitions::SCREEN_CENTER-SS::screenShakeX;
        if(px>xB-GlobalAppDefinitions::SCREEN_CENTER){p.screenXPos=GlobalAppDefinitions::SCREEN_CENTER+(px-(xB-GlobalAppDefinitions::SCREEN_CENTER))+SS::screenShakeX;SS::xScrollOffset=xB-GlobalAppDefinitions::SCREEN_XSIZE-SS::screenShakeX;}
    } else {p.screenXPos=px-xA+SS::screenShakeX;SS::xScrollOffset=xA-SS::screenShakeX;}
    int32_t yA=SS::yScrollA, yB=SS::yScrollB;
    computeYScroll(p,yA,yB);
    applyYScroll(p,yA,yB);
    shakeUpdate();
}

void PlayerSystem::ProcessPlayerControl(PlayerObject& p)
{
    using SS=StageSystem;
    switch(p.controlMode){
    case -1:
        delayUp<<=1;   delayUp  |=(uint16_t)p.up;
        delayDown<<=1; delayDown|=(uint16_t)p.down;
        delayLeft<<=1; delayLeft|=(uint16_t)p.left;
        delayRight<<=1;delayRight|=(uint16_t)p.right;
        delayJumpPress<<=1;delayJumpPress|=(uint16_t)p.jumpPress;
        delayJumpHold<<=1; delayJumpHold |=(uint16_t)p.jumpHold;
        break;
    case 0:
        p.up=SS::gKeyDown.up; p.down=SS::gKeyDown.down;
        if(SS::gKeyDown.left==1&&SS::gKeyDown.right==1){p.left=0;p.right=0;}
        else{p.left=SS::gKeyDown.left;p.right=SS::gKeyDown.right;}
        p.jumpHold=(uint8_t)((uint32_t)SS::gKeyDown.buttonA|(uint32_t)SS::gKeyDown.buttonB|(uint32_t)SS::gKeyDown.buttonC);
        p.jumpPress=(uint8_t)((uint32_t)SS::gKeyPress.buttonA|(uint32_t)SS::gKeyPress.buttonB|(uint32_t)SS::gKeyPress.buttonC);
        delayUp<<=1;   delayUp  |=(uint16_t)p.up;
        delayDown<<=1; delayDown|=(uint16_t)p.down;
        delayLeft<<=1; delayLeft|=(uint16_t)p.left;
        delayRight<<=1;delayRight|=(uint16_t)p.right;
        delayJumpPress<<=1;delayJumpPress|=(uint16_t)p.jumpPress;
        delayJumpHold<<=1; delayJumpHold |=(uint16_t)p.jumpHold;
        break;
    case 1:
        p.up       =(uint8_t)(delayUp>>15);
        p.down     =(uint8_t)(delayDown>>15);
        p.left     =(uint8_t)(delayLeft>>15);
        p.right    =(uint8_t)(delayRight>>15);
        p.jumpPress=(uint8_t)(delayJumpPress>>15);
        p.jumpHold =(uint8_t)(delayJumpHold>>15);
        break;
    }
}

void PlayerSystem::ProcessPlayerTileCollisions(PlayerObject& p)
{
    p.flailing[0]=p.flailing[1]=p.flailing[2]=0;
    ObjectSystem::scriptEng.checkResult=0;
    if(p.gravity==1) ProcessAirCollision(p);
    else ProcessPathGrip(p);
}

static void doFloorCol(PlayerObject& p, CollisionSensor& cs)
{
    using SS=StageSystem;
    int32_t origY=cs.yPos>>16;
    for(int step=0;step<48;step+=16){
        if(cs.collided) break;
        int32_t x=cs.xPos>>16;
        int32_t y=(cs.yPos>>16)-16+step;
        if(x<=-1||y<=-1) continue;
        int32_t tx=x>>7,tc=(x&127)>>4,ty=y>>7,tr=(y&127)>>4;
        int32_t idx=((int32_t)SS::stageLayouts[0].tileMap[tx+(ty<<8)]<<6)+(tc+(tr<<3));
        int32_t t=(int32_t)SS::tile128x128.tile16x16[idx];
        if(SS::tile128x128.collisionFlag[(int32_t)p.collisionPlane][idx]==2||SS::tile128x128.collisionFlag[(int32_t)p.collisionPlane][idx]==3) continue;
        switch(SS::tile128x128.direction[idx]){
        case 0:{int32_t m=(x&15)+(t<<4);if((y&15)>(int32_t)SS::tileCollisions[(int32_t)p.collisionPlane].floorMask[m]-16+step&&SS::tileCollisions[(int32_t)p.collisionPlane].floorMask[m]<15){cs.yPos=(int32_t)SS::tileCollisions[(int32_t)p.collisionPlane].floorMask[m]+(ty<<7)+(tr<<4);cs.collided=1;cs.angle=(int32_t)SS::tileCollisions[(int32_t)p.collisionPlane].angle[t]&255;}break;}
        case 1:{int32_t m=15-(x&15)+(t<<4);if((y&15)>(int32_t)SS::tileCollisions[(int32_t)p.collisionPlane].floorMask[m]-16+step&&SS::tileCollisions[(int32_t)p.collisionPlane].floorMask[m]<15){cs.yPos=(int32_t)SS::tileCollisions[(int32_t)p.collisionPlane].floorMask[m]+(ty<<7)+(tr<<4);cs.collided=1;cs.angle=256-((int32_t)SS::tileCollisions[(int32_t)p.collisionPlane].angle[t]&255);}break;}
        case 2:{int32_t m=(x&15)+(t<<4);if((y&15)>15-(int32_t)SS::tileCollisions[(int32_t)p.collisionPlane].roofMask[m]-16+step){cs.yPos=15-(int32_t)SS::tileCollisions[(int32_t)p.collisionPlane].roofMask[m]+(ty<<7)+(tr<<4);cs.collided=1;cs.angle=384-(int32_t)((SS::tileCollisions[(int32_t)p.collisionPlane].angle[t]&0xFF000000u)>>24)&255;}break;}
        case 3:{int32_t m=15-(x&15)+(t<<4);if((y&15)>15-(int32_t)SS::tileCollisions[(int32_t)p.collisionPlane].roofMask[m]-16+step){cs.yPos=15-(int32_t)SS::tileCollisions[(int32_t)p.collisionPlane].roofMask[m]+(ty<<7)+(tr<<4);cs.collided=1;cs.angle=256-(384-(int32_t)((SS::tileCollisions[(int32_t)p.collisionPlane].angle[t]&0xFF000000u)>>24)&255);}break;}
        }
        if(cs.collided){
            if(cs.angle<0)cs.angle+=256; if(cs.angle>255)cs.angle-=256;
            int32_t d=cs.yPos-origY;
            if(d>14||d<-17){cs.yPos=origY<<16;cs.collided=0;}
        }
    }
}

static void doFindFloor(PlayerObject& p, CollisionSensor& cs, int32_t prev)
{
    using SS=StageSystem;
    int32_t origAngle=cs.angle;
    for(int step=0;step<48;step+=16){
        if(cs.collided) break;
        int32_t x=cs.xPos>>16;
        int32_t y=(cs.yPos>>16)-16+step;
        if(x<=-1||y<=-1) continue;
        int32_t tx=x>>7,tc=(x&127)>>4,ty=y>>7,tr=(y&127)>>4;
        int32_t idx=((int32_t)SS::stageLayouts[0].tileMap[tx+(ty<<8)]<<6)+(tc+(tr<<3));
        int32_t t=(int32_t)SS::tile128x128.tile16x16[idx];
        if(SS::tile128x128.collisionFlag[(int32_t)p.collisionPlane][idx]==2||SS::tile128x128.collisionFlag[(int32_t)p.collisionPlane][idx]==3) continue;
        switch(SS::tile128x128.direction[idx]){
        case 0:{int32_t m=(x&15)+(t<<4);if(SS::tileCollisions[(int32_t)p.collisionPlane].floorMask[m]<64){cs.yPos=(int32_t)SS::tileCollisions[(int32_t)p.collisionPlane].floorMask[m]+(ty<<7)+(tr<<4);cs.collided=1;cs.angle=(int32_t)SS::tileCollisions[(int32_t)p.collisionPlane].angle[t]&255;}break;}
        case 1:{int32_t m=15-(x&15)+(t<<4);if(SS::tileCollisions[(int32_t)p.collisionPlane].floorMask[m]<64){cs.yPos=(int32_t)SS::tileCollisions[(int32_t)p.collisionPlane].floorMask[m]+(ty<<7)+(tr<<4);cs.collided=1;cs.angle=256-((int32_t)SS::tileCollisions[(int32_t)p.collisionPlane].angle[t]&255);}break;}
        case 2:{int32_t m=(x&15)+(t<<4);if(SS::tileCollisions[(int32_t)p.collisionPlane].roofMask[m]>(int8_t)-64){cs.yPos=15-(int32_t)SS::tileCollisions[(int32_t)p.collisionPlane].roofMask[m]+(ty<<7)+(tr<<4);cs.collided=1;cs.angle=384-(int32_t)((SS::tileCollisions[(int32_t)p.collisionPlane].angle[t]&0xFF000000u)>>24)&255;}break;}
        case 3:{int32_t m=15-(x&15)+(t<<4);if(SS::tileCollisions[(int32_t)p.collisionPlane].roofMask[m]>(int8_t)-64){cs.yPos=15-(int32_t)SS::tileCollisions[(int32_t)p.collisionPlane].roofMask[m]+(ty<<7)+(tr<<4);cs.collided=1;cs.angle=256-(384-(int32_t)((SS::tileCollisions[(int32_t)p.collisionPlane].angle[t]&0xFF000000u)>>24)&255);}break;}
        }
        if(cs.collided){
            if(cs.angle<0)cs.angle+=256; if(cs.angle>255)cs.angle-=256;
            if(std::abs(cs.angle-origAngle)>32&&std::abs(cs.angle-256-origAngle)>32&&std::abs(cs.angle+256-origAngle)>32){cs.yPos=prev<<16;cs.collided=0;cs.angle=origAngle;break;}
            if(cs.yPos-prev>14||cs.yPos-prev<-14){cs.yPos=prev<<16;cs.collided=0;}
        }
    }
}

static void doLWall(PlayerObject& p, CollisionSensor& cs)
{
    using SS=StageSystem;
    int32_t origX=cs.xPos>>16;
    for(int step=0;step<48;step+=16){
        if(cs.collided) break;
        int32_t x=(cs.xPos>>16)-16+step;
        int32_t y=cs.yPos>>16;
        if(x<=-1||y<=-1) continue;
        int32_t tx=x>>7,tc=(x&127)>>4,ty=y>>7,tr=(y&127)>>4;
        int32_t idx=((int32_t)SS::stageLayouts[0].tileMap[tx+(ty<<8)]<<6)+(tc+(tr<<3));
        int32_t t=(int32_t)SS::tile128x128.tile16x16[idx];
        if(SS::tile128x128.collisionFlag[(int32_t)p.collisionPlane][idx]==1||SS::tile128x128.collisionFlag[(int32_t)p.collisionPlane][idx]>=3) continue;
        switch(SS::tile128x128.direction[idx]){
        case 0:{int32_t m=(y&15)+(t<<4);if((x&15)>(int32_t)SS::tileCollisions[(int32_t)p.collisionPlane].leftWallMask[m]-16+step){cs.xPos=(int32_t)SS::tileCollisions[(int32_t)p.collisionPlane].leftWallMask[m]+(tx<<7)+(tc<<4);cs.collided=1;}break;}
        case 1:{int32_t m=(y&15)+(t<<4);if((x&15)>15-(int32_t)SS::tileCollisions[(int32_t)p.collisionPlane].rightWallMask[m]-16+step){cs.xPos=15-(int32_t)SS::tileCollisions[(int32_t)p.collisionPlane].rightWallMask[m]+(tx<<7)+(tc<<4);cs.collided=1;}break;}
        case 2:{int32_t m=15-(y&15)+(t<<4);if((x&15)>(int32_t)SS::tileCollisions[(int32_t)p.collisionPlane].leftWallMask[m]-16+step){cs.xPos=(int32_t)SS::tileCollisions[(int32_t)p.collisionPlane].leftWallMask[m]+(tx<<7)+(tc<<4);cs.collided=1;}break;}
        case 3:{int32_t m=15-(y&15)+(t<<4);if((x&15)>15-(int32_t)SS::tileCollisions[(int32_t)p.collisionPlane].rightWallMask[m]-16+step){cs.xPos=15-(int32_t)SS::tileCollisions[(int32_t)p.collisionPlane].rightWallMask[m]+(tx<<7)+(tc<<4);cs.collided=1;}break;}
        }
        if(cs.collided){int32_t d=cs.xPos-origX;if(d>15||d<-15){cs.xPos=origX<<16;cs.collided=0;}}
    }
}

static void doFindLWall(PlayerObject& p, CollisionSensor& cs, int32_t prev)
{
    using SS=StageSystem;
    int32_t origAngle=cs.angle;
    for(int step=0;step<48;step+=16){
        if(cs.collided) break;
        int32_t x=(cs.xPos>>16)-16+step;
        int32_t y=cs.yPos>>16;
        if(x<=-1||y<=-1) continue;
        int32_t tx=x>>7,tc=(x&127)>>4,ty=y>>7,tr=(y&127)>>4;
        int32_t idx=((int32_t)SS::stageLayouts[0].tileMap[tx+(ty<<8)]<<6)+(tc+(tr<<3));
        int32_t t=(int32_t)SS::tile128x128.tile16x16[idx];
        if(SS::tile128x128.collisionFlag[(int32_t)p.collisionPlane][idx]>=3) continue;
        switch(SS::tile128x128.direction[idx]){
        case 0:{int32_t m=(y&15)+(t<<4);if(SS::tileCollisions[(int32_t)p.collisionPlane].leftWallMask[m]<64){cs.xPos=(int32_t)SS::tileCollisions[(int32_t)p.collisionPlane].leftWallMask[m]+(tx<<7)+(tc<<4);cs.collided=1;cs.angle=(int32_t)((SS::tileCollisions[(int32_t)p.collisionPlane].angle[t]&65280u)>>8);}break;}
        case 1:{int32_t m=(y&15)+(t<<4);if(SS::tileCollisions[(int32_t)p.collisionPlane].rightWallMask[m]>(int8_t)-64){cs.xPos=15-(int32_t)SS::tileCollisions[(int32_t)p.collisionPlane].rightWallMask[m]+(tx<<7)+(tc<<4);cs.collided=1;cs.angle=256-(int32_t)((SS::tileCollisions[(int32_t)p.collisionPlane].angle[t]&0xFF0000u)>>16);}break;}
        case 2:{int32_t m=15-(y&15)+(t<<4);if(SS::tileCollisions[(int32_t)p.collisionPlane].leftWallMask[m]<64){cs.xPos=(int32_t)SS::tileCollisions[(int32_t)p.collisionPlane].leftWallMask[m]+(tx<<7)+(tc<<4);cs.collided=1;cs.angle=384-(int32_t)((SS::tileCollisions[(int32_t)p.collisionPlane].angle[t]&65280u)>>8)&255;}break;}
        case 3:{int32_t m=15-(y&15)+(t<<4);if(SS::tileCollisions[(int32_t)p.collisionPlane].rightWallMask[m]>(int8_t)-64){cs.xPos=15-(int32_t)SS::tileCollisions[(int32_t)p.collisionPlane].rightWallMask[m]+(tx<<7)+(tc<<4);cs.collided=1;cs.angle=256-(384-(int32_t)((SS::tileCollisions[(int32_t)p.collisionPlane].angle[t]&0xFF0000u)>>16)&255);}break;}
        }
        if(cs.collided){
            if(cs.angle<0)cs.angle+=256;if(cs.angle>255)cs.angle-=256;
            if(std::abs(origAngle-cs.angle)>32){cs.xPos=prev<<16;cs.collided=0;cs.angle=origAngle;break;}
            if(cs.xPos-prev>14||cs.xPos-prev<-14){cs.xPos=prev<<16;cs.collided=0;}
        }
    }
}

static void doRWall(PlayerObject& p, CollisionSensor& cs)
{
    using SS=StageSystem;
    int32_t origX=cs.xPos>>16;
    for(int step=0;step<48;step+=16){
        if(cs.collided) break;
        int32_t x=(cs.xPos>>16)+16-step;
        int32_t y=cs.yPos>>16;
        if(x<=-1||y<=-1) continue;
        int32_t tx=x>>7,tc=(x&127)>>4,ty=y>>7,tr=(y&127)>>4;
        int32_t idx=((int32_t)SS::stageLayouts[0].tileMap[tx+(ty<<8)]<<6)+(tc+(tr<<3));
        int32_t t=(int32_t)SS::tile128x128.tile16x16[idx];
        if(SS::tile128x128.collisionFlag[(int32_t)p.collisionPlane][idx]==1||SS::tile128x128.collisionFlag[(int32_t)p.collisionPlane][idx]>=3) continue;
        switch(SS::tile128x128.direction[idx]){
        case 0:{int32_t m=(y&15)+(t<<4);if((x&15)<(int32_t)SS::tileCollisions[(int32_t)p.collisionPlane].rightWallMask[m]+16-step){cs.xPos=(int32_t)SS::tileCollisions[(int32_t)p.collisionPlane].rightWallMask[m]+(tx<<7)+(tc<<4);cs.collided=1;}break;}
        case 1:{int32_t m=(y&15)+(t<<4);if((x&15)<15-(int32_t)SS::tileCollisions[(int32_t)p.collisionPlane].leftWallMask[m]+16-step){cs.xPos=15-(int32_t)SS::tileCollisions[(int32_t)p.collisionPlane].leftWallMask[m]+(tx<<7)+(tc<<4);cs.collided=1;}break;}
        case 2:{int32_t m=15-(y&15)+(t<<4);if((x&15)<(int32_t)SS::tileCollisions[(int32_t)p.collisionPlane].rightWallMask[m]+16-step){cs.xPos=(int32_t)SS::tileCollisions[(int32_t)p.collisionPlane].rightWallMask[m]+(tx<<7)+(tc<<4);cs.collided=1;}break;}
        case 3:{int32_t m=15-(y&15)+(t<<4);if((x&15)<15-(int32_t)SS::tileCollisions[(int32_t)p.collisionPlane].leftWallMask[m]+16-step){cs.xPos=15-(int32_t)SS::tileCollisions[(int32_t)p.collisionPlane].leftWallMask[m]+(tx<<7)+(tc<<4);cs.collided=1;}break;}
        }
        if(cs.collided){int32_t d=cs.xPos-origX;if(d>15||d<-15){cs.xPos=origX<<16;cs.collided=0;}}
    }
}

static void doFindRWall(PlayerObject& p, CollisionSensor& cs, int32_t prev)
{
    using SS=StageSystem;
    int32_t origAngle=cs.angle;
    for(int step=0;step<48;step+=16){
        if(cs.collided) break;
        int32_t x=(cs.xPos>>16)+16-step;
        int32_t y=cs.yPos>>16;
        if(x<=-1||y<=-1) continue;
        int32_t tx=x>>7,tc=(x&127)>>4,ty=y>>7,tr=(y&127)>>4;
        int32_t idx=((int32_t)SS::stageLayouts[0].tileMap[tx+(ty<<8)]<<6)+(tc+(tr<<3));
        int32_t t=(int32_t)SS::tile128x128.tile16x16[idx];
        if(SS::tile128x128.collisionFlag[(int32_t)p.collisionPlane][idx]>=3) continue;
        switch(SS::tile128x128.direction[idx]){
        case 0:{int32_t m=(y&15)+(t<<4);if(SS::tileCollisions[(int32_t)p.collisionPlane].rightWallMask[m]>(int8_t)-64){cs.xPos=(int32_t)SS::tileCollisions[(int32_t)p.collisionPlane].rightWallMask[m]+(tx<<7)+(tc<<4);cs.collided=1;cs.angle=(int32_t)((SS::tileCollisions[(int32_t)p.collisionPlane].angle[t]&0xFF0000u)>>16);}break;}
        case 1:{int32_t m=(y&15)+(t<<4);if(SS::tileCollisions[(int32_t)p.collisionPlane].leftWallMask[m]<64){cs.xPos=15-(int32_t)SS::tileCollisions[(int32_t)p.collisionPlane].leftWallMask[m]+(tx<<7)+(tc<<4);cs.collided=1;cs.angle=256-(int32_t)((SS::tileCollisions[(int32_t)p.collisionPlane].angle[t]&65280u)>>8);}break;}
        case 2:{int32_t m=15-(y&15)+(t<<4);if(SS::tileCollisions[(int32_t)p.collisionPlane].rightWallMask[m]>(int8_t)-64){cs.xPos=(int32_t)SS::tileCollisions[(int32_t)p.collisionPlane].rightWallMask[m]+(tx<<7)+(tc<<4);cs.collided=1;cs.angle=384-(int32_t)((SS::tileCollisions[(int32_t)p.collisionPlane].angle[t]&0xFF0000u)>>16)&255;}break;}
        case 3:{int32_t m=15-(y&15)+(t<<4);if(SS::tileCollisions[(int32_t)p.collisionPlane].leftWallMask[m]<64){cs.xPos=15-(int32_t)SS::tileCollisions[(int32_t)p.collisionPlane].leftWallMask[m]+(tx<<7)+(tc<<4);cs.collided=1;cs.angle=256-(384-(int32_t)((SS::tileCollisions[(int32_t)p.collisionPlane].angle[t]&65280u)>>8)&255);}break;}
        }
        if(cs.collided){
            if(cs.angle<0)cs.angle+=256;if(cs.angle>255)cs.angle-=256;
            if(std::abs(cs.angle-origAngle)>32){cs.xPos=prev<<16;cs.collided=0;cs.angle=origAngle;break;}
            if(cs.xPos-prev>14){cs.xPos=prev>>16;cs.collided=0;}
            if(cs.xPos-prev<-14){cs.xPos=prev<<16;cs.collided=0;}
        }
    }
}

static void doRoof(PlayerObject& p, CollisionSensor& cs)
{
    using SS=StageSystem;
    int32_t origY=cs.yPos>>16;
    for(int step=0;step<48;step+=16){
        if(cs.collided) break;
        int32_t x=cs.xPos>>16;
        int32_t y=(cs.yPos>>16)+16-step;
        if(x<=-1||y<=-1) continue;
        int32_t tx=x>>7,tc=(x&127)>>4,ty=y>>7,tr=(y&127)>>4;
        int32_t idx=((int32_t)SS::stageLayouts[0].tileMap[tx+(ty<<8)]<<6)+(tc+(tr<<3));
        int32_t t=(int32_t)SS::tile128x128.tile16x16[idx];
        if(SS::tile128x128.collisionFlag[(int32_t)p.collisionPlane][idx]==1||SS::tile128x128.collisionFlag[(int32_t)p.collisionPlane][idx]>=3) continue;
        switch(SS::tile128x128.direction[idx]){
        case 0:{int32_t m=(x&15)+(t<<4);if((y&15)<(int32_t)SS::tileCollisions[(int32_t)p.collisionPlane].roofMask[m]+16-step){cs.yPos=(int32_t)SS::tileCollisions[(int32_t)p.collisionPlane].roofMask[m]+(ty<<7)+(tr<<4);cs.collided=1;cs.angle=(int32_t)((SS::tileCollisions[(int32_t)p.collisionPlane].angle[t]&0xFF000000u)>>24);}break;}
        case 1:{int32_t m=15-(x&15)+(t<<4);if((y&15)<(int32_t)SS::tileCollisions[(int32_t)p.collisionPlane].roofMask[m]+16-step){cs.yPos=(int32_t)SS::tileCollisions[(int32_t)p.collisionPlane].roofMask[m]+(ty<<7)+(tr<<4);cs.collided=1;cs.angle=256-(int32_t)((SS::tileCollisions[(int32_t)p.collisionPlane].angle[t]&0xFF000000u)>>24);}break;}
        case 2:{int32_t m=(x&15)+(t<<4);if((y&15)<15-(int32_t)SS::tileCollisions[(int32_t)p.collisionPlane].floorMask[m]+16-step){cs.yPos=15-(int32_t)SS::tileCollisions[(int32_t)p.collisionPlane].floorMask[m]+(ty<<7)+(tr<<4);cs.collided=1;cs.angle=384-((int32_t)SS::tileCollisions[(int32_t)p.collisionPlane].angle[t]&255)&255;}break;}
        case 3:{int32_t m=15-(x&15)+(t<<4);if((y&15)<15-(int32_t)SS::tileCollisions[(int32_t)p.collisionPlane].floorMask[m]+16-step){cs.yPos=15-(int32_t)SS::tileCollisions[(int32_t)p.collisionPlane].floorMask[m]+(ty<<7)+(tr<<4);cs.collided=1;cs.angle=256-(384-((int32_t)SS::tileCollisions[(int32_t)p.collisionPlane].angle[t]&255)&255);}break;}
        }
        if(cs.collided){
            if(cs.angle<0)cs.angle+=256;if(cs.angle>255)cs.angle-=256;
            int32_t d=cs.yPos-origY;
            if(d>14||d<-14){cs.yPos=origY<<16;cs.collided=0;}
        }
    }
}

static void doFindRoof(PlayerObject& p, CollisionSensor& cs, int32_t prev)
{
    using SS=StageSystem;
    int32_t origAngle=cs.angle;
    for(int step=0;step<48;step+=16){
        if(cs.collided) break;
        int32_t x=cs.xPos>>16;
        int32_t y=(cs.yPos>>16)+16-step;
        if(x<=-1||y<=-1) continue;
        int32_t tx=x>>7,tc=(x&127)>>4,ty=y>>7,tr=(y&127)>>4;
        int32_t idx=((int32_t)SS::stageLayouts[0].tileMap[tx+(ty<<8)]<<6)+(tc+(tr<<3));
        int32_t t=(int32_t)SS::tile128x128.tile16x16[idx];
        if(SS::tile128x128.collisionFlag[(int32_t)p.collisionPlane][idx]>=3) continue;
        switch(SS::tile128x128.direction[idx]){
        case 0:{int32_t m=(x&15)+(t<<4);if(SS::tileCollisions[(int32_t)p.collisionPlane].roofMask[m]>(int8_t)-64){cs.yPos=(int32_t)SS::tileCollisions[(int32_t)p.collisionPlane].roofMask[m]+(ty<<7)+(tr<<4);cs.collided=1;cs.angle=(int32_t)((SS::tileCollisions[(int32_t)p.collisionPlane].angle[t]&0xFF000000u)>>24);}break;}
        case 1:{int32_t m=15-(x&15)+(t<<4);if(SS::tileCollisions[(int32_t)p.collisionPlane].roofMask[m]>(int8_t)-64){cs.yPos=(int32_t)SS::tileCollisions[(int32_t)p.collisionPlane].roofMask[m]+(ty<<7)+(tr<<4);cs.collided=1;cs.angle=256-(int32_t)((SS::tileCollisions[(int32_t)p.collisionPlane].angle[t]&0xFF000000u)>>24);}break;}
        case 2:{int32_t m=(x&15)+(t<<4);if(SS::tileCollisions[(int32_t)p.collisionPlane].floorMask[m]<64){cs.yPos=15-(int32_t)SS::tileCollisions[(int32_t)p.collisionPlane].floorMask[m]+(ty<<7)+(tr<<4);cs.collided=1;cs.angle=384-((int32_t)SS::tileCollisions[(int32_t)p.collisionPlane].angle[t]&255)&255;}break;}
        case 3:{int32_t m=15-(x&15)+(t<<4);if(SS::tileCollisions[(int32_t)p.collisionPlane].floorMask[m]<64){cs.yPos=15-(int32_t)SS::tileCollisions[(int32_t)p.collisionPlane].floorMask[m]+(ty<<7)+(tr<<4);cs.collided=1;cs.angle=256-(384-((int32_t)SS::tileCollisions[(int32_t)p.collisionPlane].angle[t]&255)&255);}break;}
        }
        if(cs.collided){
            if(cs.angle<0)cs.angle+=256;if(cs.angle>255)cs.angle-=256;
            if(std::abs(cs.angle-origAngle)>32){cs.yPos=prev<<16;cs.collided=0;cs.angle=origAngle;break;}
            if(cs.yPos-prev>15||cs.yPos-prev<-15){cs.yPos=prev<<16;cs.collided=0;}
        }
    }
}

void PlayerSystem::FloorCollision(PlayerObject& p, CollisionSensor& cs) { doFloorCol(p,cs); }
void PlayerSystem::FindFloorPosition(PlayerObject& p, CollisionSensor& cs, int32_t prev) { doFindFloor(p,cs,prev); }
void PlayerSystem::LWallCollision(PlayerObject& p, CollisionSensor& cs) { doLWall(p,cs); }
void PlayerSystem::FindLWallPosition(PlayerObject& p, CollisionSensor& cs, int32_t prev) { doFindLWall(p,cs,prev); }
void PlayerSystem::RWallCollision(PlayerObject& p, CollisionSensor& cs) { doRWall(p,cs); }
void PlayerSystem::FindRWallPosition(PlayerObject& p, CollisionSensor& cs, int32_t prev) { doFindRWall(p,cs,prev); }
void PlayerSystem::RoofCollision(PlayerObject& p, CollisionSensor& cs) { doRoof(p,cs); }
void PlayerSystem::FindRoofPosition(PlayerObject& p, CollisionSensor& cs, int32_t prev) { doFindRoof(p,cs,prev); }

void PlayerSystem::ProcessAirCollision(PlayerObject& p)
{
    auto& cb=getCBox(p);
    collisionLeft=(int32_t)cb.left[0]; collisionTop=(int32_t)cb.top[0];
    collisionRight=(int32_t)cb.right[0]; collisionBottom=(int32_t)cb.bottom[0];
    uint8_t doR=p.xVelocity>=0?1:0;
    uint8_t doL=p.xVelocity<=0?1:0;
    if(doR){cSensor[0].yPos=p.yPos+131072;cSensor[0].collided=0;cSensor[0].xPos=p.xPos+(collisionRight<<16);}
    if(doL){cSensor[1].yPos=p.yPos+131072;cSensor[1].collided=0;cSensor[1].xPos=p.xPos+((collisionLeft-1)<<16);}
    cSensor[2].xPos=p.xPos+((int32_t)cb.left[1]<<16);  cSensor[3].xPos=p.xPos+((int32_t)cb.right[1]<<16);
    cSensor[2].collided=cSensor[3].collided=0;
    cSensor[4].xPos=cSensor[2].xPos; cSensor[5].xPos=cSensor[3].xPos;
    cSensor[4].collided=cSensor[5].collided=0;
    uint8_t doFloor=p.yVelocity>=0?1:0;
    if(doFloor){cSensor[2].yPos=p.yPos+(collisionBottom<<16);cSensor[3].yPos=cSensor[2].yPos;}
    cSensor[4].yPos=p.yPos+((collisionTop-1)<<16); cSensor[5].yPos=cSensor[4].yPos;
    int32_t steps=std::abs(p.xVelocity)>std::abs(p.yVelocity)?(std::abs(p.xVelocity)>>19)+1:(std::abs(p.yVelocity)>>19)+1;
    int32_t dxStep=p.xVelocity/steps, dyStep=p.yVelocity/steps;
    int32_t dxLast=p.xVelocity-dxStep*(steps-1), dyLast=p.yVelocity-dyStep*(steps-1);
    while(steps>0){
        int32_t dx=steps<2?dxLast:dxStep;
        int32_t dy=steps<2?dyLast:dyStep;
        --steps;
        if(doR==1){cSensor[0].xPos+=dx+65536;cSensor[0].yPos+=dy;LWallCollision(p,cSensor[0]);if(cSensor[0].collided==1)doR=2;}
        if(doL==1){cSensor[1].xPos+=dx-65536;cSensor[1].yPos+=dy;RWallCollision(p,cSensor[1]);if(cSensor[1].collided==1)doL=2;}
        if(doR==2){p.xVelocity=0;p.speed=0;p.xPos=(cSensor[0].xPos-collisionRight)<<16;cSensor[2].xPos=p.xPos+((collisionLeft+1)<<16);cSensor[3].xPos=p.xPos+((collisionRight-2)<<16);cSensor[4].xPos=cSensor[2].xPos;cSensor[5].xPos=cSensor[3].xPos;dx=dxLast=0;doR=3;}
        if(doL==2){p.xVelocity=0;p.speed=0;p.xPos=(cSensor[1].xPos-collisionLeft+1)<<16;cSensor[2].xPos=p.xPos+((collisionLeft+1)<<16);cSensor[3].xPos=p.xPos+((collisionRight-2)<<16);cSensor[4].xPos=cSensor[2].xPos;cSensor[5].xPos=cSensor[3].xPos;dx=dxLast=0;doL=3;}
        if(doFloor==1){
            for(int i=2;i<4;++i){if(!cSensor[i].collided){cSensor[i].xPos+=dx;cSensor[i].yPos+=dy;FloorCollision(p,cSensor[i]);}}
            if(cSensor[2].collided|cSensor[3].collided){doFloor=2;steps=0;}
        }
        if(1){
            for(int i=4;i<6;++i){if(!cSensor[i].collided){cSensor[i].xPos+=dx;cSensor[i].yPos+=dy;RoofCollision(p,cSensor[i]);}}
            if(cSensor[4].collided|cSensor[5].collided){steps=0;}
        }
    }
    if(doR<2&&doL<2) p.xPos+=p.xVelocity;
    if(!cSensor[4].collided&&!cSensor[5].collided&&doFloor<2){p.yPos+=p.yVelocity;return;}
    if(doFloor==2){
        p.gravity=0;
        CollisionSensor* best=nullptr;
        if(cSensor[2].collided&&cSensor[3].collided) best=cSensor[2].yPos<cSensor[3].yPos?&cSensor[2]:&cSensor[3];
        else if(cSensor[2].collided) best=&cSensor[2];
        else if(cSensor[3].collided) best=&cSensor[3];
        if(best){p.yPos=(best->yPos-collisionBottom)<<16;p.angle=best->angle;}
        if(p.angle>160&&p.angle<224&&p.collisionMode!=1){p.collisionMode=1;p.xPos-=262144;}
        if(p.angle>32&&p.angle<96&&p.collisionMode!=3){p.collisionMode=3;p.xPos+=262144;}
        if(p.angle<32||p.angle>224) p.controlLock=0;
        p.objectPtr->rotation=p.angle<<1;
        int32_t spd;
        if(!p.down){
            if(p.angle>=128&&p.angle<=240){if(p.angle<=224){spd=std::abs(p.xVelocity)<=std::abs(p.yVelocity)?-p.yVelocity:p.xVelocity;}else{spd=std::abs(p.xVelocity)<=std::abs(p.yVelocity>>1)?-(p.yVelocity>>1):p.xVelocity;}}
            else if(p.angle>=16&&p.angle<128){if(p.angle>=32){spd=std::abs(p.xVelocity)<=std::abs(p.yVelocity)?p.yVelocity:p.xVelocity;}else{spd=std::abs(p.xVelocity)<=std::abs(p.yVelocity>>1)?p.yVelocity>>1:p.xVelocity;}}
            else spd=p.xVelocity;
        } else {
            if(p.angle>=128&&p.angle<=240){if(p.angle<=224){spd=std::abs(p.xVelocity)<=std::abs(p.yVelocity)?-(p.yVelocity+p.yVelocity/12):p.xVelocity;}else{spd=std::abs(p.xVelocity)<=std::abs(p.yVelocity>>1)?-(p.yVelocity+p.yVelocity/12>>1):p.xVelocity;}}
            else if(p.angle>=16&&p.angle<128){if(p.angle>=32){spd=std::abs(p.xVelocity)<=std::abs(p.yVelocity)?p.yVelocity+p.yVelocity/12:p.xVelocity;}else{spd=std::abs(p.xVelocity)<=std::abs(p.yVelocity>>1)?p.yVelocity+p.yVelocity/12>>1:p.xVelocity;}}
            else spd=p.xVelocity;
        }
        if(spd<-1572864)spd=-1572864; if(spd>1572864)spd=1572864;
        p.speed=spd; p.yVelocity=0;
        ObjectSystem::scriptEng.checkResult=1;
    }
    if(cSensor[4].collided||cSensor[5].collided){
        CollisionSensor* best=nullptr;
        int32_t ang=0;
        if(cSensor[4].collided&&cSensor[5].collided){if(cSensor[4].yPos>cSensor[5].yPos){best=&cSensor[4];ang=cSensor[4].angle;}else{best=&cSensor[5];ang=cSensor[5].angle;}}
        else if(cSensor[4].collided){best=&cSensor[4];ang=cSensor[4].angle;}
        else{best=&cSensor[5];ang=cSensor[5].angle;}
        if(best)p.yPos=(best->yPos-collisionTop+1)<<16;
        int32_t na=ang&255;
        int32_t ma=(int32_t)GlobalAppDefinitions::ArcTanLookup(p.xVelocity,p.yVelocity);
        if(na>64&&na<98&&ma>160&&ma<194){p.gravity=0;p.angle=na;p.objectPtr->rotation=na<<1;p.collisionMode=3;p.xPos+=262144;p.yPos-=131072;p.speed=p.angle<=96?p.yVelocity:p.yVelocity>>1;}
        if(na>158&&na<192&&ma>190&&ma<224){p.gravity=0;p.angle=na;p.objectPtr->rotation=na<<1;p.collisionMode=1;p.xPos-=262144;p.yPos-=131072;p.speed=p.angle>=160?-p.yVelocity:-p.yVelocity>>1;}
        if(p.yVelocity<0)p.yVelocity=0;
        ObjectSystem::scriptEng.checkResult=2;
    }
}

void PlayerSystem::SetPathGripSensors(PlayerObject& p)
{
    auto& cb=getCBox(p);
    switch(p.collisionMode){
    case 0:
        collisionLeft=(int32_t)cb.left[0]; collisionTop=(int32_t)cb.top[0];
        collisionRight=(int32_t)cb.right[0]; collisionBottom=(int32_t)cb.bottom[0];
        cSensor[0].yPos=cSensor[4].yPos+(collisionBottom<<16);
        cSensor[1].yPos=cSensor[0].yPos; cSensor[2].yPos=cSensor[0].yPos;
        cSensor[3].yPos=cSensor[4].yPos+262144;
        cSensor[0].xPos=cSensor[4].xPos+((int32_t)cb.left[1]-1<<16);
        cSensor[1].xPos=cSensor[4].xPos;
        cSensor[2].xPos=cSensor[4].xPos+((int32_t)cb.right[1]<<16);
        cSensor[3].xPos=p.speed>0?cSensor[4].xPos+((collisionRight+1)<<16):cSensor[4].xPos+((collisionLeft-1)<<16);
        break;
    case 1:
        collisionLeft=(int32_t)cb.left[2]; collisionTop=(int32_t)cb.top[2];
        collisionRight=(int32_t)cb.right[2]; collisionBottom=(int32_t)cb.bottom[2];
        cSensor[0].xPos=cSensor[4].xPos+(collisionRight<<16);
        cSensor[1].xPos=cSensor[0].xPos; cSensor[2].xPos=cSensor[0].xPos;
        cSensor[3].xPos=cSensor[4].xPos+262144;
        cSensor[0].yPos=cSensor[4].yPos+((int32_t)cb.top[3]-1<<16);
        cSensor[1].yPos=cSensor[4].yPos;
        cSensor[2].yPos=cSensor[4].yPos+((int32_t)cb.bottom[3]<<16);
        cSensor[3].yPos=p.speed>0?cSensor[4].yPos+(collisionTop<<16):cSensor[4].yPos+((collisionBottom-1)<<16);
        break;
    case 2:
        collisionLeft=(int32_t)cb.left[4]; collisionTop=(int32_t)cb.top[4];
        collisionRight=(int32_t)cb.right[4]; collisionBottom=(int32_t)cb.bottom[4];
        cSensor[0].yPos=cSensor[4].yPos+((collisionTop-1)<<16);
        cSensor[1].yPos=cSensor[0].yPos; cSensor[2].yPos=cSensor[0].yPos;
        cSensor[3].yPos=cSensor[4].yPos-262144;
        cSensor[0].xPos=cSensor[4].xPos+((int32_t)cb.left[5]-1<<16);
        cSensor[1].xPos=cSensor[4].xPos;
        cSensor[2].xPos=cSensor[4].xPos+((int32_t)cb.right[5]<<16);
        cSensor[3].xPos=p.speed<0?cSensor[4].xPos+((collisionRight+1)<<16):cSensor[4].xPos+((collisionLeft-1)<<16);
        break;
    case 3:
        collisionLeft=(int32_t)cb.left[6]; collisionTop=(int32_t)cb.top[6];
        collisionRight=(int32_t)cb.right[6]; collisionBottom=(int32_t)cb.bottom[6];
        cSensor[0].xPos=cSensor[4].xPos+((collisionLeft-1)<<16);
        cSensor[1].xPos=cSensor[0].xPos; cSensor[2].xPos=cSensor[0].xPos;
        cSensor[3].xPos=cSensor[4].xPos-262144;
        cSensor[0].yPos=cSensor[4].yPos+((int32_t)cb.top[7]-1<<16);
        cSensor[1].yPos=cSensor[4].yPos;
        cSensor[2].yPos=cSensor[4].yPos+((int32_t)cb.bottom[7]<<16);
        cSensor[3].yPos=p.speed>0?cSensor[4].yPos+(collisionBottom<<16):cSensor[4].yPos+((collisionTop-1)<<16);
        break;
    }
}

void PlayerSystem::ProcessPathGrip(PlayerObject& p)
{
    int32_t best=-1;
    cSensor[4].xPos=p.xPos; cSensor[4].yPos=p.yPos;
    cSensor[3].collided=0;
    cSensor[0].angle=cSensor[1].angle=cSensor[2].angle=p.angle;
    SetPathGripSensors(p);

    int32_t spd=std::abs(p.speed);
    int32_t steps=spd>>18;
    int32_t rem=spd&262143;

    while(steps>-1){
        int32_t dx,dy;
        if(steps<1){dx=GlobalAppDefinitions::CosValue256[p.angle]*rem>>8;dy=GlobalAppDefinitions::SinValue256[p.angle]*rem>>8;steps=-1;}
        else{dx=GlobalAppDefinitions::CosValue256[p.angle]<<10;dy=GlobalAppDefinitions::SinValue256[p.angle]<<10;--steps;}
        if(p.speed<0){dx=-dx;dy=-dy;}
        cSensor[0].collided=cSensor[1].collided=cSensor[2].collided=0;
        cSensor[4].xPos+=dx; cSensor[4].yPos+=dy;
        switch(p.collisionMode){
        case 0:
            cSensor[3].xPos+=dx; cSensor[3].yPos+=dy;
            if(p.speed>0)LWallCollision(p,cSensor[3]); else if(p.speed<0)RWallCollision(p,cSensor[3]);
            if(cSensor[3].collided){dx=0;steps=-1;}
            for(int i=0;i<3;++i){cSensor[i].xPos+=dx;cSensor[i].yPos+=dy;FindFloorPosition(p,cSensor[i],cSensor[i].yPos>>16);}
            best=-1;
            for(int i=0;i<3;++i){
                if(best>-1){if(cSensor[i].collided&&cSensor[i].yPos<cSensor[best].yPos)best=i;if(cSensor[i].collided&&cSensor[i].yPos==cSensor[best].yPos&&(cSensor[i].angle<8||cSensor[i].angle>248))best=i;}
                else if(cSensor[i].collided)best=i;
            }
            if(best>-1){
                {int32_t _by=cSensor[best].yPos,_ba=cSensor[best].angle;
                for(int i=0;i<3;++i){cSensor[i].yPos=_by<<16;cSensor[i].angle=_ba;}}
                cSensor[3].yPos=cSensor[0].yPos-262144; cSensor[3].angle=cSensor[0].angle;
                cSensor[4].xPos=cSensor[1].xPos; cSensor[4].yPos=cSensor[1].yPos-(collisionBottom<<16);
            } else steps=-1;
            if(cSensor[0].angle<222&&cSensor[0].angle>128)p.collisionMode=1;
            if(cSensor[0].angle>34&&cSensor[0].angle<128)p.collisionMode=3;
            break;
        case 1:
            cSensor[3].xPos+=dx;cSensor[3].yPos+=dy;
            if(p.speed>0)RoofCollision(p,cSensor[3]);else if(p.speed<0)FloorCollision(p,cSensor[3]);
            if(cSensor[3].collided){dy=0;steps=-1;}
            for(int i=0;i<3;++i){cSensor[i].xPos+=dx;cSensor[i].yPos+=dy;FindLWallPosition(p,cSensor[i],cSensor[i].xPos>>16);}
            best=-1;
            for(int i=0;i<3;++i){if(best>-1){if(cSensor[i].xPos<cSensor[best].xPos&&cSensor[i].collided)best=i;}else if(cSensor[i].collided)best=i;}
            if(best>-1){
                {int32_t _bx=cSensor[best].xPos,_ba=cSensor[best].angle;
                for(int i=0;i<3;++i){cSensor[i].xPos=_bx<<16;cSensor[i].angle=_ba;}}
                cSensor[4].yPos=cSensor[1].yPos; cSensor[4].xPos=cSensor[1].xPos-(collisionRight<<16);
            } else steps=-1;
            if(cSensor[0].angle>226)p.collisionMode=0;
            if(cSensor[0].angle<158)p.collisionMode=2;
            break;
        case 2:
            cSensor[3].xPos+=dx;cSensor[3].yPos+=dy;
            if(p.speed>0)RWallCollision(p,cSensor[3]);else if(p.speed<0)LWallCollision(p,cSensor[3]);
            if(cSensor[3].collided){dx=0;steps=-1;}
            for(int i=0;i<3;++i){cSensor[i].xPos+=dx;cSensor[i].yPos+=dy;FindRoofPosition(p,cSensor[i],cSensor[i].yPos>>16);}
            best=-1;
            for(int i=0;i<3;++i){if(best>-1){if(cSensor[i].yPos>cSensor[best].yPos&&cSensor[i].collided)best=i;}else if(cSensor[i].collided)best=i;}
            if(best>-1){
                {int32_t _by=cSensor[best].yPos,_ba=cSensor[best].angle;
                for(int i=0;i<3;++i){cSensor[i].yPos=_by<<16;cSensor[i].angle=_ba;}}
                cSensor[3].yPos=cSensor[0].yPos+262144;cSensor[3].angle=cSensor[0].angle;
                cSensor[4].xPos=cSensor[1].xPos;cSensor[4].yPos=cSensor[1].yPos-((collisionTop-1)<<16);
            } else steps=-1;
            if(cSensor[0].angle>162)p.collisionMode=1;
            if(cSensor[0].angle<94)p.collisionMode=3;
            break;
        case 3:
            cSensor[3].xPos+=dx;cSensor[3].yPos+=dy;
            if(p.speed>0)FloorCollision(p,cSensor[3]);else if(p.speed<0)RoofCollision(p,cSensor[3]);
            if(cSensor[3].collided){dy=0;steps=-1;}
            for(int i=0;i<3;++i){cSensor[i].xPos+=dx;cSensor[i].yPos+=dy;FindRWallPosition(p,cSensor[i],cSensor[i].xPos>>16);}
            best=-1;
            for(int i=0;i<3;++i){if(best>-1){if(cSensor[i].xPos>cSensor[best].xPos&&cSensor[i].collided)best=i;}else if(cSensor[i].collided)best=i;}
            if(best>-1){
                {int32_t _bx=cSensor[best].xPos,_ba=cSensor[best].angle;
                for(int i=0;i<3;++i){cSensor[i].xPos=_bx<<16;cSensor[i].angle=_ba;}}
                cSensor[4].yPos=cSensor[1].yPos;cSensor[4].xPos=cSensor[1].xPos-((collisionLeft-1)<<16);
            } else steps=-1;
            if(cSensor[0].angle<30)p.collisionMode=0;
            if(cSensor[0].angle>98)p.collisionMode=2;
            break;
        }
        if(best>-1) p.angle=cSensor[0].angle;
        if(cSensor[3].collided)steps=-2;
        else SetPathGripSensors(p);
    }

    switch(p.collisionMode){
    case 0:
        if(!cSensor[0].collided&&!cSensor[1].collided&&!cSensor[2].collided){
            Log::Info("GRIP->AIR xPos=%d yPos=%d angle=%d spd=%d sin=%d cos=%d cs3col=%d colBot=%d",
                cSensor[4].xPos>>16, cSensor[4].yPos>>16,
                p.angle, p.speed,
                (int)GlobalAppDefinitions::SinValue256[p.angle],
                (int)GlobalAppDefinitions::CosValue256[p.angle],
                (int)cSensor[3].collided, collisionBottom);
            p.gravity=1;p.collisionMode=0;
            p.xVelocity=GlobalAppDefinitions::CosValue256[p.angle]*p.speed>>8;
            p.yVelocity=GlobalAppDefinitions::SinValue256[p.angle]*p.speed>>8;
            Log::Info("  rawXV=%d rawYV=%d (cap check)",p.xVelocity,p.yVelocity);
            if(p.yVelocity<-1048576)p.yVelocity=-1048576;
            if(p.yVelocity>1048576)p.yVelocity=1048576;
            p.speed=p.xVelocity; p.angle=0;
            if(cSensor[3].collided){
                if(p.speed>0)p.xPos=(cSensor[3].xPos-collisionRight)<<16;
                if(p.speed<0)p.xPos=(cSensor[3].xPos-collisionLeft+1)<<16;
                p.speed=0;
                if((p.left||p.right)&&p.pushing<2)++p.pushing;
            } else {p.pushing=0;p.xPos+=p.xVelocity;}
            p.yPos+=p.yVelocity;
        } else {
            p.angle=cSensor[0].angle; p.objectPtr->rotation=p.angle<<1;
            p.flailing[0]=cSensor[0].collided;p.flailing[1]=cSensor[1].collided;p.flailing[2]=cSensor[2].collided;
            if(cSensor[3].collided){
                if(p.speed>0)p.xPos=(cSensor[3].xPos-collisionRight)<<16;
                if(p.speed<0)p.xPos=(cSensor[3].xPos-collisionLeft+1)<<16;
                p.speed=0;
                if((p.left||p.right)&&p.pushing<2)++p.pushing;
            } else {p.pushing=0;p.xPos=cSensor[4].xPos;}
            if(cSensor[4].yPos>>16 < 100){
                Log::Info("SNAP_GRND cs4Y=%d cs0Y=%d cs1Y=%d cs2Y=%d best=%d colBot=%d prevPY=%d",
                    cSensor[4].yPos>>16, cSensor[0].yPos, cSensor[1].yPos, cSensor[2].yPos,
                    best, collisionBottom, p.yPos>>16);
            }
            p.yPos=cSensor[4].yPos;
        }
        break;
    case 1:
        if(!cSensor[0].collided&&!cSensor[1].collided&&!cSensor[2].collided){
            p.gravity=1;p.collisionMode=0;
            p.xVelocity=GlobalAppDefinitions::CosValue256[p.angle]*p.speed>>8;
            p.yVelocity=GlobalAppDefinitions::SinValue256[p.angle]*p.speed>>8;
            if(p.yVelocity<-1048576)p.yVelocity=-1048576;
            if(p.yVelocity>1048576)p.yVelocity=1048576;
            p.speed=p.xVelocity;p.angle=0;
        } else if(p.speed<163840&&p.speed>-163840&&p.controlLock==0){
            p.gravity=1;p.angle=0;p.collisionMode=0;p.speed=p.xVelocity;p.controlLock=30;
        } else {p.angle=cSensor[0].angle;p.objectPtr->rotation=p.angle<<1;}
        if(cSensor[3].collided){if(p.speed>0)p.yPos=(cSensor[3].yPos-collisionTop)<<16;if(p.speed<0)p.yPos=(cSensor[3].yPos-collisionBottom)<<16;p.speed=0;}
        else p.yPos=cSensor[4].yPos;
        p.xPos=cSensor[4].xPos;
        break;
    case 2:
        if(!cSensor[0].collided&&!cSensor[1].collided&&!cSensor[2].collided){
            p.gravity=1;p.collisionMode=0;
            p.xVelocity=GlobalAppDefinitions::CosValue256[p.angle]*p.speed>>8;
            p.yVelocity=GlobalAppDefinitions::SinValue256[p.angle]*p.speed>>8;
            p.flailing[0]=p.flailing[1]=p.flailing[2]=0;
            if(p.yVelocity<-1048576)p.yVelocity=-1048576;
            if(p.yVelocity>1048576)p.yVelocity=1048576;
            p.angle=0;p.speed=p.xVelocity;
            if(cSensor[3].collided){if(p.speed>0)p.xPos=(cSensor[3].xPos-collisionRight)<<16;if(p.speed<0)p.xPos=(cSensor[3].xPos-collisionLeft+1)<<16;p.speed=0;}
            else p.xPos+=p.xVelocity;
        } else if(p.speed>-163840&&p.speed<163840){
            p.gravity=1;p.angle=0;p.collisionMode=0;p.speed=p.xVelocity;
            p.flailing[0]=p.flailing[1]=p.flailing[2]=0;
            if(cSensor[3].collided){if(p.speed>0)p.xPos=(cSensor[3].xPos-collisionRight)<<16;if(p.speed<0)p.xPos=(cSensor[3].xPos-collisionLeft+1)<<16;p.speed=0;}
            else p.xPos+=p.xVelocity;
        } else {
            p.angle=cSensor[0].angle;p.objectPtr->rotation=p.angle<<1;
            if(cSensor[3].collided){if(p.speed<0)p.xPos=(cSensor[3].xPos-collisionRight)<<16;if(p.speed>0)p.xPos=(cSensor[3].xPos-collisionLeft+1)<<16;p.speed=0;}
            else p.xPos=cSensor[4].xPos;
        }
        p.yPos=cSensor[4].yPos;
        break;
    case 3:
        if(!cSensor[0].collided&&!cSensor[1].collided&&!cSensor[2].collided){
            p.gravity=1;p.collisionMode=0;
            p.xVelocity=GlobalAppDefinitions::CosValue256[p.angle]*p.speed>>8;
            p.yVelocity=GlobalAppDefinitions::SinValue256[p.angle]*p.speed>>8;
            if(p.yVelocity<-1048576)p.yVelocity=-1048576;
            if(p.yVelocity>1048576)p.yVelocity=1048576;
            p.speed=p.xVelocity;p.angle=0;
        } else if(p.speed>-163840&&p.speed<163840&&p.controlLock==0){
            p.gravity=1;p.angle=0;p.collisionMode=0;p.speed=p.xVelocity;p.controlLock=30;
        } else {p.angle=cSensor[0].angle;p.objectPtr->rotation=p.angle<<1;}
        if(cSensor[3].collided){if(p.speed>0)p.yPos=(cSensor[3].yPos-collisionBottom)<<16;if(p.speed<0)p.yPos=(cSensor[3].yPos-collisionTop+1)<<16;p.speed=0;}
        else p.yPos=cSensor[4].yPos;
        p.xPos=cSensor[4].xPos;
        break;
    }
}

}
