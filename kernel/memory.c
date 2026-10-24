#include <stdint.h>
#include "memory.h"
#include "panic.h"
#include "vga_console.h"

/* From linker.ld */
extern uint8_t _kernel_start;
extern uint8_t _kernel_end;


#define PAGE_SIZE                   4096                          // 4.0 KiB each
#define MAX_FRAMES                  (1024*1024*1024 / PAGE_SIZE)  // 1.0 GiB in total
#define FRAMES_BEGIN                0x00
#define BITMAP_WORDS                (MAX_FRAMES / 32)

#define MULTIBOOT2_MAGIC            0x36D76289
#define MULTIBOOT_TAG_TYPE_END      0
#define MULTIBOOT_TAG_TYPE_MMAP     6
#define MULTIBOOT_MEMORY_AVAILABLE  1


/*---------- Multiboot2 Infos ----------*/
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

struct multiboot_info
{
    uint32_t total_size;
    uint32_t reserved;
};


/*---------- Bitmap Operations ----------*/
static uint32_t frame_bitmap[BITMAP_WORDS];

static inline void bitmap_set(uint64_t frame)
{
    frame_bitmap[frame / 32] |= (1u << (frame % 32));
}

static inline void bitmap_clear(uint64_t frame)
{
    frame_bitmap[frame / 32] &= ~(1u << (frame % 32));
}

static inline int bitmap_test(uint64_t frame)
{
    return (frame_bitmap[frame / 32] >> (frame % 32)) & 1u;
}

static void mark_region_used(uint64_t start, uint64_t end)
{
    uint64_t first = start / PAGE_SIZE;
    uint64_t last = (end+PAGE_SIZE-1) / PAGE_SIZE;
    for(uint64_t i = first; i<last && i<MAX_FRAMES; ++i)
        bitmap_set(i);
}


/*---------- PMM ----------*/
/* From include/memory.h */
void pmm_init(uint64_t mb_magic, uint64_t mb_info)
{
    // set all frames as used
    for(int i=0; i<BITMAP_WORDS; ++i)
        frame_bitmap[i] = 0xFFFFFFFF;
    
    // check mb2 magic
    if(mb_magic != MULTIBOOT2_MAGIC)
        kernel_panic("Wrong MB2 magic! ");

    // find mmap
    struct multiboot_info *mbi = (struct multiboot_info *)mb_info;
    uint8_t *tag_ptr = (uint8_t *)mbi + sizeof(struct multiboot_info);
    uint8_t *end_ptr = (uint8_t *)mbi + mbi->total_size;
    struct multiboot_tag_mmap *mmap_tag = NULL;

    while(tag_ptr < end_ptr)
    {
        struct multiboot_tag *tag = (struct multiboot_tag *)tag_ptr;
        if(tag->type == MULTIBOOT_TAG_TYPE_END)
            break;
        if(tag->type == MULTIBOOT_TAG_TYPE_MMAP)
        {
            mmap_tag = (struct multiboot_tag_mmap *)tag;
            break;
        }
        tag_ptr += (tag->size + 7) & ~7;
    }

    if(!mmap_tag)
        kernel_panic("MMap not found! ");
    
    // mark usable regions according to mmap
    uint8_t *entry_ptr = (uint8_t *)mmap_tag->entries;
    uint8_t *mmap_end  = (uint8_t *)mmap_tag + mmap_tag->size;

    while(entry_ptr < mmap_end)
    {
        struct multiboot_mmap_entry *entry = (struct multiboot_mmap_entry *)entry_ptr;

        if(entry->type == MULTIBOOT_MEMORY_AVAILABLE)
        {
            uint64_t start = entry->addr;
            uint64_t end   = entry->addr + entry->len;

            uint64_t first = (start + PAGE_SIZE - 1) / PAGE_SIZE;
            uint64_t last  = end / PAGE_SIZE;

            for(uint64_t i = first; i<last && i<MAX_FRAMES; ++i)
            {
                bitmap_clear(i);
            }
        }
        entry_ptr += mmap_tag->entry_size;
    }

    // protect frame 0
    bitmap_set(0);

    // mark kernel & mb2 regions as used
    mark_region_used(_kernel_start, _kernel_end);
    mark_region_used((uint64_t)mb_info, (uint64_t)mb_info + mbi->total_size);
}

void *pmm_alloc_frame(void)
{
    for(uint64_t i=0; i<MAX_FRAMES; ++i)
    {
        if(!bitmap_test(i))
        {
            bitmap_set(i);
            return (void*)(i*PAGE_SIZE+FRAMES_BEGIN);
        }
    }
    kernel_panic("No unused frames! ");
    return (void*)0; // will never return
}

void pmm_free_frame(void* addr)
{
    uint64_t frame = ((uint64_t)addr - FRAMES_BEGIN) / PAGE_SIZE;
    uint64_t end = (frame+1) * PAGE_SIZE + FRAMES_BEGIN;

    for(
        uint64_t ptr = frame * PAGE_SIZE + FRAMES_BEGIN;
        ptr < end;
        ptr+=8
    )
        *(uint64_t*)ptr = 0;

    bitmap_clear(frame);
}

void *pmm_alloc_frames(size_t)
{
    kernel_panic("function pmm_alloc_frames is still developing");
    return (void*)0; // will never return
}

void pmm_free_frames(void *addr, size_t count)
{
    uint8_t *p = (uint8_t *)addr;
    for(uint64_t i=0; i<count && i<MAX_FRAMES; ++i)
    {
        pmm_free_frame(p);
        p += PAGE_SIZE;
    }
}

size_t pmm_free_count(void)
{
    size_t count = 0;
    for(uint64_t i=0; i<MAX_FRAMES; ++i)
        if(!bitmap_test(i))  count++;
    return count;
}