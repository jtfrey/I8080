;
; A game ball exists as four interleaved 16-bit values using
; fixed-precision format (8-bit fractional component) and an
; 8-bit sprite slot id.
;
; A value of x=12.25, dx=0.125, for example:
;
;           uint8_t         x_frac      = 0b01000000 = 0x40
;           uint8_t         dx_frac     = 0b00100000 = 0x20
;           uint8_t         x_whole     = 0b00001100 = 0x0C
;           uint8_t         dx_whole    = 0b00000000 = 0x00
;
; For the y-coordinate, the structure is the same and follows
; the x-coordinate; last is the single byte holding the sprite
; slot containing this object.
;
; There are <N> 16-bit pointer slots that can be set to the
; address of a game ball structure.  The subroutine processes
; structures until a NULL pointer is encountered in the list.
;
; When a ball is added to a scene, it must be added to a
; sprite slot first; then a game ball record is selected and
; its pointer added to the next-available slot in the pointer
; list.
;
; When a ball is removed from the scene, it should first be
; inactivated in its sprite slot (for slot reuse) then
; its pointer removed from the pointer list; the pointer list
; must be compacted.
;
; There are no arguments to this subroutine.  The stack pointer
; register is used to facilitate fetching of the next 16-bit
; value from the pointer list (10 cycles rather than 2 x 7
; using two mov instructions -- every little bit helps).  The
; 16-bit register pairs are used for the 16-bit addition of
; the fixed-point values -- more efficient than running 8-bit
; components through the accumulator.
;
MVBALLS:    push    BC
            push    DE
            push    HL
            push    PSW
            
            lxi     HL, 0000h
            dad     SP
            shld    MVBALLS_SP                  ; save the stack pointer
            
            lxi     HL, BALL_ADDRS
            sphl                                ; set the stack pointer to the ball
                                                ; address table
MVBALLS_CHECK:
            pop     DE                          ; grab the next ball pointer
            mov     A, D
            ora     E                           ; (hi-byte) | (lo-byte) == 0 => NULL
            jz      MVBALLS_DONE
            xchg                                ; HL <= DE (ball pointer)
            
MVBALLS_COORD_FIX:
            mov     E, M                        ; C <= x-coordinate, low-byte
            inx     HL
            mov     C, M                        ; E <= dx, low-byte
            inx     HL
            mov     D, M                        ; B <= x-coordinate, high-byte
            inx     HL
            mov     B, M                        ; D <= dx, high-byte
            xchg                                ; HL <=> DE
            dad     BC                          ; HL <= HL + BC = DE + BC
            xchg                                ; HL <=> DE
            ;
            ; HL again contains the pointer into the ball data structure (at dx.hi), DE
            ; has the updated coordinate, and BC has dx
            ;
            ; Now let's check the high-byte -- whole value -- for bounds:
            ;
            mov     A, D                        ; A <= D (high-byte, whole part of coordinate)
            ora     A                           ; A <= A | A = A
            jm      MVBALLS_MINUS_X             ; x < 0
            cpi     127                         ; the ball is 2 pixels wide
            jm      MVBALLS_INRANGE_X           ; x < 127
            ;
            ; the ball is off the right edge of the screen, reflect
            ; back across x=126
            ;

            jmp     MVBALLS_NEG_DX              ; go negate dx
MVBALLS_MINUS_X:
            CMA16DE_A_DE                        ; DE <= -DE (-x)
MVBALLS_NEG_DX:
            CMA16BC_A_BC                        ; BC <= -BC (dx)
