;
; # Musical Synthesis
;
; The PPU will be dutifully updating the graphics at a fixed frequency
; of 60 Hz.  If we adopt 120 BPM for the music:
;
;     60 (cyc/s) / 120 (B/min) * 60 (s/min) = 30 cyc/B
;
; A quarter note lasts just 1 beat, therefore 30 cycles.  That
; would place an eighth note at 15 cycles, and a sixteenth note...
; at 7.5 cycles.  Given our "clock" for the music, 120 BPM will
; make shorter notes impossible to time properly.  How about
; 150 BPM?
;
;     60 (cyc/s) / 150 (B/min) * 60 (s/min) = 24 cyc/B
;
; That yields 24, 12, 6, 3, 1 for quarter down through a
; sixty-fourth note.  But at 150 BPM a sixty-fourth note lasts
; just 16.67 ms — not something we're likely to notice amidst
; the action.  If that resolution is not necessary, then we
; can drop the sequencer's update frequency to every other
; PPU cycle, or 30 Hz:
;
;     30 (cyc/s) / 150 (B/min) * 60 (s/min) = 12 cyc/B
;
; This yields the following note durations:
;
;   Note        Cycles          Name
;   4           240             octuple whole
;   3           144             quadruple whole
;   2            96             double whole
;   1            48             whole
;   1/2          24             half
;   1/4          12             quarter
;   1/8           6             eighth
;   1/16          3             sixteenth
;   1/32          1             thirtysecond
;
; Note that at 60 Hz we also would have topped out at a double whole's
; having length 192 (0xC0) given durations will be a single byte in size.
;
; All audio channels are operated in 8-bit mode:  envelope levels,
; frequencies, etc. use single-byte values and the low-byte of
; corresponding registers.
;
; # The sequencer
;
; Music is comprised of two tiers of components:
;
;     - a _clip_ is a single section of notes played in
;       sequence for some fixed duration
;     - a _track_ is a sequence of zero or more _clips_
;       (with variable repetition count) and the ability
;       to skip to another location in the sequence
;       a fixed or infinite number of times
;
; Only two loop counters exist per-channel:  one for clip
; repetition and one for jump repetition (therefore jump
; loops cannot be nested).  At any one time there may be
; four tracks active concurrently across the four channels
; managed by the sequencer.  Transitions between notes
; within clips and between clips within tracks need not be
; synchronized between the channels.
;
; ## Clips
;
; A clip is a sequence of commands to alter an audio channel's
; properties.  Since the APU registers remain unchanged once
; the user code changes them, subsequent commands do not need
; to repeat values for parameters that do not change, like
; frequency, envelope, or duration.  Likewise, a rest resets
; the enable bit on the channel's state register, leaving all
; other bits untouched; when a subsequent note is played, the
; enable bit is set and all other bits function as before.
;
; Commands are a single byte, treated as an array of bits
; which individually have significance.  All channels respond
; to the following three command bytes:
;
;   00000000b = EOC : End-Of-Clip
;     Indicates no more notes remain in the clip:
;
;             DB      MUSEQ_CLIP_EOC
;
;   00000001b = CONT : CONTinue
;     Read the channel's status register and write the value back
;     to the status register.  Repeats the last-played note with
;     all properties intact and the duration supplied with the
;     command byte.  To play a quarter note using all existing
;     parameters, for example:
;
;             DB      MUSEQ_CLIP_CONT, MUSEQ_DUR_1_4
;
;   10000000b = REST : REST for duration
;     The channel is silenced for the given duration.  For example,
;     a whole rest is effected as:
;
;             DB      MUSEQ_CLIP_REST, MUSEQ_DUR_1
;
; ### Pulse channels
;
; The frequency, frequency sweep period and delta, duty cycle, envelope
; options, and duration can be set for pulse channel clips.  Commands
; consist of one or more bits indicating which properties to modify and
; zero or more bytes following the command byte in sequence as presented
; and only when indicated by a bit's being set:
;
;   01000000b = FREQ_SWPPRD : FREQuency SWeeP PeRioD
;     An additional byte follows that is copied to the channel's frequency
;     sweep period register
;   00100000b = FREQ : FREQuency
;     An additional byte follows that is copied to the channel's frequency
;     register low byte
;   00010000b = ENVELOPE : ENVELOPE level
;     An additional byte follows that is copied to the channel's envelope
;     level register low byte
;   00001000b = FSHFTDUTY : Frequency SHiFT and DUTY cycle
;     An additional byte follows that is copied to the channel's register
;     that holds the 4-bit frequency sweep shift (top nibble) and duty
;     cycle index (lower nibble)
;   00000100b = STATE = channel STATE register
;     An additional byte follows that will be copied to the channel's
;     state register to activate the changes
;
; ### Triangle channel
;
; The frequency, envelope options, and duration can be set for triangle
; channel clips.  Commands consist of one or more bits indicating which
; properties to modify and zero or more bytes following the command byte
; in sequence as presented and only when indicated by a bit's being set:
;
;   01000000b = FREQ : FREQuency
;     An additional byte follows that is copied to the channel's frequency
;     register low byte
;   00100000b = ENVELOPE : ENVELOPE level
;     An additional byte follows that is copied to the channel's envelope
;     level register low byte
;   00010000b = STATE = channel STATE register
;     An additional byte follows that will be copied to the channel's
;     state register to activate the changes
;
; ### Noise channel
;
; The envelope options and duration can be set for noise channel clips.
; Commands consist of one or more bits indicating which properties to
; modify and zero or more bytes following the command byte in sequence
; as presented and only when indicated by a bit's being set:
;
;   01000000b = ENVELOPE : ENVELOPE level
;     An additional byte follows that is copied to the channel's envelope
;     level register low byte
;   00100000b = STATE = channel STATE register
;     An additional byte follows that will be copied to the channel's
;     state register to activate the changes
;
; ## Tracks
;
; A track consists of one or more commands that control a channel's
; rendering of clips.  A track can be linear:  a sequence of clips
; (with each clip played one or more times before progressing) and
; an end.  The track can also loop back to an earlier location
; (once, multiple times, or infinitely).  An indefinite period of
; time can be filled with a musical score that minimizes repetition
; of data (and thus byte-size of the data) in much the same way the
; PPU's use of tiles minimizes the size and complexity of graphics.
;
; All channels use the same commands w.r.t. track handling.
;
;   00000000b = STP : SToP
;     No more commands present, silence the channel.
;
;   00000001b = JMP <ADDR> : JuMP to <ADDR>
;     Reset the channel's track pointer to <ADDR> then repeat
;     the command-handling code.  No other bits will be checked.
;
;   00000010b = NXC <ADDR> <N> : NeXt Clip with loop count <N>
;     Load the clip at <ADDR> and set its loop count to the value
;     in the byte following the <ADDR> byte.
;   00000110b = NXC <ADDR> 1 : NeXt Clip with loop count 1
;     Load the clip at <ADDR> and set its loop count to one.
;
;   00000100b = BTL <N> : Begin Track Loop of <N> iterations
;     Prepare a loop of <N> repeats of some clips; a subsequent
;     DBTL will return track execution to the command following
;     this one <N> times.
;   00001100b = BTL 1 : Begin Track Loop of 1 iteration
;     Prepare a loop of 1 repeat of some clips; a subsequent
;     DBTL will return track execution to the command following
;     this one one time.
;
;   00001000b = DBTL : Decrement and Branch to Track Loop
;     Decrement the counter established by the last BTL command
;     and branch back if it has not reached zero.
;
; For the NXC and BTL commands, the additional bit is used not
; only to indicate an implicit versus explicit count value is
; present, but when set it actually provides the implicit value
; as a side effect!  For example, an NXC command is decoded by
; shifting one zero bit to the right then a one bit to the right
; (and into the CY register).  At that point, the accumulator
; will hold one of two values:
;
;     0b00000000 CY=1   NXS, explicit value present with command
;     0b00000001 CY=1   NXS, implicit value of 1 for command
;
; An ORA A will set/reset the ZERO flag; on ZERO the accumulator is
; loaded with the next byte from the command sequence, on NOT-ZERO
; the value already in the accumulator (1) can be used!  E.g.
;
;                 ORA     A           ; set flags from value in accumulator
;                 JNZ     SET_LC      ; use the 1 in the accumulator
;                 MOV     A, M        ; A <= loop count from command sequence
;                 INX     H
;     SET_LC:     STA     CMD_P0_LOOP ; P0 loop count set to 1 or <N>
;

;
; The APU is mapped at this address:
;
APU_BASE                EQU     0F000h
                        INCLUDE "apu.inc"

;
; New states for each channel get stashed here:
;
MUSEQ_NEEDS_UPDATE      DB      00h         ; Bitmask of channels needing a state update
MUSEQ_P0_NEWSTATE       DB      00h         ; New state for channel P0 (to coordinate note changes)
MUSEQ_P1_NEWSTATE       DB      00h         ; New state for channel P1 (to coordinate note changes)
MUSEQ_T0_NEWSTATE       DB      00h         ; New state for channel T0 (to coordinate note changes)
MUSEQ_N0_NEWSTATE       DB      00h         ; New state for channel N0 (to coordinate note changes)

