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
ANIMLOOP:       
                ; Handle controls:
                in      65
                cpi     0
                jz      CONT
                mov     B, A
                ani     00010000b   ; start button
                jnz     EXIT
                mov     A, B
                ani     00100000b   ; select button
                jz      CONT
                lda     IS_PAUSED
                xri     1
                sta     IS_PAUSED
                jnz     NEXTCYCLE
                
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
                
                hlt

IS_PAUSED:      db      00h

                include "game_ball.asm"


                include "palettes.asm"
                include "tile_tables.asm"
                
                include "palettes.inc"
                include "tile_tables.inc"
