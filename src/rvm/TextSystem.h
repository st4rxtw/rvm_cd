#pragma once
#include <cstdint>
#include "FontCharacter.h"
#include "TextMenu.h"

namespace rvm {

struct TextSystem {
    static int32_t       textMenuSurfaceNo;
    static FontCharacter fontCharacterList[1024];

    static void Init();
    static void LoadFontFile(const char* fileName);
    static void LoadTextFile(TextMenu& tMenu, const char* fileName, uint8_t mapCode);
    static void DrawBitmapText(TextMenu& tMenu, int32_t xPos, int32_t yPos,
                               int32_t scale, int32_t spacing,
                               int32_t rowStart, int32_t numRows);
    static void SetupTextMenu(TextMenu& tMenu, int32_t numRows);
    static void AddTextMenuEntry(TextMenu& tMenu, const char* inputTxt);
    static void AddTextMenuEntryMapped(TextMenu& tMenu, const char* inputTxt);
    static void SetTextMenuEntry(TextMenu& tMenu, const char* inputTxt, int32_t rowNum);
    static void EditTextMenuEntry(TextMenu& tMenu, const char* inputTxt, int32_t rowNum);
    static void DrawTextMenuEntry(TextMenu& tMenu, int32_t rowNum,
                                  int32_t xPos, int32_t yPos, int32_t textHighL);
    static void DrawStageTextEntry(TextMenu& tMenu, int32_t rowNum,
                                   int32_t xPos, int32_t yPos, int32_t textHighL);
    static void DrawBlendedTextMenuEntry(TextMenu& tMenu, int32_t rowNum,
                                         int32_t xPos, int32_t yPos, int32_t textHighL);
    static void DrawTextMenu(TextMenu& tMenu, int32_t xPos, int32_t yPos);
    static void LoadConfigListText(TextMenu& tMenu, int32_t listNo);
};

}