;
; Each channel's track data are coincident to make
; processing them sequential accesses:
;
MUSEQ_TRK_P0_RESET      DB      80h             ; bit 7 set = disable; 0 = running, otherwise reset
MUSEQ_TRK_P0_LOOPCNT    DB      00h             ; Loop counter on track's current clip
MUSEQ_TRK_P0_LASTCLIP   DW      MUSEQ_CLIP_NULL ; Pointer to clip being played (for loop count > 1)
MUSEQ_TRK_P0_CMDPTR     DW      MUSEQ_TRK_NULL  ; Pointer to next track command
MUSEQ_TRK_P0_TLCNT      DB      00h             ; Loop counter for track looping
MUSEQ_TRK_P0_TLPTR      DW      0000h           ; Pointer to start of track loop
;
MUSEQ_TRK_P1_RESET      DB      80h             ; bit 7 set = disable; 0 = running, otherwise reset
MUSEQ_TRK_P1_LOOPCNT    DB      00h             ; Loop counter on track's current clip
MUSEQ_TRK_P1_LASTCLIP   DW      MUSEQ_CLIP_NULL ; Pointer to clip being played (for loop count > 1)
MUSEQ_TRK_P1_CMDPTR     DW      MUSEQ_TRK_NULL  ; Pointer to next track command
MUSEQ_TRK_P1_TLCNT      DB      00h             ; Loop counter for track looping
MUSEQ_TRK_P1_TLPTR      DW      0000h           ; Pointer to start of track loop
;
MUSEQ_TRK_T0_RESET      DB      80h             ; bit 7 set = disable; 0 = running, otherwise reset
MUSEQ_TRK_T0_LOOPCNT    DB      00h             ; Loop counter on track's current clip
MUSEQ_TRK_T0_LASTCLIP   DW      MUSEQ_CLIP_NULL ; Pointer to clip being played (for loop count > 1)
MUSEQ_TRK_T0_CMDPTR     DW      MUSEQ_TRK_NULL  ; Pointer to next track command
MUSEQ_TRK_T0_TLCNT      DB      00h             ; Loop counter for track looping
MUSEQ_TRK_T0_TLPTR      DW      0000h           ; Pointer to start of track loop
;
MUSEQ_TRK_N0_RESET      DB      80h             ; bit 7 set = disable; 0 = running, otherwise reset
MUSEQ_TRK_N0_LOOPCNT    DB      00h             ; Loop counter on track's current clip
MUSEQ_TRK_N0_LASTCLIP   DW      MUSEQ_CLIP_NULL ; Pointer to clip being played (for loop count > 1)
MUSEQ_TRK_N0_CMDPTR     DW      MUSEQ_TRK_NULL  ; Pointer to next track command
MUSEQ_TRK_N0_TLCNT      DB      00h             ; Loop counter for track looping
MUSEQ_TRK_N0_TLPTR      DW      0000h           ; Pointer to start of track loop

MUSEQ_TRK_NULL          DB      00h

;
; Each channel's clip data:
;
MUSEQ_CLIP_P0_PTR       DW      MUSEQ_CLIP_NULL         ; Pointer to the next note
MUSEQ_CLIP_P0_DURATION  DB      00h                     ; Duration counter
;
MUSEQ_CLIP_P1_PTR       DW      MUSEQ_CLIP_NULL         ; Pointer to the next note
MUSEQ_CLIP_P1_DURATION  DB      00h                     ; Duration counter
;
MUSEQ_CLIP_T0_PTR       DW      MUSEQ_CLIP_NULL         ; Pointer to the next note
MUSEQ_CLIP_T0_DURATION  DB      00h                     ; Duration counter
;
MUSEQ_CLIP_N0_PTR       DW      MUSEQ_CLIP_NULL         ; Pointer to the next note
MUSEQ_CLIP_N0_DURATION  DB      00h                     ; Duration counter
;

MUSEQ_CLIP_NULL         DB      MUSEQ_CLIP_CTRL_EOS

;
; MUSEQ_INIT
; MUsic SEQuencer INITialize
;
; Initialize all invariant options associated with the APU.
;
MUSEQ_INIT:             MVI     A, APU_M_STATE_8B               ; Set the APU to 8-bit E/D/F mode
                        STA     APU_RGSTR_M_STATE
                        MVI     A, 040h                         ; Master volume to 100%
                        STA     APU_RGSTR_M_ENVLVLLO
                        MVI     A, 007h                         ; Pulse 0 channel duty cycle to 50%
                        STA     APU_RGSTR_P0_DUTY_FREQSHFT
                        MVI     A, 003h                         ; Pulse 1 channel duty cycle to 25%
                        STA     APU_RGSTR_P1_DUTY_FREQSHFT
                        RET

;
; MUSEQ_UPDATE
;
; This subroutine is responsible for updating the APU status registers
; when the PPU generates a VBL interrupt.
;
; All registers are preserved (of course!) and interrupts are reenabled
; on exit.
;
MUSEQ_UPDATE:           PUSH    PSW                     ; [11] Save all registers since
                        PUSH    B                       ; [11] the interrupt happens in
                        PUSH    D                       ; [11] the middle of who knows
                        PUSH    H                       ; [11] what…
                        LXI     H, MUSEQ_NEEDS_UPDATE   ; [10] HL <= MUSEQ_NEEDS_UPDATE
                        XRA     A                       ; [ 4] A <= A ^ A = 0
                        ORA     M                       ; [ 7] A <= A | (HL), get flags set
                        JZ      $$NO_UPDATES            ; [10] Nothing needs to be updated
$$TEST_P0:              INX     H                       ; [ 5] HL++ (move to MUSEQ_P0_NEWSTATE)
                        RAL                             ; [ 4] P0 bit (7) set?
                        JNC     $$TEST_P1               ; [10] Bit 7 not set, jump to next test
                        MOV     E, A                    ; [ 5] E <= A (update flags)
                        MOV     A, M                    ; [ 7] A <= new P0 state
                        STA     APU_RGSTR_P0_STATE      ; [13] Pulse channel 0 state <= A
                        MOV     A, E                    ; [ 5] A <= E (update flags)
$$TEST_P1:              INX     H                       ; [ 5] HL++ (move to MUSEQ_P1_NEWSTATE)
                        RAL                             ; [ 4] P1 bit (6) set?
                        JNC     $$TEST_T0               ; [10] Bit 6 not set, jump to next test
                        MOV     E, A                    ; [ 5] E <= A (update flags)
                        MOV     A, M                    ; [ 7] A <= new P1 state
                        STA     APU_RGSTR_P1_STATE      ; [13] Pulse channel 1 state <= A
                        MOV     A, E                    ; [ 5] A <= E (update flags)
$$TEST_T0:              INX     H                       ; [ 5] HL++ (move to MUSEQ_T0_NEWSTATE)
                        RAL                             ; [ 4] T0 bit (5) set?
                        JNC     $$TEST_N0               ; [10] Bit 5 not set, jump to next test
                        MOV     E, A                    ; [ 5] E <= A (update flags)
                        MOV     A, M                    ; [ 7] A <= new T0 state
                        STA     APU_RGSTR_T0_STATE      ; [13] Triangle channel 0 state <= A
                        MOV     A, E                    ; [ 5] A <= E (update flags)
$$TEST_N0:              RAL                             ; [ 4] N0 bit (4) set?
                        JNC     $$TEST_N0               ; [10] Bit 4 not set, end of tests
                        INX     H                       ; [ 5] HL++ (move to MUSEQ_N0_NEWSTATE)
                        MOV     A, M                    ; [ 7] A <= new N0 state
                        STA     APU_RGSTR_N_STATE       ; [13] Noise channel 0 state <= A
$$NO_UPDATES:           XRA     A                       ; [ 4] A <= A ^ A = 0 (remove this eventually)              
                        STA     MAINLOOP_LOCK           ; [13] MAINLOOP_LOCK <= A (remove this eventually)
                        POP     H                       ; [10] Restore all saved registers…
                        POP     D                       ; [10]
                        POP     B                       ; [10]
                        POP     PSW                     ; [10]
                        EI                              ; [ 4] Reenable interrupts
                        RET                             ; [10] Return from interrupt handler

;
; MUSEQ_P0_STEP
; MUsical SEQuencer, Pulse 0 STEP
;
; Progress the state of pulse channel 0.  On entry the E register holds
; the bitmask that indicates channels' needing their state updated with
; the APU; on exit, this subroutine will have updated the bit for this
; channel.
;
; There is a secondary entry point into this subroutine at
; MUSEQ_P0_TRK_START.  This is the point where the next track command
; is loaded — or where the first track command can be loaded on the
; first call to audio rendering after MUSEQ_TRK_P0_CMDPTR was loaded.
;
MUSEQ_P0_STEP:          LDA     MUSEQ_TRK_P0_RESET      ; [13] Check for reset flag
                        ORA     A                       ; [ 4]
                        JZ      $$NORMAL_STEP           ; [10]
                        JP      $$NEW_TRK_STEP          ; [10]
                        RET                             ; [10]
$$NEW_TRK_STEP:         XRA     A                       ; [ 4]
                        STA     MUSEQ_TRK_P0_RESET      ; [13]
                        JMP     $$TRK_NEXT_CMD          ; [10]
                        
