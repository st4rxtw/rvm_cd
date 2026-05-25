#pragma once
#include <cstdio>

namespace rvm {

struct Log {
    static void Info (const char* fmt, ...);
    static void Warn (const char* fmt, ...);
    static void Error(const char* fmt, ...);
};

}
