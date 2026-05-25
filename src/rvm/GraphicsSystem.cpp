#include "GraphicsSystem.h"
#include "GlobalAppDefinitions.h"
#include "FileIO.h"
#include "GifLoader.h"
#include "Log.h"

namespace rvm {

bool         GraphicsSystem::render3DEnabled        = false;
uint8_t      GraphicsSystem::fadeMode               = 0;
uint8_t      GraphicsSystem::fadeR                  = 0;
uint8_t      GraphicsSystem::fadeG                  = 0;
uint8_t      GraphicsSystem::fadeB                  = 0;
uint8_t      GraphicsSystem::fadeA                  = 0;
uint8_t      GraphicsSystem::paletteMode            = 0;
uint8_t      GraphicsSystem::colourMode             = 0;
uint16_t     GraphicsSystem::texBuffer[1048576]     {};
uint8_t      GraphicsSystem::texBufferMode          = 0;
uint8_t      GraphicsSystem::tileGfx[262144]        {};
uint8_t      GraphicsSystem::graphicData[2097152]   {};
GfxSurfaceDesc GraphicsSystem::gfxSurface[24]       {};
uint32_t     GraphicsSystem::gfxDataPosition        = 0;
DrawVertex   GraphicsSystem::gfxPolyList[8192]      {};
DrawVertex3D GraphicsSystem::polyList3D[6404]       {};
int16_t      GraphicsSystem::gfxPolyListIndex[49152]{};
uint16_t     GraphicsSystem::gfxVertexSize          = 0;
uint16_t     GraphicsSystem::gfxVertexSizeOpaque    = 0;
uint16_t     GraphicsSystem::gfxIndexSize           = 0;
uint16_t     GraphicsSystem::gfxIndexSizeOpaque     = 0;
uint16_t     GraphicsSystem::vertexSize3D           = 0;
uint16_t     GraphicsSystem::indexSize3D            = 0;
float        GraphicsSystem::tileUVArray[4096]      {};
float        GraphicsSystem::floor3DPosX            = 0.0f;
float        GraphicsSystem::floor3DPosY            = 0.0f;
float        GraphicsSystem::floor3DPosZ            = 0.0f;
float        GraphicsSystem::floor3DAngle           = 0.0f;
uint16_t     GraphicsSystem::blendLookupTable[8192]       {};
uint16_t     GraphicsSystem::subtractiveLookupTable[8192] {};
PaletteEntry GraphicsSystem::tilePalette[256]       {};
uint16_t     GraphicsSystem::tilePalette16_Data[8][256]   {};
int32_t      GraphicsSystem::texPaletteNum          = 0;
int32_t      GraphicsSystem::waterDrawPos           = 320;
bool         GraphicsSystem::videoPlaying           = false;
int32_t      GraphicsSystem::currentVideoFrame      = 0;

static constexpr float TEX_INV = 0.0009765625f;

static inline void SetVert(DrawVertex& v,
                           float px, float py,
                           float tu, float tv,
                           uint8_t r=255, uint8_t g=255, uint8_t b=255, uint8_t a=255)
{
    v.x = px; v.y = py; v.u = tu; v.v = tv;
    v.color.r = r; v.color.g = g; v.color.b = b; v.color.a = a;
}

void GraphicsSystem::SetScreenRenderSize(int32_t gfxWidth, int32_t /*gfxPitch*/)
{
    GlobalAppDefinitions::SCREEN_XSIZE        = gfxWidth;
    GlobalAppDefinitions::SCREEN_CENTER       = gfxWidth / 2;
    GlobalAppDefinitions::SCREEN_SCROLL_LEFT  = GlobalAppDefinitions::SCREEN_CENTER - 8;
    GlobalAppDefinitions::SCREEN_SCROLL_RIGHT = GlobalAppDefinitions::SCREEN_CENTER + 8;
    GlobalAppDefinitions::OBJECT_BORDER_X1    = 128;
    GlobalAppDefinitions::OBJECT_BORDER_X2    = gfxWidth + 128;
}

uint16_t GraphicsSystem::RGB_16BIT5551(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
    return (uint16_t)(((uint32_t)(r >> 3) << 11) |
                      ((uint32_t)(g >> 3) << 6)  |
                      ((uint32_t)(b >> 3) << 1)  |
                      (a & 1u));
}

void GraphicsSystem::LoadPalette(const char* fileName, int32_t paletteNum,
                                  int32_t destPoint, int32_t startPoint, int32_t endPoint)
{
    char strA[64]{};
    FileIO::StrCopy(strA, 64, "Data/Palettes/", 64);
    FileIO::StrAdd(strA, 64, fileName, 64);
    FileData fData{};
    uint8_t byteP[3]{};
    if (!FileIO::LoadFile(strA, fData)) return;
    FileIO::SetFilePosition((uint32_t)(startPoint * 3));
    if (paletteNum < 0 || paletteNum > 7) paletteNum = 0;
    if (paletteNum == 0) {
        for (int i = startPoint; i < endPoint; ++i) {
            FileIO::ReadByteArray(byteP, 3);
            tilePalette16_Data[0][destPoint] = RGB_16BIT5551(byteP[0], byteP[1], byteP[2], 1);
            tilePalette[destPoint].red   = byteP[0];
            tilePalette[destPoint].green = byteP[1];
            tilePalette[destPoint].blue  = byteP[2];
            ++destPoint;
        }
        tilePalette16_Data[0][0] = RGB_16BIT5551(byteP[0], byteP[1], byteP[2], 0);
    } else {
        for (int i = startPoint; i < endPoint; ++i) {
            FileIO::ReadByteArray(byteP, 3);
            tilePalette16_Data[paletteNum][destPoint] = RGB_16BIT5551(byteP[0], byteP[1], byteP[2], 1);
            ++destPoint;
        }
        tilePalette16_Data[paletteNum][0] = RGB_16BIT5551(byteP[0], byteP[1], byteP[2], 0);
    }
    FileIO::CloseFile();
}

uint8_t GraphicsSystem::AddGraphicsFile(const char* fileName)
{
    char fileName1[64]{};
    FileIO::StrCopy(fileName1, 64, "Data/Sprites/", 64);
    FileIO::StrAdd(fileName1, 64, fileName, 64);
    for (uint8_t s = 0; s < 24; ++s) {
        if (FileIO::StringLength(gfxSurface[s].fileName) > 0) {
            if (FileIO::StringComp(gfxSurface[s].fileName, fileName1))
                return s;
        } else {
            int idx = FileIO::StringLength(fileName1) - 1;
            switch (fileName1[idx]) {
                case 'f': LoadGIFFile(fileName1, s); break;
                case 'p': LoadBMPFile(fileName1, s); break;
            }
            return s;
        }
    }
    return 0;
}

void GraphicsSystem::RemoveGraphicsFile(const char* fileName, int32_t surfaceNum)
{
    if (surfaceNum < 0) {
        for (uint32_t i = 0; i < 24; ++i) {
            if (FileIO::StringLength(gfxSurface[i].fileName) > 0 &&
                FileIO::StringComp(gfxSurface[i].fileName, fileName))
                surfaceNum = (int32_t)i;
        }
    }
    if (surfaceNum < 0 || FileIO::StringLength(gfxSurface[surfaceNum].fileName) == 0)
        return;
    FileIO::StrClear(gfxSurface[surfaceNum].fileName, 64);
    uint32_t dataStart = gfxSurface[surfaceNum].dataStart;
    uint32_t endIdx    = dataStart + (uint32_t)(gfxSurface[surfaceNum].width * gfxSurface[surfaceNum].height);
    for (uint32_t n = 2097152 - endIdx; n > 0; --n) {
        graphicData[dataStart] = graphicData[endIdx];
        ++dataStart; ++endIdx;
    }
    gfxDataPosition -= (uint32_t)(gfxSurface[surfaceNum].width * gfxSurface[surfaceNum].height);
    for (uint32_t i = 0; i < 24; ++i) {
        if (gfxSurface[i].dataStart > gfxSurface[surfaceNum].dataStart)
            gfxSurface[i].dataStart -= (uint32_t)(gfxSurface[surfaceNum].width * gfxSurface[surfaceNum].height);
    }
}

void GraphicsSystem::ClearGraphicsData()
{
    for (int i = 0; i < 24; ++i)
        FileIO::StrClear(gfxSurface[i].fileName, 64);
    gfxDataPosition = 0;
}

bool GraphicsSystem::CheckSurfaceSize(int32_t size)
{
    for (int i = 2; i < 2048; i <<= 1)
        if (i == size) return true;
    return false;
}

void GraphicsSystem::SetupPolygonLists()
{
    int idx1 = 0;
    for (int i = 0; i < 8192; ++i) {
        gfxPolyListIndex[idx1++] = (int16_t)(i << 2);
        gfxPolyListIndex[idx1++] = (int16_t)((i << 2) + 1);
        gfxPolyListIndex[idx1++] = (int16_t)((i << 2) + 2);
        gfxPolyListIndex[idx1++] = (int16_t)((i << 2) + 1);
        gfxPolyListIndex[idx1++] = (int16_t)((i << 2) + 3);
        gfxPolyListIndex[idx1++] = (int16_t)((i << 2) + 2);
    }
    for (int i = 0; i < 8192; ++i) {
        gfxPolyList[i].color.r = 255; gfxPolyList[i].color.g = 255;
        gfxPolyList[i].color.b = 255; gfxPolyList[i].color.a = 255;
    }
    for (int i = 0; i < 6404; ++i) {
        polyList3D[i].color.r = 255; polyList3D[i].color.g = 255;
        polyList3D[i].color.b = 255; polyList3D[i].color.a = 255;
    }
}

void GraphicsSystem::UpdateTextureBufferWithTiles()
{
    int num1 = 0;
    if (texBufferMode == 0) {
        for (int y = 0; y < 512; y += 16) {
            for (int x = 0; x < 512; x += 16) {
                int src  = num1 << 8;
                ++num1;
                int dst  = x + (y << 10);
                for (int row = 0; row < 16; ++row) {
                    for (int col = 0; col < 16; ++col) {
                        texBuffer[dst] = (tileGfx[src] <= 0) ? 0
                            : tilePalette16_Data[texPaletteNum][(int)tileGfx[src]];
                        ++dst; ++src;
                    }
                    dst += 1008;
                }
            }
        }
    } else {
        for (int y = 0; y < 504; y += 18) {
            for (int x = 0; x < 504; x += 18) {
                int src = num1 << 8;
                ++num1;
                if (num1 == 783) num1 = 1023;
                int dst = x + (y << 10);
                texBuffer[dst] = (tileGfx[src] <= 0) ? 0
                    : tilePalette16_Data[texPaletteNum][(int)tileGfx[src]];
                int d2 = dst + 1;
                for (int c = 0; c < 15; ++c) {
                    texBuffer[d2] = (tileGfx[src] <= 0) ? 0
                        : tilePalette16_Data[texPaletteNum][(int)tileGfx[src]];
                    ++d2; ++src;
                }
                if (tileGfx[src] > 0) {
                    texBuffer[d2] = tilePalette16_Data[texPaletteNum][(int)tileGfx[src]];
                    ++d2;
                    texBuffer[d2] = tilePalette16_Data[texPaletteNum][(int)tileGfx[src]];
                } else {
                    texBuffer[d2] = 0; ++d2; texBuffer[d2] = 0;
                }
                ++d2;
                int src2 = src - 15;
                int d3 = d2 + 1006;
                for (int row = 0; row < 16; ++row) {
                    texBuffer[d3] = (tileGfx[src2] <= 0) ? 0
                        : tilePalette16_Data[texPaletteNum][(int)tileGfx[src2]];
                    int d4 = d3 + 1;
                    for (int c = 0; c < 15; ++c) {
                        texBuffer[d4] = (tileGfx[src2] <= 0) ? 0
                            : tilePalette16_Data[texPaletteNum][(int)tileGfx[src2]];
                        ++d4; ++src2;
                    }
                    if (tileGfx[src2] > 0) {
                        texBuffer[d4] = tilePalette16_Data[texPaletteNum][(int)tileGfx[src2]];
                        ++d4;
                        texBuffer[d4] = tilePalette16_Data[texPaletteNum][(int)tileGfx[src2]];
                    } else {
                        texBuffer[d4] = 0; ++d4; texBuffer[d4] = 0;
                    }
                    ++d4; ++src2;
                    d3 = d4 + 1006;
                }
                int src3 = src2 - 16;
                texBuffer[d3] = (tileGfx[src3] <= 0) ? 0
                    : tilePalette16_Data[texPaletteNum][(int)tileGfx[src3]];
                int d5 = d3 + 1;
                for (int c = 0; c < 15; ++c) {
                    texBuffer[d5] = (tileGfx[src3] <= 0) ? 0
                        : tilePalette16_Data[texPaletteNum][(int)tileGfx[src3]];
                    ++d5; ++src3;
                }
                if (tileGfx[src3] > 0) {
                    texBuffer[d5] = tilePalette16_Data[texPaletteNum][(int)tileGfx[src3]];
                    ++d5;
                    texBuffer[d5] = tilePalette16_Data[texPaletteNum][(int)tileGfx[src3]];
                } else {
                    texBuffer[d5] = 0; ++d5; texBuffer[d5] = 0;
                }

            }
        }
    }

    int di = 0;
    for (int row = 0; row < 16; ++row) {
        for (int col = 0; col < 16; ++col)
            texBuffer[di++] = RGB_16BIT5551(255, 255, 255, 1);
        di += 1008;
    }
}

void GraphicsSystem::UpdateTextureBufferWithSortedSprites()
{
    uint8_t  order[24]{};
    uint8_t  cnt   = 0;
    bool     flag  = true;

    for (int i = 0; i < 24; ++i) gfxSurface[i].texStartX = -1;

    for (int pass = 0; pass < 24; ++pass) {
        int32_t best = 0;
        int8_t  bestIdx = -1;
        for (int i = 0; i < 24; ++i) {
            if (FileIO::StringLength(gfxSurface[i].fileName) > 0 && gfxSurface[i].texStartX == -1) {
                if (CheckSurfaceSize(gfxSurface[i].width) && CheckSurfaceSize(gfxSurface[i].height)) {
                    if (gfxSurface[i].width + gfxSurface[i].height > best) {
                        best = gfxSurface[i].width + gfxSurface[i].height;
                        bestIdx = (int8_t)i;
                    }
                } else {
                    Log::Info("Sprite[%d] '%s' SKIP non-pow2 %dx%d",
                        i, gfxSurface[i].fileName, gfxSurface[i].width, gfxSurface[i].height);
                    gfxSurface[i].texStartX = 0;
                }
            }
        }
        if (bestIdx == -1) break;
        gfxSurface[bestIdx].texStartX = 0;
        order[cnt++] = (uint8_t)bestIdx;
    }

    for (int i = 0; i < 24; ++i) gfxSurface[i].texStartX = -1;

    for (int oi = 0; oi < (int)cnt; ++oi) {
        int8_t s = (int8_t)order[oi];
        gfxSurface[s].texStartX = 0;
        gfxSurface[s].texStartY = 0;
        int done = 0;
        while (!done) {
            done = 1;
            if (gfxSurface[s].height == 1024) flag = false;
            if (flag) {
                if (gfxSurface[s].texStartX < 512 && gfxSurface[s].texStartY < 512) {
                    done = 0;
                    gfxSurface[s].texStartX += gfxSurface[s].width;
                    if (gfxSurface[s].texStartX + gfxSurface[s].width > 1024) {
                        gfxSurface[s].texStartX = 0;
                        gfxSurface[s].texStartY += gfxSurface[s].height;
                    }
                } else {
                    for (int k = 0; k < 24; ++k) {
                        if (gfxSurface[k].texStartX > -1 && k != (int)s &&
                            gfxSurface[s].texStartX < gfxSurface[k].texStartX + gfxSurface[k].width &&
                            gfxSurface[s].texStartX >= gfxSurface[k].texStartX &&
                            gfxSurface[s].texStartY < gfxSurface[k].texStartY + gfxSurface[k].height)
                        {
                            done = 0;
                            gfxSurface[s].texStartX += gfxSurface[s].width;
                            if (gfxSurface[s].texStartX + gfxSurface[s].width > 1024) {
                                gfxSurface[s].texStartX = 0;
                                gfxSurface[s].texStartY += gfxSurface[s].height;
                            }
                            break;
                        }
                    }
                }
            } else if (gfxSurface[s].width < 1024) {
                if (gfxSurface[s].texStartX < 16 && gfxSurface[s].texStartY < 16) {
                    done = 0;
                    gfxSurface[s].texStartX += gfxSurface[s].width;
                    if (gfxSurface[s].texStartX + gfxSurface[s].width > 1024) {
                        gfxSurface[s].texStartX = 0;
                        gfxSurface[s].texStartY += gfxSurface[s].height;
                    }
                } else {
                    for (int k = 0; k < 24; ++k) {
                        if (gfxSurface[k].texStartX > -1 && k != (int)s &&
                            gfxSurface[s].texStartX < gfxSurface[k].texStartX + gfxSurface[k].width &&
                            gfxSurface[s].texStartX >= gfxSurface[k].texStartX &&
                            gfxSurface[s].texStartY < gfxSurface[k].texStartY + gfxSurface[k].height)
                        {
                            done = 0;
                            gfxSurface[s].texStartX += gfxSurface[s].width;
                            if (gfxSurface[s].texStartX + gfxSurface[s].width > 1024) {
                                gfxSurface[s].texStartX = 0;
                                gfxSurface[s].texStartY += gfxSurface[s].height;
                            }
                            break;
                        }
                    }
                }
            }
        }
        if (gfxSurface[s].texStartY + gfxSurface[s].height <= 1024) {
            Log::Info("Sprite[%d] '%s' placed at (%d,%d) size %dx%d",
                (int)s, gfxSurface[s].fileName,
                gfxSurface[s].texStartX, gfxSurface[s].texStartY,
                gfxSurface[s].width, gfxSurface[s].height);
            int src = (int)gfxSurface[s].dataStart;
            int dst = gfxSurface[s].texStartX + (gfxSurface[s].texStartY << 10);
            for (int row = 0; row < gfxSurface[s].height; ++row) {
                for (int col = 0; col < gfxSurface[s].width; ++col) {
                    texBuffer[dst] = (graphicData[src] <= 0) ? 0
                        : tilePalette16_Data[texPaletteNum][(int)graphicData[src]];
                    ++dst; ++src;
                }
                dst += 1024 - gfxSurface[s].width;
            }
        } else {
            Log::Error("Sprite[%d] '%s' DOES NOT FIT: texStartX=%d texStartY=%d size %dx%d",
                (int)s, gfxSurface[s].fileName,
                gfxSurface[s].texStartX, gfxSurface[s].texStartY,
                gfxSurface[s].width, gfxSurface[s].height);
        }
    }
}

void GraphicsSystem::UpdateTextureBufferWithSprites()
{
    for (int i = 0; i < 24; ++i) {
        if (gfxSurface[i].texStartY + gfxSurface[i].height <= 1024 &&
            gfxSurface[i].texStartX > -1)
        {
            int src = (int)gfxSurface[i].dataStart;
            int dst = gfxSurface[i].texStartX + (gfxSurface[i].texStartY << 10);
            for (int row = 0; row < gfxSurface[i].height; ++row) {
                for (int col = 0; col < gfxSurface[i].width; ++col) {
                    texBuffer[dst] = (graphicData[src] <= 0) ? 0
                        : tilePalette16_Data[texPaletteNum][(int)graphicData[src]];
                    ++dst; ++src;
                }
                dst += 1024 - gfxSurface[i].width;
            }
        }
    }
}

void GraphicsSystem::LoadBMPFile(const char* fileName, int32_t surfaceNum)
{
    FileData fData{};
    if (!FileIO::LoadFile(fileName, fData)) return;
    FileIO::StrCopy(gfxSurface[surfaceNum].fileName, 64, fileName, 64);
    FileIO::SetFilePosition(18);
    gfxSurface[surfaceNum].width  = (int)FileIO::ReadByte();
    gfxSurface[surfaceNum].width += (int)FileIO::ReadByte() << 8;
    gfxSurface[surfaceNum].width += (int)FileIO::ReadByte() << 16;
    gfxSurface[surfaceNum].width += (int)FileIO::ReadByte() << 24;
    gfxSurface[surfaceNum].height  = (int)FileIO::ReadByte();
    gfxSurface[surfaceNum].height += (int)FileIO::ReadByte() << 8;
    gfxSurface[surfaceNum].height += (int)FileIO::ReadByte() << 16;
    gfxSurface[surfaceNum].height += (int)FileIO::ReadByte() << 24;
    FileIO::SetFilePosition((uint32_t)(fData.fileSize -
        (uint32_t)(gfxSurface[surfaceNum].width * gfxSurface[surfaceNum].height)));
    gfxSurface[surfaceNum].dataStart = gfxDataPosition;
    int dst = (int)gfxSurface[surfaceNum].dataStart +
              gfxSurface[surfaceNum].width * (gfxSurface[surfaceNum].height - 1);
    for (int row = 0; row < gfxSurface[surfaceNum].height; ++row) {
        for (int col = 0; col < gfxSurface[surfaceNum].width; ++col)
            graphicData[dst++] = FileIO::ReadByte();
        dst -= gfxSurface[surfaceNum].width << 1;
    }
    gfxDataPosition += (uint32_t)(gfxSurface[surfaceNum].width * gfxSurface[surfaceNum].height);
    if (gfxDataPosition >= 4194304) gfxDataPosition = 0;
    FileIO::CloseFile();
}

void GraphicsSystem::LoadGIFFile(const char* fileName, int32_t surfaceNum)
{
    FileData fData{};
    uint8_t  byteP[3]{};
    bool     interlaced = false;
    if (!FileIO::LoadFile(fileName, fData)) { Log::Error("LoadGIFFile FAILED: %s", fileName); return; }
    Log::Info("LoadGIFFile OK: %s", fileName);
    FileIO::StrCopy(gfxSurface[surfaceNum].fileName, 64, fileName, 64);
    FileIO::SetFilePosition(6);
    byteP[0] = FileIO::ReadByte(); int w = (int)byteP[0];
    byteP[0] = FileIO::ReadByte(); w += (int)byteP[0] << 8;
    byteP[0] = FileIO::ReadByte(); int h = (int)byteP[0];
    byteP[0] = FileIO::ReadByte(); h += (int)byteP[0] << 8;
    FileIO::ReadByte(); FileIO::ReadByte(); FileIO::ReadByte();
    for (int i = 0; i < 256; ++i) FileIO::ReadByteArray(byteP, 3);
    byteP[0] = FileIO::ReadByte();
    while (byteP[0] != 44) byteP[0] = FileIO::ReadByte();
    if (byteP[0] == 44) {
        FileIO::ReadByteArray(byteP, 2); FileIO::ReadByteArray(byteP, 2);
        FileIO::ReadByteArray(byteP, 2); FileIO::ReadByteArray(byteP, 2);
        byteP[0] = FileIO::ReadByte();
        if (((int)byteP[0] & 64) >> 6 == 1) interlaced = true;
        if (((int)byteP[0] & 128) >> 7 == 1) {
            for (int i = 128; i < 256; ++i) FileIO::ReadByteArray(byteP, 3);
        }
        gfxSurface[surfaceNum].width     = w;
        gfxSurface[surfaceNum].height    = h;
        gfxSurface[surfaceNum].dataStart = gfxDataPosition;
        gfxDataPosition += (uint32_t)(w * h);
        if (gfxDataPosition >= 4194304) gfxDataPosition = 0;
        else GifLoader::ReadGifPictureData(w, h, interlaced, graphicData,
                                           (int)gfxSurface[surfaceNum].dataStart);
    }
    FileIO::CloseFile();
}

void GraphicsSystem::LoadStageGIFFile(int32_t zNumber)
{
    FileData fData{};
    uint8_t  byteP[3]{};
    bool     interlaced = false;
    if (!FileIO::LoadStageFile("16x16Tiles.gif", zNumber, fData)) return;
    FileIO::SetFilePosition(6);
    byteP[0] = FileIO::ReadByte(); int w = (int)byteP[0];
    byteP[0] = FileIO::ReadByte(); w += (int)byteP[0] << 8;
    byteP[0] = FileIO::ReadByte(); int h = (int)byteP[0];
    byteP[0] = FileIO::ReadByte(); h += (int)byteP[0] << 8;
    FileIO::ReadByte(); FileIO::ReadByte(); FileIO::ReadByte();
    for (int i = 128; i < 256; ++i) FileIO::ReadByteArray(byteP, 3);
    for (int i = 128; i < 256; ++i) {
        FileIO::ReadByteArray(byteP, 3);
        tilePalette[i].red   = byteP[0];
        tilePalette[i].green = byteP[1];
        tilePalette[i].blue  = byteP[2];
        tilePalette16_Data[texPaletteNum][i] = RGB_16BIT5551(byteP[0], byteP[1], byteP[2], 1);
    }
    byteP[0] = FileIO::ReadByte();
    if (byteP[0] == 44) {
        FileIO::ReadByteArray(byteP, 2); FileIO::ReadByteArray(byteP, 2);
        FileIO::ReadByteArray(byteP, 2); FileIO::ReadByteArray(byteP, 2);
        byteP[0] = FileIO::ReadByte();
        if (((int)byteP[0] & 64) >> 6 == 1) interlaced = true;
        if (((int)byteP[0] & 128) >> 7 == 1) {
            for (int i = 128; i < 256; ++i) FileIO::ReadByteArray(byteP, 3);
        }
        GifLoader::ReadGifPictureData(w, h, interlaced, tileGfx, 0);
        uint8_t transparent = tileGfx[0];
        for (int i = 0; i < 262144; ++i)
            if (tileGfx[i] == transparent) tileGfx[i] = 0;
    }
    FileIO::CloseFile();
}

void GraphicsSystem::Copy16x16Tile(int32_t tDest, int32_t tSource)
{
    tSource <<= 2; tDest <<= 2;
    tileUVArray[tDest]   = tileUVArray[tSource];
    tileUVArray[tDest+1] = tileUVArray[tSource+1];
    tileUVArray[tDest+2] = tileUVArray[tSource+2];
    tileUVArray[tDest+3] = tileUVArray[tSource+3];
}

#define VERT(vx,vy,tu,tv,aa) \
    SetVert(gfxPolyList[gfxVertexSize++], (vx), (vy), (tu), (tv), 255,255,255,(aa))
#define VERT_C(vx,vy,tu,tv,cr,cg,cb,aa) \
    SetVert(gfxPolyList[gfxVertexSize++], (vx), (vy), (tu), (tv), (cr),(cg),(cb),(aa))
#define PREV(n) gfxPolyList[gfxVertexSize-(n)]

void GraphicsSystem::ClearScreen(uint8_t clearColour)
{
    if (gfxVertexSize >= 8192) return;
    uint8_t r = tilePalette[clearColour].red;
    uint8_t g = tilePalette[clearColour].green;
    uint8_t b = tilePalette[clearColour].blue;
    float rx = (float)(GlobalAppDefinitions::SCREEN_XSIZE << 4);
    VERT_C(0.0f,  0.0f,  0.0f, 0.0f, r,g,b,255);
    VERT_C(rx,    0.0f,  0.0f, 0.0f, r,g,b,255);
    VERT_C(0.0f,  3840.0f, 0.0f, 0.0f, r,g,b,255);
    VERT_C(rx,    3840.0f, 0.0f, 0.0f, r,g,b,255);
    gfxIndexSize += 2;
}

void GraphicsSystem::DrawSprite(int32_t xPos, int32_t yPos, int32_t xSize, int32_t ySize,
                                 int32_t xBegin, int32_t yBegin, int32_t surfaceNum)
{
    if (gfxSurface[surfaceNum].texStartX <= -1 || gfxVertexSize >= 8192 ||
        xPos <= -512 || xPos >= 872 || yPos <= -512 || yPos >= 752) return;
    float tx0 = (float)(gfxSurface[surfaceNum].texStartX + xBegin) * TEX_INV;
    float ty0 = (float)(gfxSurface[surfaceNum].texStartY + yBegin) * TEX_INV;
    float tx1 = (float)(gfxSurface[surfaceNum].texStartX + xBegin + xSize) * TEX_INV;
    float ty1 = (float)(gfxSurface[surfaceNum].texStartY + yBegin + ySize) * TEX_INV;
    float px0 = (float)(xPos << 4), py0 = (float)(yPos << 4);
    float px1 = (float)((xPos+xSize) << 4), py1 = (float)((yPos+ySize) << 4);
    VERT(px0,py0,tx0,ty0,255);
    VERT(px1,py0,tx1,ty0,255);
    VERT(px0,py1,tx0,ty1,255);
    VERT(px1,py1,tx1,ty1,255);
    gfxIndexSize += 2;
}

void GraphicsSystem::DrawSpriteFlipped(int32_t xPos, int32_t yPos, int32_t xSize, int32_t ySize,
                                        int32_t xBegin, int32_t yBegin,
                                        int32_t direction, int32_t surfaceNum)
{
    if (gfxSurface[surfaceNum].texStartX <= -1 || gfxVertexSize >= 8192 ||
        xPos <= -512 || xPos >= 872 || yPos <= -512 || yPos >= 752) return;
    int tx = gfxSurface[surfaceNum].texStartX;
    int ty = gfxSurface[surfaceNum].texStartY;
    float u0 = (float)(tx+xBegin)*TEX_INV,          u1 = (float)(tx+xBegin+xSize)*TEX_INV;
    float v0 = (float)(ty+yBegin)*TEX_INV,          v1 = (float)(ty+yBegin+ySize)*TEX_INV;
    float px0 = (float)(xPos<<4), py0 = (float)(yPos<<4);
    float px1 = (float)((xPos+xSize)<<4), py1 = (float)((yPos+ySize)<<4);
    switch (direction) {
        case 0: VERT(px0,py0,u0,v0,255); VERT(px1,py0,u1,v0,255);
                VERT(px0,py1,u0,v1,255); VERT(px1,py1,u1,v1,255); break;
        case 1: VERT(px0,py0,u1,v0,255); VERT(px1,py0,u0,v0,255);
                VERT(px0,py1,u1,v1,255); VERT(px1,py1,u0,v1,255); break;
        case 2: VERT(px0,py0,u0,v1,255); VERT(px1,py0,u1,v1,255);
                VERT(px0,py1,u0,v0,255); VERT(px1,py1,u1,v0,255); break;
        case 3: VERT(px0,py0,u1,v1,255); VERT(px1,py0,u0,v1,255);
                VERT(px0,py1,u1,v0,255); VERT(px1,py1,u0,v0,255); break;
    }
    gfxIndexSize += 2;
}

void GraphicsSystem::DrawBlendedSprite(int32_t xPos, int32_t yPos, int32_t xSize, int32_t ySize,
                                        int32_t xBegin, int32_t yBegin, int32_t surfaceNum)
{
    if (gfxSurface[surfaceNum].texStartX <= -1 || gfxVertexSize >= 8192 ||
        xPos <= -512 || xPos >= 872 || yPos <= -512 || yPos >= 752) return;
    float u0 = (float)(gfxSurface[surfaceNum].texStartX+xBegin)*TEX_INV;
    float v0 = (float)(gfxSurface[surfaceNum].texStartY+yBegin)*TEX_INV;
    float u1 = (float)(gfxSurface[surfaceNum].texStartX+xBegin+xSize)*TEX_INV;
    float v1 = (float)(gfxSurface[surfaceNum].texStartY+yBegin+ySize)*TEX_INV;
    float px0=(float)(xPos<<4), py0=(float)(yPos<<4);
    float px1=(float)((xPos+xSize)<<4), py1=(float)((yPos+ySize)<<4);
    VERT(px0,py0,u0,v0,128); VERT(px1,py0,u1,v0,128);
    VERT(px0,py1,u0,v1,128); VERT(px1,py1,u1,v1,128);
    gfxIndexSize += 2;
}

void GraphicsSystem::DrawAlphaBlendedSprite(int32_t xPos, int32_t yPos, int32_t xSize, int32_t ySize,
                                             int32_t xBegin, int32_t yBegin,
                                             int32_t alpha, int32_t surfaceNum)
{
    if (gfxSurface[surfaceNum].texStartX <= -1 || gfxVertexSize >= 8192 ||
        xPos <= -512 || xPos >= 872 || yPos <= -512 || yPos >= 752) return;
    uint8_t a = (uint8_t)alpha;
    float u0=(float)(gfxSurface[surfaceNum].texStartX+xBegin)*TEX_INV;
    float v0=(float)(gfxSurface[surfaceNum].texStartY+yBegin)*TEX_INV;
    float u1=(float)(gfxSurface[surfaceNum].texStartX+xBegin+xSize)*TEX_INV;
    float v1=(float)(gfxSurface[surfaceNum].texStartY+yBegin+ySize)*TEX_INV;
    float px0=(float)(xPos<<4), py0=(float)(yPos<<4);
    float px1=(float)((xPos+xSize)<<4), py1=(float)((yPos+ySize)<<4);
    VERT(px0,py0,u0,v0,a); VERT(px1,py0,u1,v0,a);
    VERT(px0,py1,u0,v1,a); VERT(px1,py1,u1,v1,a);
    gfxIndexSize += 2;
}

void GraphicsSystem::DrawAdditiveBlendedSprite(int32_t xPos, int32_t yPos,
                                                int32_t xSize, int32_t ySize,
                                                int32_t xBegin, int32_t yBegin,
                                                int32_t alpha, int32_t surfaceNum)
{
    DrawAlphaBlendedSprite(xPos, yPos, xSize, ySize, xBegin, yBegin, alpha, surfaceNum);
}

void GraphicsSystem::DrawSubtractiveBlendedSprite(int32_t xPos, int32_t yPos,
                                                   int32_t xSize, int32_t ySize,
                                                   int32_t xBegin, int32_t yBegin,
                                                   int32_t alpha, int32_t surfaceNum)
{
    DrawAlphaBlendedSprite(xPos, yPos, xSize, ySize, xBegin, yBegin, alpha, surfaceNum);
}

void GraphicsSystem::DrawRectangle(int32_t xPos, int32_t yPos, int32_t xSize, int32_t ySize,
                                    int32_t r, int32_t g, int32_t b, int32_t alpha)
{
    if (alpha > 255) alpha = 255;
    if (gfxVertexSize >= 8192) return;
    uint8_t cr=(uint8_t)r, cg=(uint8_t)g, cb=(uint8_t)b, ca=(uint8_t)alpha;
    VERT_C((float)(xPos<<4),(float)(yPos<<4),0.0f,0.0f,cr,cg,cb,ca);
    VERT_C((float)((xPos+xSize)<<4),(float)(yPos<<4),0.01f,0.0f,cr,cg,cb,ca);
    VERT_C((float)(xPos<<4),(float)((yPos+ySize)<<4),0.0f,0.01f,cr,cg,cb,ca);
    VERT_C((float)((xPos+xSize)<<4),(float)((yPos+ySize)<<4),0.01f,0.01f,cr,cg,cb,ca);
    gfxIndexSize += 2;
}

static void DrawRotatedSpriteImpl(bool flipped,
    int32_t xPos, int32_t yPos,
    int32_t xPivot, int32_t yPivot,
    int32_t xBegin, int32_t yBegin, int32_t xSize, int32_t ySize,
    int32_t rotAngle, int32_t surfaceNum,
    DrawVertex* list, uint16_t& vs, uint16_t& is)
{
    xPos <<= 4; yPos <<= 4;
    rotAngle -= (rotAngle >> 9) << 9;
    if (rotAngle < 0) rotAngle += 512;
    if (rotAngle != 0) rotAngle = 512 - rotAngle;
    int32_t S = GlobalAppDefinitions::SinValue512[rotAngle];
    int32_t C = GlobalAppDefinitions::CosValue512[rotAngle];

    if (GraphicsSystem::gfxSurface[surfaceNum].texStartX <= -1 || vs >= 8192 ||
        xPos <= -8192 || xPos >= 13952 || yPos <= -8192 || yPos >= 12032) return;

    float u0 = (float)(GraphicsSystem::gfxSurface[surfaceNum].texStartX + xBegin) * TEX_INV;
    float v0 = (float)(GraphicsSystem::gfxSurface[surfaceNum].texStartY + yBegin) * TEX_INV;
    float u1 = (float)(GraphicsSystem::gfxSurface[surfaceNum].texStartX + xBegin + xSize) * TEX_INV;
    float v1 = (float)(GraphicsSystem::gfxSurface[surfaceNum].texStartY + yBegin + ySize) * TEX_INV;

    auto rotPx = [&](int32_t lx, int32_t ly) -> float { return (float)(xPos + ((lx*C + ly*S) >> 5)); };
    auto rotPy = [&](int32_t lx, int32_t ly) -> float { return (float)(yPos + ((ly*C - lx*S) >> 5)); };

    if (!flipped) {
        int32_t ax=-xPivot, ay=-yPivot, bx=xSize-xPivot, by=-yPivot;
        int32_t cx=-xPivot, cy=ySize-yPivot, dx=xSize-xPivot, dy=ySize-yPivot;
        SetVert(list[vs++], rotPx(ax,ay), rotPy(ax,ay), u0, v0);
        SetVert(list[vs++], rotPx(bx,by), rotPy(bx,by), u1, v0);
        SetVert(list[vs++], rotPx(cx,cy), rotPy(cx,cy), u0, v1);
        SetVert(list[vs++], rotPx(dx,dy), rotPy(dx,dy), u1, v1);
    } else {
        int32_t ax=xPivot, ay=-yPivot, bx=xPivot-xSize, by=-yPivot;
        int32_t cx=xPivot, cy=ySize-yPivot, dx=xPivot-xSize, dy=ySize-yPivot;
        SetVert(list[vs++], rotPx(ax,ay), rotPy(ax,ay), u0, v0);
        SetVert(list[vs++], rotPx(bx,by), rotPy(bx,by), u1, v0);
        SetVert(list[vs++], rotPx(cx,cy), rotPy(cx,cy), u0, v1);
        SetVert(list[vs++], rotPx(dx,dy), rotPy(dx,dy), u1, v1);
    }
    is += 2;
}

void GraphicsSystem::DrawRotatedSprite(int32_t direction, int32_t xPos, int32_t yPos,
                                        int32_t pivotX, int32_t pivotY,
                                        int32_t uStart, int32_t vStart,
                                        int32_t xSize, int32_t ySize,
                                        int32_t rotAngle, int32_t surfaceNum)
{
    DrawRotatedSpriteImpl(direction != 0,
        xPos, yPos, pivotX, pivotY,
        uStart, vStart, xSize, ySize,
        rotAngle, surfaceNum,
        gfxPolyList, gfxVertexSize, gfxIndexSize);
}

void GraphicsSystem::DrawScaledSprite(uint8_t direction, int32_t xPos, int32_t yPos,
                                       int32_t xPivot, int32_t yPivot,
                                       int32_t xScale, int32_t yScale,
                                       int32_t xSize, int32_t ySize,
                                       int32_t xBegin, int32_t yBegin, int32_t surfaceNum)
{
    if (gfxVertexSize >= 8192 || xPos <= -512 || xPos >= 872 || yPos <= -512 || yPos >= 752) return;
    xScale <<= 2; yScale <<= 2;
    xPos   -= xPivot * xScale >> 11;   xScale = xSize * xScale >> 11;
    yPos   -= yPivot * yScale >> 11;   yScale = ySize * yScale >> 11;
    if (gfxSurface[surfaceNum].texStartX <= -1) return;
    float u0=(float)(gfxSurface[surfaceNum].texStartX+xBegin)*TEX_INV;
    float v0=(float)(gfxSurface[surfaceNum].texStartY+yBegin)*TEX_INV;
    float u1=(float)(gfxSurface[surfaceNum].texStartX+xBegin+xSize)*TEX_INV;
    float v1=(float)(gfxSurface[surfaceNum].texStartY+yBegin+ySize)*TEX_INV;
    float px0=(float)(xPos<<4), py0=(float)(yPos<<4);
    float px1=(float)((xPos+xScale)<<4), py1=(float)((yPos+yScale)<<4);
    (void)direction;
    VERT(px0,py0,u0,v0,255); VERT(px1,py0,u1,v0,255);
    VERT(px0,py1,u0,v1,255); VERT(px1,py1,u1,v1,255);
    gfxIndexSize += 2;
}

void GraphicsSystem::DrawScaledChar(uint8_t /*direction*/,
                                     int32_t xPos, int32_t yPos,
                                     int32_t xPivot, int32_t yPivot,
                                     int32_t xScale, int32_t yScale,
                                     int32_t xSize, int32_t ySize,
                                     int32_t xBegin, int32_t yBegin, int32_t surfaceNum)
{
    if (gfxVertexSize >= 8192 || xPos <= -8192 || xPos >= 13951 || yPos <= -1024 || yPos >= 4864) return;
    xPos   -= xPivot * xScale >> 5;    xScale = xSize * xScale >> 5;
    yPos   -= yPivot * yScale >> 5;    yScale = ySize * yScale >> 5;
    if (gfxSurface[surfaceNum].texStartX <= -1 || gfxVertexSize >= 4096) return;
    float u0=(float)(gfxSurface[surfaceNum].texStartX+xBegin)*TEX_INV;
    float v0=(float)(gfxSurface[surfaceNum].texStartY+yBegin)*TEX_INV;
    float u1=(float)(gfxSurface[surfaceNum].texStartX+xBegin+xSize)*TEX_INV;
    float v1=(float)(gfxSurface[surfaceNum].texStartY+yBegin+ySize)*TEX_INV;

    SetVert(gfxPolyList[gfxVertexSize++], (float)xPos,        (float)yPos,        u0, v0);
    SetVert(gfxPolyList[gfxVertexSize++], (float)(xPos+xScale),(float)yPos,        u1, v0);
    SetVert(gfxPolyList[gfxVertexSize++], (float)xPos,        (float)(yPos+yScale),u0, v1);
    SetVert(gfxPolyList[gfxVertexSize++], (float)(xPos+xScale),(float)(yPos+yScale),u1, v1);
    gfxIndexSize += 2;
}

void GraphicsSystem::DrawQuad(const Quad2D& face, int32_t rgbVal)
{
    if (gfxVertexSize >= 8192) return;
    uint8_t r = (uint8_t)((rgbVal >> 16) & 0xFF);
    uint8_t g = (uint8_t)((rgbVal >> 8)  & 0xFF);
    uint8_t b = (uint8_t)( rgbVal        & 0xFF);
    int32_t av = (rgbVal & 0x7F000000) >> 23;
    uint8_t a = (av <= 253) ? (uint8_t)av : 255;
    for (int i = 0; i < 4; ++i) {
        gfxPolyList[gfxVertexSize].x = (float)(face.vertex[i].x << 4);
        gfxPolyList[gfxVertexSize].y = (float)(face.vertex[i].y << 4);
        gfxPolyList[gfxVertexSize].u = 0.01f;
        gfxPolyList[gfxVertexSize].v = 0.01f;
        gfxPolyList[gfxVertexSize].color = {r,g,b,a};
        ++gfxVertexSize;
    }
    gfxIndexSize += 2;
}

void GraphicsSystem::DrawTexturedQuad(const Quad2D& face, int32_t surfaceNum)
{
    if (gfxVertexSize >= 8192) return;
    int tx = gfxSurface[surfaceNum].texStartX;
    int ty = gfxSurface[surfaceNum].texStartY;
    for (int i = 0; i < 4; ++i) {
        gfxPolyList[gfxVertexSize].x = (float)(face.vertex[i].x << 4);
        gfxPolyList[gfxVertexSize].y = (float)(face.vertex[i].y << 4);
        gfxPolyList[gfxVertexSize].u = (float)(tx + face.vertex[i].u) * TEX_INV;
        gfxPolyList[gfxVertexSize].v = (float)(ty + face.vertex[i].v) * TEX_INV;
        gfxPolyList[gfxVertexSize].color = {255,255,255,255};
        ++gfxVertexSize;
    }
    gfxIndexSize += 2;
}

void GraphicsSystem::SetPaletteEntry(uint8_t /*paletteNum*/, uint8_t index,
                                      uint8_t r, uint8_t g, uint8_t b)
{

    tilePalette16_Data[texPaletteNum][index] = (index == 0)
        ? RGB_16BIT5551(r, g, b, 0)
        : RGB_16BIT5551(r, g, b, 1);
    tilePalette[index].red   = r;
    tilePalette[index].green = g;
    tilePalette[index].blue  = b;
}

void GraphicsSystem::SetFadeHQ(uint8_t r, uint8_t g, uint8_t b, int32_t a)
{
    fadeMode = 1;
    if (a > 255) a = 255;
    fadeR = r; fadeG = g; fadeB = b; fadeA = (uint8_t)a;
}

void GraphicsSystem::SetFade(uint8_t r, uint8_t g, uint8_t b, uint16_t a)
{
    fadeMode = 1;
    if (a > 255) a = 255;
    fadeR = r; fadeG = g; fadeB = b; fadeA = (uint8_t)a;
}

void GraphicsSystem::SetLimitedFade(uint8_t paletteNum,
                                     uint8_t clrR, uint8_t clrG, uint8_t clrB,
                                     uint16_t clrA, int32_t fStart, int32_t fEnd)
{
    paletteMode = paletteNum;
    if (paletteNum >= 8) return;
    if (clrA > 255) clrA = 255;
    if (fEnd < 256) ++fEnd;
    uint8_t rgb[3];
    for (int i = fStart; i < fEnd; ++i) {
        rgb[0] = (uint8_t)((tilePalette[i].red   * (255-(int)clrA) + (int)clrA*(int)clrR) >> 8);
        rgb[1] = (uint8_t)((tilePalette[i].green * (255-(int)clrA) + (int)clrA*(int)clrG) >> 8);
        rgb[2] = (uint8_t)((tilePalette[i].blue  * (255-(int)clrA) + (int)clrA*(int)clrB) >> 8);
        tilePalette16_Data[0][i] = RGB_16BIT5551(rgb[0], rgb[1], rgb[2], 1);
    }
    tilePalette16_Data[0][0] = RGB_16BIT5551(rgb[0], rgb[1], rgb[2], 0);
}

void GraphicsSystem::SetActivePalette(uint8_t paletteNum, int32_t /*minY*/, int32_t /*maxY*/)
{
    if (paletteNum >= 8) return;
    texPaletteNum = (int32_t)paletteNum;
}

void GraphicsSystem::CopyPalette(uint8_t paletteSource, uint8_t paletteDest)
{
    if (paletteSource >= 8 || paletteDest >= 8) return;
    for (int i = 0; i < 256; ++i)
        tilePalette16_Data[paletteDest][i] = tilePalette16_Data[paletteSource][i];
}

void GraphicsSystem::RotatePalette(uint8_t pStart, uint8_t pEnd, uint8_t pDir)
{
    if (pDir == 0) {
        uint16_t tmp = tilePalette16_Data[texPaletteNum][pStart];
        for (uint8_t i = pStart; i < pEnd; ++i)
            tilePalette16_Data[texPaletteNum][i] = tilePalette16_Data[texPaletteNum][i+1];
        tilePalette16_Data[texPaletteNum][pEnd] = tmp;
    } else {
        uint16_t tmp = tilePalette16_Data[texPaletteNum][pEnd];
        for (uint8_t i = pEnd; i > pStart; --i)
            tilePalette16_Data[texPaletteNum][i] = tilePalette16_Data[texPaletteNum][i-1];
        tilePalette16_Data[texPaletteNum][pStart] = tmp;
    }
}

void GraphicsSystem::GenerateBlendLookupTable()
{
    int idx = 0;
    for (int i = 0; i < 256; ++i) {
        for (int j = 0; j < 32; ++j) {
            blendLookupTable[idx]       = (uint16_t)(j * i >> 8);
            subtractiveLookupTable[idx] = (uint16_t)((31-j) * i >> 8);
            ++idx;
        }
    }
}

void GraphicsSystem::DrawRotoZoomSprite(uint8_t direction,
    int32_t xPos, int32_t yPos, int32_t xPivot, int32_t yPivot,
    int32_t xBegin, int32_t yBegin, int32_t xSize, int32_t ySize,
    int32_t rotAngle, int32_t scale, int32_t surfaceNum)
{
    xPos <<= 4; yPos <<= 4;
    rotAngle -= (rotAngle >> 9) << 9;
    if (rotAngle < 0) rotAngle += 512;
    if (rotAngle != 0) rotAngle = 512 - rotAngle;
    int32_t S = GlobalAppDefinitions::SinValue512[rotAngle] * scale >> 9;
    int32_t C = GlobalAppDefinitions::CosValue512[rotAngle] * scale >> 9;
    if (gfxSurface[surfaceNum].texStartX <= -1 || gfxVertexSize >= 8192 ||
        xPos <= -8192 || xPos >= 13952 || yPos <= -8192 || yPos >= 12032) return;
    float u0=(float)(gfxSurface[surfaceNum].texStartX+xBegin)*TEX_INV;
    float v0=(float)(gfxSurface[surfaceNum].texStartY+yBegin)*TEX_INV;
    float u1=(float)(gfxSurface[surfaceNum].texStartX+xBegin+xSize)*TEX_INV;
    float v1=(float)(gfxSurface[surfaceNum].texStartY+yBegin+ySize)*TEX_INV;
    auto rX=[&](int32_t lx,int32_t ly)->float{return (float)(xPos+((lx*C+ly*S)>>5));};
    auto rY=[&](int32_t lx,int32_t ly)->float{return (float)(yPos+((ly*C-lx*S)>>5));};
    if (direction == 0) {
        int32_t ax=-xPivot, ay=-yPivot, bx=xSize-xPivot, by=-yPivot;
        int32_t cx=-xPivot, cy=ySize-yPivot, dx=xSize-xPivot, dy=ySize-yPivot;
        SetVert(gfxPolyList[gfxVertexSize++],rX(ax,ay),rY(ax,ay),u0,v0);
        SetVert(gfxPolyList[gfxVertexSize++],rX(bx,by),rY(bx,by),u1,v0);
        SetVert(gfxPolyList[gfxVertexSize++],rX(cx,cy),rY(cx,cy),u0,v1);
        SetVert(gfxPolyList[gfxVertexSize++],rX(dx,dy),rY(dx,dy),u1,v1);
    } else {
        int32_t ax=xPivot, ay=-yPivot, bx=xPivot-xSize, by=-yPivot;
        int32_t cx=xPivot, cy=ySize-yPivot, dx=xPivot-xSize, dy=ySize-yPivot;
        SetVert(gfxPolyList[gfxVertexSize++],rX(ax,ay),rY(ax,ay),u0,v0);
        SetVert(gfxPolyList[gfxVertexSize++],rX(bx,by),rY(bx,by),u1,v0);
        SetVert(gfxPolyList[gfxVertexSize++],rX(cx,cy),rY(cx,cy),u0,v1);
        SetVert(gfxPolyList[gfxVertexSize++],rX(dx,dy),rY(dx,dy),u1,v1);
    }
    gfxIndexSize += 2;
}

void GraphicsSystem::DrawFadedBackground() { /* stub */ }
void GraphicsSystem::DrawHLineScroll(uint8_t /*layerID*/) { /* implemented in StageSystem */ }
void GraphicsSystem::DrawVLineScroll(uint8_t /*layerID*/) { /* implemented in StageSystem */ }
void GraphicsSystem::Draw3DFloor()         { /* stub */ }
void GraphicsSystem::DrawTintRectangle(int32_t, int32_t, int32_t, int32_t) {}
void GraphicsSystem::DrawTintSpriteMask(int32_t, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t) {}
void GraphicsSystem::DrawScaledTintMask(uint8_t, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t) {}

#undef VERT
#undef VERT_C
#undef PREV

}