$$NORMAL_STEP:          LDA     MUSEQ_CLIP_P0_DURATION  ; [13] Load duration slot
                        ORA     A                       ; [ 4] Has duration reached zero?
                        JZ      $$CLIP_NEXT_STEP        ; [10] Yes, load a new step from the sequence
                        DCR     A                       ; [ 5] No, decrement the counter…
                        STA     MUSEQ_CLIP_P0_DURATION  ; [13] …store it back…
                        RET                             ; [10] …and exit.

$$CLIP_NEXT_STEP:       LHLD    MUSEQ_CLIP_P0_PTR       ; [16] HL <= sequence pointer
                        MOV     A, M                    ; [ 7] A <= control byte
                        ORA     A                       ; [ 4] A <= A | A (set flags)
                        JNZ     $$CLIP_NOT_EOS          ; [10] control byte is not SEQ_CRTL_EOS
                        
                        ;
                        ; The current clip has reached its end, so attempt to
                        ; progress the track state:
                        ;
                        LDA     MUSEQ_TRK_P0_LOOPCNT    ; [13] A <= clip's loop count
                        DCR     A                       ; [ 5] A--
                        JZ      $$TRK_NEXT_CMD          ; [10] If we've reached zero, move to the next command
                        STA     MUSEQ_TRK_P0_LOOPCNT    ; [13]
                        LHLD    MUSEQ_TRK_P0_LASTCLIP   ; [16] HL <= pointer to clip that will be repeated
                        SHLD    MUSEQ_CLIP_P0_PTR       ; [16] Reset clip pointer to the clip being repeated
                        JMP     $$CLIP_NEXT_STEP        ; [10] Start from beginning of that loop...

$$TRK_NEXT_CMD:         LHLD    MUSEQ_TRK_P0_CMDPTR     ; [16] HL <= pointer to next track command
$$TRK_LOAD_CMD:         MOV     A, M                    ; [ 7] A <= track command byte
                        ORA     A                       ; [ 4] Get flags set for value in A
                        JNZ     $$TRK_CMD_IS_JMP        ; [10] If the command is not STP (0), move on

$$TRK_STOP:             STA     MUSEQ_P0_NEWSTATE       ; [13] A is already 0 (STP command), set the channel
                                                        ;      state to that
                        ORI     MUSEQ_P0_UPDATE_BIT     ; [ 7] …set the state change bit…
                        MOV     E, A                    ; [ 5] …in the E register…
                        MVI     A, 0FFh                 ; [ 7] …and set duration to max…
                        STA     MUSEQ_CLIP_P0_DURATION  ; [13] …to avoid checking a MUSEQ_CLIP_CTRL_EOS every
                                                        ;       step now that we're done…
                        STA     MUSEQ_TRK_P0_LOOPCNT    ; [13] …and also loop the track STP command for the
                                                        ;      same reason…
                        RET                             ; [10] …then return.

$$TRK_CMD_IS_JMP:       RAR                             ; [ 4] The ORA A above cleared carry, so we're doing a
                                                        ;      logical shift right, sending bit 0 into CY
                        JNC     $$TRK_CMD_IS_NXC        ; [10] Skip ahead to NeXt Clip check
$$TRK_JMP:              PUSH    D                       ; [11] Stash DE
                        INX     H                       ; [ 5] HL++
                        MOV     E, M                    ; [ 7] E <= low-byte of <ADDR>
                        INX     H                       ; [ 5] HL++
                        MOV     D, M                    ; [ 7] D <= high-byte of <ADDR>
                        XCHG                            ; [ 4] HL <=> DE
                        POP     D                       ; [10] Restore previous DE
                        SHLD    MUSEQ_TRK_P0_CMDPTR     ; [16] Pointer to next track command <= HL
                        JMP     $$TRK_LOAD_CMD          ; [10] Go back and attempt to load the command we've jumped to

$$TRK_CMD_IS_NXC:       RAR                             ; [ 4] The CY flag is still not set, so this is still a
                                                        ;      logical shift right, sending bit 1 into CY
                        JNC     $$TRK_CMD_IS_BTL        ; [10] Skip ahead to Begin Clip Loop
$$TRK_NXC:              PUSH    D                       ; [11] Stash DE
                        INX     H                       ; [ 5] HL++
                        MOV     E, M                    ; [ 7] E <= low-byte of <ADDR>
                        INX     H                       ; [ 5] HL++
                        MOV     D, M                    ; [ 7] D <= high-byte of <ADDR>
                        INX     H                       ; [ 5] HL++
                        ORA     A                       ; [ 4] Get flags set for A = remnant of command byte
                        JNZ     $$TRK_NCX_SET           ; [10] If it's not zero, it's the implicit one loop count
                        MOV     A, M                    ; [ 7] A <= clip loop count
                        INX     H                       ; [ 5] HL++
$$TRK_NCX_SET:          STA     MUSEQ_TRK_P0_LOOPCNT    ; [13] Clip loop count <= A
                        SHLD    MUSEQ_TRK_P0_CMDPTR     ; [16] Pointer to next track command <= HL                        
                        XCHG                            ; [ 4] HL <=> DE
                        POP     D                       ; [10] Restore previous DE
                        SHLD    MUSEQ_TRK_P0_LASTCLIP   ; [16] Pointer to current clip in track <= HL
                        SHLD    MUSEQ_CLIP_P0_PTR       ; [16] Pointer to current clip <= HL
                        JMP     $$CLIP_NEXT_STEP        ; [10] Go back and start processing the clip
                        
$$TRK_CMD_IS_BTL:       RAR                             ; [ 4] The CY flag is still not set, so this is still a
                                                        ;      logical shift right, sending bit 2 into CY
                        JNC     $$TRK_CMD_IS_DBTL       ; [10] Skip ahead to Decrement and Branch to Clip Loop
$$TRK_BTL:              ORA     A                       ; [ 4] Get flags set to check for zero or not
                        INX     H                       ; [ 5] HL++
                        JNZ     $$TRK_BTL_SET           ; [10] If it's not zero, it's the implicit two loop count
                        MOV     A, M                    ; [ 7] A <= clip loop count
                        INX     H
$$TRK_BTL_SET:          STA     MUSEQ_TRK_P0_TLCNT      ; [13] Track loop count <= A
                        SHLD    MUSEQ_TRK_P0_TLPTR      ; [16] Track loop pointer <= HL (next command)
                        JMP     $$TRK_LOAD_CMD          ; [10] Go back and process the next track command
                        
$$TRK_CMD_IS_DBTL:                                      ; We will assume that any non-zero command IS DBTL
                                                        ; without bothering to check; assume the programmer knows
                                                        ; what he is doing!
$$TRK_DBTL:             LDA     MUSEQ_TRK_P0_TLCNT      ; [13] A <= track loop count
                        DCR     A                       ; [ 5] A--
                        JP      $$TRK_DBTL_LOOP         ; [10] Skip ahead to looping on non-negative
$$TRK_DBTL_EXIT:        INX     H                       ; [ 5] HL++
                        JMP     $$TRK_LOAD_CMD          ; [10] Go back and process the next track command
$$TRK_DBTL_LOOP:        STA     MUSEQ_TRK_P0_TLCNT      ; [13] Track loop count <= A
                        LHLD    MUSEQ_TRK_P0_TLPTR      ; [16] HL <= track loop pointer (next command)
                        JMP     $$TRK_LOAD_CMD          ; [10] Go back and process the next track command


$$CLIP_NOT_EOS:         INX     H                       ; [ 5] sequence ptr++
                        MOV     B, A                    ; [ 5] B <= copy of control byte
                        MOV     A, M                    ; [ 7] A <= duration (everything will have one)
                        INX     H                       ; [ 5] sequence ptr++
                        STA     MUSEQ_CLIP_P0_DURATION  ; [13] Update the duration
                        MOV     A, B                    ; [ 5] A <= control byte
                        ANI     ~MUSEQ_CLIP_CTRL_CONT   ; [ 7] A <= control byte & ~MUSEQ_CLIP_CTRL_CONT
                        JNZ     $$CLIP_NOT_CONT         ; [10] Not MUSEQ_CLIP_CTRL_CONT, handle any changes
                        SHLD    MUSEQ_CLIP_P0_PTR       ; [16] Save the sequence pointer…
                        MOV     A, E                    ; [ 5]
                        ORI     MUSEQ_P0_UPDATE_BIT     ; [ 7] …set the state change bit to reuse the last
                                                        ;      state set…
                        MOV     E, A                    ; [ 5]
                        RET                             ; [10] …and return.
                        
$$CLIP_NOT_CONT:        JP      $$CLIP_NOT_REST         ; [10] The CPI MUSEQ_CLIP_CTRL_CONT above will have
                                                        ;      set the M flag if bit 7 was set
                        SHLD    MUSEQ_CLIP_P0_PTR       ; [16] Save the sequence pointer…
                        LDA     MUSEQ_P0_NEWSTATE       ; [13] …then load the existing channel state…
                        ANI     01111111b               ; [ 7] …drop the enable bit…
                        STA     MUSEQ_P0_NEWSTATE       ; [13] …and write it back to stop the channel…
                        MOV     A, E                    ; [ 5]
                        ORI     MUSEQ_P0_UPDATE_BIT     ; [ 7] …set the state change bit…
                        MOV     E, A                    ; [ 5]
                        RET                             ; [10] …and return.

