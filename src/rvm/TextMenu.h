#pragma once
#include <cstdint>

namespace rvm {

struct TextMenu {
    char     textData[10240]{};
    int32_t  entryStart[512]{};
    int32_t  entrySize[512]{};
    uint8_t  entryHighlight[512]{};
    int32_t  textDataPos{};
    int32_t  selection1{};
    int32_t  selection2{};
    uint16_t numRows{};
    uint16_t numVisibleRows{};
    uint16_t visibleRowOffset{};
    uint8_t  alignment{};
    uint8_t  numSelections{};
    int8_t   timer{};
};

}
