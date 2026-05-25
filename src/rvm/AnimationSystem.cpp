#include "AnimationSystem.h"
#include "FileIO.h"
#include "GraphicsSystem.h"

namespace rvm {

SpriteFrame       AnimationSystem::animationFrames[4096] {};
int32_t           AnimationSystem::animationFramesNo     = 0;
SpriteAnimation   AnimationSystem::animationList[1024]   {};
int32_t           AnimationSystem::animationListNo       = 0;
AnimationFileList AnimationSystem::animationFile[256]    {};
int32_t           AnimationSystem::animationFileNo       = 0;
CollisionBox      AnimationSystem::collisionBoxList[32]  {};
int32_t           AnimationSystem::collisionBoxNo        = 0;

void AnimationSystem::Init()
{

}

void AnimationSystem::LoadAnimationFile(const char* filePath)
{
    char fileName[32]{};
    uint8_t numArray[24]{};
    FileData fData{};
    if (!FileIO::LoadFile(filePath, fData))
        return;

    uint8_t num1 = FileIO::ReadByte();
    for (int i = 0; i < 24; ++i) numArray[i] = 0;
    for (int i = 0; i < (int)num1; ++i) {
        uint8_t n = FileIO::ReadByte();
        int idx = 0;
        if (n > 0) {
            for (; idx < (int)n; ++idx)
                fileName[idx] = (char)FileIO::ReadByte();
            fileName[idx] = '\0';
            FileIO::GetFileInfo(fData);
            FileIO::CloseFile();
            numArray[i] = GraphicsSystem::AddGraphicsFile(fileName);
            FileIO::SetFileInfo(fData);
        }
    }

    uint8_t num3 = FileIO::ReadByte();
    animationFile[animationFileNo].numAnimations = (int)num3;
    animationFile[animationFileNo].aniListOffset = animationListNo;

    for (int i = 0; i < animationFile[animationFileNo].numAnimations; ++i) {
        uint8_t nameLen = FileIO::ReadByte();
        int idx;
        for (idx = 0; idx < (int)nameLen; ++idx)
            animationList[animationListNo].name[idx] = (char)FileIO::ReadByte();
        animationList[animationListNo].name[idx] = '\0';

        animationList[animationListNo].numFrames      = FileIO::ReadByte();
        animationList[animationListNo].animationSpeed = FileIO::ReadByte();
        animationList[animationListNo].loopPosition   = FileIO::ReadByte();
        animationList[animationListNo].rotationFlag   = FileIO::ReadByte();
        animationList[animationListNo].frameListOffset= animationFramesNo;

        for (int f = 0; f < (int)animationList[animationListNo].numFrames; ++f) {
            uint8_t surfIdx = FileIO::ReadByte();
            animationFrames[animationFramesNo].surfaceNum  = numArray[surfIdx];
            animationFrames[animationFramesNo].collisionBox= FileIO::ReadByte();
            animationFrames[animationFramesNo].left        = (int)FileIO::ReadByte();
            animationFrames[animationFramesNo].top         = (int)FileIO::ReadByte();
            animationFrames[animationFramesNo].xSize       = (int)FileIO::ReadByte();
            animationFrames[animationFramesNo].ySize       = (int)FileIO::ReadByte();
            animationFrames[animationFramesNo].xPivot      = (int)(int8_t)FileIO::ReadByte();
            animationFrames[animationFramesNo].yPivot      = (int)(int8_t)FileIO::ReadByte();
            ++animationFramesNo;
        }
        if (animationList[animationListNo].rotationFlag == 3)
            animationList[animationListNo].numFrames >>= 1;
        ++animationListNo;
    }

    uint8_t num16 = FileIO::ReadByte();
    animationFile[animationFileNo].cbListOffset = collisionBoxNo;
    for (int i = 0; i < (int)num16; ++i) {
        for (int j = 0; j < 8; ++j) {
            collisionBoxList[collisionBoxNo].left[j]   = (int8_t)FileIO::ReadByte();
            collisionBoxList[collisionBoxNo].top[j]    = (int8_t)FileIO::ReadByte();
            collisionBoxList[collisionBoxNo].right[j]  = (int8_t)FileIO::ReadByte();
            collisionBoxList[collisionBoxNo].bottom[j] = (int8_t)FileIO::ReadByte();
        }
        ++collisionBoxNo;
    }
    FileIO::CloseFile();
}

AnimationFileList* AnimationSystem::AddAnimationFile(const char* fileName)
{
    char strA[64]{};
    FileIO::StrCopy(strA, 64, "Data/Animations/", 64);
    FileIO::StrAdd(strA, 64, fileName, 64);

    for (int i = 0; i < 256; ++i) {
        if (FileIO::StringLength(animationFile[i].fileName) > 0) {
            if (FileIO::StringComp(animationFile[i].fileName, fileName))
                return &animationFile[i];
        } else {
            FileIO::StrCopy(animationFile[i].fileName, 32, fileName, 32);
            LoadAnimationFile(strA);
            ++animationFileNo;
            return &animationFile[i];
        }
    }
    return nullptr;
}

AnimationFileList* AnimationSystem::GetDefaultAnimationRef()
{
    return &animationFile[0];
}

void AnimationSystem::ClearAnimationData()
{
    for (int i = 0; i < 4096; ++i) {
        animationFrames[i].left         = 0;
        animationFrames[i].top          = 0;
        animationFrames[i].xSize        = 0;
        animationFrames[i].ySize        = 0;
        animationFrames[i].xPivot       = 0;
        animationFrames[i].yPivot       = 0;
        animationFrames[i].surfaceNum   = 0;
        animationFrames[i].collisionBox = 0;
    }
    for (int i = 0; i < 32; ++i)
        for (int j = 0; j < 8; ++j) {
            collisionBoxList[i].left[j]   = 0;
            collisionBoxList[i].top[j]    = 0;
            collisionBoxList[i].right[j]  = 0;
            collisionBoxList[i].bottom[j] = 0;
        }
    for (int i = 0; i < 256; ++i)
        FileIO::StrClear(animationFile[i].fileName, 32);

    animationFramesNo = 0;
    animationListNo   = 0;
    animationFileNo   = 0;
    collisionBoxNo    = 0;
    animationFile[0].numAnimations = 0;
    animationList[0].frameListOffset = animationFramesNo;
    animationFile[0].aniListOffset   = animationListNo;
    animationFile[0].cbListOffset    = collisionBoxNo;
}

void AnimationSystem::ProcessObjectAnimation(SpriteAnimation& animationRef,
                                              ObjectEntity&    currentObject)
{
    if (currentObject.animationSpeed > 0) {
        if (currentObject.animationSpeed > 240)
            currentObject.animationSpeed = 240;
        currentObject.animationTimer += currentObject.animationSpeed;
    } else {
        currentObject.animationTimer += (int32_t)animationRef.animationSpeed;
    }

    if (currentObject.animation != currentObject.prevAnimation) {
        currentObject.prevAnimation  = currentObject.animation;
        currentObject.frame          = 0;
        currentObject.animationTimer = 0;
        currentObject.animationSpeed = 0;
    }

    if (currentObject.animationTimer > 239) {
        currentObject.animationTimer -= 240;
        ++currentObject.frame;
    }

    if ((int32_t)currentObject.frame < (int32_t)animationRef.numFrames)
        return;
    currentObject.frame = animationRef.loopPosition;
}

void AnimationSystem::DrawObjectAnimation(SpriteAnimation& animationRef,
                                           ObjectEntity&    currentObject,
                                           int32_t xPos, int32_t yPos)
{
    switch (animationRef.rotationFlag) {
        case 0: {
            SpriteFrame& f = animationFrames[animationRef.frameListOffset + currentObject.frame];
            switch (currentObject.direction) {
                case 0:
                    GraphicsSystem::DrawSpriteFlipped(
                        xPos + f.xPivot, yPos + f.yPivot,
                        f.xSize, f.ySize, f.left, f.top,
                        (int32_t)currentObject.direction, (int32_t)f.surfaceNum);
                    break;
                case 1:
                    GraphicsSystem::DrawSpriteFlipped(
                        xPos - f.xSize - f.xPivot, yPos + f.yPivot,
                        f.xSize, f.ySize, f.left, f.top,
                        (int32_t)currentObject.direction, (int32_t)f.surfaceNum);
                    break;
                case 2:
                    GraphicsSystem::DrawSpriteFlipped(
                        xPos + f.xPivot, yPos - f.ySize - f.yPivot,
                        f.xSize, f.ySize, f.left, f.top,
                        (int32_t)currentObject.direction, (int32_t)f.surfaceNum);
                    break;
                case 3:
                    GraphicsSystem::DrawSpriteFlipped(
                        xPos - f.xSize - f.xPivot, yPos - f.ySize - f.yPivot,
                        f.xSize, f.ySize, f.left, f.top,
                        (int32_t)currentObject.direction, (int32_t)f.surfaceNum);
                    break;
            }
            break;
        }
        case 1: {
            SpriteFrame& f = animationFrames[animationRef.frameListOffset + currentObject.frame];
            GraphicsSystem::DrawRotatedSprite(
                currentObject.direction, xPos, yPos,
                -f.xPivot, -f.yPivot,
                f.left, f.top, f.xSize, f.ySize,
                currentObject.rotation, (int32_t)f.surfaceNum);
            break;
        }
        case 2: {
            SpriteFrame& f = animationFrames[animationRef.frameListOffset + currentObject.frame];
            int32_t rotAngle = (currentObject.rotation >= 256)
                ? 512 - (532 - currentObject.rotation >> 6 << 6)
                : ((currentObject.rotation + 20) >> 6 << 6);
            GraphicsSystem::DrawRotatedSprite(
                currentObject.direction, xPos, yPos,
                -f.xPivot, -f.yPivot,
                f.left, f.top, f.xSize, f.ySize,
                rotAngle, (int32_t)f.surfaceNum);
            break;
        }
        case 3: {
            int32_t rotAngle = (currentObject.rotation >= 256)
                ? 8 - ((532 - currentObject.rotation) >> 6)
                : ((currentObject.rotation + 20) >> 6);
            int32_t frame = (int32_t)currentObject.frame;
            switch (rotAngle) {
                case 0: case 8: rotAngle = 0; break;
                case 1:
                    frame += (int32_t)animationRef.numFrames;
                    rotAngle = (currentObject.direction != 0) ? 0 : 128;
                    break;
                case 2: rotAngle = 128; break;
                case 3:
                    frame += (int32_t)animationRef.numFrames;
                    rotAngle = (currentObject.direction != 0) ? 128 : 256;
                    break;
                case 4: rotAngle = 256; break;
                case 5:
                    frame += (int32_t)animationRef.numFrames;
                    rotAngle = (currentObject.direction != 0) ? 256 : 384;
                    break;
                case 6: rotAngle = 384; break;
                case 7:
                    frame += (int32_t)animationRef.numFrames;
                    rotAngle = (currentObject.direction != 0) ? 384 : 0;
                    break;
            }
            SpriteFrame& f = animationFrames[animationRef.frameListOffset + frame];
            GraphicsSystem::DrawRotatedSprite(
                currentObject.direction, xPos, yPos,
                -f.xPivot, -f.yPivot,
                f.left, f.top, f.xSize, f.ySize,
                rotAngle, (int32_t)f.surfaceNum);
            break;
        }
    }
}

}
