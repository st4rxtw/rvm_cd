#include "TextSystem.h"
#include "FileIO.h"
#include "GraphicsSystem.h"

namespace rvm {

int32_t       TextSystem::textMenuSurfaceNo      = 0;
FontCharacter TextSystem::fontCharacterList[1024]{};

void TextSystem::Init()
{

}

void TextSystem::LoadFontFile(const char* fileName)
{
    int index = 0;
    FileData fData{};
    if (!FileIO::LoadFile(fileName, fData))
        return;

    while (!FileIO::ReachedEndOfFile()) {
        uint8_t n1 = FileIO::ReadByte();
        fontCharacterList[index].id  = (int32_t)n1;
        uint8_t n2 = FileIO::ReadByte();
        fontCharacterList[index].id += (int32_t)n2 << 8;
        uint8_t n3 = FileIO::ReadByte();
        fontCharacterList[index].id += (int32_t)n3 << 16;
        uint8_t n4 = FileIO::ReadByte();
        fontCharacterList[index].id += (int32_t)n4 << 24;

        uint8_t n5 = FileIO::ReadByte();
        fontCharacterList[index].left  = (int16_t)n5;
        uint8_t n6 = FileIO::ReadByte();
        fontCharacterList[index].left  = (int16_t)((int32_t)fontCharacterList[index].left + ((int32_t)n6 << 8));

        uint8_t n7 = FileIO::ReadByte();
        fontCharacterList[index].top   = (int16_t)n7;
        uint8_t n8 = FileIO::ReadByte();
        fontCharacterList[index].top   = (int16_t)((int32_t)fontCharacterList[index].top + ((int32_t)n8 << 8));

        uint8_t n9 = FileIO::ReadByte();
        fontCharacterList[index].xSize = (int16_t)n9;
        uint8_t n10 = FileIO::ReadByte();
        fontCharacterList[index].xSize = (int16_t)((int32_t)fontCharacterList[index].xSize + ((int32_t)n10 << 8));

        uint8_t n11 = FileIO::ReadByte();
        fontCharacterList[index].ySize = (int16_t)n11;
        uint8_t n12 = FileIO::ReadByte();
        fontCharacterList[index].ySize = (int16_t)((int32_t)fontCharacterList[index].ySize + ((int32_t)n12 << 8));

        uint8_t n13 = FileIO::ReadByte();
        fontCharacterList[index].xPivot = (int16_t)n13;
        uint8_t n14 = FileIO::ReadByte();
        if (n14 > 128) {
            fontCharacterList[index].xPivot = (int16_t)((int32_t)fontCharacterList[index].xPivot + ((int32_t)(n14 - 128) << 8));
            fontCharacterList[index].xPivot = (int16_t)-(32768 - (int32_t)fontCharacterList[index].xPivot);
        } else {
            fontCharacterList[index].xPivot = (int16_t)((int32_t)fontCharacterList[index].xPivot + ((int32_t)n14 << 8));
        }

        uint8_t n15 = FileIO::ReadByte();
        fontCharacterList[index].yPivot = (int16_t)n15;
        uint8_t n16 = FileIO::ReadByte();
        if (n16 > 128) {
            fontCharacterList[index].yPivot = (int16_t)((int32_t)fontCharacterList[index].yPivot + ((int32_t)(n16 - 128) << 8));

            fontCharacterList[index].yPivot = (int16_t)-(32768 - (int32_t)fontCharacterList[index].xPivot);
        } else {
            fontCharacterList[index].yPivot = (int16_t)((int32_t)fontCharacterList[index].yPivot + ((int32_t)n16 << 8));
        }

        uint8_t n17 = FileIO::ReadByte();
        fontCharacterList[index].xAdvance = (int16_t)n17;
        uint8_t n18 = FileIO::ReadByte();
        if (n18 > 128) {
            fontCharacterList[index].xAdvance = (int16_t)((int32_t)fontCharacterList[index].xAdvance + ((int32_t)(n18 - 128) << 8));
            fontCharacterList[index].xAdvance = (int16_t)-(32768 - (int32_t)fontCharacterList[index].xAdvance);
        } else {
            fontCharacterList[index].xAdvance = (int16_t)((int32_t)fontCharacterList[index].xAdvance + ((int32_t)n18 << 8));
        }

        FileIO::ReadByte();
        FileIO::ReadByte();
        ++index;
    }
    FileIO::CloseFile();
}

void TextSystem::LoadTextFile(TextMenu& tMenu, const char* fileName, uint8_t mapCode)
{
    bool flag = false;
    FileData fData{};
    if (!FileIO::LoadFile(fileName, fData))
        return;

    tMenu.textDataPos = 0;
    tMenu.numRows     = 0;
    tMenu.entryStart[tMenu.numRows] = tMenu.textDataPos;
    tMenu.entrySize[tMenu.numRows]  = 0;

    auto mapChar = [&](uint16_t c) -> uint16_t {
        if (mapCode == 1) {
            int i = 0;
            for (; i < 1024; ++i) {
                if (fontCharacterList[i].id == (int32_t)c) { c = (uint16_t)i; i = 1025; }
            }
            if (i == 1024) c = 0;
        }
        return c;
    };

    uint8_t firstByte = FileIO::ReadByte();
    if (firstByte == 0xFF) {

        FileIO::ReadByte();
        while (!flag) {
            uint16_t c = (uint16_t)FileIO::ReadByte() + ((uint16_t)((uint16_t)FileIO::ReadByte() << 8));
            switch (c) {
                case 10:
                    flag = FileIO::ReachedEndOfFile();
                    if (tMenu.textDataPos >= 10240) flag = true;
                    break;
                case 13:
                    ++tMenu.numRows;
                    if (tMenu.numRows > 511) { flag = true; break; }
                    tMenu.entryStart[tMenu.numRows] = tMenu.textDataPos;
                    tMenu.entrySize[tMenu.numRows]  = 0;
                    flag = FileIO::ReachedEndOfFile();
                    if (tMenu.textDataPos >= 10240) flag = true;
                    break;
                default:
                    c = mapChar(c);
                    tMenu.textData[tMenu.textDataPos] = (char)c;
                    ++tMenu.textDataPos;
                    ++tMenu.entrySize[tMenu.numRows];
                    flag = FileIO::ReachedEndOfFile();
                    if (tMenu.textDataPos >= 10240) flag = true;
                    break;
            }
        }
    } else {

        uint16_t c = (uint16_t)firstByte;
        auto processChar = [&](uint16_t ch) {
            switch (ch) {
                case 10:
                    flag = FileIO::ReachedEndOfFile();
                    if (tMenu.textDataPos >= 10240) flag = true;
                    break;
                case 13:
                    ++tMenu.numRows;
                    if (tMenu.numRows > 511) { flag = true; return; }
                    tMenu.entryStart[tMenu.numRows] = tMenu.textDataPos;
                    tMenu.entrySize[tMenu.numRows]  = 0;
                    flag = FileIO::ReachedEndOfFile();
                    if (tMenu.textDataPos >= 10240) flag = true;
                    break;
                default:
                    ch = mapChar(ch);
                    tMenu.textData[tMenu.textDataPos] = (char)ch;
                    ++tMenu.textDataPos;
                    ++tMenu.entrySize[tMenu.numRows];
                    flag = FileIO::ReachedEndOfFile();
                    if (tMenu.textDataPos >= 10240) flag = true;
                    break;
            }
        };
        processChar(c);
        while (!flag) {
            uint16_t ch = (uint16_t)FileIO::ReadByte();
            processChar(ch);
        }
    }
    ++tMenu.numRows;
    FileIO::CloseFile();
}

void TextSystem::DrawBitmapText(TextMenu& tMenu, int32_t xPos, int32_t yPos,
                                 int32_t scale, int32_t spacing,
                                 int32_t rowStart, int32_t numRows)
{
    int32_t y = yPos << 9;
    if (numRows < 0)
        numRows = (int32_t)tMenu.numRows;
    if (rowStart + numRows > (int32_t)tMenu.numRows)
        numRows = (int32_t)tMenu.numRows - rowStart;

    for (; numRows > 0; --numRows) {
        int32_t x   = xPos << 9;
        int32_t sz  = tMenu.entrySize[rowStart];
        for (int i = 0; i < sz; ++i) {
            int fc = (int)(unsigned char)tMenu.textData[tMenu.entryStart[rowStart] + i];
            GraphicsSystem::DrawScaledChar(
                0, x >> 5, y >> 5,
                -(int32_t)fontCharacterList[fc].xPivot,
                -(int32_t)fontCharacterList[fc].yPivot,
                scale, scale,
                (int32_t)fontCharacterList[fc].xSize,
                (int32_t)fontCharacterList[fc].ySize,
                (int32_t)fontCharacterList[fc].left,
                (int32_t)fontCharacterList[fc].top,
                textMenuSurfaceNo);
            x += (int32_t)fontCharacterList[fc].xAdvance * scale;
        }
        y += spacing * scale;
        ++rowStart;
    }
}

void TextSystem::SetupTextMenu(TextMenu& tMenu, int32_t numRows)
{
    tMenu.textDataPos = 0;
    tMenu.numRows     = (uint16_t)numRows;
}

void TextSystem::AddTextMenuEntry(TextMenu& tMenu, const char* inputTxt)
{
    tMenu.entryStart[tMenu.numRows] = tMenu.textDataPos;
    tMenu.entrySize[tMenu.numRows]  = 0;
    for (int i = 0; inputTxt[i] != '\0'; ++i) {
        tMenu.textData[tMenu.textDataPos] = inputTxt[i];
        ++tMenu.textDataPos;
        ++tMenu.entrySize[tMenu.numRows];
    }
    ++tMenu.numRows;
}

void TextSystem::AddTextMenuEntryMapped(TextMenu& tMenu, const char* inputTxt)
{
    tMenu.entryStart[tMenu.numRows] = tMenu.textDataPos;
    tMenu.entrySize[tMenu.numRows]  = 0;
    for (int i = 0; inputTxt[i] != '\0'; ++i) {
        uint16_t c = (uint16_t)(unsigned char)inputTxt[i];
        int idx = 0;
        for (; idx < 1024; ++idx) {
            if (fontCharacterList[idx].id == (int32_t)c) { c = (uint16_t)idx; idx = 1025; }
        }
        if (idx == 1024) c = 0;
        tMenu.textData[tMenu.textDataPos] = (char)c;
        ++tMenu.textDataPos;
        ++tMenu.entrySize[tMenu.numRows];
    }
    ++tMenu.numRows;
}

void TextSystem::SetTextMenuEntry(TextMenu& tMenu, const char* inputTxt, int32_t rowNum)
{
    tMenu.entryStart[rowNum] = tMenu.textDataPos;
    tMenu.entrySize[rowNum]  = 0;
    for (int i = 0; inputTxt[i] != '\0'; ++i) {
        tMenu.textData[tMenu.textDataPos] = inputTxt[i];
        ++tMenu.textDataPos;
        ++tMenu.entrySize[rowNum];
    }
}

void TextSystem::EditTextMenuEntry(TextMenu& tMenu, const char* inputTxt, int32_t rowNum)
{
    int32_t pos = tMenu.entryStart[rowNum];
    tMenu.entrySize[rowNum] = 0;
    for (int i = 0; inputTxt[i] != '\0'; ++i) {
        tMenu.textData[pos] = inputTxt[i];
        ++pos;
        ++tMenu.entrySize[rowNum];
    }
}

void TextSystem::DrawTextMenuEntry(TextMenu& tMenu, int32_t rowNum,
                                    int32_t xPos, int32_t yPos, int32_t textHighL)
{
    int32_t idx = tMenu.entryStart[rowNum];
    for (int i = 0; i < tMenu.entrySize[rowNum]; ++i, ++idx) {
        int c = (int)(unsigned char)tMenu.textData[idx];
        GraphicsSystem::DrawSprite(
            xPos + (i << 3), yPos, 8, 8,
            (c & 15) << 3,
            (c >> 4 << 3) + textHighL,
            textMenuSurfaceNo);
    }
}

void TextSystem::DrawStageTextEntry(TextMenu& tMenu, int32_t rowNum,
                                     int32_t xPos, int32_t yPos, int32_t textHighL)
{
    int32_t idx = tMenu.entryStart[rowNum];
    for (int i = 0; i < tMenu.entrySize[rowNum]; ++i, ++idx) {
        int c = (int)(unsigned char)tMenu.textData[idx];
        if (i == tMenu.entrySize[rowNum] - 1)
            GraphicsSystem::DrawSprite(
                xPos + (i << 3), yPos, 8, 8,
                (c & 15) << 3, c >> 4 << 3,
                textMenuSurfaceNo);
        else
            GraphicsSystem::DrawSprite(
                xPos + (i << 3), yPos, 8, 8,
                (c & 15) << 3, (c >> 4 << 3) + textHighL,
                textMenuSurfaceNo);
    }
}

void TextSystem::DrawBlendedTextMenuEntry(TextMenu& tMenu, int32_t rowNum,
                                           int32_t xPos, int32_t yPos, int32_t textHighL)
{
    int32_t idx = tMenu.entryStart[rowNum];
    for (int i = 0; i < tMenu.entrySize[rowNum]; ++i, ++idx) {
        int c = (int)(unsigned char)tMenu.textData[idx];
        GraphicsSystem::DrawBlendedSprite(
            xPos + (i << 3), yPos, 8, 8,
            (c & 15) << 3, (c >> 4 << 3) + textHighL,
            textMenuSurfaceNo);
    }
}

void TextSystem::DrawTextMenu(TextMenu& tMenu, int32_t xPos, int32_t yPos)
{
    int32_t num;
    if (tMenu.numVisibleRows > 0) {
        num = (int32_t)tMenu.numVisibleRows + (int32_t)tMenu.visibleRowOffset;
    } else {
        tMenu.visibleRowOffset = 0;
        num = (int32_t)tMenu.numRows;
    }
    if (tMenu.numSelections == 3) {
        tMenu.selection2 = -1;
        for (int i = 0; i <= tMenu.selection1; ++i)
            if (tMenu.entryHighlight[i] == 1)
                tMenu.selection2 = i;
    }

    for (int row = (int)tMenu.visibleRowOffset; row < num; ++row) {
        int32_t rx = xPos;
        switch (tMenu.alignment) {
            case 1: rx = xPos - (tMenu.entrySize[row] << 3);           break;
            case 2: rx = xPos - (tMenu.entrySize[row] >> 1 << 3);      break;
        }
        switch (tMenu.numSelections) {
            case 1:
                DrawTextMenuEntry(tMenu, row, rx, yPos,
                    (row == tMenu.selection1) ? 128 : 0);
                break;
            case 2:
                DrawTextMenuEntry(tMenu, row, rx, yPos,
                    (row == tMenu.selection1 || row == tMenu.selection2) ? 128 : 0);
                break;
            case 3:
                DrawTextMenuEntry(tMenu, row, rx, yPos,
                    (row == tMenu.selection1) ? 128 : 0);
                if (row == tMenu.selection2 && row != tMenu.selection1)
                    DrawStageTextEntry(tMenu, row, rx, yPos, 128);
                break;
        }
        yPos += 8;
    }
}

void TextSystem::LoadConfigListText(TextMenu& tMenu, int32_t listNo)
{
    FileData fData{};
    char inputTxt[32]{};
    if (!FileIO::LoadFile("Data/Game/GameConfig.bin", fData))
        return;

    uint8_t num2;
    uint8_t n1 = FileIO::ReadByte();
    for (int i = 0; i < (int)n1; ++i) num2 = FileIO::ReadByte();
    uint8_t n2 = FileIO::ReadByte();
    for (int i = 0; i < (int)n2; ++i) num2 = FileIO::ReadByte();
    uint8_t n3 = FileIO::ReadByte();
    for (int i = 0; i < (int)n3; ++i) num2 = FileIO::ReadByte();
    uint8_t n4 = FileIO::ReadByte();
    for (int i = 0; i < (int)n4; ++i) {
        uint8_t n = FileIO::ReadByte();
        for (int j = 0; j < (int)n; ++j) num2 = FileIO::ReadByte();
    }
    for (int i = 0; i < (int)n4; ++i) {
        uint8_t n = FileIO::ReadByte();
        for (int j = 0; j < (int)n; ++j) num2 = FileIO::ReadByte();
    }
    uint8_t n5 = FileIO::ReadByte();
    for (int i = 0; i < (int)n5; ++i) {
        uint8_t n = FileIO::ReadByte();
        for (int j = 0; j < (int)n; ++j) num2 = FileIO::ReadByte();
        num2 = FileIO::ReadByte(); num2 = FileIO::ReadByte();
        num2 = FileIO::ReadByte(); num2 = FileIO::ReadByte();
    }
    uint8_t n6 = FileIO::ReadByte();
    for (int i = 0; i < (int)n6; ++i) {
        uint8_t n = FileIO::ReadByte();
        for (int j = 0; j < (int)n; ++j) num2 = FileIO::ReadByte();
    }
    uint8_t n7 = FileIO::ReadByte();
    for (int i = 0; i < (int)n7; ++i) {
        uint8_t n = FileIO::ReadByte();
        int idx;
        for (idx = 0; idx < (int)n; ++idx)
            inputTxt[idx] = (char)FileIO::ReadByte();
        inputTxt[idx] = '\0';
        if (listNo == 0) AddTextMenuEntry(tMenu, inputTxt);
    }
    for (int li = 1; li < 5; ++li) {
        uint8_t n8 = FileIO::ReadByte();
        for (int i = 0; i < (int)n8; ++i) {
            uint8_t na = FileIO::ReadByte();
            for (int j = 0; j < (int)na; ++j) num2 = FileIO::ReadByte();
            uint8_t nb = FileIO::ReadByte();
            for (int j = 0; j < (int)nb; ++j) num2 = FileIO::ReadByte();
            uint8_t nc = FileIO::ReadByte();
            int idx;
            for (idx = 0; idx < (int)nc; ++idx)
                inputTxt[idx] = (char)FileIO::ReadByte();
            inputTxt[idx] = '\0';
            uint8_t highlight = FileIO::ReadByte();
            if (listNo == li) {
                tMenu.entryHighlight[i] = highlight;
                AddTextMenuEntry(tMenu, inputTxt);
            }
        }
    }
    FileIO::CloseFile();
}

}
