#include "panic.h"
#include "vga_console.h"

void kernel_panic(const char* msg)
{
    vga_clear();
    vga_print("\nKernel Panic: ");
    vga_print(msg);
    while(true)
        __asm__ volatile ("hlt");
}