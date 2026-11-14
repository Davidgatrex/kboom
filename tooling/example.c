#include "kboom.h"

extern const uint8_t _VIRT_BASE_[];
extern const uint8_t _FILE_SZ_[];
extern const uint8_t _BSS_START_[];
extern const uint8_t _BSS_END_[];
extern void _start(); // Change it to your entry point

#define NUM_REGIONS 256

kboom1_video_info vinfo;
kboom1_memory_region region_table[NUM_REGIONS]

DEF_KBOOM1(&_VIRT_BASE_[0], _start, &_FILE_SZ_[0], &_BSS_START_[0], &_BSS_END_[0], &vinfo, &region_table[0], NUM_REGIONS);
// This will produce a valid KBOOM1-compliant kernel if used along with the provided linker script
