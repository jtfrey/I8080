
                include "params.inc"

                org 0400h
                
                mvi     d, NUMERATOR
                mvi     e, DENOMINATOR
                call    div
                hlt


div:            push    bc
                mov     b, a
                mov     a, d
                sub     e           ; the difference between numerator
                cpi     ALGTHRESH   ; and denominator determines which
                mov     a, b        ; algorithm to use
                pop     bc
                jc      rptdsub
                jmp     divmod8


;
; rptdsub
; 8-bit integer divide, returning remainder.  Repeated
; subtraction is used for a simple, tight loop.
;
; On input, the two integers are in D (numerator) and
; E (denominator).
;
; On return, the quotient is in D and the remainder
; is in E.
;
rptdsub:        push    psw
                push    bc
                mvi     a, '-'
                out     0
                inr     e
                dcr     e
                jz      rptdsub_exit0
                mov     a, d                ; A <= D (numerator)
                cpi     00h
                jz      rptdsub_exit0       ; numerator is zero
                cmp     e
                jc      rptdsub_exitrem     ; numerator < denominator
                mvi     b, 00h
rptdsub_loop:   sub     e                   ; A <= A - E (denominator)
                inr     b                   ; B <= B + 1 (increment quotient)
                cmp     e
                jnc     rptdsub_loop
                mov     e, a
                mov     d, b
                jmp     rptdsub_exit
rptdsub_exitrem:
                mov     e, d
                mvi     d, 00h
                jmp     rptdsub_exit
rptdsub_exit0:  mvi     d, 00h
                mvi     e, 00h
rptdsub_exit:   pop     bc
                pop     psw
                ret

;
; divmod8
; 8-bit integer divide, returning remainder.  Bit shifts
; are employed to align the MSb of the numerator and
; denominator, then binary long division is employed
; for just the width of the denominator.
;
; On input, the two integers are in D (numerator) and
; E (denominator).
;
; On return, the quotient is in D and the remainder
; is in E.
;
divmod8:        mvi     a, '8'
                out     0
                inr     e               ; is the divisor
                dcr     e               ; zero? (10 cycles)
                jnz     divmod8_0dvdnd  ; non-zero, not trivial
divmod8_exit0:  mvi     d, 00h          ; it's not correct, but
                mvi     e, 00h          ; there is no Inf int
                ret
divmod8_0dvdnd: inr     d               ; is the dividend
                dcr     d               ; zero? (10 cycles)
                jz      divmod8_exit0   ; early exit, as well

                push    psw
                push    bc
                push    hl              ; save all the registers
                push    de
        
                mvi     b, 00h          ; B will hold n_trail
divmod8_0shft:  mvi     a, 01h          ; A <= 1
                ana     d               ; A <= A & D (numerator)
                jnz     divmod8_1test
                mvi     a, 01h
                ana     e               ; A <= A & E (denominator)
                jnz     divmod8_1test
                mov     a, d
                rrc
                mov     d, a            ; D >>= 1 (numerator)
                mov     a, e
                rrc
                mov     e, a            ; E >>= 1 (denominator)
                inr     b               ; n_trail++
                jmp     divmod8_0shft   ; next iteration

divmod8_1test:  mov     a, e
                cpi     01h             ; E == 1? (denominator of 1)
                jnz     divmod8_fndmsb
                mov     a, d
                pop     de
                mvi     e, 0            ; restore DE, set remainder
                mov     d, a            ; and quotient
                pop     hl
                pop     bc              ; to zero and return
                pop     psw
                ret

divmod8_fndmsb: mvi     h, 080h         ; H will be used for mask_N
                mvi     c, 080h         ; C will be used for msb_N
                mvi     l, 8            ; L will be used for w_N
                
divmod8_msbD:   mov     a, d            ; A <= D (numerator, N)
                ana     h               ; A <= A & H (mask_N)
                jnz     divmod8_msbfnd  ; all done?
                dcr     l               ; L <= L - 1 (w_N)
                mov     a, c
                rrc
                mov     c, a            ; C <= C >> 1 (msb_N)
                ora     h               ; A <= A | H = msb_N | H
                mov     h, a            ; H <= A = msb_N | E = mask_N
                jmp     divmod8_msbD    ; next iteration
                
                ;
                ; On exit from the above:
                ;       D       numerator
                ;       E       denominator
                ;       H       mask_N
                ;       L       w_N
                ;       B       n_trail
                ;       C       msb_N
                ;
                ; The next step is to check for
                ;       (E & mask_N) > msb_N
