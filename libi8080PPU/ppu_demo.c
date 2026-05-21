/*
 * I8080
 * An implementation of the I8080 processor.
 *
 * Dr. Jeffrey Frey
 * University of Delaware
 *
 * Demo of the I8080 Curses Graphics Adapter.
 */

#include "I8080Memory.h"
#include "I8080PPU.h"
#include "I8080Timing.h"

I8080PPUPalette_t   ptbl[I8080_PPU_PTBLDIM] = {
        { .cidx = { 0x22, 0x29, 0x1A, 0x0F} },
        { .cidx = { 0x0F, 0x36, 0x17, 0x07} },
        { .cidx = { 0x0F, 0x30, 0x21, 0x0F} },
        { .cidx = { 0x0F, 0x27, 0x17, 0x0F} },
        { .cidx = { 0x22, 0x16, 0x27, 0x18} },
        { .cidx = { 0x0F, 0x1A, 0x30, 0x27} },
        { .cidx = { 0x0F, 0x16, 0x30, 0x27} },
        { .cidx = { 0x0F, 0x0F, 0x36, 0x17} }
    };
 
I8080PPUTile_t      ttbl[I8080_PPU_TTBLDIM] = {
                                                    /*  00:  solid color 0 */
[0x00]= { .pixels = { 0b00000000, 0b00000000,       /*  0000 0000   */
                      0b00000000, 0b00000000,       /*  0000 0000   */
                      0b00000000, 0b00000000,       /*  0000 0000   */
                      0b00000000, 0b00000000 } },   /*  0000 0000   */
                      
                                                    /*  01:  solid color 1 */
[0x01]= { .pixels = { 0b01010101, 0b01010101,       /*  1111 1111   */
                      0b01010101, 0b01010101,       /*  1111 1111   */
                      0b01010101, 0b01010101,       /*  1111 1111   */
                      0b01010101, 0b01010101 } },   /*  1111 1111   */
                      
                                                    /*  02:  solid color 2 */
[0x02]= { .pixels = { 0b10101010, 0b10101010,       /*  2222 2222   */
                      0b10101010, 0b10101010,       /*  2222 2222   */
                      0b10101010, 0b10101010,       /*  2222 2222   */
                      0b10101010, 0b10101010 } },   /*  2222 2222   */
                      
                                                    /*  03:  solid color 3 */
[0x03]= { .pixels = { 0b11111111, 0b11111111,       /*  3333 3333   */
                      0b11111111, 0b11111111,       /*  3333 3333   */
                      0b11111111, 0b11111111,       /*  3333 3333   */
                      0b11111111, 0b11111111 } },   /*  3333 3333   */
                      
                                                    /*  04:  top-left ground block */
[0x04]= { .pixels = { 0b01010110, 0b01010101,       /*  2111 1111   */
                      0b10101001, 0b10101110,       /*  1222 2322   */
                      0b10111001, 0b10101010,       /*  1232 2222   */
                      0b10101010, 0b10101011 } },   /*  2222 3222   */
                      
                                                    /*  05:  top-middle ground block */
[0x05]= { .pixels = { 0b01010101, 0b01010101,       /*  1111 1111   */
                      0b10111010, 0b10101010,       /*  2232 2222   */
                      0b10101010, 0b10111010,       /*  2222 2232   */
                      0b10101010, 0b10101010 } },    /*  2222 2222   */
                      
                                                    /*  06:  top-right ground block */
[0x06]= { .pixels = { 0b01010101, 0b10010101,       /*  1111 1112   */
                      0b11101010, 0b11101010,       /*  2223 2223   */
                      0b10101110, 0b11101010,       /*  2322 2223   */
                      0b10101010, 0b11101110 } },   /*  2222 2323   */
                      
                                                    /*  07:  bottom-left ground block */
[0x07]= { .pixels = { 0b10101010, 0b10111010,       /*  2222 2232   */
                      0b11101011, 0b10101010,       /*  3223 2222   */
                      0b10111110, 0b11101110,       /*  2332 2323   */
                      0b10111111, 0b11101111 } },   /*  3332 3323  */
                      
                                                    /*  08:  bottom-middle ground block */
[0x08]= { .pixels = { 0b10101010, 0b10101110,       /*  2222 2322   */
                      0b10101010, 0b10101010,       /*  2222 2222   */
                      0b10111110, 0b10111010,       /*  2332 2232   */
                      0b11111011, 0b11111111 } },   /*  3233 3333  */
                      
                                                    /*  09:  bottom-right ground block */
[0x09]= { .pixels = { 0b10101010, 0b10101010,       /*  2222 2222   */
                      0b10111010, 0b10101110,       /*  2232 2322   */
                      0b11111110, 0b10111110,       /*  2333 2332   */
                      0b11111111, 0b11111011 } },   /*  3333 3233  */
                      
                                                    /*  1A:  Mario, top-left-top */
[0x1A]= { .pixels = { 0b00000000, 0b01010100,       /*  0000 0111   */
                      0b00000000, 0b01010101,       /*  0000 1111   */
                      0b00000000, 0b10111111,       /*  0000 3332   */
                      0b11000000, 0b10101110 } },   /*  0003 2322   */
                      
                                                    /*  1B:  Mario, top-right-top */
[0x1B]= { .pixels = { 0b00000101, 0b00000000,       /*  1100 0000   */
                      0b01010101, 0b00000001,       /*  1111 1000   */
                      0b00101110, 0b00000000,       /*  2320 0000   */
                      0b10101110, 0b00000010 } },   /*  2322 2000   */
                      
                                                    /*  1C:  Mario, top-left-mid */
[0x1C]= { .pixels = { 0b11000000, 0b10111110,       /*  0003 2332   */
                      0b11000000, 0b10101011,       /*  0003 3222   */
                      0b00000000, 0b10101000,       /*  0000 0222   */
                      0b00000000, 0b11011111 } },   /*  0000 3313   */
                      
                                                    /*  1D:  Mario, top-right-mid */
[0x1D]= { .pixels = { 0b10111010, 0b00001010,       /*  2232 2200   */
                      0b11111110, 0b00000011,       /*  2333 3000   */
                      0b10101010, 0b00000000,       /*  2222 0000   */
                      0b00001111, 0b00000000 } },   /*  3300 0000   */
                      
                                                    /*  1E:  Mario, bottom-leftright-mid */
[0x1E]= { .pixels = { 0b11000000, 0b11011111,       /*  0003 3313   */
                      0b11110000, 0b01011111,       /*  0033 3311   */
                      0b10100000, 0b01100111,       /*  0022 3121   */
                      0b10100000, 0b01010110 } },   /*  0022 2111   */
                      
                                                    /*  1F:  Mario, bottom-leftright-bottom */
[0x1F]= { .pixels = { 0b10100000, 0b01010101,       /*  0022 1111   */
                      0b00000000, 0b00010101,       /*  0000 1110   */
                      0b11000000, 0b00001111,       /*  0003 3300   */
                      0b11110000, 0b00001111 } },   /*  0033 3300   */
            
    };

