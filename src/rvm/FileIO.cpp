#include "FileIO.h"
#include "Log.h"
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <cerrno>

namespace rvm {

char     FileIO::basePath[512]         = "";

static char s_rsdkPath[512] = "";
static char s_savePath[512] = "";

void FileIO::SetBasePath(const char* path)
{
    std::snprintf(basePath, sizeof(basePath), "%s", path);
    std::snprintf(s_rsdkPath, sizeof(s_rsdkPath), "%sData.rsdk", path);
    std::snprintf(s_savePath, sizeof(s_savePath), "%sSGame.bin", path);
    Log::Info("Base path: %s", basePath);
    Log::Info("RSDK path: %s", s_rsdkPath);
}

uint8_t  FileIO::fileBuffer[8192]{};
uint32_t FileIO::bufferPosition = 0;
uint32_t FileIO::fileSize       = 0;
uint32_t FileIO::readSize       = 0;
uint32_t FileIO::readPos        = 0;
bool     FileIO::useRSDKFile    = false;
bool     FileIO::useByteCode    = false;
uint32_t FileIO::vFileSize      = 0;
uint32_t FileIO::virtualFileOffset = 0;
int32_t  FileIO::saveRAM[8192]{};
char     FileIO::encryptionStringA[22] = "4RaS9D7KaEbxcp2o5r6t";
char     FileIO::encryptionStringB[14] = "3tRaUxLmEaSn";
uint8_t  FileIO::eStringPosA   = 0;
uint8_t  FileIO::eStringPosB   = 0;
uint8_t  FileIO::eStringNo     = 0;
bool     FileIO::eNybbleSwap   = false;
char     FileIO::currentStageFolder[8]{};
StageList FileIO::pStageList[64]{};
StageList FileIO::zStageList[128]{};
StageList FileIO::bStageList[64]{};
StageList FileIO::sStageList[64]{};
uint8_t  FileIO::activeStageList       = 0;
uint8_t  FileIO::noPresentationStages  = 0;
uint8_t  FileIO::noZoneStages          = 0;
uint8_t  FileIO::noBonusStages         = 0;
uint8_t  FileIO::noSpecialStages       = 0;
int32_t  FileIO::actNumber             = 0;
FILE*    FileIO::fileReader            = nullptr;

void FileIO::Init()
{
    std::memset(pStageList, 0, sizeof(pStageList));
    std::memset(zStageList, 0, sizeof(zStageList));
    std::memset(bStageList, 0, sizeof(bStageList));
    std::memset(sStageList, 0, sizeof(sStageList));
}

void FileIO::StrCopy(char* strA, int strALen, const char* strB, int strBLen)
{
    int i = 0;
    while (i < strBLen && i < strALen) {
        strA[i] = strB[i];
        if (strB[i] == '\0') break;
        ++i;
    }

    while (i < strALen)
        strA[i++] = '\0';
}

void FileIO::StrClear(char* strA, int len)
{
    for (int i = 0; i < len; ++i)
        strA[i] = '\0';
}

void FileIO::StrCopy2D(char strA[][32], const char* strB, int strBLen, int strPos)
{
    int i = 0;
    while (i < strBLen && i < 32) {
        strA[strPos][i] = strB[i];
        if (strB[i] == '\0') break;
        ++i;
    }
    while (i < 32)
        strA[strPos][i++] = '\0';
}

void FileIO::StrAdd(char* strA, int strALen, const char* strB, int strBLen)
{
    int i = 0;
    while (i < strALen && strA[i] != '\0') ++i;
    int j = 0;
    while (j < strBLen && i < strALen) {
        strA[i] = strB[j];
        if (strB[j] == '\0') break;
        ++i; ++j;
    }
    while (i < strALen)
        strA[i++] = '\0';
}

bool FileIO::StringComp(const char* strA, const char* strB)
{
    int i = 0, j = 0;
    while (true) {
        char a = strA[i], b = strB[j];
        if (a != b && a != b + 32 && a != b - 32)
            return false;
        if (a == '\0')
            return true;
        ++i; ++j;
    }
}

int32_t FileIO::StringLength(const char* strA)
{
    int i = 0;
    while (strA[i] != '\0') ++i;
    return i;
}

int32_t FileIO::FindStringToken(const char* strA, const char* token, char instance)
{
    int result = -1;
    int count  = 0;
    for (int pos = 0; strA[pos] != '\0'; ++pos) {
        bool match = true;
        for (int k = 0; token[k] != '\0'; ++k) {
            if (strA[pos + k] == '\0' || strA[pos + k] != token[k]) {
                match = false; break;
            }
        }
        if (match) {
            ++count;
            if (count == (int)instance)
                return pos;
        }
    }
    return result;
}

bool FileIO::ConvertStringToInteger(const char* strA, int32_t& sValue)
{
    int i = 0;
    bool neg = false;
    sValue = 0;
    if ((strA[i] < '0' || strA[i] > '9') && strA[i] != '-' && strA[i] != '+')
        return false;
    int len = StringLength(strA) - 1;
    if (strA[i] == '-') { neg = true; ++i; --len; }
    else if (strA[i] == '+') { ++i; --len; }
    while (len > -1) {
        if (strA[i] < '0' || strA[i] > '9') return false;
        if (len > 0) {
            int n = strA[i] - '0';
            for (int p = len; p > 0; --p) n *= 10;
            sValue += n;
        } else {
            sValue += strA[i] - '0';
        }
        --len; ++i;
    }
    if (neg) sValue = -sValue;
    return true;
}

bool FileIO::CheckRSDKFile()
{
    FileData fData{};
    fileReader = fopen(s_rsdkPath, "rb");
    if (!fileReader) { Log::Error("Cannot open Data.rsdk at: %s", s_rsdkPath); useRSDKFile = false; useByteCode = false; return false; }
    fseek(fileReader, 0, SEEK_END);
    long len = ftell(fileReader);
    fclose(fileReader);
    fileReader = nullptr;
    if (len > 0) {
        useRSDKFile = true;
        useByteCode = false;
        if (!LoadFile("Data/Scripts/ByteCode/GlobalCode.bin", fData))
            return false;
        useByteCode = true;
        CloseFile();
        return true;
    }
    useRSDKFile = false;
    useByteCode = false;
    return false;
}

bool FileIO::LoadFile(const char* filePath, FileData& fData)
{
    int i = 0;
    while (filePath[i] != '\0' && i < 63) {
        fData.fileName[i] = filePath[i];
        ++i;
    }
    fData.fileName[i] = '\0';

    if (fileReader) { fclose(fileReader); fileReader = nullptr; }
    fileReader = fopen(s_rsdkPath, "rb");
    if (!fileReader) { Log::Error("LoadFile: cannot open Data.rsdk"); return false; }
    fseek(fileReader, 0, SEEK_END);
    fileSize = (uint32_t)ftell(fileReader);
    fseek(fileReader, 0, SEEK_SET);
    bufferPosition = 0;
    readSize       = 0;
    readPos        = 0;

    if (!ParseVirtualFileSystem(fData.fileName)) {
        fclose(fileReader); fileReader = nullptr;
        return false;
    }
    fData.fileSize          = vFileSize;
    fData.virtualFileOffset = virtualFileOffset;
    bufferPosition = 0;
    readSize       = 0;
    return true;
}

void FileIO::CloseFile()
{
    if (fileReader) { fclose(fileReader); fileReader = nullptr; }
}

bool FileIO::CheckCurrentStageFolder(int32_t sNumber)
{
    const char* folder = nullptr;
    switch (activeStageList) {
        case 0: folder = pStageList[sNumber].stageFolderName; break;
        case 1: folder = zStageList[sNumber].stageFolderName; break;
        case 2: folder = bStageList[sNumber].stageFolderName; break;
        case 3: folder = sStageList[sNumber].stageFolderName; break;
        default: return false;
    }
    if (StringComp(currentStageFolder, folder))
        return true;
    StrCopy(currentStageFolder, 8, folder, 8);
    return false;
}

void FileIO::ResetCurrentStageFolder()
{
    currentStageFolder[0] = '\0';
}

bool FileIO::LoadStageFile(const char* filePath, int32_t sNumber, FileData& fData)
{
    char strA[64]{};
    StrCopy(strA, 64, "Data/Stages/", 64);
    switch (activeStageList) {
        case 0: StrAdd(strA, 64, pStageList[sNumber].stageFolderName, 8); break;
        case 1: StrAdd(strA, 64, zStageList[sNumber].stageFolderName, 8); break;
        case 2: StrAdd(strA, 64, bStageList[sNumber].stageFolderName, 8); break;
        case 3: StrAdd(strA, 64, sStageList[sNumber].stageFolderName, 8); break;
    }
    StrAdd(strA, 64, "/", 2);
    StrAdd(strA, 64, filePath, 64);
    return LoadFile(strA, fData);
}

bool FileIO::LoadActFile(const char* filePath, int32_t sNumber, FileData& fData)
{
    char strA[64]{};
    StrCopy(strA, 64, "Data/Stages/", 64);
    switch (activeStageList) {
        case 0:
            StrAdd(strA, 64, pStageList[sNumber].stageFolderName, 8);
            break;
        case 1:
            StrAdd(strA, 64, zStageList[sNumber].stageFolderName, 8);
            break;
        case 2:
            StrAdd(strA, 64, bStageList[sNumber].stageFolderName, 8);
            break;
        case 3:
            StrAdd(strA, 64, sStageList[sNumber].stageFolderName, 8);
            break;
    }
    StrAdd(strA, 64, "/Act", 5);
    switch (activeStageList) {
        case 0:
            StrAdd(strA, 64, pStageList[sNumber].actNumber, 4);
            ConvertStringToInteger(pStageList[sNumber].actNumber, actNumber);
            break;
        case 1:
            StrAdd(strA, 64, zStageList[sNumber].actNumber, 4);
            ConvertStringToInteger(zStageList[sNumber].actNumber, actNumber);
            break;
        case 2:
            StrAdd(strA, 64, bStageList[sNumber].actNumber, 4);
            ConvertStringToInteger(bStageList[sNumber].actNumber, actNumber);
            break;
        case 3:
            StrAdd(strA, 64, sStageList[sNumber].actNumber, 4);
            ConvertStringToInteger(sStageList[sNumber].actNumber, actNumber);
            break;
    }
    StrAdd(strA, 64, filePath, 64);
    return LoadFile(strA, fData);
}

bool FileIO::ParseVirtualFileSystem(const char* filePath)
{
    char strA1[64]{}, strA2[64]{}, strB[64]{};
    int  lastSlash = 0, afterSlash = 0;

    for (int i = 0; filePath[i] != '\0'; ++i) {
        if (filePath[i] == '/') { lastSlash = i; afterSlash = 0; }
        else ++afterSlash;
        strA1[i] = filePath[i];
    }

    int idx3 = lastSlash + 1;
    for (int i = 0; i < afterSlash; ++i)
        strA2[i] = filePath[idx3 + i];
    strA2[afterSlash] = '\0';
    strA1[idx3] = '\0';

    fseek(fileReader, 0, SEEK_SET);
    useRSDKFile    = false;
    bufferPosition = 0;
    readSize       = 0;
    readPos        = 0;
    virtualFileOffset = 0;

    int32_t num2 = (int32_t)ReadByte() | ((int32_t)ReadByte() << 8) |
                   ((int32_t)ReadByte() << 16) | ((int32_t)ReadByte() << 24);
    uint16_t num3 = (uint16_t)ReadByte() | ((uint16_t)ReadByte() << 8);

    int32_t num4 = 0, num5 = 0;
    while (num4 < (int32_t)num3) {
        uint8_t num6 = ReadByte();
        int idx5;
        for (idx5 = 0; idx5 < (int32_t)num6; ++idx5)
            strB[idx5] = (char)((uint32_t)ReadByte() ^ (uint32_t)255 - (uint32_t)num6);
        strB[idx5] = '\0';
        if (StringComp(strA1, strB)) {
            num4 = (int32_t)num3;
            num5 = (int32_t)ReadByte() | ((int32_t)ReadByte() << 8) |
                   ((int32_t)ReadByte() << 16) | ((int32_t)ReadByte() << 24);
        } else {
            num5 = -1;
            ReadByte(); ReadByte(); ReadByte(); ReadByte();
            ++num4;
        }
    }

    if (num5 == -1) { useRSDKFile = true; return false; }

    fseek(fileReader, (long)(num2 + num5), SEEK_SET);
    bufferPosition = 0; readSize = 0; readPos = 0;
    virtualFileOffset = (uint32_t)(num2 + num5);

    while (true) {
        uint8_t num9 = ReadByte();
        ++virtualFileOffset;
        int idx6 = 0;
        while (idx6 < (int32_t)num9) {
            strB[idx6] = (char)((uint32_t)ReadByte() ^ (uint32_t)255);
            ++idx6;
            ++virtualFileOffset;
        }
        strB[idx6] = '\0';
        if (StringComp(strA2, strB)) {
            int32_t num10 = (int32_t)ReadByte() | ((int32_t)ReadByte() << 8) |
                            ((int32_t)ReadByte() << 16) | ((int32_t)ReadByte() << 24);
            virtualFileOffset += 4;
            vFileSize = (uint32_t)num10;
            fseek(fileReader, (long)virtualFileOffset, SEEK_SET);
            bufferPosition = 0; readSize = 0;
            readPos = virtualFileOffset;
            break;
        } else {
            int32_t num11 = (int32_t)ReadByte() | ((int32_t)ReadByte() << 8) |
                            ((int32_t)ReadByte() << 16) | ((int32_t)ReadByte() << 24);
            virtualFileOffset += 4;
            virtualFileOffset += (uint32_t)num11;
            fseek(fileReader, (long)virtualFileOffset, SEEK_SET);
            bufferPosition = 0; readSize = 0;
            readPos = virtualFileOffset;
        }
    }

    eStringNo      = (uint8_t)((vFileSize & 508u) >> 2);
    eStringPosB    = (uint8_t)(1 + (int32_t)eStringNo % 9);
    eStringPosA    = (uint8_t)(1 + (int32_t)eStringNo % (int32_t)eStringPosB);
    eNybbleSwap    = false;
    useRSDKFile    = true;
    return true;
}

uint8_t FileIO::ReadByte()
{
    uint8_t out = 0;
    if (readPos > fileSize) return out;
    if (!useRSDKFile) {
        if (bufferPosition == readSize) FillFileBuffer();
        out = fileBuffer[bufferPosition++];
    } else {
        if (bufferPosition == readSize) FillFileBuffer();
        uint8_t b = (uint8_t)(fileBuffer[bufferPosition] ^ eStringNo ^
                              (uint8_t)encryptionStringB[eStringPosB]);
        if (eNybbleSwap) b = (uint8_t)((b >> 4) | ((b & 15) << 4));
        out = (uint8_t)(b ^ (uint8_t)encryptionStringA[eStringPosA]);
        ++eStringPosA; ++eStringPosB;
        if (eStringPosA > 19 && eStringPosB > 11) {
            ++eStringNo; eStringNo &= 127;
            if (!eNybbleSwap) {
                eNybbleSwap = true;
                eStringPosA = (uint8_t)(3 + (int32_t)eStringNo % 15);
                eStringPosB = (uint8_t)(1 + (int32_t)eStringNo % 7);
            } else {
                eNybbleSwap = false;
                eStringPosA = (uint8_t)(6 + (int32_t)eStringNo % 12);
                eStringPosB = (uint8_t)(4 + (int32_t)eStringNo % 5);
            }
        } else {
            if (eStringPosA > 19) { eStringPosA = 1; eNybbleSwap = !eNybbleSwap; }
            if (eStringPosB > 11) { eStringPosB = 1; eNybbleSwap = !eNybbleSwap; }
        }
        ++bufferPosition;
    }
    return out;
}

void FileIO::ReadByteArray(uint8_t* byteP, int32_t numBytes)
{
    if (readPos > fileSize) return;
    int idx = 0;
    if (!useRSDKFile) {
        for (; numBytes > 0; --numBytes) {
            if (bufferPosition == readSize) FillFileBuffer();
            byteP[idx++] = fileBuffer[bufferPosition++];
        }
    } else {
        for (; numBytes > 0; --numBytes) {
            if (bufferPosition >= readSize) FillFileBuffer();
            byteP[idx] = (uint8_t)(fileBuffer[bufferPosition] ^ eStringNo ^
                                   (uint8_t)encryptionStringB[eStringPosB]);
            if (eNybbleSwap)
                byteP[idx] = (uint8_t)((byteP[idx] >> 4) | ((byteP[idx] & 15) << 4));
            byteP[idx] ^= (uint8_t)encryptionStringA[eStringPosA];
            ++eStringPosA; ++eStringPosB;
            if (eStringPosA > 19 && eStringPosB > 11) {
                ++eStringNo; eStringNo &= 127;
                if (!eNybbleSwap) {
                    eNybbleSwap = true;
                    eStringPosA = (uint8_t)(3 + eStringNo % 15);
                    eStringPosB = (uint8_t)(1 + eStringNo % 7);
                } else {
                    eNybbleSwap = false;
                    eStringPosA = (uint8_t)(6 + eStringNo % 12);
                    eStringPosB = (uint8_t)(4 + eStringNo % 5);
                }
            } else {
                if (eStringPosA > 19) { eStringPosA = 1; eNybbleSwap = !eNybbleSwap; }
                if (eStringPosB > 11) { eStringPosB = 1; eNybbleSwap = !eNybbleSwap; }
            }
            ++bufferPosition;
            ++idx;
        }
    }
}

void FileIO::ReadCharArray(char* charP, int32_t numBytes)
{
    if (readPos > fileSize) return;
    int idx = 0;
    if (!useRSDKFile) {
        for (; numBytes > 0; --numBytes) {
            if (bufferPosition == readSize) FillFileBuffer();
            charP[idx++] = (char)fileBuffer[bufferPosition++];
        }
    } else {
        for (; numBytes > 0; --numBytes) {
            if (bufferPosition == readSize) FillFileBuffer();
            uint8_t b = (uint8_t)(fileBuffer[bufferPosition] ^ eStringNo ^
                                  (uint8_t)encryptionStringB[eStringPosB]);
            if (eNybbleSwap) b = (uint8_t)((b >> 4) | ((b & 15) << 4));
            charP[idx++] = (char)(b ^ (uint8_t)encryptionStringA[eStringPosA]);
            ++eStringPosA; ++eStringPosB;
            if (eStringPosA > 19 && eStringPosB > 11) {
                ++eStringNo; eStringNo &= 127;
                if (!eNybbleSwap) {
                    eNybbleSwap = true;
                    eStringPosA = (uint8_t)(3 + eStringNo % 15);
                    eStringPosB = (uint8_t)(1 + eStringNo % 7);
                } else {
                    eNybbleSwap = false;
                    eStringPosA = (uint8_t)(6 + eStringNo % 12);
                    eStringPosB = (uint8_t)(4 + eStringNo % 5);
                }
            } else {
                if (eStringPosA > 19) { eStringPosA = 1; eNybbleSwap = !eNybbleSwap; }
                if (eStringPosB > 11) { eStringPosB = 1; eNybbleSwap = !eNybbleSwap; }
            }
            ++bufferPosition;
        }
    }
}

void FileIO::FillFileBuffer()
{
    readSize = (readPos + 8192 <= fileSize) ? 8192 : fileSize - readPos;
    if (fileReader && readSize > 0)
        (void)fread(fileBuffer, 1, readSize, fileReader);
    readPos       += readSize;
    bufferPosition = 0;
}

void FileIO::GetFileInfo(FileData& fData)
{
    fData.bufferPos      = bufferPosition;
    fData.position       = readPos - readSize;
    fData.eStringPosA    = eStringPosA;
    fData.eStringPosB    = eStringPosB;
    fData.eStringNo      = eStringNo;
    fData.eNybbleSwap    = eNybbleSwap;
}

void FileIO::SetFileInfo(FileData& fData)
{
    if (!useRSDKFile) return;
    if (fileReader) { fclose(fileReader); fileReader = nullptr; }
    fileReader = fopen(s_rsdkPath, "rb");
    if (!fileReader) return;
    virtualFileOffset = fData.virtualFileOffset;
    vFileSize         = fData.fileSize;
    fseek(fileReader, 0, SEEK_END);
    fileSize  = (uint32_t)ftell(fileReader);
    readPos   = fData.position;
    fseek(fileReader, (long)readPos, SEEK_SET);
    FillFileBuffer();
    bufferPosition = fData.bufferPos;
    eStringPosA    = fData.eStringPosA;
    eStringPosB    = fData.eStringPosB;
    eStringNo      = fData.eStringNo;
    eNybbleSwap    = fData.eNybbleSwap;
}

uint32_t FileIO::GetFilePosition()
{
    if (useRSDKFile)
        return readPos - readSize + bufferPosition - virtualFileOffset;
    return readPos - readSize + bufferPosition;
}

void FileIO::SetFilePosition(uint32_t newFilePos)
{
    if (useRSDKFile) {
        readPos    = newFilePos + virtualFileOffset;
        eStringNo  = (uint8_t)((vFileSize & 508u) >> 2);
        eStringPosB= (uint8_t)(1 + (int32_t)eStringNo % 9);
        eStringPosA= (uint8_t)(1 + (int32_t)eStringNo % (int32_t)eStringPosB);
        eNybbleSwap= false;
        for (uint32_t p = newFilePos; p > 0; --p) {
            ++eStringPosA; ++eStringPosB;
            if (eStringPosA > 19 && eStringPosB > 11) {
                ++eStringNo; eStringNo &= 127;
                if (!eNybbleSwap) {
                    eNybbleSwap = true;
                    eStringPosA = (uint8_t)(3 + eStringNo % 15);
                    eStringPosB = (uint8_t)(1 + eStringNo % 7);
                } else {
                    eNybbleSwap = false;
                    eStringPosA = (uint8_t)(6 + eStringNo % 12);
                    eStringPosB = (uint8_t)(4 + eStringNo % 5);
                }
            } else {
                if (eStringPosA > 19) { eStringPosA = 1; eNybbleSwap = !eNybbleSwap; }
                if (eStringPosB > 11) { eStringPosB = 1; eNybbleSwap = !eNybbleSwap; }
            }
        }
    } else {
        readPos = newFilePos;
    }
    if (fileReader) fseek(fileReader, (long)readPos, SEEK_SET);
    FillFileBuffer();
}

bool FileIO::ReachedEndOfFile()
{
    if (useRSDKFile)
        return (readPos - readSize + bufferPosition - virtualFileOffset) >= vFileSize;
    return (readPos - readSize + bufferPosition) >= fileSize;
}

uint8_t FileIO::ReadSaveRAMData()
{
    FILE* f = fopen(s_savePath, "rb");
    if (f) {
        for (int i = 0; i < 8192; ++i) {
            if (fread(&saveRAM[i], 4, 1, f) != 1) break;
        }
        fclose(f);
    }
    return 1;
}

uint8_t FileIO::WriteSaveRAMData()
{
    FILE* f = fopen(s_savePath, "wb");
    if (f) {
        for (int i = 0; i < 8192; ++i)
            fwrite(&saveRAM[i], 4, 1, f);
        fclose(f);
    }
    return 1;
}

}
