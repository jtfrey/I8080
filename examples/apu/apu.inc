;
; This file assumes that APU_BASE has been set elsewhere in
; the source, and makes all definitions offset from that:
;

APU_RGSTR_M_STATE           equ     APU_BASE +   00h
APU_RGSTR_M_ENVLVLLO        equ     APU_BASE +   01h
APU_RGSTR_M_ENVLVLHI        equ     APU_BASE +   02h

APU_RGSTR_P0_STATE          equ     APU_BASE +   03h
APU_RGSTR_P0_DUTY_FREQSHFT  equ     APU_BASE +   04h
APU_RGSTR_P0_ENVLVLLO       equ     APU_BASE +   05h
APU_RGSTR_P0_ENVLVLHI       equ     APU_BASE +   06h
APU_RGSTR_P0_DURLO          equ     APU_BASE +   07h
APU_RGSTR_P0_DURHI          equ     APU_BASE +   08h
APU_RGSTR_P0_FREQLO         equ     APU_BASE +   09h
APU_RGSTR_P0_FREQHI         equ     APU_BASE +   0Ah
APU_RGSTR_P0_FREQSWPPERIOD  equ     APU_BASE +   0Bh

APU_RGSTR_P1_STATE          equ     APU_BASE +   0Ch
APU_RGSTR_P1_DUTY_FREQSHFT  equ     APU_BASE +   0Dh
APU_RGSTR_P1_ENVLVLLO       equ     APU_BASE +   0Eh
APU_RGSTR_P1_ENVLVLHI       equ     APU_BASE +   0Fh
APU_RGSTR_P1_DURLO          equ     APU_BASE +   10h
APU_RGSTR_P1_DURHI          equ     APU_BASE +   11h
APU_RGSTR_P1_FREQLO         equ     APU_BASE +   12h
APU_RGSTR_P1_FREQHI         equ     APU_BASE +   13h
APU_RGSTR_P1_FREQSWPPERIOD  equ     APU_BASE +   14h

APU_RGSTR_T0_STATE          equ     APU_BASE +   15h
APU_RGSTR_T0_ENVLVLLO       equ     APU_BASE +   16h
APU_RGSTR_T0_ENVLVLHI       equ     APU_BASE +   17h
APU_RGSTR_T0_DURLO          equ     APU_BASE +   18h
APU_RGSTR_T0_DURHI          equ     APU_BASE +   19h
APU_RGSTR_T0_FREQLO         equ     APU_BASE +   1Ah
APU_RGSTR_T0_FREQHI         equ     APU_BASE +   1Bh

APU_RGSTR_T1_STATE          equ     APU_BASE +   1Ch
APU_RGSTR_T1_ENVLVLLO       equ     APU_BASE +   1Dh
APU_RGSTR_T1_ENVLVLHI       equ     APU_BASE +   1Eh
APU_RGSTR_T1_DURLO          equ     APU_BASE +   1Fh
APU_RGSTR_T1_DURHI          equ     APU_BASE +   20h
APU_RGSTR_T1_FREQLO         equ     APU_BASE +   21h
APU_RGSTR_T1_FREQHI         equ     APU_BASE +   22h

APU_RGSTR_N_STATE           equ     APU_BASE +   23h
APU_RGSTR_N_ENVLVLLO        equ     APU_BASE +   24h
APU_RGSTR_N_ENVLVLHI        equ     APU_BASE +   25h
APU_RGSTR_N_DURLO           equ     APU_BASE +   26h
APU_RGSTR_N_DURHI           equ     APU_BASE +   27h

APU_RGSTR_DPCM_STATE        equ     APU_BASE +   28h
APU_RGSTR_DPCM_ADDR_LO      equ     APU_BASE +   29h
APU_RGSTR_DPCM_ADDR_HI      equ     APU_BASE +   2Ah
APU_RGSTR_DPCM_LENGTH_LO    equ     APU_BASE +   2Bh
APU_RGSTR_DPCM_LENGTH_HI    equ     APU_BASE +   2Ch


APU_M_STATE_8BFREQ          equ     10000000b
APU_M_STATE_8BDUR           equ     1000000b
APU_M_STATE_8BENV           equ     100000b
APU_M_STATE_8B              equ     11100000b


APU_P_STATE_ENABLE          equ     10000000b
APU_P_STATE_ENVCONST        equ     1000000b
APU_P_STATE_ENVLOOP         equ     100000b
APU_P_STATE_FREQSWP         equ     10000b
APU_P_DUTY_MASK             equ     1111b
APU_P_FREQSHFT_MASK         equ     11110000b


APU_T_STATE_ENABLE          equ     10000000b
APU_T_STATE_ENVCONST        equ     1000000b
APU_T_STATE_ENVLOOP         equ     100000b
APU_T_STATE_SAWTOOTH        equ     10000b


APU_N_STATE_ENABLE          equ     10000000b
APU_N_STATE_ENVCONST        equ     1000000b
APU_N_STATE_ENVLOOP         equ     100000b
APU_N_STATE_SHORTLOOP       equ     10000b
APU_N_STATE_PERIOD_MASK     equ     1111b


APU_DPCM_STATE_ENABLE       equ     10000000b
APU_DPCM_STATE_LOOP         equ     1000000b
APU_DPCM_RATE_MASK          equ     1111b

