                ;
                ; Mapping address of PPU device:
                ;
PPU_BASE        equ     08000h
                ;
                ; PPU equates and macros:
                ;
                include "ppu.inc"
                
                ;
                ; Mathematical macros:
                ;
                include "math.inc"

;
; PC = $0400
;
                org     0400h

                ;
                ; Copy the PPU interrupt handler into
                ; $0000
                ;
                JMP     CPYINTE
INTE:           HLT
CPYINTE:        LDA     INTE
                STA     0000h

                ;
                ; Start by loading a palette into the PPU:
                ;
                lxi     HL, PALETTE_1
                LOADPLTE
                
                lxi     HL, TTBL_00
                LOADTTBL
                
                lxi     HL, PPU_SPRITE_TABLE
                xra     A           ; A = 0
                mov     M, A
                inx     HL
                mov     M, A
                inx     HL
                lxi     BC, ((00000000b | TTBL_00_GAME_BALL) << 8) | (PPU_SPRITE_ENABLE_MASK | PPU_SPRITE_FOREGROUND)
                mov     M, C
                inx     HL
                mov     M, B
                
                inx     HL          ; next sprite
                xra     A           ; A = 0
                mov     M, A
                inx     HL
                mov     M, A
                inx     HL
                lxi     BC, ((00100000b | TTBL_00_GAME_BALL) << 8) | (PPU_SPRITE_ENABLE_MASK | PPU_SPRITE_FOREGROUND)
                mov     M, C
                inx     HL
                mov     M, B
                
                inx     HL          ; next sprite
                xra     A           ; A = 0
                mov     M, A
                inx     HL
                mov     M, A
                inx     HL
                lxi     BC, ((01000000b | TTBL_00_GAME_BALL) << 8) | (PPU_SPRITE_ENABLE_MASK | PPU_SPRITE_FOREGROUND)
                mov     M, C
                inx     HL
                mov     M, B
                
                inx     HL          ; next sprite
                xra     A           ; A = 0
                mov     M, A
                inx     HL
                mov     M, A
                inx     HL
                lxi     BC, ((01100000b | TTBL_00_GAME_BALL) << 8) | (PPU_SPRITE_ENABLE_MASK | PPU_SPRITE_FOREGROUND)
                mov     M, C
                inx     HL
                mov     M, B
                
                inx     HL          ; next sprite
                xra     A           ; A = 0
                mov     M, A
                inx     HL
                mov     M, A
                inx     HL
                lxi     BC, ((10000000b | TTBL_00_GAME_BALL) << 8) | (PPU_SPRITE_ENABLE_MASK | PPU_SPRITE_FOREGROUND)
                mov     M, C
                inx     HL
                mov     M, B
                
                inx     HL          ; next sprite
                xra     A           ; A = 0
                mov     M, A
                inx     HL
                mov     M, A
                inx     HL
                lxi     BC, ((10100000b | TTBL_00_GAME_BALL) << 8) | (PPU_SPRITE_ENABLE_MASK | PPU_SPRITE_FOREGROUND)
                mov     M, C
                inx     HL
                mov     M, B

                ;
                ; Add tiles around the edge
                ;
                lxi     HL, PPU_TILE_MAP_0
                mvi     D, 2
                mvi     C, (1 << 5) | 14
OTILELOOP:      mvi     B, 16
ITILELOOP:      mov     M, C
                inx     HL
                dcr     B
                jnz     ITILELOOP
                dcr     D
                jz      ETILELOOP
                lxi     HL, PPU_TILE_MAP_0 + (16*15)
                jmp     OTILELOOP
                
ETILELOOP:      mvi     a, 00000000b
                call    DRAW_START

                mvi     A, PPU_MODE_DEFAULT
                sta     PPU_RGSTR_MODE
                
                WAITRNDRSTRT
ANIMLOOP:       
                ; Handle controls:
                in      65
                cpi     0
                jz      CONT
                mov     B, A
                ani     00010000b   ; start button
BUTTON_START:   jnz     EXIT
                mov     A, B
                ani     00100000b   ; select button
                jz      CHECK_PAUSE
BUTTON_SELECT:  lda     IS_PAUSED
                xri     1
                sta     IS_PAUSED
                jnz     NEXTCYCLE
CHECK_PAUSE:    lda     IS_PAUSED
                ora     A
                jnz     NEXTCYCLE
BUTTON_UP:      mov     A, B
                ani     00001100b       ; up/down
                jz      BUTTON_LEFTRIGHT
                rlc
                rlc
                rlc
                rlc
                call    DRAW_START
                jmp     CONT
