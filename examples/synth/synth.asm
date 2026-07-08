;
; The sequencer consists of subroutines for each channel that progress
; through a NULL-terminated series of bytes that describe changes to be
; made to the APU registers in order to play timed tones (music).
;
                CPU     8080

;
; The APU is mapped at this address:
;
APU_BASE        EQU     0F000h
                INCLUDE "apu.inc"

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
                MVI     A, SYNTH_UPDATE & 000FFh        ; low-byte of SYNTH_UPDATE subroutine
                MOV     M, A
                INX     H
                MVI     A, (SYNTH_UPDATE & 0FF00h) >> 8 ; high-byte of SYNTH_UPDATE subroutine
                MOV     M, A

;
; Get the synthesizer initialized:
;
                CALL    SYNTH_INIT

;
; Prepare the synthesizer update timer.  We're aiming for 120 BPM
; tempo, so a sixteenth note would last 1/8 s = 125 ms.  Get timer 0
; configured that far, but don't enable it just yet.
;
                LXI     H, TMR0_MSECWHOLEB3             ; start at the msecWholeB3 register
                MVI     A, 000h                         ; whole ms = 0000007D
                MOV     M, A                            ; B3...
                DCR     L
                MOV     M, A                            ; ...B2...
                DCR     L
                MOV     M, A                            ; ...B1...
                DCR     L
                MVI     M, 07Dh                         ; ...B0.
                DCR     L
                MOV     M, A                            ; msecFrac is zero, too
                DCR     L
                MVI     M, 0CFh                         ; RST 1 opcode
                
                MVI     A, 082h                         ; Start the timer
                STA     TMR0_ENABLE
                
LOOP:           JMP     LOOP
                HLT




                        
SEQ_CTRL_EOS            EQU     00000000b           ; End of sequence
SEQ_CTRL_CONT           EQU     00000001b           ; Continue sequence
SEQ_CTRL_REST           EQU     10000000b           ; A rest; includes only a duration
                                                    ; byte
SEQ_CTRL_FREQ_SWPPRD    EQU     01000000b           ; A frequency sweep period byte is
                                                    ; included
SEQ_CTRL_FREQ           EQU     00100000b           ; A frequency timer index byte is
                                                    ; included
SEQ_CTRL_ENVELOPE       EQU     00010000b           ; An envelope byte is included
SEQ_CTRL_FSHFTDUTY      EQU     00001000b           ; A frequency shift and duty cycle
                                                    ; byte is included
SEQ_CTRL_STATE          EQU     00000100b           ; An altered state byte is included

SEQ_DUR_4               EQU     040h - 1
SEQ_DUR_3               EQU     030h - 1
SEQ_DUR_2               EQU     020h - 1
SEQ_DUR_1               EQU     010h - 1
SEQ_DUR_1_2             EQU     008h - 1
SEQ_DUR_1_4             EQU     004h - 1
SEQ_DUR_1_8             EQU     002h - 1
SEQ_DUR_1_16            EQU     001h - 1

SEQ_DECAY_PORTATO       EQU     020h
SEQ_DECAY_STCTO         EQU     010h


SEQ_P0_DURATION:        DB      000h                ; Counter for note duration, pulse 0
SEQ_P0_PTR:             DW      TWINKLE_MELODY      ; Pointer to the sequence being played, pulse 0

SEQ_P1_DURATION:        DB      000h                ; Counter for note duration, pulse 1
SEQ_P1_PTR:             DW      TWINKLE_HARMONY     ; Pointer to the sequence being played, pulse 1

;
; SEQ_P0_STEP
; SEQuencer Pulse 0 STEP
;
; Progress the state of pulse channel 0.
;
SEQ_P0_STEP:            LDA     SEQ_P0_DURATION     ; [13] Load duration slot
                        ORA     A                   ; [ 4] Has duration reached zero?
                        JZ      $$NEXT_STEP         ; [10] Yes, load a new step from the sequence
                        DCR     A                   ; [ 5] No, decrement the counter…
                        STA     SEQ_P0_DURATION     ; [13] …store it back…
                        RET                         ; [10] …and exit.

