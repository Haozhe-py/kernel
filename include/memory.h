#include <stdint.h>
#include <stddef.h>

typedef uint64_t pte_t;

void mm_init(uint64_t mb_magic, uint64_t mb_info);


/*---------- PMM ----------*/
void pmm_init(uint64_t mb_magic, uint64_t mb_info);
void *pmm_alloc_frame(void);
void pmm_free_frame(void*);
void *pmm_alloc_frames(size_t);
void pmm_free_frames(void*, size_t);
size_t pmm_free_count(void);


/*---------- VMM ----------*/
void vmm_init();

// pte
uint64_t pte_addr(pte_t);
pte_t pte_make(uint64_t phys, uint64_t flags);
int pte_present(pte_t);
int pte_huge(pte_t);
// addr
void *phys_to_virt(uint64_t);
uint64_t virt_to_phys(void*);
// tlb
void vmm_flush_tlb(uint64_t);

// walk
// ...


/*---------- KHeap ----------*/
void kheap_init();


/*---------- ASpace ----------*/
void aspace_init();


/*---------- Mapping ----------*/
void mapping_init();