BUTTON_LEFTRIGHT:
                mov     A, B
                ani     00000011b   ; left/right
                jz      CONT
                rlc
                rlc
                call    DRAW_START

CONT:           lda     IS_PAUSED
                cpi     0
                jnz     NEXTCYCLE
                
                WAITRNDROVR
                
                ; Move the sprites
                call    MVBALLS
           
NEXTCYCLE:
                ; Wind down any unused CPU time...
                WAITRNDRSTRT
                ; ...before starting the next game loop.
                jmp     ANIMLOOP

EXIT:
                mvi     A, 0
                sta     PPU_RGSTR_MODE
                
                lda     START_POS
                
                hlt

START_POS:      db      (7 << 4) + 5
DRAW_START:     push    de
                push    bc
                push    hl
                lxi     bc, 0000h               ; start with zero offset in BC
                ;
                ; are we moving the placard?
                ;
                ana     a
                jz      DRAW_START_NEW_TILES
                mov     d, a                    ; stash a copy of A in D
                ;
                ; Isolate vertical movement:
                ;
                ani     0f0h
                jz      DRAW_START_HORIZ_MOVE
                lda     START_POS               ; prep for the offset address…
                jp      DRAW_START_VERT_DOWN    ; if S is clear, it was 01000000b, "DOWN"
                ;
                ; Moving down will affect tiles at START_POS; moving
                ; up will affect the second row:
                ;
                cpi     (2 << 4)                ; don't move up into the top border
                jc      DRAW_START_HORIZ_MOVE
                adi     16
                mvi     b, -16                  ; B holds the change in Y
                jmp     DRAW_START_VERT_RESET
DRAW_START_VERT_DOWN:
                cpi     (13 << 4)               ; don't move down into the bottom border
                jnc     DRAW_START_HORIZ_MOVE
                mvi     b, +16                  ; B holds the change in Y
DRAW_START_VERT_RESET:
                mov     e, a
                xra     a
                mov     d, a
                lxi     hl, PPU_TILE_MAP_0
                dad     de
                mvi     m, 0
                inx     hl
                mvi     m, 0
                inx     hl
                mvi     m, 0

DRAW_START_HORIZ_MOVE:
                mov     a, d
                ani     00fh
                jz      FINISH_MOVE
                ani     004h                    ; right key?
                lda     START_POS               ; prep for the offset address…
                mvi     c, 0                    ; horiz offset
                jnz     DRAW_START_HORIZ_RIGHT
                ;
                ; Moving right will affect tiles at START_POS; moving
                ; left will affect the end of the row:
                ;
                ani     0fh                    ; just the x-coordinate
                jz      FINISH_MOVE
                lda     START_POS
                adi     2
                mvi     c, -1
                jmp     DRAW_START_HORIZ_RESET
DRAW_START_HORIZ_RIGHT:
                ani     0fh                    ; just the x-coordinate
                cpi     (16 - 3)
                jnc     FINISH_MOVE
                lda     START_POS
                mvi     c, +1
DRAW_START_HORIZ_RESET:
                mov     e, a
                xra     a
                mov     d, a
                lxi     hl, PPU_TILE_MAP_0
                dad     de
                mvi     m, 0
                mov     a, l
                adi     16
                mov     l, a
                mov     a, h
                aci     0
                mov     h, a
                mvi     m, 0

FINISH_MOVE:
                ; 
                ; Update START_POS
                ;
                lda     START_POS
                add     B
                add     C
                sta     START_POS
                
DRAW_START_NEW_TILES:
                lda     START_POS
                mov     e, a
                xra     a
                mov     d, a
                lxi     hl, PPU_TILE_MAP_0
                dad     de
                mvi     d, (6 << 5) | TTBL_00_START ; load the first tile in the sequence
                mov     M, D
                inx     HL
                inr     D
                mov     M, D
                inx     HL
                inr     D
                mov     M, D
                inr     D
                mvi     a, 14                       ; add 14 to HL to get to two tiles back on
                                                    ; the next row
                add     l
                mov     l, a
                mov     a, h
                aci     0
                mov     h, a
                mov     M, D
                inx     HL
                inr     D
                mov     M, D
                inx     HL
                inr     D
                mov     M, D
DRAW_START_RET: pop     hl
                pop     bc
                pop     de
                ret

IS_PAUSED:      db      00h

                include "game_ball.asm"


                include "palettes.asm"
                include "tile_tables.asm"
                
                include "palettes.inc"
                include "tile_tables.inc"
