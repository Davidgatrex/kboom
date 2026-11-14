#pragma once
#include <stdint.h>

typedef struct
{
    uint32_t type;
    uint64_t base;
    uint64_t size;
    uint16_t next_idx;
    uint16_t used;
    uint64_t flags;
} kboom1_memory_region;

typedef struct
{
    uint64_t fbaddr;
    uint32_t fbwid;
    uint32_t fbhei;
    uint32_t fbpitch;
    uint32_t fbbpp;
} kboom1_video_info;

#define KBOOM_V1_MAGIC {'_','K','B','O','O','M','1','_'}
struct kboom1_header
{
    char magic[8];
    uint64_t virt_base;
    uint64_t entry;
    uint64_t size;
    uint64_t bss_base;
    uint64_t bss_end;
    kboom1_video_info *video_info;
    kboom1_memory_region *region_table;
    uint64_t region_table_sz;
} __attribute__((packed));

#define DEF_KBOOM1(_vb, _e, _s, _bb, _bs, _vi, _rt, _rts) \
    struct kboom1_header __attribute__((section(".kboom_1"))) __kboom1__header__ = \
    {\
        .magic = KBOOM_V1_MAGIC,\
        .virt_base = (uint64_t) _vb, \
        .entry = (uint64_t) _e, \
        .size = (uint64_t) _s, \
        .bss_base = (uint64_t) _bb, \
        .bss_end =(uint64_t) _bs, \
        .video_info = (kboom1_video_info *)_vi, \
        .region_table = (kboom1_memory_region *)_rt, \
        .region_table_sz = (uint64_t)_rts \
    }

#define KBOOM_V1_SUPPORTED