$$CLIP_NOT_REST:        RAL                             ; [ 4] A <= A << 1, CY <= bit 7 (MUSEQ_CLIP_CTRL_REST)
                        RAL                             ; [ 4] A <= A << 1, CY <= bit 6
                                                        ;      (MUSEQ_CLIP_CTRL_P_FREQ_SWPPRD)
                        JNC     $$CLIP_NO_FREQ_SWPPRD   ; [10] No frequency sweep period
                        MOV     B, A                    ; [ 5] B <= copy of shifted control byte
                        MOV     A, M                    ; [ 7] A <= frequency sweep period byte
                        INX     H                       ; [ 5] sequence ptr++
                        STA     APU_RGSTR_P0_FREQSWPPERIOD
                                                        ; [13] Set frequency sweep period
                        MOV     A, B                    ; [ 5] A <= shifted control byte

$$CLIP_NO_FREQ_SWPPRD:  RAL                             ; [ 4] A <= A << 1, CY <= bit 5 (MUSEQ_CLIP_CTRL_P_FREQ)
                        JNC     $$CLIP_NO_FREQ          ; [10] No frequency change
                        MOV     B, A                    ; [ 5] B <= copy of shifted control byte
                        MOV     A, M                    ; [ 7] A <= frequency byte
                        INX     H                       ; [ 5] sequence ptr++
                        STA     APU_RGSTR_P0_FREQLO     ; [13] Set frequency index
                        MOV     A, B                    ; [ 5] A <= shifted control byte

$$CLIP_NO_FREQ:         RAL                             ; [ 4] A <= A << 1, CY <= bit 4
                                                        ;      (MUSEQ_CLIP_CTRL_P_ENVELOPE)
                        JNC     $$CLIP_NO_ENVELOPE      ; [10] No frequency change
                        MOV     B, A                    ; [ 5] B <= copy of shifted control byte
                        MOV     A, M                    ; [ 7] A <= frequency byte
                        INX     H                       ; [ 5] sequence ptr++
                        STA     APU_RGSTR_P0_ENVLVLLO
                                                        ; [13] Set envelope option (level or decay
                                                        ;      period)
                        MOV     A, B                    ; [ 5] A <= shifted control byte

$$CLIP_NO_ENVELOPE:     RAL                             ; [ 4] A <= A << 1, CY <= bit 3
                                                        ;      (MUSEQ_CLIP_CTRL_P_FSHFTDUTY)
                        JNC     $$CLIP_NO_FSHFTDUTY     ; [10] No frequency shift + duty cycle change
                        MOV     B, A                    ; [ 5] B <= copy of shifted control byte
                        MOV     A, M                    ; [ 7] A <= frequency shift + duty cycle byte
                        INX     H                       ; [ 5] sequence ptr++
                        STA     APU_RGSTR_P0_DUTY_FREQSHFT
                                                        ; [13] Set frequency shift + duty cycle
                        MOV     A, B                    ; [ 5] A <= shifted control byte

$$CLIP_NO_FSHFTDUTY:    RAL                             ; [ 4] A <= A << 1, CY <= bit 2 (MUSEQ_CLIP_CTRL_P_STATE)
                        JNC     $$CLIP_NO_STATE         ; [10] No state byte changes (besides enable)
                        MOV     A, M                    ; [ 7] A <= new state byte
                        INX     H                       ; [ 5] sequence ptr++
                        SHLD    MUSEQ_CLIP_P0_PTR       ; [16] Save the sequence pointer…
                        STA     MUSEQ_P0_NEWSTATE       ; [13] …write the state register…
                        MOV     A, E                    ; [ 5]
                        ORI     MUSEQ_P0_UPDATE_BIT     ; [ 7] …set the state change bit to reuse the last
                                                        ;      state set…
                        MOV     E, A                    ; [ 5]
                        RET                             ; [10] …and return.

$$CLIP_NO_STATE:        SHLD    MUSEQ_CLIP_P0_PTR       ; [16] Save the sequence pointer…
                        LDA     MUSEQ_P0_NEWSTATE       ; [13] …then load the existing state…
                        ORI     10000000b               ; [ 7] …ensure the channel is enabled…
                        STA     MUSEQ_P0_NEWSTATE       ; [13] …write the state register…
                        MOV     A, E                    ; [ 5]
                        ORI     MUSEQ_P0_UPDATE_BIT     ; [ 7] …set the state change bit to reuse the last
                                                        ;      state set…
                        MOV     E, A                    ; [ 5]
                        RET                             ; [10] …and return.

MUSEQ_P0_TRK_START      EQU     $$TRK_NEXT_CMD

;
; MUSEQ_P1_STEP
; MUsical SEQuencer, Pulse 1 STEP
;
; Progress the state of pulse channel 1.  On entry the E register holds
; the bitmask that indicates channels' needing their state updated with
; the APU; on exit, this subroutine will have updated the bit for this
; channel.
;
; There is a secondary entry point into this subroutine at
; MUSEQ_P1_TRK_START.  This is the point where the next track command
; is loaded — or where the first track command can be loaded on the
; first call to audio rendering after MUSEQ_TRK_P1_CMDPTR was loaded.
;
MUSEQ_P1_STEP:          LDA     MUSEQ_TRK_P1_RESET      ; [13] Check for reset flag
                        ORA     A                       ; [ 4]
                        JZ      $$NORMAL_STEP           ; [10]
                        JP      $$NEW_TRK_STEP          ; [10]
                        RET                             ; [10]
$$NEW_TRK_STEP:         XRA     A                       ; [ 4]
                        STA     MUSEQ_TRK_P1_RESET      ; [13]
                        JMP     $$TRK_NEXT_CMD          ; [10]
                        
$$NORMAL_STEP:          LDA     MUSEQ_CLIP_P1_DURATION  ; [13] Load duration slot
                        ORA     A                       ; [ 4] Has duration reached zero?
                        JZ      $$CLIP_NEXT_STEP        ; [10] Yes, load a new step from the sequence
                        DCR     A                       ; [ 5] No, decrement the counter…
                        STA     MUSEQ_CLIP_P1_DURATION  ; [13] …store it back…
                        RET                             ; [10] …and exit.

$$CLIP_NEXT_STEP:       LHLD    MUSEQ_CLIP_P1_PTR       ; [16] HL <= sequence pointer
                        MOV     A, M                    ; [ 7] A <= control byte
                        ORA     A                       ; [ 4] A <= A | A (set flags)
                        JNZ     $$CLIP_NOT_EOS          ; [10] control byte is not SEQ_CRTL_EOS
                        
                        ;
                        ; The current clip has reached its end, so attempt to
                        ; progress the track state:
                        ;
                        LDA     MUSEQ_TRK_P1_LOOPCNT    ; [13] A <= clip's loop count
                        DCR     A                       ; [ 5] A--
                        JZ      $$TRK_NEXT_CMD          ; [10] If we've reached zero, move to the next command
                        STA     MUSEQ_TRK_P1_LOOPCNT    ; [13]
                        LHLD    MUSEQ_TRK_P1_LASTCLIP   ; [16] HL <= pointer to clip that will be repeated
                        SHLD    MUSEQ_CLIP_P1_PTR       ; [16] Reset clip pointer to the clip being repeated
                        JMP     $$CLIP_NEXT_STEP        ; [10] Start from beginning of that loop...

$$TRK_NEXT_CMD:         LHLD    MUSEQ_TRK_P1_CMDPTR     ; [16] HL <= pointer to next track command
$$TRK_LOAD_CMD:         MOV     A, M                    ; [ 7] A <= track command byte
                        ORA     A                       ; [ 4] Get flags set for value in A
                        JNZ     $$TRK_CMD_IS_JMP        ; [10] If the command is not STP (0), move on

$$TRK_STOP:             STA     MUSEQ_P1_NEWSTATE       ; [13] A is already 0 (STP command), set the channel
                                                        ;      state to that
                        ORI     MUSEQ_P1_UPDATE_BIT     ; [ 7] …set the state change bit…
                        MOV     E, A                    ; [ 5] …in the E register…
                        MVI     A, 0FFh                 ; [ 7] …and set duration to max…
                        STA     MUSEQ_CLIP_P1_DURATION  ; [13] …to avoid checking a MUSEQ_CLIP_CTRL_EOS every
                                                        ;       step now that we're done…
                        STA     MUSEQ_TRK_P1_LOOPCNT    ; [13] …and also loop the track STP command for the
                                                        ;      same reason…
                        RET                             ; [10] …then return.

$$TRK_CMD_IS_JMP:       RAR                             ; [ 4] The ORA A above cleared carry, so we're doing a
                                                        ;      logical shift right, sending bit 0 into CY
                        JNC     $$TRK_CMD_IS_NXC        ; [10] Skip ahead to NeXt Clip check
$$TRK_JMP:              PUSH    D                       ; [11] Stash DE
                        INX     H                       ; [ 5] HL++
                        MOV     E, M                    ; [ 7] E <= low-byte of <ADDR>
                        INX     H                       ; [ 5] HL++
                        MOV     D, M                    ; [ 7] D <= high-byte of <ADDR>
                        XCHG                            ; [ 4] HL <=> DE
                        POP     D                       ; [10] Restore previous DE
                        SHLD    MUSEQ_TRK_P1_CMDPTR     ; [16] Pointer to next track command <= HL
                        JMP     $$TRK_LOAD_CMD          ; [10] Go back and attempt to load the command we've jumped to

