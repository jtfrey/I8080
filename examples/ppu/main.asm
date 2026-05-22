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
                
                
                mvi     A, PPU_MODE_DEFAULT
                sta     PPU_RGSTR_MODE
                
                WAITRNDRSTRT
ANIMLOOP:       WAITRNDROVR
                
                ; Move the sprites
                call    MVBALLS
                
                ; Wind down any unused CPU time...
                WAITRNDRSTRT
                ; ...before starting the next game loop.
                jmp     ANIMLOOP
                
                hlt


                include "game_ball.asm"


                include "palettes.asm"
                include "tile_tables.asm"
                
                include "palettes.inc"
                include "tile_tables.inc"
