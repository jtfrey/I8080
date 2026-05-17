
            include "timer_registers.inc"

            org     0400h
            
            ;
            ; The timer will issue a "RST 1" when it
            ; elapses, so we need to stash some code
            ; in that vector ($0008 - $000F); the
            ; "jmp timerfire" at label "rst1" is
            ; just what we want:
            ;
            lxi     hl, rst1
            mov     a, m
            sta     00008h
            inx     hl
            mov     a, m
            sta     00009h
            inx     hl
            mov     a, m
            sta     0000Ah
            
            ;
            ; Configure and start timer 2:
            ;
            lxi     hl, TMR2_MSECWHOLEB3    ; start at the msecWholeB3 register
            mvi     m, 00h                  ;
            dcr     l                       ; decrement HL to write to msecWholeB2
            mvi     m, 00h                  ;
            dcr     l                       ; decrement HL to write to msecWholeB1
            mvi     m, 02eh                 ;
            dcr     l                       ; decrement HL to write to msecWholeB0
            mvi     m, 0e0h                 ;
            dcr     l                       ; decrement HL to write to msecFrac
            mvi     m, 080h                 ;
            dcr     l                       ; decrement HL to write to opcode
            mvi     m, 0CFh                 ; RST 1 = $CF
            dcr     l                       ; decrement HL to write to enable
            mvi     m, 082h                  ; enable <= $82 ("auto re-enable")
            
            lxi     hl, prompt
outchar:    mov     a, M
            cpi     00h
            jz      wait
            out     0
            inx     hl
            jmp     outchar

wait:       lda     exit_loop
            cpi     0
            jnz     wait
            hlt

prompt:     db      0Ah, 'Please wait for three timer intervals to elapse...', 0Ah, 00h
exit_loop:  db      03h

rst1:       jmp     timerfire
timerfire:  push    hl
            push    psw
            lxi     hl, firestr
timerfirel: mov     a, m
            cpi     00h
            jz      timerfirex
            out     0
            inx     hl
            jmp     timerfirel
timerfirex: lxi     hl, exit_loop
            dcr     m               ; signal loop end
            pop     psw
            pop     hl
            ei
            ret

firestr     db      '12000.5 ms has elapsed', 0Ah, 00h