APU_NOTE_C1_IDX		        equ		000h
APU_NOTE_CSHARP1_IDX	    equ		002h
APU_NOTE_D1_IDX		        equ		004h
APU_NOTE_DSHARP1_IDX	    equ		006h
APU_NOTE_E1_IDX		        equ		008h
APU_NOTE_F1_IDX		        equ		00Ah
APU_NOTE_FSHARP1_IDX	    equ		00Ch
APU_NOTE_G1_IDX		        equ		00Eh
APU_NOTE_GSHARP1_IDX	    equ		010h
APU_NOTE_A1_IDX		        equ		012h
APU_NOTE_ASHARP1_IDX	    equ		014h
APU_NOTE_B1_IDX		        equ		016h

APU_NOTE_C2_IDX		        equ		01Ah
APU_NOTE_CSHARP2_IDX	    equ		01Eh
APU_NOTE_D2_IDX		        equ		022h
APU_NOTE_DSHARP2_IDX	    equ		026h
APU_NOTE_E2_IDX		        equ		02Ah
APU_NOTE_F2_IDX		        equ		02Eh
APU_NOTE_FSHARP2_IDX	    equ		032h
APU_NOTE_G2_IDX		        equ		036h
APU_NOTE_GSHARP2_IDX	    equ		03Ah
APU_NOTE_A2_IDX		        equ		03Eh
APU_NOTE_ASHARP2_IDX	    equ		042h
APU_NOTE_B2_IDX		        equ		046h

APU_NOTE_C3_IDX		        equ		04Ah
APU_NOTE_CSHARP3_IDX	    equ		04Eh
APU_NOTE_D3_IDX		        equ		052h
APU_NOTE_DSHARP3_IDX	    equ		056h
APU_NOTE_E3_IDX		        equ		05Ah
APU_NOTE_F3_IDX		        equ		05Eh
APU_NOTE_FSHARP3_IDX	    equ		062h
APU_NOTE_G3_IDX		        equ		066h
APU_NOTE_GSHARP3_IDX	    equ		06Ah
APU_NOTE_A3_IDX		        equ		06Eh
APU_NOTE_ASHARP3_IDX	    equ		072h
APU_NOTE_B3_IDX		        equ		076h

APU_NOTE_C4_IDX		        equ		07Ah
APU_NOTE_CSHARP4_IDX	    equ		07Eh
APU_NOTE_D4_IDX		        equ		082h
APU_NOTE_DSHARP4_IDX	    equ		086h
APU_NOTE_E4_IDX		        equ		08Ah
APU_NOTE_F4_IDX		        equ		08Eh
APU_NOTE_FSHARP4_IDX	    equ		092h
APU_NOTE_G4_IDX		        equ		096h
APU_NOTE_GSHARP4_IDX	    equ		09Ah
APU_NOTE_A4_IDX		        equ		09Eh
APU_NOTE_ASHARP4_IDX	    equ		0A2h
APU_NOTE_B4_IDX		        equ		0A6h

APU_NOTE_C5_IDX		        equ		0AAh
APU_NOTE_CSHARP5_IDX	    equ		0AEh
APU_NOTE_D5_IDX		        equ		0B2h
APU_NOTE_DSHARP5_IDX	    equ		0B6h
APU_NOTE_E5_IDX		        equ		0BAh
APU_NOTE_F5_IDX		        equ		0BEh
APU_NOTE_FSHARP5_IDX	    equ		0C2h
APU_NOTE_G5_IDX		        equ		0C6h
APU_NOTE_GSHARP5_IDX	    equ		0CAh
APU_NOTE_A5_IDX		        equ		0CEh
APU_NOTE_ASHARP5_IDX	    equ		0D2h
APU_NOTE_B5_IDX		        equ		0D6h

APU_NOTE_C6_IDX		        equ		0DAh
APU_NOTE_CSHARP6_IDX	    equ		0DEh
APU_NOTE_D6_IDX		        equ		0E2h
APU_NOTE_DSHARP6_IDX	    equ		0E6h
APU_NOTE_E6_IDX		        equ		0EAh
APU_NOTE_F6_IDX		        equ		0EEh
APU_NOTE_FSHARP6_IDX	    equ		0F2h
APU_NOTE_G6_IDX		        equ		0F6h
APU_NOTE_GSHARP6_IDX	    equ		0F8h
APU_NOTE_A6_IDX		        equ		0FAh
APU_NOTE_ASHARP6_IDX	    equ		0FCh
APU_NOTE_B6_IDX		        equ		0FEh

APU_INIT:       MACRO
                MVI     A, APU_M_STATE_8B               ;  7
                STA     APU_RGSTR_M_STATE               ; 13
                MVI     A, 040h                         ;  7
                STA     APU_RGSTR_M_ENVLVLLO            ; 13
                MVI     A, 020h                         ;  7
                STA     APU_RGSTR_P0_ENVLVLLO           ; 13
                MVI     A, 007h                         ;  7
                STA     APU_RGSTR_P0_DUTY_FREQSHFT      ; 13
                ENDM

APU_P0_NOTE:    MACRO   NOTEIDX
    IF "NOTEIDX" <> "A"
                MVI     A, NOTEIDX                                      ;  7
    ENDIF
                STA     APU_RGSTR_P0_FREQLO                             ; 13
                MVI     A, APU_P_STATE_ENABLE | APU_P_STATE_ENVCONST    ; 7
                STA     APU_RGSTR_P0_STATE                              ; 13
                ENDM
