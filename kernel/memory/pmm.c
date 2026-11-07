#include <stdint.h>
#include "memory.h"
#include "panic.h"

/* From linker.ld */
extern uint8_t _kernel_start;
extern uint8_t _kernel_end;

#define MAX_FRAMES                    (PMM_MANAGED_SIZE / PMM_PAGE_SIZE)
#define BITMAP_WORDS                  (MAX_FRAMES / 32)

#define MULTIBOOT2_MAGIC              0x36D76289
#define MULTIBOOT_TAG_TYPE_END        0
#define MULTIBOOT_TAG_TYPE_MODULE     3
#define MULTIBOOT_TAG_TYPE_MMAP       6
#define MULTIBOOT_MEMORY_AVAILABLE    1

struct multiboot_tag
{
    uint32_t type;
    uint32_t size;
};

struct multiboot_mmap_entry
{
    uint64_t addr;
    uint64_t len;
    uint32_t type;
    uint32_t zero;
} __attribute__((packed));

struct multiboot_tag_mmap
{
    uint32_t type;
    uint32_t size;
    uint32_t entry_size;
    uint32_t entry_version;
    struct multiboot_mmap_entry entries[];
};

struct multiboot_tag_module
{
    uint32_t type;
    uint32_t size;
    uint32_t mod_start;
    uint32_t mod_end;
};

struct multiboot_info
{
    uint32_t total_size;
    uint32_t reserved;
};

static uint32_t frame_bitmap[BITMAP_WORDS];
static uint64_t managed_base;

static uint64_t align_up_page(uint64_t addr)
{
    if(addr > UINT64_MAX - (PMM_PAGE_SIZE - 1))
        return UINT64_MAX;
    return (addr + PMM_PAGE_SIZE - 1) & ~(PMM_PAGE_SIZE - 1);
}

static uint64_t mmap_entry_end(const struct multiboot_mmap_entry *entry)
{
    if(entry->len > UINT64_MAX - entry->addr)
        return UINT64_MAX;
    return entry->addr + entry->len;
}

static struct multiboot_tag_mmap *find_mmap_tag(struct multiboot_info *mbi, uint8_t **mb_end_out)
{
    if(mbi->total_size < sizeof(*mbi))
        kernel_panic("Invalid MB2 info size! ");

    uint8_t *tag_ptr = (uint8_t *)mbi + sizeof(*mbi);
    uint8_t *mb_end = (uint8_t *)mbi + mbi->total_size;
    struct multiboot_tag_mmap *mmap_tag = NULL;

    while(tag_ptr <= mb_end && (size_t)(mb_end - tag_ptr) >= sizeof(struct multiboot_tag))
    {
        struct multiboot_tag *tag = (struct multiboot_tag *)tag_ptr;
        size_t remaining = (size_t)(mb_end - tag_ptr);
        if(tag->size < sizeof(*tag) || tag->size > remaining)
            kernel_panic("Invalid MB2 tag size! ");

        if(tag->type == MULTIBOOT_TAG_TYPE_END)
            break;
        if(tag->type == MULTIBOOT_TAG_TYPE_MMAP && !mmap_tag)
            mmap_tag = (struct multiboot_tag_mmap *)tag;

        uint64_t aligned_size = ((uint64_t)tag->size + 7) & ~7ULL;
        if(aligned_size > remaining)
            kernel_panic("Invalid MB2 tag alignment! ");
        tag_ptr += aligned_size;
    }

    if(!mmap_tag)
        kernel_panic("MMap not found! ");
    if(mmap_tag->size < sizeof(*mmap_tag) ||
       mmap_tag->entry_size < sizeof(struct multiboot_mmap_entry) ||
       (mmap_tag->size - sizeof(*mmap_tag)) % mmap_tag->entry_size != 0)
        kernel_panic("Invalid MMap tag! ");

    *mb_end_out = mb_end;
    return mmap_tag;
}

