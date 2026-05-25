#pragma once
#include <cstdint>
#include "DrawVertex.h"
#include "DrawVertex3D.h"
#include "GfxSurfaceDesc.h"
#include "PaletteEntry.h"
#include "Quad2D.h"

namespace rvm {

struct GraphicsSystem {
    static constexpr int32_t NUM_SPRITESHEETS = 24;
    static constexpr int32_t GRAPHIC_DATASIZE = 2097152;
    static constexpr int32_t VERTEX_LIMIT     = 8192;
    static constexpr int32_t INDEX_LIMIT      = 49152;

    static bool      render3DEnabled;
    static uint8_t   fadeMode;
    static uint8_t   fadeR;
    static uint8_t   fadeG;
    static uint8_t   fadeB;
    static uint8_t   fadeA;
    static uint8_t   paletteMode;
    static uint8_t   colourMode;
    static uint16_t  texBuffer[1048576];
    static uint8_t   texBufferMode;
    static uint8_t   tileGfx[262144];
    static uint8_t   graphicData[2097152];
    static GfxSurfaceDesc gfxSurface[24];
    static uint32_t  gfxDataPosition;
    static DrawVertex   gfxPolyList[8192];
    static DrawVertex3D polyList3D[6404];
    static int16_t   gfxPolyListIndex[49152];
    static uint16_t  gfxVertexSize;
    static uint16_t  gfxVertexSizeOpaque;
    static uint16_t  gfxIndexSize;
    static uint16_t  gfxIndexSizeOpaque;
    static uint16_t  vertexSize3D;
    static uint16_t  indexSize3D;
    static float     tileUVArray[4096];

    static float     floor3DPosX, floor3DPosY, floor3DPosZ;
    static float     floor3DAngle;
    static uint16_t  blendLookupTable[8192];
    static uint16_t  subtractiveLookupTable[8192];
    static PaletteEntry tilePalette[256];
    static uint16_t  tilePalette16_Data[8][256];
    static int32_t   texPaletteNum;
    static int32_t   waterDrawPos;
    static bool      videoPlaying;
    static int32_t   currentVideoFrame;

    static void SetScreenRenderSize(int32_t gfxWidth, int32_t gfxPitch);
    static uint16_t RGB_16BIT5551(uint8_t r, uint8_t g, uint8_t b, uint8_t a);
    static void LoadPalette(const char* fileName, int32_t paletteNum,
                            int32_t destPoint, int32_t startPoint, int32_t endPoint);
    static void GenerateBlendLookupTable();
    static void SetActivePalette(uint8_t paletteNum, int32_t startLine, int32_t endLine);
    static void SetPaletteEntry(uint8_t paletteNum, uint8_t index,
                                uint8_t r, uint8_t g, uint8_t b);
    static void CopyPalette(uint8_t paletteSource, uint8_t paletteDest);
    static void RotatePalette(uint8_t pStart, uint8_t pEnd, uint8_t pDir);
    static void SetFade(uint8_t r, uint8_t g, uint8_t b, uint16_t a);
    static void SetFadeHQ(uint8_t r, uint8_t g, uint8_t b, int32_t a);
    static void SetLimitedFade(uint8_t paletteNum, uint8_t r, uint8_t g, uint8_t b,
                               uint16_t a, int32_t fStart, int32_t fEnd);
    static uint8_t AddGraphicsFile(const char* fileName);
    static void RemoveGraphicsFile(const char* filePath, int32_t fileID);
    static void ClearGraphicsData();
    static bool CheckSurfaceSize(int32_t size);
    static void SetupPolygonLists();
    static void UpdateTextureBufferWithTiles();
    static void UpdateTextureBufferWithSortedSprites();
    static void UpdateTextureBufferWithSprites();
    static void LoadBMPFile(const char* fileName, int32_t surfaceNum);
    static void LoadGIFFile(const char* fileName, int32_t surfaceNum);
    static void LoadStageGIFFile(int32_t zNumber);
    static void Copy16x16Tile(int32_t tDest, int32_t tSource);

