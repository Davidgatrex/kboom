#include <stdint.h>
#include "mboot.h"
#include "string.h"
#include "paging.h"
#include "alloc.h"
#include "../src/kboom.h"

extern uint64_t _BSS_END_;
extern void far_to_kernel(uint64_t entry, uint32_t gdtr, uint32_t pml4, uint32_t ldaddr);

#define MAX_REGIONS 256
#define VGA_ADDRESS 0xB8000
#define MAX_COLS 80
#define MAX_ROWS 25
#define ALIGN_2MB 0x200000UL

#ifdef LOG_REGIONS
#undef LOG_REGIONS
#endif

int xptr = 0, yptr = 0;

kboom1_memory_region *regions;

#define AF_PRESENT  0b10000000
#define AF_DPL      0b01100000
#define AF_ISDATA   0b00010000 // Descriptor type
#define AF_EXEC     0b00001000
#define AF_RW       0b00000010

#define CODE_ACCESS 0x9A
#define DATA_ACCESS 0x92

#define GF_LONGMODE (1 << 5) // L
#define GF_DB       (1 << 3) // 0 when L is enabled

#define CODE_FLAGS 0xA
#define DATA_FLAGS 0xC

struct __attribute__((packed)) gdt_entry {
    uint16_t limit_low;       // Límite (bits 0-15) - Ignorado en 64-bit
    uint16_t base_low;        // Base (bits 0-15)   - Ignorado en 64-bit
    uint8_t  base_middle;     // Base (bits 16-23)  - Ignorado en 64-bit
    uint8_t  access;          // Byte de acceso (privilegios y tipo)
    uint8_t  flags;
    uint8_t  base_high;       // Base (bits 24-31)  - Ignorado en 64-bit
};

struct __attribute__((packed)) gdt_descriptor {
    uint16_t limit;           // Tamaño de la GDT - 1
    uint64_t base;            // Dirección virtual de la GDT
};

struct gdt_entry __attribute__((aligned(64))) gdt[3];
struct gdt_descriptor __attribute__((aligned(64))) gdtd;

// Función para escribir un carácter en una posición específica (columna, fila)
void print_char(char c, unsigned char color, int col, int row) {
    volatile unsigned char *vga = (volatile unsigned char *) VGA_ADDRESS;
    int index = (row * MAX_COLS + col) * 2;
    
    vga[index] = c;         // Escribir el carácter ASCII
    vga[index + 1] = color; // Escribir el byte de atributos (color)
}

#define VGA_WIDTH  80
#define VGA_HEIGHT 25

static volatile uint16_t *const vga = (volatile uint16_t *)0xB8000;

extern int xptr, yptr;

static void scroll(void)
{
    // Subir todas las filas una posición
    for(int i = 0; i < (VGA_HEIGHT - 1) * VGA_WIDTH; i++)
        vga[i] = vga[i + VGA_WIDTH];

    // Limpiar la última fila
    for(int i = (VGA_HEIGHT - 1) * VGA_WIDTH; i < VGA_HEIGHT * VGA_WIDTH; i++)
        vga[i] = (0x0F << 8) | ' ';

    yptr = VGA_HEIGHT - 1;
}

void print(const char *s)
{
    for(int i = 0; s[i] != 0; i++)
    {
        if(xptr >= VGA_WIDTH || s[i] == '\n')
        {
            xptr = 0;
            yptr++;
        }

        if(yptr >= VGA_HEIGHT)
            scroll();

        if(s[i] == '\n')
            continue;

        print_char(s[i], 0x0F, xptr++, yptr);
    }
}

const char *hexchars = "0123456789ABCDEF";

void uitoh(uint32_t u, char* bf)
{
    for(int i = 0; i < 8; i++)
    {
        bf[7 - i] = hexchars[u & 0xF];
        u >>= 4;
    }
    bf[8] = 0;
}

void printx(uint32_t val)
{
    char buffer[9];
    uitoh(val, buffer);
    print(buffer);
}

void printllx(uint64_t val)
{
    printx(val >> 32);
    printx(val);
}

multiboot_module_t * find_kernel(const multiboot_info_t *mbi)
{
    if (!(mbi->flags & (1 << 3)))
        return 0;

    print("Looking for kernel\n");
    
    const multiboot_module_t *mods =
        (const multiboot_module_t *)(uintptr_t)mbi->mods_addr;

    for (uint32_t i = 0; i < mbi->mods_count; i++) {
        uint32_t mod_sz = mods[i].mod_end - mods[i].mod_start;
        if(mod_sz < sizeof(struct kboom1_header))
            continue;

        struct kboom1_header *kboom = (struct kboom1_header*)(uintptr_t)mods[i].mod_start;
        char localMagicChars[8] = KBOOM_V1_MAGIC;
        uint64_t magic = *(uint64_t*)kboom;
        uint64_t lmagic = *(uint64_t*)localMagicChars;

        if(magic != lmagic)
            continue;

        print("Found KBOOM 1 signature\n");
        return (multiboot_module_t *)&mods[i];
    }
    
    return 0;
}

