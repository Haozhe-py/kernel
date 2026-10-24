#include <stdint.h>
#include <stddef.h>

typedef uint64_t pte_t;

void pmm_init(uint64_t mb_magic, uint64_t mb_info);
void *pmm_alloc_frame(void);
void pmm_free_frame(void*);
void *pmm_alloc_frames(size_t);
void pmm_free_frames(void*, size_t);

size_t pmm_free_count(void);