$$TRK_CMD_IS_NXC:       RAR                             ; [ 4] The CY flag is still not set, so this is still a
                                                        ;      logical shift right, sending bit 1 into CY
                        JNC     $$TRK_CMD_IS_BTL        ; [10] Skip ahead to Begin Clip Loop
$$TRK_NXC:              PUSH    D                       ; [11] Stash DE
                        INX     H                       ; [ 5] HL++
                        MOV     E, M                    ; [ 7] E <= low-byte of <ADDR>
                        INX     H                       ; [ 5] HL++
                        MOV     D, M                    ; [ 7] D <= high-byte of <ADDR>
                        INX     H                       ; [ 5] HL++
                        ORA     A                       ; [ 4] Get flags set for A = remnant of command byte
                        JNZ     $$TRK_NCX_SET           ; [10] If it's not zero, it's the implicit one loop count
                        MOV     A, M                    ; [ 7] A <= clip loop count
                        INX     H                       ; [ 5] HL++
$$TRK_NCX_SET:          STA     MUSEQ_TRK_P1_LOOPCNT    ; [13] Clip loop count <= A
                        SHLD    MUSEQ_TRK_P1_CMDPTR     ; [16] Pointer to next track command <= HL                        
                        XCHG                            ; [ 4] HL <=> DE
                        POP     D                       ; [10] Restore previous DE
                        SHLD    MUSEQ_TRK_P1_LASTCLIP   ; [16] Pointer to current clip in track <= HL
                        SHLD    MUSEQ_CLIP_P1_PTR       ; [16] Pointer to current clip <= HL
                        JMP     MUSEQ_P1_STEP           ; [10] Go back and start processing the clip
                        
$$TRK_CMD_IS_BTL:       RAR                             ; [ 4] The CY flag is still not set, so this is still a
                                                        ;      logical shift right, sending bit 2 into CY
                        JNC     $$TRK_CMD_IS_DBTL       ; [10] Skip ahead to Decrement and Branch to Clip Loop
$$TRK_BTL:              ORA     A                       ; [ 4] Get flags set to check for zero or not
                        INX     H                       ; [ 5] HL++
                        JNZ     $$TRK_BTL_SET           ; [10] If it's not zero, it's the implicit two loop count
                        MOV     A, M                    ; [ 7] A <= clip loop count
                        INX     H
$$TRK_BTL_SET:          STA     MUSEQ_TRK_P1_TLCNT      ; [13] Track loop count <= A
                        SHLD    MUSEQ_TRK_P1_TLPTR      ; [16] Track loop pointer <= HL (next command)
                        JMP     $$TRK_LOAD_CMD          ; [10] Go back and process the next track command
                        
$$TRK_CMD_IS_DBTL:                                      ; We will assume that any non-zero command IS DBTL
                                                        ; without bothering to check; assume the programmer knows
                                                        ; what he is doing!
$$TRK_DBTL:             LDA     MUSEQ_TRK_P1_TLCNT      ; [13] A <= track loop count
                        DCR     A                       ; [ 5] A--
                        JP      $$TRK_DBTL_LOOP         ; [10] Skip ahead to looping on non-negative
$$TRK_DBTL_EXIT:        INX     H                       ; [ 5] HL++
                        JMP     $$TRK_LOAD_CMD          ; [10] Go back and process the next track command
$$TRK_DBTL_LOOP:        STA     MUSEQ_TRK_P1_TLCNT      ; [13] Track loop count <= A
                        LHLD    MUSEQ_TRK_P1_TLPTR      ; [16] HL <= track loop pointer (next command)
                        JMP     $$TRK_LOAD_CMD          ; [10] Go back and process the next track command


$$CLIP_NOT_EOS:         INX     H                       ; [ 5] sequence ptr++
                        MOV     B, A                    ; [ 5] B <= copy of control byte
                        MOV     A, M                    ; [ 7] A <= duration (everything will have one)
                        INX     H                       ; [ 5] sequence ptr++
                        STA     MUSEQ_CLIP_P1_DURATION  ; [13] Update the duration
                        MOV     A, B                    ; [ 5] A <= control byte
                        ANI     ~MUSEQ_CLIP_CTRL_CONT   ; [ 7] A <= control byte & ~MUSEQ_CLIP_CTRL_CONT
                        JNZ     $$CLIP_NOT_CONT         ; [10] Not MUSEQ_CLIP_CTRL_CONT, handle any changes
                        SHLD    MUSEQ_CLIP_P1_PTR       ; [16] Save the sequence pointer…
                        MOV     A, E                    ; [ 5]
                        ORI     MUSEQ_P1_UPDATE_BIT     ; [ 7] …set the state change bit to reuse the last
                                                        ;      state set…
                        MOV     E, A                    ; [ 5]
                        RET                             ; [10] …and return.
                        
$$CLIP_NOT_CONT:        JP      $$CLIP_NOT_REST         ; [10] The CPI MUSEQ_CLIP_CTRL_CONT above will have
                                                        ;      set the M flag if bit 7 was set
                        SHLD    MUSEQ_CLIP_P1_PTR       ; [16] Save the sequence pointer…
                        LDA     MUSEQ_P1_NEWSTATE       ; [13] …then load the existing channel state…
                        ANI     01111111b               ; [ 7] …drop the enable bit…
                        STA     MUSEQ_P1_NEWSTATE       ; [13] …and write it back to stop the channel…
                        MOV     A, E                    ; [ 5]
                        ORI     MUSEQ_P1_UPDATE_BIT     ; [ 7] …set the state change bit…
                        MOV     E, A                    ; [ 5]
                        RET                             ; [10] …and return.

$$CLIP_NOT_REST:        RAL                             ; [ 4] A <= A << 1, CY <= bit 7 (MUSEQ_CLIP_CTRL_REST)
                        RAL                             ; [ 4] A <= A << 1, CY <= bit 6
                                                        ;      (MUSEQ_CLIP_CTRL_P_FREQ_SWPPRD)
                        JNC     $$CLIP_NO_FREQ_SWPPRD   ; [10] No frequency sweep period
                        MOV     B, A                    ; [ 5] B <= copy of shifted control byte
                        MOV     A, M                    ; [ 7] A <= frequency sweep period byte
                        INX     H                       ; [ 5] sequence ptr++
                        STA     APU_RGSTR_P1_FREQSWPPERIOD
                                                        ; [13] Set frequency sweep period
                        MOV     A, B                    ; [ 5] A <= shifted control byte

$$CLIP_NO_FREQ_SWPPRD:  RAL                             ; [ 4] A <= A << 1, CY <= bit 5 (MUSEQ_CLIP_CTRL_P_FREQ)
                        JNC     $$CLIP_NO_FREQ          ; [10] No frequency change
                        MOV     B, A                    ; [ 5] B <= copy of shifted control byte
                        MOV     A, M                    ; [ 7] A <= frequency byte
                        INX     H                       ; [ 5] sequence ptr++
                        STA     APU_RGSTR_P1_FREQLO     ; [13] Set frequency index
                        MOV     A, B                    ; [ 5] A <= shifted control byte

$$CLIP_NO_FREQ:         RAL                             ; [ 4] A <= A << 1, CY <= bit 4
                                                        ;      (MUSEQ_CLIP_CTRL_P_ENVELOPE)
                        JNC     $$CLIP_NO_ENVELOPE      ; [10] No frequency change
                        MOV     B, A                    ; [ 5] B <= copy of shifted control byte
                        MOV     A, M                    ; [ 7] A <= frequency byte
                        INX     H                       ; [ 5] sequence ptr++
                        STA     APU_RGSTR_P1_ENVLVLLO
                                                        ; [13] Set envelope option (level or decay
                                                        ;      period)
                        MOV     A, B                    ; [ 5] A <= shifted control byte

$$CLIP_NO_ENVELOPE:     RAL                             ; [ 4] A <= A << 1, CY <= bit 3
                                                        ;      (MUSEQ_CLIP_CTRL_P_FSHFTDUTY)
                        JNC     $$CLIP_NO_FSHFTDUTY     ; [10] No frequency shift + duty cycle change
                        MOV     B, A                    ; [ 5] B <= copy of shifted control byte
                        MOV     A, M                    ; [ 7] A <= frequency shift + duty cycle byte
                        INX     H                       ; [ 5] sequence ptr++
                        STA     APU_RGSTR_P1_DUTY_FREQSHFT
                                                        ; [13] Set frequency shift + duty cycle
                        MOV     A, B                    ; [ 5] A <= shifted control byte

