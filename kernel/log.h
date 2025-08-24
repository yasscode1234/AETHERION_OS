#ifndef LOG_H
#define LOG_H
#include "types.h"
void kernel_log(const char* fmt, ...);
void kernel_info(const char* fmt, ...);
void kernel_warn(const char* fmt, ...);
void kernel_error(const char* fmt, ...);
void kernel_panic(const char* msg);
#endif
