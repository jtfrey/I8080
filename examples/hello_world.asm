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
                    in      0
                    
                    ;
                    ; This code is meant to interface with a CGA device
                    ; mapped at $4000
                    ;
                    mvi     a, 01h
                    sta     4004h       ; enable = 1 (start display)
                    mvi     a, 2
                    sta     400Dh       ; mode = 2
                    
                    lxi     b, 4010h    ; BC <= base address of pixels
                    lda     4001h       ; A <= width of display
                    mov     d, a        ; D <= A = width of display
                    dcr     d           ; pre-decrement D to avoid an extra pixel
                    mvi     a, 3        ; A <= 3 (color index)
Draw:               inr     a
                    stax    b           ; (BC) <= A (store the pixel)
                    inx     b           ; BC <= BC + 1 (next pixel address)
                    dcr     d           ; D <= D - 1
                    jm      Draw_x      ; If no more pixels remain, exit the loop now
                    dcr     a           ; A <= A - 1 = 3 (color index)
                    stax    b           ; (BC) <= A (store the pixel)
                    inx     b           ; BC <= BC + 1 (next pixel address)
                    dcr     d           ; D <= D - 1
                    jp      Draw        ; If any pixels remain, loop again
Draw_x:             
                    in      0           ; wait on a keypress
                    
                    mvi     a, 8Dh      ; A <= 0b10001011 (fill screen with 0x0D)
                    sta     400Eh       ; op = fill w/ 0b1011 = 11
                    in      0           ; wait on a keypress
                    
                    mvi     a, 8Fh      ; A <= 0b10001111 (fill screen with 0x0F)
                    sta     400Eh       ; op = fill w/ 0b1011 = 15
                    in      0           ; wait on a keypress
                    
                    mvi     a, 00h
                    sta     4004h       ; enable = 0 (end display)
                    
                    lxi     b, str0001
                    Halt

str0000:            db      0ah, 0ah, 'Hello', 2ch, ' world!  This is a 8080 assembly '
                    db      'program running on a custom kernel.', 0ah, 0h
str0001:            db      0ah, 'Program complete.', 0ah, 0h

buffer:             ds      8