$$CLIP_NO_FSHFTDUTY:    RAL                             ; [ 4] A <= A << 1, CY <= bit 2 (MUSEQ_CLIP_CTRL_P_STATE)
                        JNC     $$CLIP_NO_STATE         ; [10] No state byte changes (besides enable)
                        MOV     A, M                    ; [ 7] A <= new state byte
                        INX     H                       ; [ 5] sequence ptr++
                        SHLD    MUSEQ_CLIP_P1_PTR       ; [16] Save the sequence pointer…
                        STA     MUSEQ_P1_NEWSTATE       ; [13] …write the state register…
                        MOV     A, E                    ; [ 5]
                        ORI     MUSEQ_P1_UPDATE_BIT     ; [ 7] …set the state change bit to reuse the last
                                                        ;      state set…
                        MOV     E, A                    ; [ 5]
                        RET                             ; [10] …and return.

$$CLIP_NO_STATE:        SHLD    MUSEQ_CLIP_P1_PTR       ; [16] Save the sequence pointer…
                        LDA     MUSEQ_P1_NEWSTATE       ; [13] …then load the existing state…
                        ORI     10000000b               ; [ 7] …ensure the channel is enabled…
                        STA     MUSEQ_P1_NEWSTATE       ; [13] …write the state register…
                        MOV     A, E                    ; [ 5]
                        ORI     MUSEQ_P1_UPDATE_BIT     ; [ 7] …set the state change bit to reuse the last
                                                        ;      state set…
                        MOV     E, A                    ; [ 5]
                        RET                             ; [10] …and return.

MUSEQ_P1_TRK_START      EQU     $$TRK_NEXT_CMD


;
; MUSEQ_T0_STEP
; MUsical SEQuencer, Triangle 0 STEP
;
; Progress the state of triangle channel 0.  On entry the E register holds
; the bitmask that indicates channels' needing their state updated with
; the APU; on exit, this subroutine will have updated the bit for this
; channel.
;
; There is a secondary entry point into this subroutine at
; MUSEQ_T0_TRK_START.  This is the point where the next track command
; is loaded — or where the first track command can be loaded on the
; first call to audio rendering after MUSEQ_TRK_T0_CMDPTR was loaded.
;
MUSEQ_T0_STEP:          LDA     MUSEQ_TRK_T0_RESET      ; [13] Check for reset flag
                        ORA     A                       ; [ 4]
                        JZ      $$NORMAL_STEP           ; [10]
                        JP      $$NEW_TRK_STEP          ; [10]
                        RET                             ; [10]
$$NEW_TRK_STEP:         XRA     A                       ; [ 4]
                        STA     MUSEQ_TRK_T0_RESET      ; [13]
                        JMP     $$TRK_NEXT_CMD          ; [10]
                        
$$NORMAL_STEP:          LDA     MUSEQ_CLIP_T0_DURATION  ; [13] Load duration slot
                        ORA     A                       ; [ 4] Has duration reached zero?
                        JZ      $$CLIP_NEXT_STEP        ; [10] Yes, load a new step from the sequence
                        DCR     A                       ; [ 5] No, decrement the counter…
                        STA     MUSEQ_CLIP_T0_DURATION  ; [13] …store it back…
                        RET                             ; [10] …and exit.

$$CLIP_NEXT_STEP:       LHLD    MUSEQ_CLIP_T0_PTR       ; [16] HL <= sequence pointer
                        MOV     A, M                    ; [ 7] A <= control byte
                        ORA     A                       ; [ 4] A <= A | A (set flags)
                        JNZ     $$CLIP_NOT_EOS          ; [10] control byte is not SEQ_CRTL_EOS
                        
                        ;
                        ; The current clip has reached its end, so attempt to
                        ; progress the track state:
                        ;
                        LDA     MUSEQ_TRK_T0_LOOPCNT    ; [13] A <= clip's loop count
                        DCR     A                       ; [ 5] A--
                        JZ      $$TRK_NEXT_CMD          ; [10] If we've reached zero, move to the next command
                        STA     MUSEQ_TRK_T0_LOOPCNT    ; [13]
                        LHLD    MUSEQ_TRK_T0_LASTCLIP   ; [16] HL <= pointer to clip that will be repeated
                        SHLD    MUSEQ_CLIP_T0_PTR       ; [16] Reset clip pointer to the clip being repeated
                        JMP     $$CLIP_NEXT_STEP        ; [10] Start from beginning of that loop...

$$TRK_NEXT_CMD:         LHLD    MUSEQ_TRK_T0_CMDPTR     ; [16] HL <= pointer to next track command
$$TRK_LOAD_CMD:         MOV     A, M                    ; [ 7] A <= track command byte
                        ORA     A                       ; [ 4] Get flags set for value in A
                        JNZ     $$TRK_CMD_IS_JMP        ; [10] If the command is not STP (0), move on

$$TRK_STOP:             STA     MUSEQ_T0_NEWSTATE       ; [13] A is already 0 (STP command), set the channel
                                                        ;      state to that
                        ORI     MUSEQ_T0_UPDATE_BIT     ; [ 7] …set the state change bit…
                        MOV     E, A                    ; [ 5] …in the E register…
                        MVI     A, 0FFh                 ; [ 7] …and set duration to max…
                        STA     MUSEQ_CLIP_T0_DURATION  ; [13] …to avoid checking a MUSEQ_CLIP_CTRL_EOS every
                                                        ;       step now that we're done…
                        STA     MUSEQ_TRK_T0_LOOPCNT    ; [13] …and also loop the track STP command for the
                                                        ;      same reason…
                        RET                             ; [10] …then return.

$$TRK_CMD_IS_JMP:       RAR                             ; [ 4] The ORA A above cleared carry, so we're doing a
                                                        ;      logical shift right, sending bit 0 into CY
                        JNC     $$TRK_CMD_IS_NXC        ; [10] Skip ahead to NeXt Clip check
$$TRK_JMP:              PUSH    D                       ; [11] Stash DE
                        INX     H                       ; [ 5] HL++
                        MOV     E, M                    ; [ 7] E <= low-byte of <ADDR>
                        INX     H                       ; [ 5] HL++
                        MOV     D, M                    ; [ 7] D <= high-byte of <ADDR>
                        XCHG                            ; [ 4] HL <=> DE
                        POP     D                       ; [10] Restore previous DE
                        SHLD    MUSEQ_TRK_T0_CMDPTR     ; [16] Pointer to next track command <= HL
                        JMP     $$TRK_LOAD_CMD          ; [10] Go back and attempt to load the command we've jumped to

$$TRK_CMD_IS_NXC:       RAR                             ; [ 4] The CY flag is still not set, so this is still a
                                                        ;      logical shift right, sending bit 1 into CY
                        JNC     $$TRK_CMD_IS_BTL        ; [10] Skip ahead to Begin Clip Loop
$$TRK_NXC:              PUSH    D                       ; [11] Stash DE
                        INX     H                       ; [ 5] HL++
                        MOV     E, M                    ; [ 7] E <= low-byte of <ADDR>
                        INX     H                       ; [ 5] HL++
                        MOV     D, M                    ; [ 7] D <= high-byte of <ADDR>
                        INX     H                       ; [ 5] HL++
                        ORA     A                       ; [ 4] Get flags set for A = remnant of command byte
                        JNZ     $$TRK_NCX_SET           ; [10] If it's not zero, it's the implicit one loop count
                        MOV     A, M                    ; [ 7] A <= clip loop count
                        INX     H                       ; [ 5] HL++
$$TRK_NCX_SET:          STA     MUSEQ_TRK_T0_LOOPCNT    ; [13] Clip loop count <= A
                        SHLD    MUSEQ_TRK_T0_CMDPTR     ; [16] Pointer to next track command <= HL                        
                        XCHG                            ; [ 4] HL <=> DE
                        POP     D                       ; [10] Restore previous DE
                        SHLD    MUSEQ_TRK_T0_LASTCLIP   ; [16] Pointer to current clip in track <= HL
                        SHLD    MUSEQ_CLIP_T0_PTR       ; [16] Pointer to current clip <= HL
                        JMP     $$CLIP_NEXT_STEP        ; [10] Go back and start processing the clip
                        
$$TRK_CMD_IS_BTL:       RAR                             ; [ 4] The CY flag is still not set, so this is still a
                                                        ;      logical shift right, sending bit 2 into CY
                        JNC     $$TRK_CMD_IS_DBTL       ; [10] Skip ahead to Decrement and Branch to Clip Loop
$$TRK_BTL:              ORA     A                       ; [ 4] Get flags set to check for zero or not
                        INX     H                       ; [ 5] HL++
                        JNZ     $$TRK_BTL_SET           ; [10] If it's not zero, it's the implicit two loop count
                        MOV     A, M                    ; [ 7] A <= clip loop count
                        INX     H
$$TRK_BTL_SET:          STA     MUSEQ_TRK_T0_TLCNT      ; [13] Track loop count <= A
                        SHLD    MUSEQ_TRK_T0_TLPTR      ; [16] Track loop pointer <= HL (next command)
                        JMP     $$TRK_LOAD_CMD          ; [10] Go back and process the next track command
                        
$$TRK_CMD_IS_DBTL:                                      ; We will assume that any non-zero command IS DBTL
                                                        ; without bothering to check; assume the programmer knows
                                                        ; what he is doing!
$$TRK_DBTL:             LDA     MUSEQ_TRK_T0_TLCNT      ; [13] A <= track loop count
                        DCR     A                       ; [ 5] A--
                        JP      $$TRK_DBTL_LOOP         ; [10] Skip ahead to looping on non-negative
