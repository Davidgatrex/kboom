#pragma once

#include <stdint.h>
#include <stddef.h>

void* kmalloc(size_t sz);
void* kmalloc_a(size_t alignment, size_t sz);

void* kcmalloc(size_t n, size_t sz);
void* kcmalloc_a(size_t n, size_t alignment, size_t sz);

void kfree(void* ptr);
void kfree_a(size_t alignment, void* ptr);