static int find_next_available_start(const struct multiboot_tag_mmap *mmap_tag, uint64_t search_from, uint64_t *start_out)
{
    uint64_t earliest = UINT64_MAX;
    uint8_t *entry_ptr = (uint8_t *)mmap_tag->entries;
    uint8_t *mmap_end = (uint8_t *)mmap_tag + mmap_tag->size;

    while(entry_ptr < mmap_end)
    {
        struct multiboot_mmap_entry *entry =
            (struct multiboot_mmap_entry *)entry_ptr;
        if(entry->type == MULTIBOOT_MEMORY_AVAILABLE && entry->len != 0)
        {
            uint64_t entry_end = mmap_entry_end(entry);
            uint64_t candidate = entry->addr > search_from ? entry->addr : search_from;
            candidate = align_up_page(candidate);
            if(candidate < entry_end && candidate < earliest)
                earliest = candidate;
        }
        entry_ptr += mmap_tag->entry_size;
    }

    if(earliest == UINT64_MAX)
        return 0;
    *start_out = earliest;
    return 1;
}

static int range_is_available(const struct multiboot_tag_mmap *mmap_tag, uint64_t start, uint64_t end, uint64_t *retry_from)
{
    uint64_t cursor = start;
    uint8_t *entry_ptr;
    uint8_t *mmap_end = (uint8_t *)mmap_tag + mmap_tag->size;

    while(cursor < end)
    {
        uint64_t available_end = cursor;
        uint64_t unavailable_end = cursor;
        entry_ptr = (uint8_t *)mmap_tag->entries;

        while(entry_ptr < mmap_end)
        {
            struct multiboot_mmap_entry *entry =
                (struct multiboot_mmap_entry *)entry_ptr;
            uint64_t entry_end = mmap_entry_end(entry);
            if(entry->len != 0 && entry->addr <= cursor && cursor < entry_end)
            {
                if(entry->type == MULTIBOOT_MEMORY_AVAILABLE)
                {
                    if(entry_end > available_end)
                        available_end = entry_end;
                }
                else if(entry_end > unavailable_end)
                    unavailable_end = entry_end;
            }
            entry_ptr += mmap_tag->entry_size;
        }

        if(unavailable_end > cursor)
        {
            *retry_from = align_up_page(unavailable_end);
            return 0;
        }
        if(available_end == cursor)
        {
            *retry_from = cursor;
            return 0;
        }

        uint64_t next_unavailable_start = available_end;
        uint64_t next_unavailable_end = available_end;
        entry_ptr = (uint8_t *)mmap_tag->entries;
        while(entry_ptr < mmap_end)
        {
            struct multiboot_mmap_entry *entry =
                (struct multiboot_mmap_entry *)entry_ptr;
            if(entry->type != MULTIBOOT_MEMORY_AVAILABLE && entry->len != 0 &&
               entry->addr > cursor && entry->addr < next_unavailable_start)
            {
                next_unavailable_start = entry->addr;
                next_unavailable_end = mmap_entry_end(entry);
            }
            else if(entry->type != MULTIBOOT_MEMORY_AVAILABLE && entry->len != 0 &&
                    entry->addr == next_unavailable_start &&
                    mmap_entry_end(entry) > next_unavailable_end)
                next_unavailable_end = mmap_entry_end(entry);
            entry_ptr += mmap_tag->entry_size;
        }

        if(next_unavailable_start < available_end)
        {
            *retry_from = align_up_page(next_unavailable_end);
            return 0;
        }
        cursor = available_end;
    }
    return 1;
}

static void check_reserved_overlap(uint64_t start, uint64_t end, uint64_t reserved_start, uint64_t reserved_end, uint64_t *retry_from)
{
    if(reserved_start < end && start < reserved_end && reserved_end > *retry_from)
        *retry_from = reserved_end;
}

static int range_overlaps_reserved(
    uint64_t start,
    uint64_t end,
    struct multiboot_info *mbi,
    uint8_t *mb_end,
    uint64_t *retry_from)
{
    *retry_from = 0;
    check_reserved_overlap(start, end, 0, PMM_PAGE_SIZE, retry_from);
    check_reserved_overlap(
        start, end, (uint64_t)&_kernel_start, (uint64_t)&_kernel_end, retry_from);
    check_reserved_overlap(
        start, end, (uint64_t)mbi, (uint64_t)mbi + mbi->total_size, retry_from);

    uint8_t *tag_ptr = (uint8_t *)mbi + sizeof(*mbi);
    while(tag_ptr <= mb_end && (size_t)(mb_end - tag_ptr) >= sizeof(struct multiboot_tag))
    {
        struct multiboot_tag *tag = (struct multiboot_tag *)tag_ptr;
        size_t remaining = (size_t)(mb_end - tag_ptr);
        if(tag->size < sizeof(*tag) || tag->size > remaining)
            kernel_panic("Invalid MB2 tag size! ");
        if(tag->type == MULTIBOOT_TAG_TYPE_END)
            break;
        if(tag->type == MULTIBOOT_TAG_TYPE_MODULE)
        {
            if(tag->size < sizeof(struct multiboot_tag_module))
                kernel_panic("Invalid MB2 module tag! ");
            struct multiboot_tag_module *module =
                (struct multiboot_tag_module *)tag;
            check_reserved_overlap(
                start, end, module->mod_start, module->mod_end, retry_from);
        }
        uint64_t aligned_size = ((uint64_t)tag->size + 7) & ~7ULL;
        if(aligned_size > remaining)
            kernel_panic("Invalid MB2 tag alignment! ");
        tag_ptr += aligned_size;
    }

    return *retry_from != 0;
}

