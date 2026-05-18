
                include "params.inc"

                org     0400h


                mvi     e, M1
                mvi     d, M2
                call    mltalg3
                hlt


;
;  If E = (1 + 1 + … + 1), then
;  D * E = D * (1 + 1 + … + 1)
;        = D + D + … + D
;
; The 8-bit multiplier and multiplicand are in
; registers D and E.  On exit, the pair DE
; has the (possibly) 16-bit product.
;
mltalg1:        push        bc          ; we'll clobber BC and HL
                push        hl
                mvi         h, 0h       ; HL <- 0000h
                mvi         l, 0h
                mov         b, d        ; use B as our loop counter
                                        ; holding D...
                mvi         d, 0h       ; ..and zero-out the MSB of
                                        ; DE
                inr         b           ; we can only catch multiply
                                        ; by zero if we force a test
mltalg1_1:      dcr         b           ; of B == 0 by incrementing
                                        ; then decrementing
                jz          mltalg1_2   ; B == 0, all done
                dad         de          ; HL += DE
                jmp         mltalg1_1   ; next iteration
mltalg1_2:      xchg                    ; DE <=> HL
                pop         hl          ; restore clobbered registers
                pop         bc          ; and return with the product
                ret                     ; in DE


;
;  D * E = D * Sum(i=0…7, E_i * 2^i)
;        = Sum(i=0…7, E_i * D * 2^i)
;        = Sum(i=0…7, E_i * (D << i))
; Since E_i will be 0/1, the multiplication
; is a sum of shifted copies of the multiplier.
;
; The 8-bit multiplier and multiplicand are in
; registers D and E.  On exit, the pair DE
; has the (possibly) 16-bit product.
;
mltalg2:        push        psw         ; we'll clobber all registers
                push        bc
                push        hl
                mvi         h, 0h       ; HL <- 0000h
                mvi         l, 0h
                mvi         b, 0h       ; load the multiplicand, E, into BC
                mov         c, e
                mov         a, d        ; move the multiplier, D, to A
                mvi         d, 8        ; now use D as our loop counter
mltalg2_1:      rar                     ; A >> 1 -> carry
                jnc         mltalg2_2   ; do not add this factor
                dad         bc          ; HL += BC
mltalg2_2:      push        psw         ; save A
                xra         a           ; clear carry
                mov         a, c
                ral                     ; carry <- c << 1
                mov         c, a
                mov         a, b
                ral                     ; b << 1 <- carry
                mov         b, a        ; thus, BC = BC * 2
                pop         psw         ; restore A (the shifted multiplier)
                dcr         d           ; decrement counter and
                jp          mltalg2_1   ; go do another round
                xchg                    ; DE <=> HL
                pop         hl          ; restore clobbered registers
                pop         bc          ; and return with the product
                pop         psw         ; in DE
                ret



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

