
            CPU     8080
            
APU_BASE    EQU 4000h
            
            INCLUDE "apu.inc"
            
            ORG     0400h
            
            APU_INIT
            JMP     DPCM
            MVI     A, APU_NOTE_B6_IDX
            PUSH    PSW
NEXTNOTE:   APU_P0_NOTE A               ; 33
            LXI     D, 01201h           ; 10
-:          DCR     E                   ;  5
            JNZ     -                   ; 10
                                        ;   15 cycles in low-byte loop
            DCR     D                   ;  5
            JNZ     -                   ; 10
                                        ;   30 cycles in high-byte loop
            POP     PSW                 ; 10
            DCR     A                   ;  5
            CPI     0A0h
            PUSH    PSW                 ; 11
            JNC     NEXTNOTE            ; 10
            
            NOP
            NOP
DPCM:
            MVI     A, 00h
            STA     APU_RGSTR_P0_STATE
            MVI     A, (HELLO_SAMPLES & 0FFh)
            STA     APU_RGSTR_DPCM_ADDR_LO
            MVI     A, (HELLO_SAMPLES & 0FF00h) >> 8
            STA     APU_RGSTR_DPCM_ADDR_HI
            MVI     A, ((HELLO_SAMPLES_END - HELLO_SAMPLES) & 0FFh)
            STA     APU_RGSTR_DPCM_LENGTH_LO
            MVI     A, ((HELLO_SAMPLES_END - HELLO_SAMPLES) & 0FF00h) >> 8
            STA     APU_RGSTR_DPCM_LENGTH_HI
            MVI     A, APU_DPCM_STATE_ENABLE  | 03h
            STA     APU_RGSTR_DPCM_STATE
            
            IN      0
            
            HLT

HELLO_SAMPLES:
            BINCLUDE "glass.dmc"
HELLO_SAMPLES_END:
