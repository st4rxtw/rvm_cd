#include "Log.h"
#include <cstdarg>
#include <cstdio>

namespace rvm {

#ifdef __SWITCH__
static FILE* s_logFile = nullptr;
void Log::OpenFile(const char* path) {
    s_logFile = fopen(path, "w");
}
#endif

static void log_print(const char* lvl, const char* fmt, va_list ap)
{
#ifdef __SWITCH__
    FILE* out = s_logFile ? s_logFile : stderr;
    fprintf(out, "[%s] ", lvl);
    vfprintf(out, fmt, ap);
    fputc('\n', out);
    if (s_logFile) fflush(s_logFile);
#else
    fprintf(stderr, "[%s] ", lvl);
    vfprintf(stderr, fmt, ap);
    fputc('\n', stderr);
#endif
}

void Log::Info(const char* fmt, ...)  { va_list a; va_start(a,fmt); log_print("INFO",fmt,a); va_end(a); }
void Log::Warn(const char* fmt, ...)  { va_list a; va_start(a,fmt); log_print("WARN",fmt,a); va_end(a); }
void Log::Error(const char* fmt, ...) { va_list a; va_start(a,fmt); log_print("ERR ",fmt,a); va_end(a); }

}