MVBALLS_INRANGE_X:
            mov     M, B                        ; dx, high-byte <= B
            sta     MVBALLS_NEW_X
            dcx     HL                          ; x-coordinate, high-byte <= (HL)
            mov     M, D                        ; x-coordinate, high-byte <= D
            dcx     HL                          ; dx, low-byte <= (HL)
            mov     M, C                        ; dx, low-byte <= C
            dcx     HL                          ; x-coordinate, low-byte <= (HL)
            mov     M, E                        ; x-coordinate, low-byte <= E
            inx     HL
            inx     HL
            inx     HL
            inx     HL                          ; y-coordinate, low-byte <= (HL)
            
            ;
            ; Now handle the y-coordinate:
            ;
            mov     E, M                        ; C <= y-coordinate, low-byte
            inx     HL
            mov     C, M                        ; E <= dy, low-byte
            inx     HL
            mov     D, M                        ; B <= y-coordinate, high-byte
            inx     HL
            mov     B, M                        ; D <= dy, high-byte
            xchg                                ; HL <=> DE
            dad     BC                          ; HL <= HL + BC = DE + BC
            xchg                                ; HL <=> DE
            ;
            ; HL again contains the pointer into the ball data structure (at dy.hi), DE
            ; has the updated coordinate, and BC has dy
            ;
            ; Now let's check the high-byte -- whole value -- for bounds:
            ;
            mov     A, D                        ; A <= D (high-byte, whole part of coordinate)
            ora     A                           ; A <= A | A = A
            jm      MVBALLS_MINUS_Y             ; y < 0
            cpi     62                          ; the ball is 2 pixels tall and sprite's y-coordinates
                                                ; are delayed by one line for drawing
            jm      MVBALLS_INRANGE_Y           ; y < 62
            ;
            ; the ball is off the right edge of the screen, reflect
            ; back across y=62
            ;

            jmp     MVBALLS_NEG_DY              ; go negate dy
MVBALLS_MINUS_Y:
            CMA16DE_A_DE                        ; DE <= -DE (-y)
MVBALLS_NEG_DY:
            CMA16BC_A_BC                        ; BC <= -BC (dy)
MVBALLS_INRANGE_Y:
            mov     M, B                        ; dy, high-byte <= B
            dcx     HL                          ; y-coordinate, high-byte <= (HL)
            mov     M, D                        ; y-coordinate, high-byte <= D
            mov     B, D                        ; B <= copy of y-coordinate, high-byte
            dcx     HL                          ; dy, low-byte <= (HL)
            mov     M, C                        ; dy, low-byte <= C
            dcx     HL                          ; y-coordinate, low-byte <= (HL)
            mov     M, E                        ; y-coordinate, low-byte <= E
            inx     HL
            inx     HL
            inx     HL
            inx     HL                          ; sprite index <= (HL)
            
            mvi     D, 00h                      ; D <= 00h
            mov     A, M                        ; E <= sprite index
            rlc
            rlc                                 ; sprite slot is 4 bytes, multiply the index
            mov     E, A
            lxi     HL, PPU_SPRITE_TABLE        ; HL <= address of sprite table on PPU
            dad     DE                          ; HL <= HL + DE = sprite table + 4*index
            
            ;
            ; New y-coordinate is in B, new x-coordinate is in MVBALLS_NEW_X,
            ; DE contains the sprite index
            ;
            mov     M, B                        ; sprite.y <= B
            lda     MVBALLS_NEW_X               ; A <= saved new x-coordinate
            inx     HL
            mov     M, A                        ; sprite.y <= A
            jmp     MVBALLS_CHECK               ; move on to the next ball
            
MVBALLS_DONE:
            lhld    MVBALLS_SP
            sphl                                ; restore the original stack pointer
            pop     PSW
            pop     HL
            pop     DE
            pop     BC
            ret

MVBALLS_NEW_X:
            db      00h                         ; saved new x-coordinate
MVBALLS_SP: dw      0000h                       ; saved stack pointer

BALL0:      db      000h                        ; x-coordinate (low-byte, fixed-point, 8 fractional bits)
            db      01000000b                   ; dx (low-byte, fixed-point, 8 fractional bits)
            db      040h                        ; x-coordinate (high-byte, fixed-point, 8 whole bits)
            db      00000000b                   ; dx (high-byte, fixed-point, 8 whole bits)
            db      000h                        ; y-coordinate (low-byte, fixed-point, 8 fractional bits)
            db      01000000b                   ; dy (low-byte, fixed-point, 8 fractional bits)
            db      020h                        ; y-coordinate (high-byte, fixed-point, 8 whole bits)
            db      00000000b                   ; dy (high-byte, fixed-point, 8 whole bits)
            db      0                           ; sprite index
            