$$NEXT_STEP:            LHLD    SEQ_P0_PTR          ; [16] HL <= sequence pointer
                        
                        MOV     A, M                ; [ 7] A <= control byte
                        ORA     A                   ; [ 4] A <= A | A (set flags)
                        JNZ     $$NOT_EOS           ; [10] control byte is not SEQ_CRTL_EOS
                        MVI     A, 00000000b        ; [ 7] Disable the channel…
                        STA     APU_RGSTR_P0_STATE  ; [13]
                        MVI     A, 0FFh             ; [ 7] …and set duration to max…
                        STA     SEQ_P0_DURATION     ; [13] …to avoid checking a SEQ_CTRL_EOS every
                                                    ;       step now that we're EOS…
                        RET                         ; [10] …and return.

$$NOT_EOS:              INX     H                   ; [ 5] sequence ptr++
                        MOV     B, A                ; [ 5] B <= copy of control byte
                        MOV     A, M                ; [ 7] A <= duration (everything will have one)
                        INX     H                   ; [ 5] sequence ptr++
                        STA     SEQ_P0_DURATION     ; [13] Update the duration
                        MOV     A, B                ; [ 5] A <= control byte
                        CPI     SEQ_CTRL_CONT       ; [ 7] A == SEQ_CTRL_CONT ?
                        JNZ     $$NOT_CONT          ; [10] Not SEQ_CTRL_CONT, handle any changes
                        SHLD    SEQ_P0_PTR          ; [16] Save the sequence pointer…
                        LDA     APU_RGSTR_P0_STATE  ; [13] …then load the existing state…
                        STA     APU_RGSTR_P0_STATE  ; [13] …and write it back to re-trigger the
                                                    ;      same tone parameters…
                        RET                         ; [10] …and return.
                        
$$NOT_CONT:             JP      $$NOT_REST          ; [10] The CPI SEQ_CTRL_CONT above will have
                                                    ;      set the M flag if bit 7 was set
                        SHLD    SEQ_P0_PTR          ; [16] Save the sequence pointer…
                        LDA     APU_RGSTR_P0_STATE  ; [13] …then load the existing channel state…
                        ANI     01111111b           ; [ 7] …drop the enable bit…
                        STA     APU_RGSTR_P0_STATE  ; [13] …and write it back to stop the channel…
                        RET                         ; [10] …and return.

$$NOT_REST:             RAL                         ; [ 4] A <= A << 1, CY <= bit 7 (SEQ_CTRL_REST)
                        RAL                         ; [ 4] A <= A << 1, CY <= bit 6
                                                    ;      (SEQ_CTRL_FREQ_SWPPRD)
                        JNC     $$NO_FREQ_SWPPRD    ; [10] No frequency sweep period
                        MOV     B, A                ; [ 5] B <= copy of shifted control byte
                        MOV     A, M                ; [ 7] A <= frequency sweep period byte
                        INX     H                   ; [ 5] sequence ptr++
                        STA     APU_RGSTR_P0_FREQSWPPERIOD
                                                    ; [13] Set frequency sweep period
                        MOV     A, B                ; [ 5] A <= shifted control byte

$$NO_FREQ_SWPPRD:       RAL                         ; [ 4] A <= A << 1, CY <= bit 5 (SEQ_CTRL_FREQ)
                        JNC     $$NO_FREQ           ; [10] No frequency change
                        MOV     B, A                ; [ 5] B <= copy of shifted control byte
                        MOV     A, M                ; [ 7] A <= frequency byte
                        INX     H                   ; [ 5] sequence ptr++
                        STA     APU_RGSTR_P0_FREQLO ; [13] Set frequency index
                        MOV     A, B                ; [ 5] A <= shifted control byte

