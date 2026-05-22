;
; ROTPLTE3
; Shift color indices 1, 2, 3 up one slot, rotating the value from slot 3 to 1.
; The palette index pre-multiplied by 4 is in HL.
; BC gets clobbered.
;
ROTPLTE3        lxi     BC, PPU_PALETTE_TABLE
                dad     BC                      ; location of the palette in HL
                inx     HL                      ; HL <= &cidx[1]
                mov     B, M                    ; B <= cidx[1]
                inx     HL                      ; HL <= &cidx[2]
                mov     C, M                    ; C <= cidx[2]
                mov     M, B                    ; cidx[2] <= B = cidx[1]
                inx     HL                      ; HL <= &cidx[3]
                mov     B, M                    ; B <= cidx[3]
                mov     M, C                    ; cidx[3] <= C = cidx[2]
                dcx     HL
                dcx     HL                      ; HL <= &cidx[1]
                mov     M, B                    ; cidx[1] <= B = cidx[3]
                ret
;
; ROTPLTE4
; Shift color indices 0, 1, 2, 3 up one slot, rotating the value from slot 3 to 0.
; The palette index pre-multiplied by 4 is in HL.
; BC gets clobbered.
;
ROTPLTE4        lxi     BC, PPU_PALETTE_TABLE
                dad     BC                      ; location of the palette in HL
                mov     B, M                    ; B <= cidx[0]
                inx     HL                      ; HL <= &cidx[1]
                mov     C, M                    ; C <= cidx[1]
                mov     M, B                    ; cidx[1] <= B = cidx[0]
                inx     HL                      ; HL <= &cidx[2]
                mov     B, M                    ; B <= cidx[2]
                mov     M, C                    ; cidx[2] <= C = cidx[1]
                inx     HL                      ; HL <= &cidx[3]
                mov     C, M                    ; C <= cidx[3]
                mov     M, B                    ; cidx[3] <= B = cidx[2]
                dcx     HL
                dcx     HL                      
                dcx     HL                      ; HL <= &cidx[0]
                mov     M, C                    ; cidx[0] <= C = cidx[3]
                ret