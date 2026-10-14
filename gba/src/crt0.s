@ Arranque de la ROM: cabecera, pilas, copia de datos y salto a main.
    .section .crt0, "ax"
    .arm
    .global _start
_start:
    b       rom_start
    .fill   156, 1, 0           @ logo (lo rellena gbafix)
    .fill   12, 1, 0            @ título
    .ascii  "VAEL"              @ código de juego
    .ascii  "01"                @ fabricante
    .byte   0x96, 0x00, 0x00
    .fill   7, 1, 0
    .byte   0x00, 0x00          @ versión / checksum (gbafix)
    .byte   0x00, 0x00
rom_start:
    mov     r0, #0x12           @ modo IRQ
    msr     cpsr_c, r0
    ldr     sp, =0x03007FA0
    mov     r0, #0x1F           @ modo sistema
    msr     cpsr_c, r0
    ldr     sp, =0x03007F00
    @ copiar .iwram/.data
    ldr     r0, =__data_lma
    ldr     r1, =__iwram_start
    ldr     r2, =__iwram_end
1:  cmp     r1, r2
    ldrlt   r3, [r0], #4
    strlt   r3, [r1], #4
    blt     1b
    @ copiar .ewram
    ldr     r0, =__ewram_lma
    ldr     r1, =__ewram_start
    ldr     r2, =__ewram_end
2:  cmp     r1, r2
    ldrlt   r3, [r0], #4
    strlt   r3, [r1], #4
    blt     2b
    @ limpiar .bss y .sbss
    mov     r3, #0
    ldr     r1, =__bss_start
    ldr     r2, =__bss_end
3:  cmp     r1, r2
    strlt   r3, [r1], #4
    blt     3b
    ldr     r1, =__sbss_start
    ldr     r2, =__sbss_end
4:  cmp     r1, r2
    strlt   r3, [r1], #4
    blt     4b
    @ manejador de interrupciones
    ldr     r0, =irq_handler
    ldr     r1, =0x03007FFC
    str     r0, [r1]
    ldr     r0, =main
    mov     lr, pc
    bx      r0
5:  b       5b

@ Manejador mínimo: reconoce VBlank y avisa a la BIOS (para VBlankIntrWait).
    .arm
    .align 2
irq_handler:
    mov     r1, #0x04000000
    add     r1, r1, #0x200
    ldrh    r0, [r1, #2]        @ REG_IF
    strh    r0, [r1, #2]        @ reconocer
    ldr     r2, =0x03007FF8     @ BIOS IF
    ldrh    r3, [r2]
    orr     r3, r3, r0
    strh    r3, [r2]
    bx      lr
    .pool
