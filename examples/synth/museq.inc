;
; From the table of note durations at 30 Hz and 150 BPM.  When used
; in a clip the duration must have one subtracted from it (the
; initial cycle it is played).  The MUSEQ_DURATION macro can be used
; to formalize this.
;
MUSEQ_DUR_4                     EQU     0C0h
MUSEQ_DUR_3                     EQU     090h
MUSEQ_DUR_2                     EQU     060h
MUSEQ_DUR_1                     EQU     030h
MUSEQ_DUR_1_2                   EQU     018h
MUSEQ_DUR_1_4                   EQU     00Ch
MUSEQ_DUR_1_8                   EQU     006h
MUSEQ_DUR_1_16                  EQU     003h
MUSEQ_DUR_1_32                  EQU     001h

;
; State updated flag bits
;
MUSEQ_P0_UPDATE_BIT             EQU     10000000b
MUSEQ_P1_UPDATE_BIT             EQU     01000000b
MUSEQ_T0_UPDATE_BIT             EQU     00100000b
MUSEQ_N0_UPDATE_BIT             EQU     00010000b

;
; Track commands
;
MUSEQ_TRK_CMD_STP       EQU     00000000b
MUSEQ_TRK_CMD_JMP       EQU     00000001b
MUSEQ_TRK_CMD_NXC_N     EQU     00000010b
MUSEQ_TRK_CMD_NXC_1     EQU     00000110b
MUSEQ_TRK_CMD_BTL_N     EQU     00000100b
MUSEQ_TRK_CMD_BTL_2     EQU     00001100b
MUSEQ_TRK_CMD_DBTL      EQU     00001000b

;
; Shared indicator bits:
;
MUSEQ_CLIP_CTRL_EOS             EQU     00000000b   ; End of sequence
MUSEQ_CLIP_CTRL_CONT            EQU     00000001b   ; Continue sequence
MUSEQ_CLIP_CTRL_REST            EQU     10000000b   ; A rest; includes only a duration
                                                    ; byte
;
; Change indicator bits specific to pulse channels
;
MUSEQ_CLIP_CTRL_P_FREQ_SWPPRD   EQU     01000000b   ; A frequency sweep period byte is
                                                    ; included
MUSEQ_CLIP_CTRL_P_FREQ          EQU     00100000b   ; A frequency timer index byte is
                                                    ; included
MUSEQ_CLIP_CTRL_P_ENVELOPE      EQU     00010000b   ; An envelope byte is included
MUSEQ_CLIP_CTRL_P_FSHFTDUTY     EQU     00001000b   ; A frequency shift and duty cycle
                                                    ; byte is included
MUSEQ_CLIP_CTRL_P_STATE         EQU     00000100b   ; An altered state byte is included

;
; Change indicator bits specific to triangle channels
;
MUSEQ_CLIP_CTRL_T_FREQ          EQU     01000000b   ; A frequency timer index byte is
                                                    ; included
MUSEQ_CLIP_CTRL_T_ENVELOPE      EQU     00100000b   ; An envelope byte is included
MUSEQ_CLIP_CTRL_T_STATE         EQU     00010000b   ; An altered state byte is included

;
; Change indicator bits specific to the noise channel
;
MUSEQ_CLIP_CTRL_N_ENVELOPE      EQU     01000000b   ; An envelope byte is included
MUSEQ_CLIP_CTRL_N_STATE         EQU     00100000b   ; An altered state byte is included


;
; Envelope decay constants:
;
MUSEQ_CLIP_DECAY_PORTATO        EQU     020h        ; between legato and staccato
MUSEQ_CLIP_DECAY_STACCATO       EQU     010h        ; quickest decay to note volume

;
; MUSEQ_LOAD <TRKPTRS>
; MUSEQ LOAD track pointers at <TRKPTRS>
;
; Load the four pointers stored at <TRKPTRS> into the pulse 0,
; pulse 1, triangle 0, and noise tracks.
;
; Clobbers all registers.
;
MUSEQ_LOAD:             MACRO   TRKPTRS
                        DI                              ; [ 4] Ensure the PPU doesn't interrupt
                        LXI     H, TRKPTRS              ; [10] HL <= address of track pointers
                                                        ;            to load
                        LXI     D, MUSEQ_TRK_P0_CMDPTR  ; [10] DE <= pulse 0 command pointer
                        MOV     A, M                    ; [ 7] low byte of addr
                        STAX    D                       ; [ 7] store low byte
                        INX     H                       ; [ 5] HL++
                        INX     D                       ; [ 5] DE++
                        MOV     A, M                    ; [ 7] high byte of addr
                        STAX    D                       ; [ 7] store high byte
                        INX     H                       ; [ 5] HL++
                        LXI     D, MUSEQ_TRK_P1_CMDPTR  ; [10]DE <= pulse 1 command pointer
                        MOV     A, M                    ; [ 7] low byte of addr
                        STAX    D                       ; [ 7] store low byte
                        INX     H                       ; [ 5] HL++
                        INX     D                       ; [ 5] DE++
                        MOV     A, M                    ; [ 7] high byte of addr
                        STAX    D                       ; [ 7] store high byte
                        INX     H                       ; [ 5] HL++
                        LXI     D, MUSEQ_TRK_T0_CMDPTR  ; [10] DE <= triangle 0 command pointer
                        MOV     A, M                    ; [ 7] low byte of addr
                        STAX    D                       ; [ 7] store low byte
                        INX     H                       ; [ 5] HL++
                        INX     D                       ; [ 5] DE++
                        MOV     A, M                    ; [ 7] high byte of addr
                        STAX    D                       ; [ 7] store high byte
                        INX     H                       ; [ 5] HL++
                        LXI     D, MUSEQ_TRK_N0_CMDPTR  ; [10] DE <= noise command pointer
                        MOV     A, M                    ; [ 7] low byte of addr
                        STAX    D                       ; [ 7] store low byte
                        INX     H                       ; [ 5] HL++
                        INX     D                       ; [ 5] DE++
                        MOV     A, M                    ; [ 7] high byte of addr
                        STAX    D                       ; [ 7] store high byte
                        MVI     A, 1                    ; [ 7] A <= 1 (track reset)
                        STA     MUSEQ_TRK_P0_RESET      ; [13] pulse 0 track reset
                        STA     MUSEQ_TRK_P1_RESET      ; [13] pulse 1 track reset
                        STA     MUSEQ_TRK_T0_RESET      ; [13] triangle 0 track reset
                        STA     MUSEQ_TRK_N0_RESET      ; [13] pulse 0 track reset
                        EI                              ; [ 4] enable interrupts now…
                        ENDM