$$NO_FREQ:              RAL                         ; [ 4] A <= A << 1, CY <= bit 4
                                                    ;      (SEQ_CTRL_ENVELOPE)
                        JNC     $$NO_ENVELOPE       ; [10] No frequency change
                        MOV     B, A                ; [ 5] B <= copy of shifted control byte
                        MOV     A, M                ; [ 7] A <= frequency byte
                        INX     H                   ; [ 5] sequence ptr++
                        STA     APU_RGSTR_P0_ENVLVLLO
                                                    ; [13] Set envelope option (level or decay
                                                    ;      period)
                        MOV     A, B                ; [ 5] A <= shifted control byte

$$NO_ENVELOPE:          RAL                         ; [ 4] A <= A << 1, CY <= bit 3
                                                    ;      (SEQ_CTRL_FSHFTDUTY)
                        JNC     $$NO_FSHFTDUTY      ; [10] No frequency shift + duty cycle change
                        MOV     B, A                ; [ 5] B <= copy of shifted control byte
                        MOV     A, M                ; [ 7] A <= frequency shift + duty cycle byte
                        INX     H                   ; [ 5] sequence ptr++
                        STA     APU_RGSTR_P0_DUTY_FREQSHFT
                                                    ; [13] Set frequency shift + duty cycle
                        MOV     A, B                ; [ 5] A <= shifted control byte

$$NO_FSHFTDUTY:         RAL                         ; [ 4] A <= A << 1, CY <= bit 2 (SEQ_CTRL_STATE)
                        JNC     $$NO_STATE          ; [10] No state byte changes (besides enable)
                        MOV     A, M                ; [ 7] A <= new state byte
                        INX     H                   ; [ 5] sequence ptr++
                        SHLD    SEQ_P0_PTR          ; [16] Save the sequence pointer…
                        STA     APU_RGSTR_P0_STATE  ; [13] …write the state register…
                        RET                         ; [10] …and return.

$$NO_STATE:             SHLD    SEQ_P0_PTR          ; [16] Save the sequence pointer…
                        LDA     APU_RGSTR_P0_STATE  ; [13] …the load the existing state…
                        ORI     10000000b           ; [ 7] …ensure the channel is enabled…
                        STA     APU_RGSTR_P0_STATE  ; [13] …write the state register…
                        RET                         ; [10] …and return.

;
; SEQ_P1_STEP
; SEQuencer Pulse 1 STEP
;
; Progress the state of pulse channel 1.
;
SEQ_P1_STEP:            LDA     SEQ_P1_DURATION     ; [13] Load duration slot
                        ORA     A                   ; [ 4] Has duration reached zero?
                        JZ      $$NEXT_STEP         ; [10] Yes, load a new step from the sequence
                        DCR     A                   ; [ 5] No, decrement the counter…
                        STA     SEQ_P1_DURATION     ; [13] …store it back…
                        RET                         ; [10] …and exit.

$$NEXT_STEP:            LHLD    SEQ_P1_PTR          ; [16] HL <= sequence pointer
                        
                        MOV     A, M                ; [ 7] A <= control byte
                        ORA     A                   ; [ 4] A <= A | A (set flags)
                        JNZ     $$NOT_EOS           ; [10] control byte is not SEQ_CRTL_EOS
                        MVI     A, 00000000b        ; [ 7] Disable the channel…
                        STA     APU_RGSTR_P1_STATE  ; [13]
                        MVI     A, 0FFh             ; [ 7] …and set duration to max…
                        STA     SEQ_P1_DURATION     ; [13] …to avoid checking a SEQ_CTRL_EOS every
                                                    ;       step now that we're EOS…
                        RET                         ; [10] …and return.

$$NOT_EOS:              INX     H                   ; [ 5] sequence ptr++
                        MOV     B, A                ; [ 5] B <= copy of control byte
                        MOV     A, M                ; [ 7] A <= duration (everything will have one)
                        INX     H                   ; [ 5] sequence ptr++
                        STA     SEQ_P1_DURATION     ; [13] Update the duration
                        MOV     A, B                ; [ 5] A <= control byte
                        CPI     SEQ_CTRL_CONT       ; [ 7] A == SEQ_CTRL_CONT ?
                        JNZ     $$NOT_CONT          ; [10] Not SEQ_CTRL_CONT, handle any changes
                        SHLD    SEQ_P1_PTR          ; [16] Save the sequence pointer…
                        LDA     APU_RGSTR_P1_STATE  ; [13] …then load the existing state…
                        STA     APU_RGSTR_P1_STATE  ; [13] …and write it back to re-trigger the
                                                    ;      same tone parameters…
                        RET                         ; [10] …and return.
                        
