
                include "params.inc"

                org     0400h

                mvi     A, I1       ;   10010011        (147)
                mov     B, A
                mvi     C, I2       ; + 00011001       +( 25)
                add     C           ; = 10101100        (172)
                mov     D, A
                mvi     E, I3       ;   11001011
                sub     E           ;   00110100 + 1   -(203)
                                    ; = 00110101
                                    ; + 10101100
                                    ; = 11100001        (-31)
                hlt