;
; MUSEQ_STEP
; MUSEQ state progression STEP
;
; Updates all audio channels by a single step, stashing changes
; to the channel state registers to be applied at VBL by the
; MUSEQ_UPDATE interrupt handler.
;
; All registers are likely to be clobbered by this subroutine
; and those that it calls; the caller is responsible for saving
; any registers that need to be preserved.
;
MUSEQ_STEP:             MACRO
                        MVI     E, 000h             ; [ 7] E will hold the state update indicators
                        CALL    MUSEQ_P0_STEP       ; [17] Get pulse channel 0 updated
                        CALL    MUSEQ_P1_STEP       ; [17] Get pulse channel 1 updated
                        CALL    MUSEQ_T0_STEP       ; [17] Get triangle channel 0 updated
                        CALL    MUSEQ_N0_STEP       ; [17] Get noise channel
                        MOV     A, E                ; [ 5] Stash the state update bits…
                        STA     MUSEQ_NEEDS_UPDATE  ; [13] …where the interrupt handler gets 'em
                        ENDM

;
; MUSEQ_DURATION
; MUSEQ set DURATION <N>
;
; Formalizes the duration byte added to each note in a clip.
;
MUSEQ_DURATION:         MACRO   DT
                        DB      (DT - 1)
                        ENDM

;
; MUSEQ_CLIP_CHH_1_2
; MUSEQ CLIP Closed High-Hat, 1/2 note length
;
MUSEQ_CLIP_CHH_1_2:     MACRO
                        DB      MUSEQ_CLIP_CTRL_N_ENVELOPE|MUSEQ_CLIP_CTRL_N_STATE
                        MUSEQ_DURATION MUSEQ_DUR_1_8    ; 1/8 note
                        DB          040h                ; volume level 0x40
                        DB          APU_P_STATE_ENABLE | APU_P_STATE_ENVCONST | 01h ; noise period
                        ;
                        DB      MUSEQ_CLIP_CTRL_N_ENVELOPE|MUSEQ_CLIP_CTRL_N_STATE
                        MUSEQ_DURATION MUSEQ_DUR_1_4    ; 1/4 note
                        DB          010h                ; volume decay period
                        DB          APU_P_STATE_ENABLE | 03h ; noise period
                        ;
                        DB      MUSEQ_CLIP_CTRL_REST
                        MUSEQ_DURATION MUSEQ_DUR_1_8    ; 1/8 note length
                        ENDM


;
; MUSEQ_CLIP_OHH_1_2
; MUSEQ CLIP Open High-Hat, 1/2 note length
;
MUSEQ_CLIP_OHH_1_2:     MACRO
                        DB      MUSEQ_CLIP_CTRL_N_ENVELOPE|MUSEQ_CLIP_CTRL_N_STATE
                        MUSEQ_DURATION MUSEQ_DUR_1_8    ; 1/8 note
                        DB          060h                ; volume level 0x60
                        DB          APU_P_STATE_ENABLE | APU_P_STATE_ENVCONST | 02h ; noise period
                        ;
                        DB      MUSEQ_CLIP_CTRL_N_ENVELOPE|MUSEQ_CLIP_CTRL_N_STATE
                        MUSEQ_DURATION MUSEQ_DUR_1_8    ; 1/8 note
                        DB          050h                ; drop volume level to 0x50
                        DB          APU_P_STATE_ENABLE | APU_P_STATE_ENVCONST | 02h ; noise period
                        ;
                        DB      MUSEQ_CLIP_CTRL_N_ENVELOPE|MUSEQ_CLIP_CTRL_N_STATE
                        MUSEQ_DURATION MUSEQ_DUR_1_8    ; 1/8 note
                        DB          020h                ; drop volume level to 0x20
                        DB          APU_P_STATE_ENABLE | APU_P_STATE_ENVCONST | 02h ; noise period
                        ;
                        DB      MUSEQ_CLIP_CTRL_N_ENVELOPE|MUSEQ_CLIP_CTRL_N_STATE
                        MUSEQ_DURATION MUSEQ_DUR_1_8    ; 1/8 note
                        DB          010h                ; drop volume level to 0x10
                        DB          APU_P_STATE_ENABLE | APU_P_STATE_ENVCONST | 02h ; noise period
                        ENDM
                        