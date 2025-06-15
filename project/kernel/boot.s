.section .text
.global _start

_start:
    call kernel_main

hang:
    cli
    hlt
    jmp hang
