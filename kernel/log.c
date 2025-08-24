#include "log.h"
#include "stdio.h"   // ta libc printf -> vga
#include "vga.h"
#include <stdarg.h>

static void prefix_and_print(const char* level, const char* fmt, va_list ap){
    char buf[256];
    int n = vsnprintf(buf, sizeof(buf), fmt, ap);
    printf("[%s] %s\n", level, buf);
}

void kernel_log(const char* fmt, ...){
    va_list ap; va_start(ap, fmt); prefix_and_print("LOG", fmt, ap); va_end(ap);
}
void kernel_info(const char* fmt, ...){
    va_list ap; va_start(ap, fmt); prefix_and_print("INFO", fmt, ap); va_end(ap);
}
void kernel_warn(const char* fmt, ...){
    va_list ap; va_start(ap, fmt); prefix_and_print("WARN", fmt, ap); va_end(ap);
}
void kernel_error(const char* fmt, ...){
    va_list ap; va_start(ap, fmt); prefix_and_print("ERROR", fmt, ap); va_end(ap);
}

void kernel_panic(const char* msg){
    // Ecran rouge + msg, halt
    vga_clear_screen();
    vga_draw_text(2,2,"KERNEL PANIC", 0x4F);
    vga_draw_text(2,4,msg, 0x4F);
    for(;;) { __asm__("cli; hlt"); }
}
