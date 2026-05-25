#pragma once
#include <cstdint>
#include "SpriteFrame.h"
#include "SpriteAnimation.h"
#include "AnimationFileList.h"
#include "CollisionBox.h"
#include "ObjectEntity.h"

namespace rvm {

struct AnimationSystem {
    static SpriteFrame      animationFrames[4096];
    static int32_t          animationFramesNo;
    static SpriteAnimation  animationList[1024];
    static int32_t          animationListNo;
    static AnimationFileList animationFile[256];
    static int32_t          animationFileNo;
    static CollisionBox     collisionBoxList[32];
    static int32_t          collisionBoxNo;

    static void Init();
    static void LoadAnimationFile(const char* filePath);
    static AnimationFileList* AddAnimationFile(const char* fileName);
    static AnimationFileList* GetDefaultAnimationRef();
    static void ClearAnimationData();
    static void ProcessObjectAnimation(SpriteAnimation& animationRef,
                                       ObjectEntity& currentObject);
    static void DrawObjectAnimation(SpriteAnimation& animationRef,
                                    ObjectEntity& currentObject,
                                    int32_t xPos, int32_t yPos);
};

}
