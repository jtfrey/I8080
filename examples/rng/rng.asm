
            org     0400h

            mvi     h, '1'
            mvi     l, '0'
            in      10          ; A <- byte read from input 10
            mov     d, a
            in      10          ; A <- byte read from input 10
            mov     e, a
            dad     de
            hlt
