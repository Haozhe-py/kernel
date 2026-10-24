#include <stdint.h>
#include "vga_console.h"
#include "memory.h"

/* From boot.S */
extern unsigned int mb_magic32;
extern unsigned int mb_info32;

/*---------- Entry ----------*/
void kernel_main(void)
{
    *(volatile char*)0xB8000 = 'H'; /* Debug: Ensure function `kernel_main` is called */

    vga_clear();
    vga_print("Booting...\n");

    pmm_init((uint64_t)mb_magic32, (uint64_t)mb_info32);

    char *page1 = pmm_alloc_frame();
    char *page2 = pmm_alloc_frame();

    if(page1==page2)  vga_print("Error while calling pmm_alloc_frame()");
    else
    {
        vga_print_hex((uint64_t)page1);
        vga_print(" ");
        vga_print_hex((uint64_t)page2);
        vga_print("\n");
        page1[0] = '2';
        page1[1] = '3';
        page1[2] = '\0';

        page2[0] = '4';
        page2[1] = '\0';

        vga_print(page1);
        vga_print(page2);
        pmm_free_frame(page1);
        vga_print(page1);  // should not show any message
    }
}

