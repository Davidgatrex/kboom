#include <stddef.h>
#include "stdint.h"

struct alloc_header
{
    size_t size;
    int isUsed;
    struct alloc_header *Next;
};

// Definimos un tamaño de heap estático para el kernel (ej. 4 MB)
#define HEAP_SIZE (1024 * 1024 * 4)
static uint8_t __attribute__((section(".bss"))) heap[HEAP_SIZE];
static struct alloc_header *head = NULL;

// Inicializa el heap la primera vez que se llama a una función de asignación
static void init_heap_if_needed()
{
    if (head == NULL) {
        head = (struct alloc_header*)heap;
        head->size = HEAP_SIZE - sizeof(struct alloc_header);
        head->isUsed = 0;
        head->Next = NULL;
    }
}

void* kmalloc(size_t sz)
{
    init_heap_if_needed();

    if (sz == 0) return NULL;

    // Alinear tamaño a múltiplos de 4 bytes para evitar problemas de alineación de hardware
    sz = (sz + 3) & ~3;

    struct alloc_header *curr = head;
    while (curr != NULL) {
        if (!curr->isUsed && curr->size >= sz) {
            // Comprobar si podemos dividir el bloque libre sobrante
            if (curr->size > sz + sizeof(struct alloc_header) + 4) {
                struct alloc_header *new_block = (struct alloc_header*)((uint8_t*)(curr + 1) + sz);
                new_block->size = curr->size - sz - sizeof(struct alloc_header);
                new_block->isUsed = 0;
                new_block->Next = curr->Next;

                curr->size = sz;
                curr->Next = new_block;
            }
            curr->isUsed = 1;
            return (void*)(curr + 1); // Retornar el puntero justo después de la cabecera
        }
        curr = curr->Next;
    }

    return NULL; // Sin memoria disponible (Out of Memory)
}

void* kmalloc_a(size_t alignment, size_t sz)
{
    // Necesitamos espacio para el objeto (sz), el margen de alineamiento 
    // y los sizeof(void*) bytes para guardar el puntero original antes de la dirección alineada.
    size_t extra = alignment + sizeof(void*);
    void* ptr = kmalloc(sz + extra);
    if (!ptr) return NULL;

    uintptr_t raw_addr = (uintptr_t)ptr;
    
    // Buscamos la primera dirección múltiplo de 'alignment' que deje al 
    // menos sizeof(void*) espacio libre antes de ella para el puntero guardado.
    uintptr_t aligned_addr = (raw_addr + sizeof(void*) + alignment - 1) & ~(alignment - 1);

    // Guardamos el puntero original justo antes de la dirección alineada
    void** stored_ptr_slot = (void**)(aligned_addr - sizeof(void*));
    *stored_ptr_slot = ptr;

    return (void*)aligned_addr;
}

void* kcmalloc(size_t n, size_t sz)
{
    size_t total = n * sz;
    void* ptr = kmalloc(total);
    if (ptr) {
        // Inicializar memoria a cero (equivalente a calloc)
        uint8_t* p = (uint8_t*)ptr;
        for (size_t i = 0; i < total; i++) {
            p[i] = 0;
        }
    }
    return ptr;
}

void* kcmalloc_a(size_t n, size_t alignment, size_t sz)
{
    size_t total = n * sz;
    void* ptr = kmalloc_a(alignment, total);
    if (ptr) {
        uint8_t* p = (uint8_t*)ptr;
        for (size_t i = 0; i < total; i++) {
            p[i] = 0;
        }
    }
    return ptr;
}

void kfree(void* ptr)
{
    if (!ptr) return;

    // Obtener la cabecera restando el tamaño de la estructura respecto al puntero de datos
    struct alloc_header *header = (struct alloc_header*)ptr - 1;
    header->isUsed = 0;

    // Fusión (Coalescing): Unir bloques libres adyacentes para evitar fragmentación
    struct alloc_header *curr = head;
    while (curr != NULL && (uintptr_t)curr < (uintptr_t)(heap + HEAP_SIZE) && curr->Next != NULL && (uintptr_t)curr->Next < (uintptr_t)(heap + HEAP_SIZE)) {
        if (!curr->isUsed && !curr->Next->isUsed) {
            curr->size += sizeof(struct alloc_header) + curr->Next->size;
            curr->Next = curr->Next->Next;
            continue; // Volver a evaluar por si hay más bloques libres seguidos
        }
        curr = curr->Next;
    }
}

void kfree_a(size_t alignment, void* ptr)
{
    (void)alignment; // No es estrictamente necesario si guardamos la referencia
    if (!ptr) return;

    // Recuperar el puntero original almacenado antes de la dirección alineada
    void** stored_ptr_slot = (void**)((uintptr_t)ptr - sizeof(void*));
    kfree(*stored_ptr_slot);
}