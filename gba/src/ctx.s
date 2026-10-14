@ Cambio de contexto para las tareas cooperativas (corrutinas).
@ void ctx_switch(u32* guardar, u32* cargar): guarda r4-r11, sp, lr y carga los del otro contexto.
    .text
    .arm
    .align 2
    .global ctx_switch
    .type ctx_switch, %function
ctx_switch:
    stmia   r0, {r4-r11, sp, lr}
    ldmia   r1, {r4-r11, sp, lr}
    bx      lr
