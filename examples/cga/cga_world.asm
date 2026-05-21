;
; i8e8e -P 0x0400 -S 0x0000 -L examples/cga_demo.bin@0x0400 --cga=@0x4000
;

CGA_BASE            equ     4000h
                    include "cga.inc"
                    include "colors.inc"
                    
ADDTHRESH           equ     21

                    org     0400h
                    
                    mvi     a, 1
                    sta     4004h                   ; enable = 1 (start display)
                    mvi     a, 2
                    sta     400Dh                   ; mode = 2 (color graphics)
                    lda     400Dh
                    cpi     2                       ; did we successfully switch to mode 2?
                    jz      Main                    ; run the main program
                    hlt
                    
                    
Main:
                    mvi     a, CGA_ENABLE_DEFERDRW  ; defer drawing...
                    sta     CGA_ENABLE
                    
                    mvi     a, 80h | COLOR_LT_BLUE  ; sky
                    sta     CGA_OP
                    
                    lxi     hl, CGA_Y
                    lda     CGA_HEIGHT
                    sbi     04h
                    mov     b, a
                    sta     CGA_Y
                    mvi     a, COLOR_BROWN
                    sta     CGA_I
                    mvi     a, CGA_OP_FILLROW
                    sta     CGA_OP
                    inr     b
                    mov     m, b
                    mvi     a, COLOR_LT_GRAY
                    sta     CGA_I
                    mvi     a, CGA_OP_FILLROW
                    sta     CGA_OP
                    inr     b
                    mov     m, b
                    mvi     a, CGA_OP_FILLROW
                    sta     CGA_OP
                    mvi     a, COLOR_DK_GRAY
                    sta     CGA_I
                    inr     b
                    mov     m, b
                    mvi     a, CGA_OP_FILLROW
                    sta     CGA_OP
                    
                    
                    mvi     a, CGA_ENABLE_ON        ; ...trigger drawing
                    sta     CGA_ENABLE
                    
                    in      0

EndDisplay:                    
                    mvi     a, CGA_ENABLE_OFF
                    sta     CGA_ENABLE
                    
                    hlt



;
; The 8-bit multiplier and multiplicand are in
; registers D and E.  On exit, the pair DE
; has the (possibly) 16-bit product.
;
; This algorithm selects the minimum value as the
; multiplier, since that accelerates both the
; repeated addition and factors of 2.  If the
; muliplier is < 16, the repeated addition
; algorithm is used; otherwise, factors of 2 is
; employed.
;
mltalg3:        push        psw                 ; we'll clobber all registers
                push        bc
                push        hl
                mov         a, d                ; A <- D
                cmp         e
                jp          mltalg3_swap        ; D >= E
                jc          mltalg3_chsalg      ; D < E
mltalg3_swap:   mov         a, e                ; E is the smaller value
                mov         e, d                ; swap D into E
                ;
                ; At this point the multipler is in A
                ; and the multiplicand is in E
mltalg3_chsalg: mvi         d, 0h               ; DE contains the 16-bit
                                                ; multiplicand now

                mvi         h, 0h               ; HL <- 0000h
                mvi         l, 0h
                cpi         ADDTHRESH           ; is our multiplier < 16?
                jp          mltalg3_fctr2       ; >= 16
                jc          mltalg3_repaddalg   ; < 16 (when A has bit 7 set)
                
mltalg3_fctr2:  ;push        psw
                ;mvi         a, '2'
                ;out         0
                ;pop         psw
                mvi         b, 8                ; now use D as our loop counter
mltalg3_fctr2nxt:
                rar                             ; A >> 1 -> carry
                jnc         mltalg3_rotmltplcnd ; do not add this factor
                dad         de                  ; HL += DE
mltalg3_rotmltplcnd:
                push        psw                 ; save A
                xra         a                   ; clear carry
                mov         a, e
                ral                             ; carry <- E << 1
                mov         e, a
                mov         a, d
                ral                             ; D << 1 <- carry
                mov         d, a                ; thus, DE = DE * 2
                pop         psw                 ; restore A (the shifted multiplier)
                dcr         b                   ; decrement counter and
                jp          mltalg3_fctr2nxt    ; go do another round
                jmp         mltalg3_exit

mltalg3_repaddalg:
                ;push        psw
                ;mvi         a, 'A'
                ;out         0
                ;pop         psw
                inr         a
mltalg3_repaddnxt:
                dcr         a
                jz          mltalg3_exit
                dad         de
                jmp         mltalg3_repaddnxt
                
mltalg3_exit:   xchg                            ; DE <=> HL
                pop         hl                  ; restore clobbered registers
                pop         bc                  ; and return with the product
                pop         psw                 ; in DE
                ret
mltalg3_end: