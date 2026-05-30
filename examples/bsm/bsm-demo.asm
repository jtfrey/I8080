;
;  Compare:
;
;  i8e8e -P 0x0400 -S 0x0000 --load=bsm-demo.bin@0x400 --bsm 0xB0+0x10#18:slab
;
;     vs.
;
;  i8e8e -P 0x0400 -S 0x0000 --load=bsm-demo.bin@0x400 --bsm 0x40+0x10#18:ones:twos
;

                CPU 8080
                
                ORG 0400h
                
                LXI     D, 0B000h       ; DE <= BSM is mapped at $B000-$C000
                LXI     B, 00000h       ; BC <= $0000
-:              LDAX    D               ; A <= (DE)
                ADD     C               ; A <= A + C
                MOV     C, A            ; C <= A = A + C (C += (DE))
                MVI     A, 00h          ; A <= $00
                ADC     B               ; A <= A + B
                MOV     B, A            ; B <= A = A + B (B += CY)
                INX     D               ; DE <= DE + 1
                MOV     A, D            ; A <= D
                CPI     0C0h            ; Check if the high-byte in DE has hit $C0
                JZ      +               ; We've reached $C000, all done with the mapped region
                CPI     0B8h            ; Check if the high-byte in DE is half-way through
                JNZ     -               ; Not there yet, go add the next byte into BC
                MOV     A, E            ; Check if DE is $B800
                CPI     00h
                JNZ     -               ; Nope, do the next iteration of the summing
                ;
                ; We've reached $B800, half-way through the 4 KiB bank, so
                ; it's time to switch banks 0/1
                ;
                IN      12h             ; Read current bank...
                XRI     00000001b       ; ...flip between bank 0/1..
                OUT     12h             ; ...and switch to the other bank now.
                JMP     -               ; Continue summing...
+:              HLT