# KBOOM

## KBOOM-1

For the specification, see docs/kboom1.md.

## What you get

- 64-bit kernel entry
- Memory map
- Framebuffer information
- GRUB-bootable disk image
- No kernel self-relocation required

## Usage

1. Clone the repository.
2. Copy `src-bl` next to your source directory.
3. Use `src-bl/makefile` or `src/full-makefile-example` as a Makefile reference.
4. Copy `tooling/linker.ld` to your source directory.
5. Copy `tooling/kboom.h` to your source directory.
6. Update the include path in `stage2.c` to point to the copied `kboom.h`.
7. Add and adapt `tooling/example.c` to your kernel.
8. Build your kernel using the provided `linker.ld` or an equivalent linker script.

If everything is correct, your kernel is now ready to run.
