;
; This file assumes that PPU_BASE has been set elsewhere in
; the source, and makes all definitions offset from that:
;
PPU_RGSTR_MODE              equ     PPU_BASE +   00h
PPU_RGSTR_STATUS            equ     PPU_BASE +   01h
PPU_RGSTR_SPRITE_OFFSET     equ     PPU_BASE +   02h
PPU_RGSTR_INTE_OPCODE       equ     PPU_BASE +   03h
PPU_RGSTR_IS_RENDERING      equ     PPU_BASE +   04h
PPU_RGSTR_DMA_DEST          equ     PPU_BASE +   05h
PPU_RGSTR_DMA_SRC_OFFSET    equ     PPU_BASE +   06h
PPU_RGSTR_DMA_SRC_PAGE      equ     PPU_BASE +   07h
;
PPU_PALETTE_TABLE           equ     PPU_BASE +  010h
PPU_PALETTE_TABLE_SIZE      equ     020h
;
PPU_SPRITE_TABLE            equ     PPU_PALETTE_TABLE + PPU_PALETTE_TABLE_SIZE
PPU_SPRITE_TABLE_SIZE       equ     0D0h
;
PPU_TILE_TABLE              equ     PPU_SPRITE_TABLE + PPU_SPRITE_TABLE_SIZE
PPU_TILE_TABLE_SIZE         equ     0100h
;
PPU_TILE_MAP_0              equ     PPU_TILE_TABLE + PPU_TILE_TABLE_SIZE
PPU_TILE_MAP_SIZE           equ     0100h
;
PPU_TILE_MAP_1              equ     PPU_TILE_MAP_0 + PPU_TILE_TABLE_SIZE
;
; Mode constants
;
PPU_MODE_MAP_SELECT         equ     00000001b
PPU_MODE_RENDER_ENABLE      equ     00000010b
PPU_MODE_RENDER_BACKGROUND  equ     00000100b
PPU_MODE_RENDER_SPRITES     equ     00001000b
PPU_MODE_MAP_FLIP_FREQ_MASK equ     11110000b
;
PPU_MODE_DEFAULT            equ     00001110b
;
; Status constants
;
PPU_STATUS_SPRITE_OVERFLOW  equ     01000000b
PPU_STATUS_SPRITE_0_HIT     equ     10000000b
;
; DMA constants:
;
PPU_DMA_SPRITE_TABLE        equ     00h
PPU_DMA_TILE_TABLE          equ     01h
PPU_DMA_TILE_MAP_0          equ     02h
PPU_DMA_TILE_MAP_1          equ     03h
;
; Sprite constants:
;
PPU_SPRITE_ENABLE_MASK      equ     00000001b
PPU_SPRITE_FLIP_X_MASK      equ     00100000b
PPU_SPRITE_FLIP_Y_MASK      equ     01000000b
PPU_SPRITE_FOREGROUND       equ     10000000b


;
; HL must contain the address of the palette table to
; load into the PPU.
;
; Clobbered:    A, B, DE, HL
;
LOADPLTE:       MACRO
                mvi     B, PPU_PALETTE_TABLE_SIZE
                lxi     DE, PPU_PALETTE_TABLE
                mov     A, M                    ; <= jnz  ($ - 8)
                stax    DE
                inx     HL
                inx     DE
                dcr     B
                jnz     ($ - 5)
                ENDM

;
; HL must contain the address of the tile table to be
; loaded into the PPU.
;
; Clobbered:    A
;
LOADTTBL:       MACRO
                mov     A, L
                sta     PPU_RGSTR_DMA_SRC_OFFSET
                mvi     A, PPU_DMA_TILE_TABLE
                sta     PPU_RGSTR_DMA_DEST
                mov     A, H
                sta     PPU_RGSTR_DMA_SRC_PAGE
                ENDM

;
; Wait for the next render cycle to start; if the PPU is already in
; a render cycle the loop only makes a single pass.
;
; Clobbered:    A, HL
;
WAITRNDRSTRT:   MACRO
                lxi     HL, PPU_RGSTR_IS_RENDERING
                mvi     A, 00h
                cmp     M
                jz      ($ - 1)
                ENDM
                
;
; Wait for the next render cycle to complete; if the PPU is already
; outside of a render cycle the loop only makes a single pass.
;
; Clobbered:    A, HL
;
WAITRNDROVR:    MACRO
                lxi     HL, PPU_RGSTR_IS_RENDERING
                mvi     A, 00h
                cmp     M
                jnz     ($ - 1)
                ENDM