static int bitmap_test(uint64_t frame)
{
    return (frame_bitmap[frame / 32] >> (frame % 32)) & 1u;
}

static void bitmap_set(uint64_t frame)
{
    frame_bitmap[frame / 32] |= (1u << (frame % 32));
}

static void bitmap_clear(uint64_t frame)
{
    frame_bitmap[frame / 32] &= ~(1u << (frame % 32));
}

void pmm_init(uint64_t mb_magic, uint64_t mb_info)
{
    for(uint32_t i = 0; i < BITMAP_WORDS; ++i)
        frame_bitmap[i] = 0xFFFFFFFF;

    if(mb_magic != MULTIBOOT2_MAGIC)
        kernel_panic("Wrong MB2 magic! ");

    struct multiboot_info *mbi = (struct multiboot_info *)mb_info;
    uint8_t *mb_end;
    struct multiboot_tag_mmap *mmap_tag = find_mmap_tag(mbi, &mb_end);

    uint64_t search_from = 0;
    uint64_t candidate = 0;
    for(;;)
    {
        if(!find_next_available_start(mmap_tag, search_from, &candidate) ||
           candidate > UINT64_MAX - PMM_MANAGED_SIZE)
            kernel_panic("No contiguous 1GiB usable memory region! ");

        uint64_t candidate_end = candidate + PMM_MANAGED_SIZE;
        uint64_t retry_from;
        if(range_overlaps_reserved(
               candidate, candidate_end, mbi, mb_end, &retry_from))
        {
            search_from = align_up_page(retry_from);
            continue;
        }
        if(range_is_available(mmap_tag, candidate, candidate_end, &retry_from))
        {
            managed_base = candidate;
            break;
        }
        search_from = retry_from;
    }

    for(uint64_t i = 0; i < MAX_FRAMES; ++i)
        bitmap_clear(i);
}

uint64_t pmm_managed_base(void)
{
    return managed_base;
}

void *pmm_alloc_frame(void)
{
    for(uint64_t i = 0; i < MAX_FRAMES; ++i)
    {
        if(!bitmap_test(i))
        {
            bitmap_set(i);
            return (void *)(managed_base + i * PMM_PAGE_SIZE);
        }
    }
    kernel_panic("No unused frames! ");
    return (void *)0;  // will never return
}

void pmm_free_frame(void *addr)
{
    uint64_t address = (uint64_t)addr;
    if(address < managed_base ||
       address - managed_base >= PMM_MANAGED_SIZE ||
       (address - managed_base) % PMM_PAGE_SIZE != 0)
        kernel_panic("Frame address is outside the PMM region! ");

    uint64_t frame = (address - managed_base) / PMM_PAGE_SIZE;
    for(uint64_t ptr = address; ptr < address + PMM_PAGE_SIZE; ptr += sizeof(uint64_t))
        *(uint64_t *)ptr = 0;
    bitmap_clear(frame);
}

void *pmm_alloc_frames(size_t)
{
    kernel_panic("function pmm_alloc_frames(size_t) is still developing");
    return (void *)0;
}

void pmm_free_frames(void *addr, size_t count)
{
    uint8_t *p = (uint8_t *)addr;
    for(uint64_t i = 0; i < count && i < MAX_FRAMES; ++i)
    {
        pmm_free_frame(p);
        p += PMM_PAGE_SIZE;
    }
}

size_t pmm_free_count(void)
{
    size_t count = 0;
    for(uint64_t i = 0; i < MAX_FRAMES; ++i)
        if(!bitmap_test(i))
            count++;
    return count;
}
