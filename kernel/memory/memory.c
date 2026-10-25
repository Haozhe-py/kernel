#include <stdint.h>
#include "memory.h"

void mm_init(uint64_t mb_magic, uint64_t mb_info)
{
    pmm_init(mb_magic, mb_info);
    vmm_init();
    kheap_init();
    aspace_init();
    mapping_init();
}