int scan_memory_map(const multiboot_info_t *mbi) {
    if (!(mbi->flags & (1 << 6)))
        return 1;

    uintptr_t p = mbi->mmap_addr;
    uintptr_t end = mbi->mmap_addr + mbi->mmap_length;

    int idx = -1;
    while (p < end) {
        const multiboot_mmap_entry_t *e = (const multiboot_mmap_entry_t *)p;

        p += e->size + sizeof(e->size);
        
        idx++;
        if(idx >= MAX_REGIONS || e->addr > ~0)
            continue;

        regions[idx].base = e->addr;
        regions[idx].size = e->len;
        regions[idx].type = e->type;
        regions[idx].used = 0;
        regions[idx].next_idx = ~0;

#ifdef LOG_REGIONS
        print("New region (Type, base, size):\n");
        printx(regions[idx].type); print("\n");
        printx(regions[idx].base); print("\n");
        printx(regions[idx].size); print("\n");
#endif
        if(idx > 0)
            regions[idx - 1].next_idx = idx;
    }

    return 0;
}

int find_free_region()
{
    for(int i = 0; i < MAX_REGIONS; i++)
        if(regions[i].type == 0)
            return i;

    return -1;
}

int region_split_at(int region, uint32_t pos)
{
    int new = find_free_region();
    if(new < 0)
        return -1;

    uint32_t offset = pos - regions[region].base;
    uint32_t newsz  = regions[region].size - offset;

    regions[new].type     = regions[region].type;   // <- faltaba
    regions[new].used     = regions[region].used;   // <- faltaba
    regions[new].base     = pos;
    regions[new].size     = newsz;
    regions[new].next_idx = regions[region].next_idx;

    regions[region].size     = offset;
    regions[region].next_idx = new;
    return new;
}

void *find_memory_gap(const multiboot_info_t *mbi, uint32_t alignment, uint32_t size)
{
    uint32_t bss_end = (uint32_t)(uintptr_t)&_BSS_END_;  // si es símbolo del linker

    for (int i = 0; i >= 0 && i < MAX_REGIONS;)
    {
        kboom1_memory_region *reg = &regions[i];
        int prev = i;
        i = reg->next_idx;

        if(reg->base < 0x100000 && reg->base + reg->size > 0x100000)
        {
            // Separar lo que hay bajo 1 MB y continuar con la parte nueva
            int n = region_split_at(prev, 0x100000);
            if(n < 0) return 0;
            i = n;
            continue;
        }

        if(reg->base == 0x100000 && reg->base + reg->size > bss_end)
        {
            // Separar el stage 2 y marcarlo como ocupado
            int n = region_split_at(prev, bss_end);
            if(n < 0) return 0;
            reg->type = 2;      // reservado
            reg->used = 1;
            i = n;
            continue;
        }

        if(reg->type == 1 && !reg->used && reg->base >= 0x100000)
        {
            uint32_t off = (alignment - (reg->base & (alignment - 1))) & (alignment - 1);
            if(reg->size >= size + off)
            {
                reg->used = 1;
                return (void*)(uintptr_t)(reg->base + off);
            }
        }
    }

    return 0;
}

void gdt_setEntry(uint8_t idx, uint8_t flags, uint8_t access)
{
    gdt[idx].access = access;
    gdt[idx].flags = flags << 4 | 0x0F;
    gdt[idx].limit_low = ~0;
    gdt[idx].base_high = 0;
    gdt[idx].base_low = 0;
    gdt[idx].base_middle = 0;
}

void fbinfo_write2user(struct kboom1_header *kboom, multiboot_info_t *mbi)
{
    kboom1_video_info *vinfo = (kboom1_video_info*)(((uint64_t)kboom->video_info) - kboom->virt_base + (uint64_t)kboom);
    if(!vinfo)
        return;

    if(!(mbi->flags & (1 << 12)))
    {
        vinfo->fbaddr = 0;
        return;
    }

    vinfo->fbaddr = mbi->framebuffer_addr;
    vinfo->fbwid = mbi->framebuffer_width;
    vinfo->fbhei = mbi->framebuffer_height;
    vinfo->fbbpp = mbi->framebuffer_bpp;
    vinfo->fbpitch = mbi->framebuffer_pitch;

    volatile uint32_t* fb = (volatile uint32_t*)vinfo->fbaddr;

    for(int i = 0; i < vinfo->fbwid; i++)
        fb[i] = 0xFF00FF;
}