divmod8_msbfnd: mov     a, e
                ana     h               ; A <= E & H = (denominator & mask_N)
                cmp     c               ; A =?= C
                jp      divmod8_0rem
                jmp     divmod8_align
divmod8_0rem:   mov     a, d            ; stash numerator
                pop     de
                mov     e, a            ; restore DE, set remainder
                mvi     d, 0            ; and quotient
                pop     hl
                pop     bc              ; to zero and return
                pop     psw
                ret
                
                ;
                ; This next section aligns the denominator's
                ; MSb with that of the numerator.  At this
                ; point the msb_N is no longer needed, so
                ; C will be reused for w_D:
                ;
divmod8_align:  mov     c, l            ; C <= L (w_D = w_N)
divmod8_msbE:   mov     a, e            ; A <= E (denominator)
                ana     h               ; A <= A & H (mask_N)
                jnz     divmod8_lngdiv  ; all done?
                dcr     c               ; L <= L - 1 (w_N)
                mov     a, e
                rlc
                mov     e, a            ; E <= E << 1 (denominator)
                jmp     divmod8_msbE    ; next iteration

                ;
                ; At last, the numerator D and denominator E
                ; are aligned at their MSb.  Do the long
                ; division loop — if we calculate w_N - w_D
                ; then we're done with C, and the mask_N in
                ; H is no longer needed, either
                ;
                ;       D       numerator
                ;       E       denominator
                ;       B       n_trail
                ;       C       quotient
                ;       H       n_iter
                ;       L       w_N - w_D
                ; 
divmod8_lngdiv: mov     a, l            ; A <= L (w_N)
                sub     c               ; A <= A - C = w_N - w_D
                mov     l, a            ; L <= A = w_N - w_D
                inr     a
                mov     h, a            ; H <= A = w_N - w_D + 1
                mvi     c, 00h          ; C <= 0 (quotient)
                ana     a               ; A still has n_iter from above, test for zero
                jz      divmod8_trail   ; all done
divmod8_ldivlp: mov     a, c
                rlc                     ; A <= A << 1 = C << 1 = quotient << 1
                mov     c, a            ; C <= A = quotient << 1
                mov     a, d            ; A <= D (numerator)
                cmp     e               ; D =?= E (numerator =?= denominator)
                jm      divmod8_ldpnxt  ; N < D
                mov     a, d
                sub     e               ; A <= N - D
                mov     d, a            ; D <= A = N - D
                mov     a, c
                ori     01h             ; A <= C | 1 = quotient | 1
                mov     c, a
divmod8_ldpnxt: dcr     h
                jz      divmod8_trail
                mov     a, d
                rlc
                mov     d, a
                jmp     divmod8_ldivlp
                
                ;
                ; Exiting the long division loop, C has the
                ; quotient and D has the remainder.  All that's
                ; left is to shift the remainder back to its
                ; original position.  Recall:
                ;
                ;       L       w_N - w_D
                ;       B       n_trail
                ;       D       remainder
                ;
divmod8_trail:  mov     a, l
                sub     b               ; A <= L - B = w_N - w_D - n_trail
                mov     b, a
                mov     a, d
                stc
                cmc                     ; reset carry bit
                jz      divmod8_exit
                jm      divmod8_trlshl  ; the sum is negative, shift left
                                        ; the sum was positive non-zero,
divmod8_trlshr: rrc                     ; shift right
                dcr     b               ; B <= B - 1
                jnz     divmod8_trlshr  ; loop
                jmp     divmod8_exit
divmod8_trlshl: rlc                     ; shift left
                inr     b               ; B <= B + 1
                jnz     divmod8_trlshr  ; loop
                
                ;
                ; The quotient is in C, the remainder is
                ; in A:
                ;
divmod8_exit:   mov     d, c            ; output:  D <= quotient
                mov     e, a            ; output:  E <= remainder
                pop     hl              ; we pushed a DE last, get rid of it
                pop     hl              ; pop the rest of what we pushed
                pop     bc              ; to the stack
                pop     psw
                ret                     ; and return
divmod8_end:
