# KBOOM-1 Spec
## Preamble
### What is it?

_Note: KBOOM-1 is intended to be used on x86-64 platforms, so all the document will assume an x86-64 architecture with at least an AMD Opteron as CPU. However, it could theoretically be extended for other architectures._

KBOOM-1 _(Kernel BOOtstrap Metadata Version 1.X)_ is a kernel bootstrapping specification intended to serve as an intermediate layer between the bootloader and the kernel, so the kernel finds itself in a predictable and deterministic 64-bit environment.

It is especially (but not only) meant to easen the load process of a 64-bit higher half kernel (with emphasis on BIOS systems, but also UEFI).

### What it is **NOT**

KBOOM-1 is not:
- A bootloader
- A kernel
- A firmware

### Terminological Disambiguation
- "**MUST**": Strict condition. Otherwise, it may or may not boot, depending on the implementation.

- "**SHOULD**": Advised condition. It is advised, but the kernel is guaranteed to boot even if it's not followed. However, the information may or may not keep its complete integrity.

## KBOOM-1 Structures

### Main header

The KBOOM-1 spec has a header through which information is exchanged from the kernel to the bootstrapper and _vice versa_. The header **MUST** be placed at the byte zero of the kernel's flat binary and it **MUST** begin with an 8-Byte magic signature with the string "\_KBOOM1\_" (without null terminator) as its value.

The layout of the header is the following:
```
| Offset (bytes) | Name             | Type (C-style)
|----------------|--------------------------------------------------|
| 0x00           | magic            | char[8]                       |
| 0x08           | virt_base        | uint64_t                      |
| 0x10           | entry            | uint64_t                      |
| 0x18           | size             | uint64_t                      |
| 0x20           | bss_base         | uint64_t                      |
| 0x28           | bss_end          | uint64_t                      |
| 0x30           | video_info       | kboom1_video_info*            |
| 0x38           | region_table     | kboom1_memory_region*         |
| 0x40           | region_table_sz  | uint64_t                      |
|----------------|------------------|-------------------------------|
```

**Where:**

- ```magic```: The identification magic value (```{'_','K','B','O','O','M','1','_'}```).
- ```virt_base```: The base virtual address to which the kernel is linked.
- ```entry```: The virtual address of the entry point.
- ```size```: The file size on disk.
- ```bss_base```: The virtual address of the beginning of ```.bss``` section.
- ```bss_end```: The virtual address of the end of ```.bss``` section.
- ```video_info```: The virtual address of the video information structure where the kernel wants the video information to be placed (```NULL``` for "no video information expected").
- ```region_table```: The virtual address of the memory region structure table where the kernel wants the memory region map to be placed (```NULL``` for "no memory regions expected").
- ```region_table_sz```: The highest number of entries that the region table can hold. If zero, defaulted to 128.

### Memory region (entry, not table. The table is just an array)
```
| Offset (bytes) | Name             | Type (C-style)
|----------------|--------------------------------------------------|
| 0x00           | type             | uint32_t                      |
| 0x04           | base             | uint64_t                      |
| 0x0C           | size             | uint64_t                      |
| 0x14           | next_idx         | uint16_t                      |
| 0x16           | used             | uint16_t                      |
| 0x18           | flags            | uint64_t                      |
|----------------|------------------|-------------------------------|
```

**Where:**

- ```type```: The region type according to BIOS' [E820](https://wiki.osdev.org/Detecting_Memory_(x86)). Type 0 indicates "Free entry".
- ```base```: The physical address where the region starts.
- ```size```: The size of the region in bytes.
- ```next_idx```: The index of the next element. The terminator value is ~0 (0xFFFF).
- ```used```: To hold metadata for the PMA of the bootstrapper (it may vary between implementations. Not reliable). To be ignored by the kernel or used by its own PMA after clearing the field.
- ```flags```: The flags of the region. No flags defined yet.

### Video information
```
| Offset (bytes) | Name             | Type (C-style)
|----------------|--------------------------------------------------|
| 0x00           | fbaddr           | uint64_t                      |
| 0x08           | fbwid            | uint32_t                      |
| 0x0C           | fbhei            | uint32_t                      |
| 0x10           | fbpitch          | uint32_t                      |
| 0x14           | fbbpp            | uint32_t                      |
|----------------|------------------|-------------------------------|
```

**Where:**

- ```fbaddr```: The physical address of the framebuffer.
- ```fbwid```: The width of the framebuffer in pixels.
- ```fbhei```: The height of the framebuffer in pixels.
- ```fbpitch```: The width of a scanline in bytes.
- ```fbbpp```: The bits per pixel of the framebuffer.

## KBOOM-1 ABI Contract
### Aliases
In order to increase comprehension, the following aliases will be used to shorten certain names:

- ```m_hdr``` = Main header.
- ```memtable``` = memory region structure table.
- ```vinfo``` = video information structure.

### Preconditions

- ```m_hdr.magic``` **MUST** have the KBOOM-1 magic value (```{'_','K','B','O','O','M','1','_'}```).

- ```m_hdr.virt_base``` **MUST** be aligned to 2MB. However, the bootstrapper may or may not load it anyway, truncating it to be aligned.

- ```m_hdr.bss_end``` **MUST** be greater or equal to ```m_hdr.bss_base```.

- ```m_hdr.bss_base``` **MUST** be greater or equal to ```m_hdr.virt_base```.

- ```m_hdr.bss_base``` **SHOULD** be greater or equal to ```m_hdr.virt_base``` + ```m_hdr.size```.

- ```m_hdr.entry``` **MUST** belong to the interval [```m_hdr.virt_base```, ```m_hdr.virt_base``` + ```m_hdr.size```).

- ```m_hdr.video_info``` **MUST** belong to the interval [```m_hdr.virt_base```, ```m_hdr.bss_end```).

- ```m_hdr.region_table``` **MUST** belong to the interval [```m_hdr.virt_base```, ```m_hdr.bss_end```).

- ```m_hdr``` **MUST** be placed at the very beginning of the kernel's binary file (offset 0).

- The kernel **MUST** be given as a flat binary file.

### Guarantees

If the previous conditions are fullfilled and the bootstrapper's implementation is not broken, the following statements about the machine's state are guaranteed to be true:

#### Registers
- ```CS``` = ```0x08```
- ```DS, SS, etc.``` = ```0x10```
- ```FS, GS``` = ```0x00```
- ```RIP``` = ```m_hdr.entry```
- ```CR3```: Valid pml4 structure, but only reliable on the short term.
- ```RSP```: Valid stack, but only reliable on the short term.

_Note: RSP and CR3 are not reliable on the long term because the structures may be overwritten by the kernel if untracked._

#### Control tables
- ```IDT```: Null or not reliable.
- ```LDT```: Null or not reliable.
- ```GDT```: Valid, but only reliable on the short term.

_Note: GDT is not reliable on the long term because the structure may be overwritten by the kernel if untracked._

#### Other
- ```.bss```: Initialised to zeroes.
- ```IF``` = ```0```

**Any other structure, register or flag does not have a guaranteed state upon entry**

### Mapping
The initial paging will map, at least, the following ranges: 
- ```m_hdr.virt_base``` -> ```m_hdr.bss_end```
- ```0x0000000000000000``` -> ```0x00000000FFFFFFFF```
- ```vinfo.fbaddr``` -> ```vinfo.fbaddr + vinfo.pitch * vinfo.fbhei```

## Kernel's duties
At the very beginning, the kernel **MUST** set up its own stack, GDT and paging, as well as (optionally) an IDT.