int s2_main(unsigned int magic, multiboot_info_t *mbi)
{
    xptr = 0;
    yptr = 0;

    print("Hello\n");

    multiboot_module_t * kernel_mod = find_kernel(mbi);

    if(!kernel_mod)
    {
        print("Kernel not found\n");
        for(;;);
    }

    struct kboom1_header *kboom_h = (struct kboom1_header *)(uintptr_t)kernel_mod->mod_start;

    uint64_t kcsz = kboom_h->size;
    uint64_t kbsz = kboom_h->bss_end - kboom_h->bss_base;
    uint64_t ktsz = kboom_h->bss_end - kboom_h->virt_base;

    print("Kernel found\n");
    regions = kcmalloc(MAX_REGIONS, sizeof(kboom1_memory_region));
    if(!regions)
    {
        print("Couldn't allocate region map\n");
        for(;;);
    }

    print("Region map allocateed\n");

    if(scan_memory_map(mbi))
    {
        print("Memory map not provided\n");
        for(;;);
    }
    
    void* ldAddr = find_memory_gap(mbi, ALIGN_2MB, kbsz + kcsz);
    if(!ldAddr)
    {
        print("No gap found. Kernel too big\n");
        for(;;);
    }

    print("Gap found\n");

    uint64_t num_pages = ((ALIGN_2MB - 1) + ktsz)/ALIGN_2MB;

    PML4_t pml4 = kcmalloc_a(512, 4096, sizeof(pte_t));

    print("Mapping pages: 0x"); printx(num_pages); print("\n");

    for(int i = 0; i < num_pages; i++)
    {
        map_page(pml4, kboom_h->virt_base + i * ALIGN_2MB, (uintptr_t)(ldAddr + i * ALIGN_2MB), PAGE_WRITEABLE, PAGE_2MB);
    }

    for(int i = 0; i < 2048; i++)
    {
        map_page(pml4, i * ALIGN_2MB, (uintptr_t)(i * ALIGN_2MB), PAGE_WRITEABLE, PAGE_2MB);
    }

    if(kboom_h->region_table)
    {
        kboom1_memory_region *kreg = (kboom1_memory_region *)(((uint64_t)kboom_h->region_table) - kboom_h->virt_base);

        for(int i = 0; i < kboom_h->region_table_sz && i < MAX_REGIONS; i++)
        {
            kreg[i] = regions[i];
        }
    }

    {
        #define total kboom_h->size
        print("Copying Kernel: 0x"); printx(total); print(" bytes\n");

        uint8_t *src = (uint8_t*)kboom_h;
        uint8_t *dst = (uint8_t*)ldAddr;
        for(uint32_t i = 0; i < total; i++)
        {
            dst[i] = src[i];
        }

        #undef total
    }

    {
        #define total kbsz

        uint8_t *dst = (uint8_t*)ldAddr + (kboom_h->bss_base - kboom_h->virt_base);
        for(uint32_t i = 0; i < total; i++)
        {
            dst[i] = 0;
        }

        #undef total
    }

    fbinfo_write2user(ldAddr, mbi);

    print("Kernel copied\n");
    print("Setting up GDT\n");
    gdt_setEntry(0, 0, 0);
    gdt_setEntry(1, CODE_FLAGS, CODE_ACCESS);
    gdt_setEntry(2, DATA_FLAGS, DATA_ACCESS);

    gdtd.base = (uint64_t)&gdt[0];
    gdtd.limit = sizeof(gdt) - 1;

    print("GDT set up\n");
    print("Setting:");
    print("\n    PAE @ CR4[5] = 1");
    print("\n    CR3 = PML4 = 0x"); printx((uint32_t)(uint64_t)pml4);
    print("\n    LME @ EFER[8] = 1");
    print("\n    PAGING @ CR0[31] = 1");
    print("\n    JMP 0x"); printllx(kboom_h->entry);
    print("\n");
    print("And performing far jump to the trampoline and, from it, to the kernel\n");
    print("Stage 2 can't log anymore. Good luck!\n");

    far_to_kernel(kboom_h->entry, (uint32_t)(uint64_t)&gdtd, (uint32_t)(uint64_t)pml4, (uint64_t)ldAddr);

    for(;;);
}
