;
; i8e8e -P 0x0400 -S 0x0000 -L examples/cga_demo.bin@0x0400 --cga=@0x4000
;


                    org     0400h
                    
                    mvi     a, 1
                    sta     4004h       ; enable = 1 (start display)
                    mvi     a, 2
                    sta     400Dh       ; mode = 2 (color graphics)
                    lda     400Dh
                    cpi     2           ; did we successfully switch to mode 2?
                    jnz     TextMode    ; nope, try the text mode
                    
                    ;
                    ; This section of code draws a two-color checkerboard
                    ; pattern over the entire display.  The color alternates
                    ; between index 3 and 4 by toggling bit 0 between each
                    ; pixel; the initial color for a row is:
                    ;
                    ;     2 + (row counter % 2)
                    ;
                    mvi     a, 81h
                    sta     4004h       ; defer drawing until we've written
                                        ; all pixels…
                    lxi     b, 4010h    ; BC <= base address of pixels
                    lda     4002h       ; A <= height of display
                    mov     d, a        ; D <= A = width of display
                    dcr     d           ; pre-decrement D to avoid an extra row
CheckerRow:         lda     4001h       ; A <= width of display
                    mov     e, a
                    dcr     e           ; pre-decrement E to avoid an extra pixel
                    mov     a, d        ; A <= D = row number
                    ani     1           ; set the starting color based on
                    adi     2           ; the row we're on
CheckerDraw:        stax    b           ; store pixel to CGA array
                    inx     b           ; move to next pixel address
                    dcr     e           ; decrement in-row pixel count
                    jp      CheckerCol  ; pixels DO remain in row
                    dcr     d           ; next row?
                    jp      CheckerRow
                    jmp     ExitChecker
CheckerCol:         xri     1           ; toggle color index +1/+0
                    jmp     CheckerDraw

ExitChecker:
                    in      0           ; wait for keypress THEN we'll draw…
                    mvi     a, 01h
                    sta     4004h       ; clear drawing deferral
                    in      0           ; wait for keypress

                    mvi     a, 01h
                    sta     400Eh       ; clear the display
                                       
                    ;
                    ; This section of code draws horizontal colored
                    ; bars, starting at color index 0 on the first row and
                    ; repeating the color index sequence as necessary
                    ; to span the entire height of the display
                    ;
                    mvi     a, 81h
                    sta     4004h       ; defer drawing until we've written
                                        ; all pixels…
                    lxi     b, 4010h    ; BC <= base address of pixels
                    lda     4002h       ; A <= height of display
                    mov     d, a        ; D <= A = width of display
                    dcr     d           ; pre-decrement D to avoid an extra row
                    mvi     h, 0        ; start at color 0
BarsRow:            lda     4001h       ; A <= width of display
                    mov     e, a
                    dcr     e           ; pre-decrement E to avoid an extra pixel
                    mov     a, h
BarsDraw:           stax    b           ; store pixel to CGA array
                    inx     b           ; move to next pixel address
                    dcr     e           ; decrement in-row pixel count
                    jp      BarsDraw    ; pixels DO remain in row
                    dcr     d           ; next row?
                    jm      ExitBars
                    inr     a           ; increment color index
                    mvi     h, 40h
                    mvi     l, 03h      ; ncolors = $4003
                    cmp     m           ; have we exceeded the color count?
                    jnz     BarsNextRow
                    mvi     a, 0
BarsNextRow:        mov     h, a
                    jmp     BarsRow

ExitBars:
                    in      0           ; wait for keypress THEN we'll draw…
                    mvi     a, 01h
                    sta     4004h       ; clear drawing deferral
                    in      0           ; wait for keypress


TextMode:
                    mvi     a, 0
                    sta     400Dh       ; mode = 0 (text)
                    mvi     a, 0afh     ; 0x80 | '/'
                    sta     400Eh
                    lxi     h, TextModeStr
                    shld    400Bh       ; address of C string to u16lo/hi
                    mvi     a, 4
                    sta     4006h       ; y = 4
                    adi     4
                    sta     4005h       ; x = y + 4
                    mvi     a, 11h
                    sta     400Eh       ; op = write C string
                    in      0
                    
SpinLoop:           mvi     a, ' ' | 080h
InnerSpinLoop:      sta     400Eh
                    mvi     b, 0ffh
                    mvi     c, 0ffh
Delay:              dcr     c
                    jp      Delay
                    dcr     b
                    jp      Delay
                    inr     a
                    cpi     ('~' + 1) | 080h
                    jnz     InnerSpinLoop
                    jmp     SpinLoop

EndDisplay:                    
                    mvi     a, 00h
                    sta     4004h       ; enable = 0 (end display)
                    
                    hlt

TextModeStr         db   '  [This is text mode (mode=0)]  ', 00h
