# KBOOM
## KBOOM-1
For spec, see docs/kboom1.md

## Usage
Okay, informal guide here.
For the basic thing, just clone the repo and:

- Copy src-bl next to your source directory
- Modify your makefile using src-bl/makefile or src/full-makefile-example as a reference
- Use copy tooling/linker.ld to your source directory
- Use copy tooling/kboom.h to your source directory
- Patch stage2.c so the placeholder include points to the file you just copied
- Add (and patch according to your needs) tooling/example.c somewhere in your kernel
- Make sure to compile your kernel using the provided linker.ld or similar
- Congratulations! If you did everything correctly, your kernel is now ready to run