$$TRK_DBTL_EXIT:        INX     H                       ; [ 5] HL++
                        JMP     $$TRK_LOAD_CMD          ; [10] Go back and process the next track command
$$TRK_DBTL_LOOP:        STA     MUSEQ_TRK_T0_TLCNT      ; [13] Track loop count <= A
                        LHLD    MUSEQ_TRK_T0_TLPTR      ; [16] HL <= track loop pointer (next command)
                        JMP     $$TRK_LOAD_CMD          ; [10] Go back and process the next track command


$$CLIP_NOT_EOS:         INX     H                       ; [ 5] sequence ptr++
                        MOV     B, A                    ; [ 5] B <= copy of control byte
                        MOV     A, M                    ; [ 7] A <= duration (everything will have one)
                        INX     H                       ; [ 5] sequence ptr++
                        STA     MUSEQ_CLIP_T0_DURATION  ; [13] Update the duration
                        MOV     A, B                    ; [ 5] A <= control byte
                        ANI     ~MUSEQ_CLIP_CTRL_CONT   ; [ 7] A <= control byte & ~MUSEQ_CLIP_CTRL_CONT
                        JNZ     $$CLIP_NOT_CONT         ; [10] Not MUSEQ_CLIP_CTRL_CONT, handle any changes
                        SHLD    MUSEQ_CLIP_T0_PTR       ; [16] Save the sequence pointer…
                        MOV     A, E                    ; [ 5]
                        ORI     MUSEQ_T0_UPDATE_BIT     ; [ 7] …set the state change bit to reuse the last
                                                        ;      state set…
                        MOV     E, A                    ; [ 5]
                        RET                             ; [10] …and return.
                        
$$CLIP_NOT_CONT:        JP      $$CLIP_NOT_REST         ; [10] The CPI MUSEQ_CLIP_CTRL_CONT above will have
                                                        ;      set the M flag if bit 7 was set
                        SHLD    MUSEQ_CLIP_T0_PTR       ; [16] Save the sequence pointer…
                        LDA     MUSEQ_T0_NEWSTATE       ; [13] …then load the existing channel state…
                        ANI     01111111b               ; [ 7] …drop the enable bit…
                        STA     MUSEQ_T0_NEWSTATE       ; [13] …and write it back to stop the channel…
                        MOV     A, E                    ; [ 5]
                        ORI     MUSEQ_T0_UPDATE_BIT     ; [ 7] …set the state change bit…
                        MOV     E, A                    ; [ 5]
                        RET                             ; [10] …and return.

$$CLIP_NOT_REST:        RAL                             ; [ 4] A <= A << 1, CY <= bit 7 (MUSEQ_CLIP_CTRL_REST)
                        RAL                             ; [ 4] A <= A << 1, CY <= bit 6
                                                        ;      (MUSEQ_CLIP_CTRL_T_FREQ)
                        JNC     $$CLIP_NO_FREQ          ; [10] No frequency change
                        MOV     B, A                    ; [ 5] B <= copy of shifted control byte
                        MOV     A, M                    ; [ 7] A <= frequency byte
                        INX     H                       ; [ 5] sequence ptr++
                        STA     APU_RGSTR_T0_FREQLO     ; [13] Set frequency index
                        MOV     A, B                    ; [ 5] A <= shifted control byte

$$CLIP_NO_FREQ:         RAL                             ; [ 4] A <= A << 1, CY <= bit 5
                                                        ;      (MUSEQ_CLIP_CTRL_T_ENVELOPE)
                        JNC     $$CLIP_NO_ENVELOPE      ; [10] No frequency change
                        MOV     B, A                    ; [ 5] B <= copy of shifted control byte
                        MOV     A, M                    ; [ 7] A <= frequency byte
                        INX     H                       ; [ 5] sequence ptr++
                        STA     APU_RGSTR_T0_ENVLVLLO
                                                        ; [13] Set envelope option (level or decay
                                                        ;      period)
                        MOV     A, B                    ; [ 5] A <= shifted control byte

$$CLIP_NO_ENVELOPE:     RAL                             ; [ 4] A <= A << 1, CY <= bit 4
                                                        ;      (MUSEQ_CLIP_CTRL_T_STATE)
                        JNC     $$CLIP_NO_STATE         ; [10] No state byte changes (besides enable)
                        MOV     A, M                    ; [ 7] A <= new state byte
                        INX     H                       ; [ 5] sequence ptr++
                        SHLD    MUSEQ_CLIP_T0_PTR       ; [16] Save the sequence pointer…
                        STA     MUSEQ_T0_NEWSTATE       ; [13] …write the state register…
                        MOV     A, E                    ; [ 5]
                        ORI     MUSEQ_T0_UPDATE_BIT     ; [ 7] …set the state change bit to reuse the last
                                                        ;      state set…
                        MOV     E, A                    ; [ 5]
                        RET                             ; [10] …and return.

$$CLIP_NO_STATE:        SHLD    MUSEQ_CLIP_T0_PTR       ; [16] Save the sequence pointer…
                        LDA     MUSEQ_T0_NEWSTATE       ; [13] …then load the existing state…
                        ORI     10000000b               ; [ 7] …ensure the channel is enabled…
                        STA     MUSEQ_T0_NEWSTATE       ; [13] …write the state register…
                        MOV     A, E                    ; [ 5]
                        ORI     MUSEQ_T0_UPDATE_BIT     ; [ 7] …set the state change bit to reuse the last
                                                        ;      state set…
                        MOV     E, A                    ; [ 5]
                        RET                             ; [10] …and return.

MUSEQ_T0_TRK_START      EQU     $$TRK_NEXT_CMD


;
; MUSEQ_N0_STEP
; MUsical SEQuencer, Noise 0 STEP
;
; Progress the state of noise channel 0.  On entry the E register holds
; the bitmask that indicates channels' needing their state updated with
; the APU; on exit, this subroutine will have updated the bit for this
; channel.
;
; There is a secondary entry point into this subroutine at
; MUSEQ_N0_TRK_START.  This is the point where the next track command
; is loaded — or where the first track command can be loaded on the
; first call to audio rendering after MUSEQ_TRK_N0_CMDPTR was loaded.
;
MUSEQ_N0_STEP:          LDA     MUSEQ_TRK_N0_RESET      ; [13] Check for reset flag
                        ORA     A                       ; [ 4]
                        JZ      $$NORMAL_STEP           ; [10]
                        JP      $$NEW_TRK_STEP          ; [10]
                        RET                             ; [10]
$$NEW_TRK_STEP:         XRA     A                       ; [ 4]
                        STA     MUSEQ_TRK_N0_RESET      ; [13]
                        JMP     $$TRK_NEXT_CMD          ; [10]
                        
$$NORMAL_STEP:          LDA     MUSEQ_CLIP_N0_DURATION  ; [13] Load duration slot
                        ORA     A                       ; [ 4] Has duration reached zero?
                        JZ      $$CLIP_NEXT_STEP        ; [10] Yes, load a new step from the sequence
                        DCR     A                       ; [ 5] No, decrement the counter…
                        STA     MUSEQ_CLIP_N0_DURATION  ; [13] …store it back…
                        RET                             ; [10] …and exit.

$$CLIP_NEXT_STEP:       LHLD    MUSEQ_CLIP_N0_PTR       ; [16] HL <= sequence pointer
                        MOV     A, M                    ; [ 7] A <= control byte
                        ORA     A                       ; [ 4] A <= A | A (set flags)
                        JNZ     $$CLIP_NOT_EOS          ; [10] control byte is not SEQ_CRTL_EOS
                        
                        ;
                        ; The current clip has reached its end, so attempt to
                        ; progress the track state:
                        ;
                        LDA     MUSEQ_TRK_N0_LOOPCNT    ; [13] A <= clip's loop count
                        DCR     A                       ; [ 5] A--
                        JZ      $$TRK_NEXT_CMD          ; [10] If we've reached zero, move to the next command
                        STA     MUSEQ_TRK_N0_LOOPCNT    ; [13]
                        LHLD    MUSEQ_TRK_N0_LASTCLIP   ; [16] HL <= pointer to clip that will be repeated
                        SHLD    MUSEQ_CLIP_N0_PTR       ; [16] Reset clip pointer to the clip being repeated
                        JMP     $$CLIP_NEXT_STEP        ; [10] Start from beginning of that loop...

$$TRK_NEXT_CMD:         LHLD    MUSEQ_TRK_N0_CMDPTR     ; [16] HL <= pointer to next track command
$$TRK_LOAD_CMD:         MOV     A, M                    ; [ 7] A <= track command byte
                        ORA     A                       ; [ 4] Get flags set for value in A
                        JNZ     $$TRK_CMD_IS_JMP        ; [10] If the command is not STP (0), move on

$$TRK_STOP:             STA     MUSEQ_N0_NEWSTATE       ; [13] A is already 0 (STP command), set the channel
                                                        ;      state to that
                        ORI     MUSEQ_N0_UPDATE_BIT     ; [ 7] …set the state change bit…
                        MOV     E, A                    ; [ 5] …in the E register…
                        MVI     A, 0FFh                 ; [ 7] …and set duration to max…
                        STA     MUSEQ_CLIP_N0_DURATION  ; [13] …to avoid checking a MUSEQ_CLIP_CTRL_EOS every
                                                        ;       step now that we're done…
                        STA     MUSEQ_TRK_N0_LOOPCNT    ; [13] …and also loop the track STP command for the
                                                        ;      same reason…
                        RET                             ; [10] …then return.

