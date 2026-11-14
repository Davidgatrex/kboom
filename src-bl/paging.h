#pragma once
#include <stdint.h>

enum page_flags_e
{
    PAGE_PRESENT      = (1 << 0),
    PAGE_WRITEABLE    = (1 << 1),
    PAGE_USER         = (1 << 2),
    PAGE_WRITE_THROUGH= (1 << 3),
    PAGE_UNCACHEABLE  = (1 << 4),
    PAGE_ACCESSED     = (1 << 5),
    PAGE_DIRTY        = (1 << 6),
    PAGE_PS           = (1 << 7), // Page Size (2MB en PD, 1GB en PDPT)
    PAGE_GLOBAL       = (1 << 8),
    PAGE_ALLOCATED    = (1 << 9),
    PAGE_NX           = ((uint64_t)1 << 63)
};

enum page_size_e
{
    PAGE_4KB,
    PAGE_2MB,
    PAGE_1GB
};

// Una entrada de página es simplemente un entero de 64 bits (8 bytes)
typedef uint64_t pte_t;
typedef pte_t* PT_t;
typedef pte_t* PD_t;
typedef pte_t* PDPT_t;
typedef pte_t* PML4_t;

typedef struct
{
    uint64_t OFFSET         : 12;
    uint64_t PT_IDX         : 9;
    uint64_t PD_IDX         : 9;
    uint64_t PDPT_IDX       : 9;
    uint64_t PML4_IDX       : 9;
    uint64_t SIGN_EXTENSION : 16;
} __attribute__((packed)) virtual_address_t;

void map_page(volatile PML4_t pml4, uint64_t _virtual, uint64_t physical, uint64_t flags, enum page_size_e size);
void paging_init();