I8080PPUTileRef_t   tmap[I8080_PPU_TMAPDIM] = {
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        I8080PPUTileRefMake(0, 0),
        
        I8080PPUTileRefMake(1, 4),
        I8080PPUTileRefMake(1, 6),
        I8080PPUTileRefMake(1, 4),
        I8080PPUTileRefMake(1, 5),
        I8080PPUTileRefMake(1, 6),
        I8080PPUTileRefMake(1, 4),
        I8080PPUTileRefMake(1, 6),
        I8080PPUTileRefMake(1, 4),
        I8080PPUTileRefMake(1, 5),
        I8080PPUTileRefMake(1, 6),
        I8080PPUTileRefMake(1, 4),
        I8080PPUTileRefMake(1, 5),
        I8080PPUTileRefMake(1, 5),
        I8080PPUTileRefMake(1, 6),
        I8080PPUTileRefMake(1, 4),
        I8080PPUTileRefMake(1, 5),
        
        I8080PPUTileRefMake(1, 7),
        I8080PPUTileRefMake(1, 9),
        I8080PPUTileRefMake(1, 7),
        I8080PPUTileRefMake(1, 8),
        I8080PPUTileRefMake(1, 9),
        I8080PPUTileRefMake(1, 7),
        I8080PPUTileRefMake(1, 9),
        I8080PPUTileRefMake(1, 7),
        I8080PPUTileRefMake(1, 8),
        I8080PPUTileRefMake(1, 9),
        I8080PPUTileRefMake(1, 7),
        I8080PPUTileRefMake(1, 8),
        I8080PPUTileRefMake(1, 8),
        I8080PPUTileRefMake(1, 9),
        I8080PPUTileRefMake(1, 7),
        I8080PPUTileRefMake(1, 9)
    };

I8080PPUTileRef_t   tmap2[I8080_PPU_TMAPDIM] = {
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),

        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),

        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        I8080PPUTileRefMake(5, 2),
        
        I8080PPUTileRefMake(1, 4),
        I8080PPUTileRefMake(1, 6),
        I8080PPUTileRefMake(1, 4),
        I8080PPUTileRefMake(1, 5),
        I8080PPUTileRefMake(1, 6),
        I8080PPUTileRefMake(1, 4),
        I8080PPUTileRefMake(1, 6),
        I8080PPUTileRefMake(1, 4),
        I8080PPUTileRefMake(1, 5),
        I8080PPUTileRefMake(1, 6),
        I8080PPUTileRefMake(1, 4),
        I8080PPUTileRefMake(1, 5),
        I8080PPUTileRefMake(1, 5),
        I8080PPUTileRefMake(1, 6),
        I8080PPUTileRefMake(1, 4),
        I8080PPUTileRefMake(1, 5),
        
        I8080PPUTileRefMake(1, 7),
        I8080PPUTileRefMake(1, 9),
        I8080PPUTileRefMake(1, 7),
        I8080PPUTileRefMake(1, 8),
        I8080PPUTileRefMake(1, 9),
        I8080PPUTileRefMake(1, 7),
        I8080PPUTileRefMake(1, 9),
        I8080PPUTileRefMake(1, 7),
        I8080PPUTileRefMake(1, 8),
        I8080PPUTileRefMake(1, 9),
        I8080PPUTileRefMake(1, 7),
        I8080PPUTileRefMake(1, 8),
        I8080PPUTileRefMake(1, 8),
        I8080PPUTileRefMake(1, 9),
        I8080PPUTileRefMake(1, 7),
        I8080PPUTileRefMake(1, 9)
    };

I8080PPUSprite_t    sprites[I8080_PPU_MAXSPRT] = {
        { .y = 40, .x = 12, .options = 0b1 | kI8080PPUSpriteOptsForeground,
                .tileref = I8080PPUTileRefMake(4, 0x1A) },
        { .y = 40, .x = 20, .options = 0b1 | kI8080PPUSpriteOptsForeground,
                .tileref = I8080PPUTileRefMake(4, 0x1B) },
        { .y = 44, .x = 12, .options = 0b1 | kI8080PPUSpriteOptsForeground,
                .tileref = I8080PPUTileRefMake(4, 0x1C) },
        { .y = 44, .x = 20, .options = 0b1 | kI8080PPUSpriteOptsForeground,
                .tileref = I8080PPUTileRefMake(4, 0x1D) },
        { .y = 48, .x = 12, .options = 0b1 | kI8080PPUSpriteOptsForeground,
                .tileref = I8080PPUTileRefMake(4, 0x1E) },
        { .y = 48, .x = 20, .options = 0b1 | kI8080PPUSpriteOptsForeground | kI8080PPUSpriteOptsFlipX,
                .tileref = I8080PPUTileRefMake(4, 0x1E) },
        { .y = 52, .x = 12, .options = 0b1 | kI8080PPUSpriteOptsForeground,
                .tileref = I8080PPUTileRefMake(4, 0x1F) },
        { .y = 52, .x = 20, .options = 0b1 | kI8080PPUSpriteOptsForeground | kI8080PPUSpriteOptsFlipX,
                .tileref = I8080PPUTileRefMake(4, 0x1F) },
    };

