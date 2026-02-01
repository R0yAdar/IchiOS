# Ich1 いち
My **first** (and probably last) attempt at writing an OS (x86_64)

## Features
* Legacy Bootloader (2 stage)
* Basics: IDT, PIC, PIT
* Drivers: PS/2 (keyboard), PCI, EXT2 (+VFS), AHCI, Framebuffer (VESA), VGA, Serial (QEMU output)
* VMM, PMM, Basic Memory Protection via Paging
* Processes + ELF loader
* Scheduler + Multitasking (+sleep support, idle, blocked, etc.)
* Basic Userspace + Small Syscalls Interface
* Two Userland Programs (seen running simultaneously in the DEMO):
  * A simple example program (prints the color changing title)
  * A basic port of doomgeneric (that I've written a very minimal libc for)



![ichi-demo](https://github.com/user-attachments/assets/07a7b922-f5ca-444f-a585-bf3b5fbc3e50)

*(the gif was downgraded to save space)*

