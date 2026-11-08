#include <stdint.h>
#include <stddef.h>

typedef uint64_t pte_t;

#define PMM_PAGE_SIZE    4096ULL
#define PMM_MANAGED_SIZE (1ULL << 30)

void mm_init(uint64_t mb_magic, uint64_t mb_info);


/*---------- PMM ----------*/
void pmm_init(uint64_t mb_magic, uint64_t mb_info);
void *pmm_alloc_frame(void);
void pmm_free_frame(void*);
void *pmm_alloc_frames(size_t);
void pmm_free_frames(void*, size_t);
size_t pmm_free_count(void);
uint64_t pmm_managed_base(void);


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

// walk & operations
pte_t *vmm_walk(uint64_t *pml4, uint64_t vaddr);
uint64_t *vmm_ensure_table(pte_t *entry, uint64_t flags);
int vmm_map(uint64_t *pml4, uint64_t vaddr, uint64_t phys, uint64_t flags);
int vmm_unmap(uint64_t *pml4, uint64_t vaddr);
uint64_t vmm_get_phys(uint64_t *pml4, uint64_t vaddr);
int vmm_protect(uint64_t *pml4, uint64_t vaddr, uint64_t flags);



/*---------- KHeap ----------*/
void kheap_init();

void *kmalloc(size_t size);
void *kzalloc(size_t size);
void kfree(void *ptr);



/*---------- ASpace ----------*/
void aspace_init();
struct aspace *aspace_create(void);
void aspace_destroy(struct aspace *as);
void aspace_switch(struct aspace *as);
struct aspace *aspace_current(void);
int aspace_map(struct aspace *as, uint64_t vaddr, uint64_t phys, uint64_t flags);
int aspace_unmap(struct aspace *as, uint64_t vaddr);


/*---------- Mapping ----------*/
void mapping_init();