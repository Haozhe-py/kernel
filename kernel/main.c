#include "vga_console.h"

void kernel_main(void)
{
    *(volatile char*)0xB8000 = 'H'; /* Debug: Ensure function `kernel_main` is called */

    vga_clear();
    for(int i=1; i<=30; ++i){
        vga_print("1\n12\n123\n1234\n12345\n123456\n1234567\n");
    }
    vga_print("Bye");
}

