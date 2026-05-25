#pragma once
#include <cstdint>

namespace rvm {

struct MusicTrackInfo {
    char     trackName[64]{};
    bool     loop{};
    uint32_t loopPoint{};
};

}