    static void ClearScreen(uint8_t clearColour);
    static void DrawSprite(int32_t xPos, int32_t yPos, int32_t xSize, int32_t ySize,
                           int32_t uStart, int32_t vStart, int32_t surfaceNum);
    static void DrawSpriteFlipped(int32_t xPos, int32_t yPos, int32_t xSize, int32_t ySize,
                                  int32_t uStart, int32_t vStart, int32_t direction, int32_t surfaceNum);
    static void DrawScaledSprite(uint8_t direction, int32_t xPos, int32_t yPos,
                                 int32_t pivotX, int32_t pivotY, int32_t scaleX, int32_t scaleY,
                                 int32_t xSize, int32_t ySize, int32_t uStart, int32_t vStart, int32_t surfaceNum);
    static void DrawBlendedSprite(int32_t xPos, int32_t yPos, int32_t xSize, int32_t ySize,
                                  int32_t uStart, int32_t vStart, int32_t surfaceNum);
    static void DrawAlphaBlendedSprite(int32_t xPos, int32_t yPos, int32_t xSize, int32_t ySize,
                                       int32_t uStart, int32_t vStart, int32_t alpha, int32_t surfaceNum);
    static void DrawAdditiveBlendedSprite(int32_t xPos, int32_t yPos, int32_t xSize, int32_t ySize,
                                          int32_t uStart, int32_t vStart, int32_t alpha, int32_t surfaceNum);
    static void DrawSubtractiveBlendedSprite(int32_t xPos, int32_t yPos, int32_t xSize, int32_t ySize,
                                             int32_t uStart, int32_t vStart, int32_t alpha, int32_t surfaceNum);
    static void DrawRotatedSprite(int32_t direction, int32_t xPos, int32_t yPos,
                                  int32_t pivotX, int32_t pivotY, int32_t uStart, int32_t vStart,
                                  int32_t xSize, int32_t ySize, int32_t rotAngle, int32_t surfaceNum);
    static void DrawRotoZoomSprite(uint8_t direction, int32_t xPos, int32_t yPos,
                                   int32_t pivotX, int32_t pivotY, int32_t uStart, int32_t vStart,
                                   int32_t xSize, int32_t ySize, int32_t rotAngle, int32_t scale,
                                   int32_t surfaceNum);
    static void DrawScaledChar(uint8_t direction, int32_t xPos, int32_t yPos,
                               int32_t pivotX, int32_t pivotY, int32_t scaleX, int32_t scaleY,
                               int32_t xSize, int32_t ySize, int32_t uStart, int32_t vStart, int32_t surfaceNum);
    static void DrawTexturedQuad(const Quad2D& face, int32_t surfaceNum);
    static void DrawQuad(const Quad2D& face, int32_t color);
    static void DrawRectangle(int32_t xPos, int32_t yPos, int32_t xSize, int32_t ySize,
                              int32_t r, int32_t g, int32_t b, int32_t a);
    static void DrawTintRectangle(int32_t xPos, int32_t yPos, int32_t xSize, int32_t ySize);
    static void DrawTintSpriteMask(int32_t xPos, int32_t yPos, int32_t xSize, int32_t ySize,
                                   int32_t xBegin, int32_t yBegin, int32_t tableNo, int32_t surfaceNum);
    static void DrawScaledTintMask(uint8_t direction, int32_t xPos, int32_t yPos,
                                   int32_t xPivot, int32_t yPivot, int32_t xScale, int32_t yScale,
                                   int32_t xSize, int32_t ySize, int32_t xBegin, int32_t yBegin,
                                   int32_t surfaceNum);

    static void DrawHLineScroll(uint8_t layerID);
    static void DrawVLineScroll(uint8_t layerID);
    static void Draw3DFloor();
    static void DrawFadedBackground();
};

}
