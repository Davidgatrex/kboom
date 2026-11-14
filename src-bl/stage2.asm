[bits 32]
section .multiboot
align 4
    dd 0x1BADB002                ; Magic number para Multiboot 1
    dd 1 << 0 | 1 << 1           ; Flags (alineación en páginas + información de memoria)
    dd -(0x1BADB002 + (1 << 0 | 1 << 1)) ; Checksum: magic + flags + checksum = 0

section .text
global _start
extern s2_main
_start:
    ;mov eax, 0x2BADB002
    ;mov ebx, [esp + 4]
    mov esp, stack_top
    mov ebp, esp

    push ebx
    push eax
    call s2_main

; 
global far_to_kernel
far_to_kernel:
    cli
    mov edi, [esp + 4]      ; entry (low)
    mov ebx, [esp + 8]      ; entry (high)
    mov esi, [esp + 12]     ; gdtr
    mov edx, [esp + 16]     ; pml4
    mov ebp, [esp + 20]     ; ldaddr

    mov eax, cr4
    or  eax, 1 << 5         ; PAE
    mov cr4, eax

    mov cr3, edx

    mov ecx, 0xC0000080     ; EFER
    rdmsr
    or  eax, 1 << 8         ; LME
    wrmsr

    mov eax, cr0
    or  eax, 1 << 31        ; PG
    mov cr0, eax

    lgdt [esi]
    jmp 0x08:.amd64

[BITS 64]
.amd64:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov ss, ax
    xor eax, eax
    mov fs, ax
    mov gs, ax

    mov ebx, ebx            ; extiende a cero
    mov edi, edi
    shl rbx, 32
    or  rbx, rdi
    xor edi, edi            ; RDI = m_map (de momento 0; pasa aquí el puntero real)
    mov esi, ebp            ; RSI = dirección física de carga
    jmp rbx

section .bss
align 16

    stack_bottom:
        resb 16384
    stack_top: