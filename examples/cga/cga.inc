;
; CGA register addresses:
;
CGA_SUPMODES        equ     CGA_BASE + 00h
CGA_WIDTH           equ     CGA_BASE + 01h
CGA_HEIGHT          equ     CGA_BASE + 02h
CGA_NCOLORS         equ     CGA_BASE + 03h
CGA_ENABLE          equ     CGA_BASE + 04h
CGA_X               equ     CGA_BASE + 05h
CGA_Y               equ     CGA_BASE + 06h
CGA_I               equ     CGA_BASE + 07h
CGA_R               equ     CGA_BASE + 08h
CGA_G               equ     CGA_BASE + 09h
CGA_B               equ     CGA_BASE + 0ah
CGA_U16LO           equ     CGA_BASE + 0bh
CGA_U16HI           equ     CGA_BASE + 0ch
CGA_MODE            equ     CGA_BASE + 0dh
CGA_OP              equ     CGA_BASE + 0eh
;
CGA_VRAM            equ     CGA_BASE = 10h

;
; Enable constants:
;
CGA_ENABLE_OFF      equ     00h
CGA_ENABLE_ON       equ     01h
CGA_ENABLE_DEFERDRW equ     81h

;
; Mode constants:
;
CGA_MODE_TEXT       equ     00h
CGA_MODE_BW         equ     01h
CGA_MODE_COLOR      equ     02h

;
; Triggered operation constants:
;
CGA_OP_CLEAR        equ     01h
CGA_OP_CLEARROW     equ     02h
CGA_OP_CLEARCOL     equ     03h
CGA_OP_CLEARROWS    equ     04h
CGA_OP_CLEARCOLS    equ     05h
CGA_OP_FILLROW      equ     06h
CGA_OP_FILLCOL      equ     07h
CGA_OP_WRITENCHARS  equ     10h
CGA_OP_WRITECSTR    equ     11h
CGA_OP_GETCOLORRGB  equ     7eh
CGA_OP_SETCOLORRGB  equ     7fh
CGA_OP_FILL_MASK    equ     80h