BALL1:      db      000h                        ; x-coordinate (low-byte, fixed-point, 8 fractional bits)
            db      10000000b                   ; dx (low-byte, fixed-point, 8 fractional bits)
            db      010h                        ; x-coordinate (high-byte, fixed-point, 8 whole bits)
            db      00000000b                   ; dx (high-byte, fixed-point, 8 whole bits)
            db      000h                        ; y-coordinate (low-byte, fixed-point, 8 fractional bits)
            db      00000000b                   ; dy (low-byte, fixed-point, 8 fractional bits)
            db      030h                        ; y-coordinate (high-byte, fixed-point, 8 whole bits)
            db      00000001b                   ; dy (high-byte, fixed-point, 8 whole bits)
            db      1                           ; sprite index
            
BALL2:      db      000h                        ; x-coordinate (low-byte, fixed-point, 8 fractional bits)
            db      00010000b                   ; dx (low-byte, fixed-point, 8 fractional bits)
            db      038h                        ; x-coordinate (high-byte, fixed-point, 8 whole bits)
            db      00000000b                   ; dx (high-byte, fixed-point, 8 whole bits)
            db      000h                        ; y-coordinate (low-byte, fixed-point, 8 fractional bits)
            db      00100000b                   ; dy (low-byte, fixed-point, 8 fractional bits)
            db      010h                        ; y-coordinate (high-byte, fixed-point, 8 whole bits)
            db      00000000b                   ; dy (high-byte, fixed-point, 8 whole bits)
            db      2                           ; sprite index
            
BALL3:      db      000h                        ; x-coordinate (low-byte, fixed-point, 8 fractional bits)
            db      00000000b                   ; dx (low-byte, fixed-point, 8 fractional bits)
            db      001h                        ; x-coordinate (high-byte, fixed-point, 8 whole bits)
            db      00000001b                   ; dx (high-byte, fixed-point, 8 whole bits)
            db      000h                        ; y-coordinate (low-byte, fixed-point, 8 fractional bits)
            db      00000000b                   ; dy (low-byte, fixed-point, 8 fractional bits)
            db      028h                        ; y-coordinate (high-byte, fixed-point, 8 whole bits)
            db      00000010b                   ; dy (high-byte, fixed-point, 8 whole bits)
            db      3                           ; sprite index
            
BALL4:      db      000h                        ; x-coordinate (low-byte, fixed-point, 8 fractional bits)
            db      00000010b                   ; dx (low-byte, fixed-point, 8 fractional bits)
            db      01Eh                        ; x-coordinate (high-byte, fixed-point, 8 whole bits)
            db      00000000b                   ; dx (high-byte, fixed-point, 8 whole bits)
            db      000h                        ; y-coordinate (low-byte, fixed-point, 8 fractional bits)
            db      00000000b                   ; dy (low-byte, fixed-point, 8 fractional bits)
            db      01Eh                        ; y-coordinate (high-byte, fixed-point, 8 whole bits)
            db      00000000b                   ; dy (high-byte, fixed-point, 8 whole bits)
            db      4                           ; sprite index
            
BALL5:      db      000h                        ; x-coordinate (low-byte, fixed-point, 8 fractional bits)
            db      00001000b                   ; dx (low-byte, fixed-point, 8 fractional bits)
            db      014h                        ; x-coordinate (high-byte, fixed-point, 8 whole bits)
            db      00000000b                   ; dx (high-byte, fixed-point, 8 whole bits)
            db      000h                        ; y-coordinate (low-byte, fixed-point, 8 fractional bits)
            db      00000000b                   ; dy (low-byte, fixed-point, 8 fractional bits)
            db      014h                        ; y-coordinate (high-byte, fixed-point, 8 whole bits)
            db      00000000b                   ; dy (high-byte, fixed-point, 8 whole bits)
            db      5                           ; sprite index
;
BALL_ADDRS  dw      BALL0
            dw      BALL1
            dw      BALL2
            dw      BALL3
            dw      BALL4
            dw      BALL5
            dw      0000h