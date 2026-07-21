;
; The sequencer consists of subroutines for each channel that progress
; through a NULL-terminated series of bytes that describe changes to be
; made to the APU registers in order to play timed tones (music).
;
                CPU     8080


                INCLUDE "museq.inc"

;
; The time device is mapped at this address:
;
TMR_BASE        EQU     0F800h
                INCLUDE "timers.inc"
            
                ORG     0100h

;
; The timer will issue a RST 1 instruction when it elapses,
; so we need to stash some code at $0008-$000F to react when
; that happens
;
                LXI     H, 00008h                       ; RST 1 vector address
                MVI     A, 0C3h                         ; JMP opcode
                MOV     M, A
                INX     H
                MVI     A, MUSEQ_UPDATE & 000FFh        ; low-byte of MUSEQ_UPDATE subroutine
                MOV     M, A
                INX     H
                MVI     A, (MUSEQ_UPDATE & 0FF00h) >> 8 ; high-byte of MUSEQ_UPDATE subroutine
                MOV     M, A

;
; Get the synthesizer initialized:
;
                CALL    MUSEQ_INIT
                MUSEQ_LOAD TRK_0

;
; The musical synthesizer is going to run at a rate of 150 BPM, so get those durations
; defined:
;               
                LXI     H, TMR0_MSECWHOLEB3     ; start at the msecWholeB3 register
                MVI     A, 000h                 ; whole ms
                MOV     M, A                    ; B3...
                DCR     L
                MOV     M, A                    ; ...B2...
                DCR     L
                MOV     M, A                    ; ...B1...
                DCR     L
                MVI     M, 33                   ; 33 ms
                DCR     L
                MVI     M, 055h                 ; (1/3) ms ≈ 85/256
                DCR     L
                MVI     M, 0CFh                 ; RST 1 opcode
                DCR     L
                MVI     M, 082h                 ; Start the timer
                
MAINLOOP:       LDA     MAINLOOP_LOCK
                ORA     A
                JNZ     MAINLOOP
                
                MUSEQ_STEP
                MVI     A, 1
                STA     MAINLOOP_LOCK
                JMP     MAINLOOP
                HLT

MAINLOOP_LOCK:  DB      00h

                INCLUDE "museq.asm"
                INCLUDE "soundtrack.asm"
                
