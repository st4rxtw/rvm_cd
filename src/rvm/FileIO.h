#pragma once
#include <cstdint>
#include <cstdio>
#include "FileData.h"
#include "StageList.h"

namespace rvm {

struct FileIO {
    static constexpr int32_t PRESENTATION_STAGE = 0;
    static constexpr int32_t ZONE_STAGE         = 1;
    static constexpr int32_t BONUS_STAGE        = 2;
    static constexpr int32_t SPECIAL_STAGE      = 3;

    static uint8_t  fileBuffer[8192];
    static uint32_t bufferPosition;
    static uint32_t fileSize;
    static uint32_t readSize;
    static uint32_t readPos;
    static bool     useRSDKFile;
    static bool     useByteCode;
    static uint32_t vFileSize;
    static uint32_t virtualFileOffset;
    static int32_t  saveRAM[8192];
    static char     encryptionStringA[22];
    static char     encryptionStringB[14];
    static uint8_t  eStringPosA;
    static uint8_t  eStringPosB;
    static uint8_t  eStringNo;
    static bool     eNybbleSwap;
    static char     currentStageFolder[8];
    static StageList pStageList[64];
    static StageList zStageList[128];
    static StageList bStageList[64];
    static StageList sStageList[64];
    static uint8_t  activeStageList;
    static uint8_t  noPresentationStages;
    static uint8_t  noZoneStages;
    static uint8_t  noBonusStages;
    static uint8_t  noSpecialStages;
    static int32_t  actNumber;

    static char     basePath[512];
    static void SetBasePath(const char* path);

    static void Init();

    static void StrCopy(char* strA, int strALen, const char* strB, int strBLen);
    static void StrClear(char* strA, int len);
    static void StrCopy2D(char strA[][32], const char* strB, int strBLen, int strPos);
    static void StrAdd(char* strA, int strALen, const char* strB, int strBLen);
    static bool StringComp(const char* strA, const char* strB);
    static int32_t StringLength(const char* strA);
    static int32_t FindStringToken(const char* strA, const char* token, char instance);
    static bool ConvertStringToInteger(const char* strA, int32_t& sValue);

    static bool CheckRSDKFile();
    static bool LoadFile(const char* filePath, FileData& fData);
    static void CloseFile();
    static bool CheckCurrentStageFolder(int32_t sNumber);
    static void ResetCurrentStageFolder();
    static bool LoadStageFile(const char* filePath, int32_t sNumber, FileData& fData);
    static bool LoadActFile(const char* filePath, int32_t sNumber, FileData& fData);
    static bool ParseVirtualFileSystem(const char* filePath);

    static uint8_t ReadByte();
    static void    ReadByteArray(uint8_t* byteP, int32_t numBytes);
    static void    ReadCharArray(char* charP, int32_t numBytes);
    static void    FillFileBuffer();

    static void     GetFileInfo(FileData& fData);
    static void     SetFileInfo(FileData& fData);
    static uint32_t GetFilePosition();
    static void     SetFilePosition(uint32_t newFilePos);
    static bool     ReachedEndOfFile();

    static uint8_t ReadSaveRAMData();
    static uint8_t WriteSaveRAMData();

private:
    static FILE* fileReader;
};

}
