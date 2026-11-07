#include "memory.h"
#include "asm.h"
#include "panic.h"
#include <string.h>


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

#define PT_POOL_BASE  0x100000ULL
#define PT_POOL_END   0x200000ULL

#define PAGE_SIZE     4096

// from boot.S
extern uint64_t *pml4;

static uint64_t pt_pool_ptr = PT_POOL_BASE;

static uint64_t *pt_pool_alloc(void)
{
    if (pt_pool_ptr + PAGE_SIZE > PT_POOL_END)
        kernel_panic("vmm_init: page table pool exhausted");

    uint64_t *p = (uint64_t *)pt_pool_ptr;
    pt_pool_ptr += PAGE_SIZE;
    memset(p, 0, PAGE_SIZE);
    return p;
}

void vmm_init(void)
{
    uint64_t base    = pmm_managed_base();
    uint64_t raw_end = base + (1024ULL * 1024 * 1024);

    uint64_t start = base & ~0x1FFFFFULL;
    uint64_t end   = (raw_end + 0x1FFFFFULL) & ~0x1FFFFFULL;

    for (uint64_t addr = start; addr < end; addr += 0x200000) {
        uint64_t i4 = PML4_INDEX(addr);
        uint64_t i3 = PDPT_INDEX(addr);
        uint64_t i2 = PD_INDEX(addr);

        if (!(pml4[i4] & PTE_PRESENT)) {
            uint64_t *new_pdpt = pt_pool_alloc();
            pml4[i4] = (uint64_t)new_pdpt | PTE_PRESENT | PTE_WRITABLE;
        }
        uint64_t *pdpt = (uint64_t *)(pml4[i4] & 0x000FFFFFFFFFF000);

        if (!(pdpt[i3] & PTE_PRESENT)) {
            uint64_t *new_pd = pt_pool_alloc();
            pdpt[i3] = (uint64_t)new_pd | PTE_PRESENT | PTE_WRITABLE;
        }
        if (pdpt[i3] & PTE_HUGE)
            continue;

        uint64_t *pd = (uint64_t *)(pdpt[i3] & 0x000FFFFFFFFFF000);

        if (pd[i2] & PTE_PRESENT) {
            if (!(pd[i2] & PTE_HUGE))
                kernel_panic("vmm_init: unexpected 4KB page table");
            if ((pd[i2] & 0x000FFFFFFFE00000) != (addr & 0x000FFFFFFFE00000))
                kernel_panic("vmm_init: conflicting mapping");
            continue;
        }

        pd[i2] = (addr & 0x000FFFFFFFE00000)
               | PTE_PRESENT | PTE_WRITABLE | PTE_HUGE | PTE_GLOBAL;
    }

    uint64_t cr3;
    __asm__ volatile("mov %%cr3, %0" : "=r"(cr3));
    __asm__ volatile("mov %0, %%cr3" : : "r"(cr3) : "memory");
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