int
main(
    int         argc,
    const char* argv[]
)
{
    I8080MemRef                 sysmem = I8080MemCreate();
    I8080MemMapperRef_t         ppu_mapper = { .addr_range = I8080AddrRangeCreate(0x4000, 0x1000),
                                                .callbacks = *I8080PPUMapperCallbacks };
    I8080PPUMapperContextPtr    ppu = NULL;
    I8080CGAPaletteId_t         palette_id = kI8080CGAPaletteNES2C02;
    int                         i;
    
    if ( argc > 1 ) {
        long        id = strtol(argv[1], NULL, 0);
        if ( id >= kI8080CGAPaletteIdDefault && id < kI8080CGAPaletteIdMax ) palette_id = id;
    }
    
    ppu = I8080PPUMapperContextCreate(NULL, palette_id);
    ppu_mapper.context = ppu;
    I8080MemRegisterMapper(sysmem, &ppu_mapper);
    if ( ppu ) {
        int8_t                  x, y, i;
        double                  dx = 0.25, ddx = 0.25/ 8;
        int                     idx;
        bool                    is_running = true;
        
        ppu_mapper.callbacks.reset(sysmem, ppu_mapper.addr_range, ppu);
        I8080PPUMapperContextLoadPalettes(ppu, 0, I8080_PPU_PTBLDIM, ptbl);
        I8080PPUMapperContextLoadSprites(ppu, 0, I8080_PPU_MAXSPRT, sprites);
        I8080PPUMapperContextLoadTiles(ppu, 0, I8080_PPU_TTBLDIM, ttbl);
        I8080PPUMapperContextLoadTileMap(ppu, 0, tmap);
        I8080PPUMapperContextLoadTileMap(ppu, 1, tmap2);
        
        fgetc(stdin);
        I8080PPUMapperContextSetRegister(
                ppu,
                kI8080PPURegisterIndexMode,
                kI8080PPUModeRenderEnable | kI8080PPUModeRenderBackground | kI8080PPUModeRenderSprites
            );
        nodelay(stdscr, TRUE);

        while ( is_running ) {
            idx = round(dx>4.0?4.0:dx);
            for ( i = 0; i < 8; i++ ) {
                sprites[i].x += idx;
                I8080MemWrite(sysmem, 0x4000 + I8080PPUAddrOffsetSpriteTable + 1 + i * sizeof(I8080PPUSprite_t), sprites[i].x);
            }
            I8080TimingSleep(1e6/30.0);
            dx += ddx;
            if ( dx > 4 || dx < -4 ) {
                ddx = -ddx;
            }
            else if ( fabs(dx) < 1e-4 ) {
                I8080PPUTileRef_t   swap;
    
                for ( i = 0; i < 4; i++ ) {
                    if ( i % 2 == 0 ) {
                        swap = sprites[i].tileref, sprites[i].tileref = sprites[i+1].tileref, sprites[i+1].tileref = swap;
                        I8080MemWrite(sysmem,
                            0x4000 + I8080PPUAddrOffsetSpriteTable + 3 + i * sizeof(I8080PPUSprite_t),
                            *((uint8_t*)&sprites[i].tileref));
                        I8080MemWrite(sysmem,
                            0x4000 + I8080PPUAddrOffsetSpriteTable + 3 + (i + 1) * sizeof(I8080PPUSprite_t),
                            *((uint8_t*)&sprites[i + 1].tileref));
                    }
                    sprites[i].options ^= kI8080PPUSpriteOptsFlipX;
                    I8080MemWrite(sysmem,
                        0x4000 + I8080PPUAddrOffsetSpriteTable + 2 + i * sizeof(I8080PPUSprite_t),
                        *((uint8_t*)&sprites[i].options));
                }
            }
            switch ( getch() ) {
                case 'q':
                case 'Q':
                    is_running = false;
                    break;
                
                case 'b':
                case 'B': {
                    uint8_t mode = I8080PPUMapperContextGetRegister(ppu, kI8080PPURegisterIndexMode);
                    I8080PPUMapperContextSetRegister(ppu, kI8080PPURegisterIndexMode, mode ^ kI8080PPUModeRenderBackground);
                    break;
                }
                
                case 's':
                case 'S': {
                    uint8_t mode = I8080PPUMapperContextGetRegister(ppu, kI8080PPURegisterIndexMode);
                    I8080PPUMapperContextSetRegister(ppu, kI8080PPURegisterIndexMode, mode ^ kI8080PPUModeRenderSprites);
                    break;
                }
                
                case 'e':
                case 'E': {
                    uint8_t mode = I8080PPUMapperContextGetRegister(ppu, kI8080PPURegisterIndexMode);
                    I8080PPUMapperContextSetRegister(ppu, kI8080PPURegisterIndexMode, mode ^ kI8080PPUModeRenderEnable);
                    break;
                }
            }
        }
        
    }
    I8080MemDestroy(sysmem);
    
    return 0;
}