$$NOT_CONT:             JP      $$NOT_REST          ; [10] The CPI SEQ_CTRL_CONT above will have
                                                    ;      set the M flag if bit 7 was set
                        SHLD    SEQ_P1_PTR          ; [16] Save the sequence pointer…
                        LDA     APU_RGSTR_P1_STATE  ; [13] …then load the existing channel state…
                        ANI     01111111b           ; [ 7] …drop the enable bit…
                        STA     APU_RGSTR_P1_STATE  ; [13] …and write it back to stop the channel…
                        RET                         ; [10] …and return.

$$NOT_REST:             RAL                         ; [ 4] A <= A << 1, CY <= bit 7 (SEQ_CTRL_REST)
                        RAL                         ; [ 4] A <= A << 1, CY <= bit 6
                                                    ;      (SEQ_CTRL_FREQ_SWPPRD)
                        JNC     $$NO_FREQ_SWPPRD    ; [10] No frequency sweep period
                        MOV     B, A                ; [ 5] B <= copy of shifted control byte
                        MOV     A, M                ; [ 7] A <= frequency sweep period byte
                        INX     H                   ; [ 5] sequence ptr++
                        STA     APU_RGSTR_P1_FREQSWPPERIOD
                                                    ; [13] Set frequency sweep period
                        MOV     A, B                ; [ 5] A <= shifted control byte

$$NO_FREQ_SWPPRD:       RAL                         ; [ 4] A <= A << 1, CY <= bit 5 (SEQ_CTRL_FREQ)
                        JNC     $$NO_FREQ           ; [10] No frequency change
                        MOV     B, A                ; [ 5] B <= copy of shifted control byte
                        MOV     A, M                ; [ 7] A <= frequency byte
                        INX     H                   ; [ 5] sequence ptr++
                        STA     APU_RGSTR_P1_FREQLO ; [13] Set frequency index
                        MOV     A, B                ; [ 5] A <= shifted control byte

$$NO_FREQ:              RAL                         ; [ 4] A <= A << 1, CY <= bit 4
                                                    ;      (SEQ_CTRL_ENVELOPE)
                        JNC     $$NO_ENVELOPE       ; [10] No frequency change
                        MOV     B, A                ; [ 5] B <= copy of shifted control byte
                        MOV     A, M                ; [ 7] A <= frequency byte
                        INX     H                   ; [ 5] sequence ptr++
                        STA     APU_RGSTR_P1_ENVLVLLO
                                                    ; [13] Set envelope option (level or decay
                                                    ;      period)
                        MOV     A, B                ; [ 5] A <= shifted control byte

$$NO_ENVELOPE:          RAL                         ; [ 4] A <= A << 1, CY <= bit 3
                                                    ;      (SEQ_CTRL_FSHFTDUTY)
                        JNC     $$NO_FSHFTDUTY      ; [10] No frequency shift + duty cycle change
                        MOV     B, A                ; [ 5] B <= copy of shifted control byte
                        MOV     A, M                ; [ 7] A <= frequency shift + duty cycle byte
                        INX     H                   ; [ 5] sequence ptr++
                        STA     APU_RGSTR_P1_DUTY_FREQSHFT
                                                    ; [13] Set frequency shift + duty cycle
                        MOV     A, B                ; [ 5] A <= shifted control byte

$$NO_FSHFTDUTY:         RAL                         ; [ 4] A <= A << 1, CY <= bit 2 (SEQ_CTRL_STATE)
                        JNC     $$NO_STATE          ; [10] No state byte changes (besides enable)
                        MOV     A, M                ; [ 7] A <= new state byte
                        INX     H                   ; [ 5] sequence ptr++
                        SHLD    SEQ_P1_PTR          ; [16] Save the sequence pointer…
                        STA     APU_RGSTR_P1_STATE  ; [13] …write the state register…
                        RET                         ; [10] …and return.