$$TRK_CMD_IS_JMP:       RAR                             ; [ 4] The ORA A above cleared carry, so we're doing a
                                                        ;      logical shift right, sending bit 0 into CY
                        JNC     $$TRK_CMD_IS_NXC        ; [10] Skip ahead to NeXt Clip check
$$TRK_JMP:              PUSH    D                       ; [11] Stash DE
                        INX     H                       ; [ 5] HL++
                        MOV     E, M                    ; [ 7] E <= low-byte of <ADDR>
                        INX     H                       ; [ 5] HL++
                        MOV     D, M                    ; [ 7] D <= high-byte of <ADDR>
                        XCHG                            ; [ 4] HL <=> DE
                        POP     D                       ; [10] Restore previous DE
                        SHLD    MUSEQ_TRK_N0_CMDPTR     ; [16] Pointer to next track command <= HL
                        JMP     $$TRK_LOAD_CMD          ; [10] Go back and attempt to load the command we've jumped to

$$TRK_CMD_IS_NXC:       RAR                             ; [ 4] The CY flag is still not set, so this is still a
                                                        ;      logical shift right, sending bit 1 into CY
                        JNC     $$TRK_CMD_IS_BTL        ; [10] Skip ahead to Begin Clip Loop
$$TRK_NXC:              PUSH    D                       ; [11] Stash DE
                        INX     H                       ; [ 5] HL++
                        MOV     E, M                    ; [ 7] E <= low-byte of <ADDR>
                        INX     H                       ; [ 5] HL++
                        MOV     D, M                    ; [ 7] D <= high-byte of <ADDR>
                        INX     H                       ; [ 5] HL++
                        ORA     A                       ; [ 4] Get flags set for A = remnant of command byte
                        JNZ     $$TRK_NCX_SET           ; [10] If it's not zero, it's the implicit one loop count
                        MOV     A, M                    ; [ 7] A <= clip loop count
                        INX     H                       ; [ 5] HL++
$$TRK_NCX_SET:          STA     MUSEQ_TRK_N0_LOOPCNT    ; [13] Clip loop count <= A
                        SHLD    MUSEQ_TRK_N0_CMDPTR     ; [16] Pointer to next track command <= HL                        
                        XCHG                            ; [ 4] HL <=> DE
                        POP     D                       ; [10] Restore previous DE
                        SHLD    MUSEQ_TRK_N0_LASTCLIP   ; [16] Pointer to current clip in track <= HL
                        SHLD    MUSEQ_CLIP_N0_PTR       ; [16] Pointer to current clip <= HL
                        JMP     $$CLIP_NEXT_STEP        ; [10] Go back and start processing the clip
                        
$$TRK_CMD_IS_BTL:       RAR                             ; [ 4] The CY flag is still not set, so this is still a
                                                        ;      logical shift right, sending bit 2 into CY
                        JNC     $$TRK_CMD_IS_DBTL       ; [10] Skip ahead to Decrement and Branch to Clip Loop
$$TRK_BTL:              ORA     A                       ; [ 4] Get flags set to check for zero or not
                        INX     H                       ; [ 5] HL++
                        JNZ     $$TRK_BTL_SET           ; [10] If it's not zero, it's the implicit two loop count
                        MOV     A, M                    ; [ 7] A <= clip loop count
                        INX     H
$$TRK_BTL_SET:          STA     MUSEQ_TRK_N0_TLCNT      ; [13] Track loop count <= A
                        SHLD    MUSEQ_TRK_N0_TLPTR      ; [16] Track loop pointer <= HL (next command)
                        JMP     $$TRK_LOAD_CMD          ; [10] Go back and process the next track command
                        
$$TRK_CMD_IS_DBTL:                                      ; We will assume that any non-zero command IS DBTL
                                                        ; without bothering to check; assume the programmer knows
                                                        ; what he is doing!
$$TRK_DBTL:             LDA     MUSEQ_TRK_N0_TLCNT      ; [13] A <= track loop count
                        DCR     A                       ; [ 5] A--
                        JP      $$TRK_DBTL_LOOP         ; [10] Skip ahead to looping on non-negative
$$TRK_DBTL_EXIT:        INX     H                       ; [ 5] HL++
                        JMP     $$TRK_LOAD_CMD          ; [10] Go back and process the next track command
$$TRK_DBTL_LOOP:        STA     MUSEQ_TRK_N0_TLCNT      ; [13] Track loop count <= A
                        LHLD    MUSEQ_TRK_N0_TLPTR      ; [16] HL <= track loop pointer (next command)
                        JMP     $$TRK_LOAD_CMD          ; [10] Go back and process the next track command


$$CLIP_NOT_EOS:         INX     H                       ; [ 5] sequence ptr++
                        MOV     B, A                    ; [ 5] B <= copy of control byte
                        MOV     A, M                    ; [ 7] A <= duration (everything will have one)
                        INX     H                       ; [ 5] sequence ptr++
                        STA     MUSEQ_CLIP_N0_DURATION  ; [13] Update the duration
                        MOV     A, B                    ; [ 5] A <= control byte
                        ANI     ~MUSEQ_CLIP_CTRL_CONT   ; [ 7] A <= control byte & ~MUSEQ_CLIP_CTRL_CONT
                        JNZ     $$CLIP_NOT_CONT         ; [10] Not MUSEQ_CLIP_CTRL_CONT, handle any changes
                        SHLD    MUSEQ_CLIP_N0_PTR       ; [16] Save the sequence pointer…
                        MOV     A, E                    ; [ 5]
                        ORI     MUSEQ_N0_UPDATE_BIT     ; [ 7] …set the state change bit to reuse the last
                                                        ;      state set…
                        MOV     E, A                    ; [ 5]
                        RET                             ; [10] …and return.
                        
$$CLIP_NOT_CONT:        JP      $$CLIP_NOT_REST         ; [10] The CPI MUSEQ_CLIP_CTRL_CONT above will have
                                                        ;      set the M flag if bit 7 was set
                        SHLD    MUSEQ_CLIP_N0_PTR       ; [16] Save the sequence pointer…
                        LDA     MUSEQ_N0_NEWSTATE       ; [13] …then load the existing channel state…
                        ANI     01111111b               ; [ 7] …drop the enable bit…
                        STA     MUSEQ_N0_NEWSTATE       ; [13] …and write it back to stop the channel…
                        MOV     A, E                    ; [ 5]
                        ORI     MUSEQ_N0_UPDATE_BIT     ; [ 7] …set the state change bit…
                        MOV     E, A                    ; [ 5]
                        RET                             ; [10] …and return.

$$CLIP_NOT_REST:        RAL                             ; [ 4] A <= A << 1, CY <= bit 7 (MUSEQ_CLIP_CTRL_REST)
                        RAL                             ; [ 4] A <= A << 1, CY <= bit 6
                                                        ;      (MUSEQ_CLIP_CTRL_N_ENVELOPE)
                        JNC     $$CLIP_NO_ENVELOPE      ; [10] No frequency change
                        MOV     B, A                    ; [ 5] B <= copy of shifted control byte
                        MOV     A, M                    ; [ 7] A <= frequency byte
                        INX     H                       ; [ 5] sequence ptr++
                        STA     APU_RGSTR_N_ENVLVLLO
                                                        ; [13] Set envelope option (level or decay
                                                        ;      period)
                        MOV     A, B                    ; [ 5] A <= shifted control byte

$$CLIP_NO_ENVELOPE:     RAL                             ; [ 4] A <= A << 1, CY <= bit 4
                                                        ;      (MUSEQ_CLIP_CTRL_N_STATE)
                        JNC     $$CLIP_NO_STATE         ; [10] No state byte changes (besides enable)
                        MOV     A, M                    ; [ 7] A <= new state byte
                        INX     H                       ; [ 5] sequence ptr++
                        SHLD    MUSEQ_CLIP_N0_PTR       ; [16] Save the sequence pointer…
                        STA     MUSEQ_N0_NEWSTATE       ; [13] …write the state register…
                        MOV     A, E                    ; [ 5]
                        ORI     MUSEQ_N0_UPDATE_BIT     ; [ 7] …set the state change bit to reuse the last
                                                        ;      state set…
                        MOV     E, A                    ; [ 5]
                        RET                             ; [10] …and return.

$$CLIP_NO_STATE:        SHLD    MUSEQ_CLIP_N0_PTR       ; [16] Save the sequence pointer…
                        LDA     MUSEQ_N0_NEWSTATE       ; [13] …then load the existing state…
                        ORI     10000000b               ; [ 7] …ensure the channel is enabled…
                        STA     MUSEQ_N0_NEWSTATE       ; [13] …write the state register…
                        MOV     A, E                    ; [ 5]
                        ORI     MUSEQ_N0_UPDATE_BIT     ; [ 7] …set the state change bit to reuse the last
                                                        ;      state set…
                        MOV     E, A                    ; [ 5]
                        RET                             ; [10] …and return.

MUSEQ_N0_TRK_START      EQU     $$TRK_NEXT_CMD