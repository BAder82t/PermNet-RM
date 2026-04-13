/* Minimal Cortex-M0 vector table for ELMO simulation. */

    .syntax unified
    .thumb

    .section .vectors, "a"
    .global vector_table
    .align 2

vector_table:
    .word _stack            /* Initial stack pointer */
    .word reset_handler     /* Reset handler */
    .word 0                 /* NMI */
    .word 0                 /* HardFault */
    .word 0                 /* Reserved */
    .word 0                 /* Reserved */
    .word 0                 /* Reserved */
    .word 0                 /* Reserved */
    .word 0                 /* Reserved */
    .word 0                 /* Reserved */
    .word 0                 /* Reserved */
    .word 0                 /* SVCall */
    .word 0                 /* Reserved */
    .word 0                 /* Reserved */
    .word 0                 /* PendSV */
    .word 0                 /* SysTick */

    .text
    .thumb_func
    .global reset_handler
reset_handler:
    /* Zero BSS */
    ldr r0, =_ebss
    movs r1, #0
    ldr r2, =_ebss
    b .Lzbss_check
.Lzbss_loop:
    str r1, [r2]
    adds r2, #4
.Lzbss_check:
    cmp r2, r0
    blo .Lzbss_loop

    /* Call main */
    bl main

    /* Halt */
.Lhalt:
    b .Lhalt