$$NO_STATE:             SHLD    SEQ_P1_PTR          ; [16] Save the sequence pointer…
                        LDA     APU_RGSTR_P1_STATE  ; [13] …the load the existing state…
                        ORI     10000000b           ; [ 7] …ensure the channel is enabled…
                        STA     APU_RGSTR_P1_STATE  ; [13] …write the state register…
                        RET                         ; [10] …and return.

;
; SYNTH_INIT
; SYNTHesizer INITialize
;
; Initialize all invariant options associated with the APU.
;
SYNTH_INIT:             MVI     A, APU_M_STATE_8B               ; Set the APU to 8-bit E/D/F mode
                        STA     APU_RGSTR_M_STATE
                        MVI     A, 040h                         ; Master volume to 100%
                        STA     APU_RGSTR_M_ENVLVLLO
                        MVI     A, 007h                         ; Pulse 0 channel duty cycle to 50%
                        STA     APU_RGSTR_P0_DUTY_FREQSHFT
                        MVI     A, 003h                         ; Pulse 1 channel duty cycle to 25%
                        STA     APU_RGSTR_P1_DUTY_FREQSHFT
                        RET
                
;
; SYNTH_UPDATE
; SYNTHesizer UPDATE
;
; This subroutine is responsible for progressing the channel instruction
; streams by a single synthesis cycle.
;
SYNTH_UPDATE:           PUSH    PSW
                        PUSH    B
                        PUSH    H
                        CALL    SEQ_P0_STEP
                        CALL    SEQ_P1_STEP
                        POP     H
                        POP     B
                        POP     PSW
                        EI
                        RET


