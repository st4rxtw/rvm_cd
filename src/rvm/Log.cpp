#include "Log.h"
#include <cstdarg>

namespace rvm {

static void log_print(const char* lvl, const char* fmt, va_list ap)
{
    fprintf(stderr, "[%s] ", lvl);
    vfprintf(stderr, fmt, ap);
    fputc('\n', stderr);
}

void Log::Info(const char* fmt, ...)  { va_list a; va_start(a,fmt); log_print("INFO",fmt,a); va_end(a); }
void Log::Warn(const char* fmt, ...)  { va_list a; va_start(a,fmt); log_print("WARN",fmt,a); va_end(a); }
void Log::Error(const char* fmt, ...) { va_list a; va_start(a,fmt); log_print("ERR ",fmt,a); va_end(a); }

}
