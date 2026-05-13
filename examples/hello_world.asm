;
; i8e8e -P 0x0400 -S 0x0000 -L examples/hello_world.bin@0x0400 -R examples/kernel.bin@0x0000
;

KrnlFn              MACRO
                    rst     1
                    ENDM
LDAIdxIndrct        MACRO
                    rst     2
                    ENDM
STAIdxIndrct        MACRO
                    rst     3
                    ENDM
Halt                MACRO
                    jmp     0040h
                    ENDM

                    org     0400h
                    
                    lxi     b, str0000
                    mvi     d, 00h
                    mvi     e, 00h
                    KrnlFn
                    
                    
                    lxi     h, buffer
                    mvi     b, 8
Loop:               in      10              ; read a random byte
                    mov     M, a
                    inx     hl
                    dcr     b
                    jnz     Loop

                    
                    mvi     d, 00h
                    mvi     e, 01h
                    mvi     c, 08Ch
                    KrnlFn
                    mov     a, b
                    out     0
                    mov     a, c
                    out     0
                    
                    lxi     b, str0001
                    Halt

str0000:            db      0ah, 0ah, 'Hello', 2ch, ' world!  This is a 8080 assembly '
                    db      'program running on a custom kernel.', 0ah, 0h
str0001:            db      0ah, 'Program complete.', 0ah, 0h

buffer:             ds      8