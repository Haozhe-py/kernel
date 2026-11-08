#include <stdint.h>
#include "vga_console.h"
#include "memory.h"

/* From boot.S */
extern unsigned int mb_magic32;
extern unsigned int mb_info32;

/*---------- Entry ----------*/
void kernel_main(void)
{
    vga_clear();
    vga_print("Booting...\n");

    mm_init((uint64_t)mb_magic32, (uint64_t)mb_info32);

    vga_print_int(pmm_free_count());
    vga_print(" pages available.\n");

    /* WARNING: 262144 PAGES IN TOTAL, ENSURE YOUR PC HAVE ENOUGH MEMORY */
    for(uint64_t i=0; i<(PMM_MANAGED_SIZE / PMM_PAGE_SIZE); ++i)
    {
        vga_print("alloc page "); vga_print_int(i); vga_print("\n");
        char *page = pmm_alloc_frame();
        page[0] = '\0';
    }
    vga_print_int(pmm_free_count());
    vga_print(" pages available.\n");
    vga_print("ok.");
}

