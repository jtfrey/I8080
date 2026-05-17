;
; This file uses a base mapping address TMR_BASE and
; all other addresses are calculated relative to it:
;
TMR_BASE            equ     0DF00h
;
TMR_S               equ     TMR_BASE + 000h
TMR_M               equ     TMR_BASE + 001h
TMR_H               equ     TMR_BASE + 002h
TMR_d               equ     TMR_BASE + 003h
TMR_m               equ     TMR_BASE + 004h
TMR_Y               equ     TMR_BASE + 005h
TMR_Ycent           equ     TMR_BASE + 006h
TMR_fired           equ     TMR_BASE + 007h
;
TMR0_ENABLE         equ     TMR_BASE + 010h
TMR0_OPCODE         equ     TMR_BASE + 011h
TMR0_MSECFRAC       equ     TMR_BASE + 012h
TMR0_MSECWHOLEB0    equ     TMR_BASE + 013h
TMR0_MSECWHOLEB1    equ     TMR_BASE + 014h
TMR0_MSECWHOLEB2    equ     TMR_BASE + 015h
TMR0_MSECWHOLEB3    equ     TMR_BASE + 016h
;
TMR1_ENABLE         equ     TMR_BASE + 018h
TMR1_OPCODE         equ     TMR_BASE + 019h
TMR1_MSECFRAC       equ     TMR_BASE + 01Ah
TMR1_MSECWHOLEB0    equ     TMR_BASE + 01Bh
TMR1_MSECWHOLEB1    equ     TMR_BASE + 01Ch
TMR1_MSECWHOLEB2    equ     TMR_BASE + 01Dh
TMR1_MSECWHOLEB3    equ     TMR_BASE + 01Eh
;
TMR2_ENABLE         equ     TMR_BASE + 020h
TMR2_OPCODE         equ     TMR_BASE + 021h
TMR2_MSECFRAC       equ     TMR_BASE + 022h
TMR2_MSECWHOLEB0    equ     TMR_BASE + 023h
TMR2_MSECWHOLEB1    equ     TMR_BASE + 024h
TMR2_MSECWHOLEB2    equ     TMR_BASE + 025h
TMR2_MSECWHOLEB3    equ     TMR_BASE + 026h
;
TMR3_ENABLE         equ     TMR_BASE + 028h
TMR3_OPCODE         equ     TMR_BASE + 029h
TMR3_MSECFRAC       equ     TMR_BASE + 02Ah
TMR3_MSECWHOLEB0    equ     TMR_BASE + 02Bh
TMR3_MSECWHOLEB1    equ     TMR_BASE + 02Ch
TMR3_MSECWHOLEB2    equ     TMR_BASE + 02Dh
TMR3_MSECWHOLEB3    equ     TMR_BASE + 02Eh
;
TMR4_ENABLE         equ     TMR_BASE + 030h
TMR4_OPCODE         equ     TMR_BASE + 031h
TMR4_MSECFRAC       equ     TMR_BASE + 032h
TMR4_MSECWHOLEB0    equ     TMR_BASE + 033h
TMR4_MSECWHOLEB1    equ     TMR_BASE + 034h
TMR4_MSECWHOLEB2    equ     TMR_BASE + 035h
TMR4_MSECWHOLEB3    equ     TMR_BASE + 036h
;
TMR5_ENABLE         equ     TMR_BASE + 038h
TMR5_OPCODE         equ     TMR_BASE + 039h
TMR5_MSECFRAC       equ     TMR_BASE + 03Ah
TMR5_MSECWHOLEB0    equ     TMR_BASE + 03Bh
TMR5_MSECWHOLEB1    equ     TMR_BASE + 03Ch
TMR5_MSECWHOLEB2    equ     TMR_BASE + 03Dh
TMR5_MSECWHOLEB3    equ     TMR_BASE + 03Eh
;
TMR6_ENABLE         equ     TMR_BASE + 040h
TMR6_OPCODE         equ     TMR_BASE + 041h
TMR6_MSECFRAC       equ     TMR_BASE + 042h
TMR6_MSECWHOLEB0    equ     TMR_BASE + 043h
TMR6_MSECWHOLEB1    equ     TMR_BASE + 044h
TMR6_MSECWHOLEB2    equ     TMR_BASE + 045h
TMR6_MSECWHOLEB3    equ     TMR_BASE + 046h
;
TMR7_ENABLE         equ     TMR_BASE + 048h
TMR7_OPCODE         equ     TMR_BASE + 049h
TMR7_MSECFRAC       equ     TMR_BASE + 04Ah
TMR7_MSECWHOLEB0    equ     TMR_BASE + 04Bh
TMR7_MSECWHOLEB1    equ     TMR_BASE + 04Ch
TMR7_MSECWHOLEB2    equ     TMR_BASE + 04Dh
TMR7_MSECWHOLEB3    equ     TMR_BASE + 04Eh