TWINKLE_MELODY:         DB      SEQ_CTRL_FREQ|SEQ_CTRL_ENVELOPE|SEQ_CTRL_STATE
                        DB          SEQ_DUR_1_4, APU_NOTE_C5_IDX, SEQ_DECAY_PORTATO, 10000000b
                        DB      SEQ_CTRL_CONT
                        DB          SEQ_DUR_1_4
                        DB      SEQ_CTRL_FREQ
                        DB          SEQ_DUR_1_4, APU_NOTE_G5_IDX
                        DB      SEQ_CTRL_CONT
                        DB          SEQ_DUR_1_4
                        DB      SEQ_CTRL_FREQ
                        DB          SEQ_DUR_1_4, APU_NOTE_A5_IDX
                        DB      SEQ_CTRL_CONT
                        DB          SEQ_DUR_1_4
                        DB      SEQ_CTRL_FREQ|SEQ_CTRL_ENVELOPE|SEQ_CTRL_STATE
                        DB          SEQ_DUR_1_2, APU_NOTE_G5_IDX, 040h, 11000000b
                        
                        DB      SEQ_CTRL_FREQ|SEQ_CTRL_ENVELOPE|SEQ_CTRL_STATE
                        DB          SEQ_DUR_1_4, APU_NOTE_F5_IDX, SEQ_DECAY_PORTATO, 10000000b
                        DB      SEQ_CTRL_CONT
                        DB          SEQ_DUR_1_4
                        DB      SEQ_CTRL_FREQ
                        DB          SEQ_DUR_1_4, APU_NOTE_E5_IDX
                        DB      SEQ_CTRL_CONT
                        DB          SEQ_DUR_1_4
                        DB      SEQ_CTRL_FREQ
                        DB          SEQ_DUR_1_4, APU_NOTE_D5_IDX
                        DB      SEQ_CTRL_CONT
                        DB          SEQ_DUR_1_4
                        DB      SEQ_CTRL_FREQ|SEQ_CTRL_ENVELOPE|SEQ_CTRL_STATE
                        DB          SEQ_DUR_1_2, APU_NOTE_C5_IDX, 040h, 11000000b
                        
                        DB      SEQ_CTRL_FREQ|SEQ_CTRL_ENVELOPE|SEQ_CTRL_STATE
                        DB          SEQ_DUR_1_4, APU_NOTE_G5_IDX, SEQ_DECAY_PORTATO, 10000000b
                        DB      SEQ_CTRL_CONT
                        DB          SEQ_DUR_1_4
                        DB      SEQ_CTRL_FREQ
                        DB          SEQ_DUR_1_4, APU_NOTE_F5_IDX
                        DB      SEQ_CTRL_CONT
                        DB          SEQ_DUR_1_4
                        DB      SEQ_CTRL_FREQ
                        DB          SEQ_DUR_1_4, APU_NOTE_E5_IDX
                        DB      SEQ_CTRL_CONT
                        DB          SEQ_DUR_1_4
                        DB      SEQ_CTRL_FREQ|SEQ_CTRL_ENVELOPE|SEQ_CTRL_STATE
                        DB          SEQ_DUR_1_2, APU_NOTE_D5_IDX, 040h, 11000000b
                        
                        DB      SEQ_CTRL_FREQ|SEQ_CTRL_ENVELOPE|SEQ_CTRL_STATE
                        DB          SEQ_DUR_1_4, APU_NOTE_G5_IDX, SEQ_DECAY_PORTATO, 10000000b
                        DB      SEQ_CTRL_CONT
                        DB          SEQ_DUR_1_4
                        DB      SEQ_CTRL_FREQ
                        DB          SEQ_DUR_1_4, APU_NOTE_F5_IDX
                        DB      SEQ_CTRL_CONT
                        DB          SEQ_DUR_1_4
                        DB      SEQ_CTRL_FREQ
                        DB          SEQ_DUR_1_4, APU_NOTE_E5_IDX
                        DB      SEQ_CTRL_CONT
                        DB          SEQ_DUR_1_4
                        DB      SEQ_CTRL_FREQ|SEQ_CTRL_ENVELOPE|SEQ_CTRL_STATE
                        DB          SEQ_DUR_1_2, APU_NOTE_D5_IDX, 040h, 11000000b
                        
                        DB      SEQ_CTRL_FREQ|SEQ_CTRL_ENVELOPE|SEQ_CTRL_STATE
                        DB          SEQ_DUR_1_4, APU_NOTE_C5_IDX, SEQ_DECAY_PORTATO, 10000000b
                        DB      SEQ_CTRL_CONT
                        DB          SEQ_DUR_1_4
                        DB      SEQ_CTRL_FREQ
                        DB          SEQ_DUR_1_4, APU_NOTE_G5_IDX
                        DB      SEQ_CTRL_CONT
                        DB          SEQ_DUR_1_4
                        DB      SEQ_CTRL_FREQ
                        DB          SEQ_DUR_1_4, APU_NOTE_A5_IDX
                        DB      SEQ_CTRL_CONT
                        DB          SEQ_DUR_1_4
                        DB      SEQ_CTRL_FREQ|SEQ_CTRL_ENVELOPE|SEQ_CTRL_STATE
                        DB          SEQ_DUR_1_2, APU_NOTE_G5_IDX, 040h, 11000000b
                        
                        DB      SEQ_CTRL_FREQ|SEQ_CTRL_ENVELOPE|SEQ_CTRL_STATE
                        DB          SEQ_DUR_1_4, APU_NOTE_F5_IDX, SEQ_DECAY_PORTATO, 10000000b
                        DB      SEQ_CTRL_CONT
                        DB          SEQ_DUR_1_4
                        DB      SEQ_CTRL_FREQ
                        DB          SEQ_DUR_1_4, APU_NOTE_E5_IDX
                        DB      SEQ_CTRL_CONT
                        DB          SEQ_DUR_1_4
                        DB      SEQ_CTRL_FREQ
                        DB          SEQ_DUR_1_4, APU_NOTE_D5_IDX
                        DB      SEQ_CTRL_CONT
                        DB          SEQ_DUR_1_4
                        DB      SEQ_CTRL_FREQ|SEQ_CTRL_ENVELOPE|SEQ_CTRL_STATE
                        DB          SEQ_DUR_1_2, APU_NOTE_C5_IDX, 040h, 11000000b
                        DB      SEQ_CTRL_ENVELOPE|SEQ_CTRL_STATE
                        DB          SEQ_DUR_1_2, SEQ_DECAY_PORTATO, 10000000b
                        
                        DB      SEQ_CTRL_EOS

