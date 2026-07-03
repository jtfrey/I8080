
            CPU     8080
            
APU_BASE    EQU 4000h
            
            INCLUDE "apu.inc"
            
            ORG     0400h
            
            APU_INIT
            MVI     A, APU_NOTE_B6_IDX
            PUSH    PSW
NEXTNOTE:   APU_P0_NOTE A               ; 33
            LXI     D, 01001h           ; 10
-:          DCR     E                   ;  5
            JNZ     -                   ; 10
                                        ;   15 cycles in low-byte loop
            DCR     D                   ;  5
            JNZ     -                   ; 10
                                        ;   30 cycles in high-byte loop
            POP     PSW                 ; 10
            DCR     A                   ;  5
            PUSH    PSW                 ; 11
            JNZ     NEXTNOTE            ; 10
            HLT
            