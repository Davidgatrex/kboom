#include "paging.h"
#include "alloc.h"
#include <stdint.h>
#include <stddef.h>

pte_t pml4_table[512];

// Funciones auxiliares para extraer/inyectar dirección y flags en un uint64_t de forma segura
static inline uint64_t pte_addr(volatile pte_t entry) {
    return (uint64_t)(entry & 0x000FFFFFFFFFF000ULL);
}

static inline void pte_set(volatile pte_t *entry, uint64_t physical, uint64_t flags) {
    // Permite únicamente los 12 bits de flags inferiores y el bit 63 (NX)
    uint64_t clean_flags = (flags & 0xFFFULL) | (flags & (1ULL << 63));
    *entry = (physical & 0x000FFFFFFFFFF000ULL) | clean_flags;
}

static inline int pte_present(pte_t entry) {
    return (entry & PAGE_PRESENT) != 0;
}

static inline int pte_allocated(pte_t entry) {
    return (entry & PAGE_ALLOCATED) != 0;
}

static inline int pte_ps(pte_t entry) {
    return (entry & PAGE_PS) != 0;
}

void free_pt(PT_t pt)
{
    if(!pt) return;
    kfree_a(4096, pt);
}

void free_pd(PD_t pd)
{
    if(!pd) return;
    for(int i = 0; i < 512; i++)
    {
        if (pte_present(pd[i]) && !pte_ps(pd[i]))
        {
            free_pt((PT_t)pte_addr(pd[i]));
        }
    }
    kfree_a(4096, pd);
}

void free_pdpt(PDPT_t pdpt)
{
    if(!pdpt) return;
    for(int i = 0; i < 512; i++)
    {
        if (pte_present(pdpt[i]) && !pte_ps(pdpt[i]))
        {
            free_pd((PD_t)pte_addr(pdpt[i]));
        }
    }
    kfree_a(4096, pdpt);
}

void map_page(volatile PML4_t pml4, uint64_t virtual, uint64_t physical, uint64_t flags, enum page_size_e size)
{
    virtual_address_t virt = *(virtual_address_t*)&virtual;
    uint64_t base_flags = PAGE_PRESENT | PAGE_WRITEABLE | PAGE_USER; // Permisos por defecto para tablas intermedias

    // 1. PML4 -> PDPT
    if(!pte_present(pml4[virt.PML4_IDX]))
    {
        volatile PDPT_t new_pdpt = (PDPT_t)kcmalloc_a(4096, 4096, 1);
        pte_set(&pml4[virt.PML4_IDX], (uint64_t)new_pdpt, base_flags);
    }
    pml4[virt.PML4_IDX] |= PAGE_PRESENT;
    volatile PDPT_t pdpt = (PDPT_t)pte_addr(pml4[virt.PML4_IDX]);

    if(size == PAGE_1GB)
    {
        if(pte_present(pdpt[virt.PDPT_IDX]) && !pte_ps(pdpt[virt.PDPT_IDX]) && pte_allocated(pdpt[virt.PDPT_IDX]))
        {
            free_pd((PD_t)pte_addr(pdpt[virt.PDPT_IDX]));
        }
        pte_set(&pdpt[virt.PDPT_IDX], physical, PAGE_PRESENT | PAGE_PS | flags);
        return;
    }

    // 2. PDPT -> PD
    if(!pte_present(pdpt[virt.PDPT_IDX]) || pte_ps(pdpt[virt.PDPT_IDX]))
    {
        volatile PD_t new_pd = (PD_t)kcmalloc_a(4096, 4096, 1);
        pte_set(&pdpt[virt.PDPT_IDX], (uint64_t)new_pd, base_flags);
    }
    pdpt[virt.PDPT_IDX] |= PAGE_PRESENT;
    volatile PD_t pd = (PD_t)pte_addr(pdpt[virt.PDPT_IDX]);

    if(size == PAGE_2MB)
    {
        if(pte_present(pd[virt.PD_IDX]) && !pte_ps(pd[virt.PD_IDX]) && pte_allocated(pd[virt.PD_IDX]))
        {
            free_pt((PT_t)pte_addr(pd[virt.PD_IDX]));
        }
        pte_set(&pd[virt.PD_IDX], physical, PAGE_PRESENT | PAGE_PS | flags);
        return;
    }

    // 3. PD -> PT (4KB)
    if(!pte_present(pd[virt.PD_IDX]) || pte_ps(pd[virt.PD_IDX]))
    {
        volatile PT_t new_pt = (PT_t)kcmalloc_a(4096, 4096, 1);
        pte_set(&pd[virt.PD_IDX], (uint64_t)new_pt, base_flags);
    }
    pd[virt.PD_IDX] |= PAGE_PRESENT;
    volatile PT_t pt = (PT_t)pte_addr(pd[virt.PD_IDX]);

    pte_set(&pt[virt.PT_IDX], physical, PAGE_PRESENT | flags);
}