TWINKLE_HARMONY:        DB      SEQ_CTRL_FREQ|SEQ_CTRL_ENVELOPE|SEQ_CTRL_STATE
                        DB          SEQ_DUR_1_2, APU_NOTE_C3_IDX, 040h, 11000000b
                        DB      SEQ_CTRL_FREQ
                        DB          SEQ_DUR_1_2, APU_NOTE_G3_IDX
                        DB      SEQ_CTRL_FREQ
                        DB          SEQ_DUR_1_2, APU_NOTE_A3_IDX
                        DB      SEQ_CTRL_FREQ
                        DB          SEQ_DUR_1_2, APU_NOTE_G3_IDX
                        
                        DB      SEQ_CTRL_FREQ
                        DB          SEQ_DUR_1_2, APU_NOTE_F3_IDX
                        DB      SEQ_CTRL_FREQ
                        DB          SEQ_DUR_1_2, APU_NOTE_E3_IDX
                        DB      SEQ_CTRL_FREQ
                        DB          SEQ_DUR_1_2, APU_NOTE_D3_IDX
                        DB      SEQ_CTRL_FREQ
                        DB          SEQ_DUR_1_2, APU_NOTE_C3_IDX
                        
                        DB      SEQ_CTRL_FREQ
                        DB          SEQ_DUR_1_2, APU_NOTE_G3_IDX
                        DB      SEQ_CTRL_FREQ
                        DB          SEQ_DUR_1_2, APU_NOTE_F3_IDX
                        DB      SEQ_CTRL_FREQ
                        DB          SEQ_DUR_1_2, APU_NOTE_E3_IDX
                        DB      SEQ_CTRL_FREQ
                        DB          SEQ_DUR_1_2, APU_NOTE_D3_IDX
                        
                        DB      SEQ_CTRL_FREQ
                        DB          SEQ_DUR_1_2, APU_NOTE_G3_IDX
                        DB      SEQ_CTRL_FREQ
                        DB          SEQ_DUR_1_2, APU_NOTE_F3_IDX
                        DB      SEQ_CTRL_FREQ
                        DB          SEQ_DUR_1_2, APU_NOTE_E3_IDX
                        DB      SEQ_CTRL_FREQ
                        DB          SEQ_DUR_1_2, APU_NOTE_D3_IDX
                        
                        DB      SEQ_CTRL_FREQ
                        DB          SEQ_DUR_1_2, APU_NOTE_C3_IDX
                        DB      SEQ_CTRL_FREQ
                        DB          SEQ_DUR_1_2, APU_NOTE_G3_IDX
                        DB      SEQ_CTRL_FREQ
                        DB          SEQ_DUR_1_2, APU_NOTE_A3_IDX
                        DB      SEQ_CTRL_FREQ
                        DB          SEQ_DUR_1_2, APU_NOTE_G3_IDX
                        
                        DB      SEQ_CTRL_FREQ
                        DB          SEQ_DUR_1_2, APU_NOTE_F3_IDX
                        DB      SEQ_CTRL_FREQ
                        DB          SEQ_DUR_1_2, APU_NOTE_E3_IDX
                        DB      SEQ_CTRL_FREQ
                        DB          SEQ_DUR_1_2, APU_NOTE_D3_IDX
                        DB      SEQ_CTRL_FREQ|SEQ_CTRL_ENVELOPE|SEQ_CTRL_STATE
                        DB          SEQ_DUR_1_2, APU_NOTE_C3_IDX, 040h, 11000000b
                        DB      SEQ_CTRL_ENVELOPE|SEQ_CTRL_STATE
                        DB          SEQ_DUR_1_2, SEQ_DECAY_PORTATO, 10000000b
                        
                        DB      SEQ_CTRL_EOS
                        