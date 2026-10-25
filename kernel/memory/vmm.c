#include "memory.h"
#include "asm.h"


#define PML4_INDEX(v) (((v) >> 39) & 0x1FF)
#define PDPT_INDEX(v) (((v) >> 30) & 0x1FF)
#define PD_INDEX(v)   (((v) >> 21) & 0x1FF)
#define PT_INDEX(v)   (((v) >> 12) & 0x1FF)
#define PAGE_OFFSET(v) ((v) & 0xFFF)

#define PTE_PRESENT   (1ULL << 0)
#define PTE_WRITABLE  (1ULL << 1)
#define PTE_USER      (1ULL << 2)
#define PTE_PWT       (1ULL << 3)
#define PTE_PCD       (1ULL << 4)
#define PTE_ACCESSED  (1ULL << 5)
#define PTE_DIRTY     (1ULL << 6)
#define PTE_HUGE      (1ULL << 7)
#define PTE_GLOBAL    (1ULL << 8)
#define PTE_NX        (1ULL << 63)


void vmm_init()
{
    // already initialized by boot.S
    return;
}


// pte
uint64_t pte_addr(pte_t pte)
{
    return pte & 0x000FFFFFFFFFF000;
}

pte_t pte_make(uint64_t phys, uint64_t flags)
{
    return (phys & 0x000FFFFFFFFFF000)|flags;
}

int pte_present(pte_t pte)
{
    return pte & PTE_PRESENT;
}

int pte_huge(pte_t pte)
{
    return pte & PTE_HUGE;
}


// addr
void *phys_to_virt(uint64_t phys)
{
    return (void*)phys;
}

uint64_t virt_to_phys(void* virt)
{
    return (uint64_t)virt;
}


// tlb
void vmm_flush_tlb(uint64_t vaddr)
{
    invlpg(vaddr);
}


// walk
// ...