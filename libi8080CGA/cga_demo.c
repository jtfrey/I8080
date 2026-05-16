/*
 * I8080
 * An implementation of the I8080 processor.
 *
 * Dr. Jeffrey Frey
 * University of Delaware
 *
 * Demo of the I8080 Curses Graphics Adapter.
 */

#include "I8080CGA.h"
#include "I8080CGAColor.h"


int
main(
    int         argc,
    const char* argv[]
)
{
    I8080CGAMapperContextPtr    context = NULL;
    I8080CGAPaletteId_t         palette_id = kI8080CGAPaletteIdDefault;
    int                         w, h;
    
    if ( argc > 1 ) {
        long        id = strtol(argv[1], NULL, 0);
        if ( id >= kI8080CGAPaletteIdDefault && id < kI8080CGAPaletteIdMax ) palette_id = id;
    }
    
    context = I8080CGAMapperContextCreateWithOriginAndSize(kI8080CGAModeClrGraphics, 4, 4, 80, 40);
    if ( context ) {
        int         x, y, r2max, c;
        
        context->color_palette = I8080CGAPalettes[palette_id];
        
        I8080CGAMapperContextSetEnable(context, 1);
        
        w = I8080CGAMapperContextGetRegister(context, kI8080CGARegisterWidth);
        h = I8080CGAMapperContextGetRegister(context, kI8080CGARegisterHeight);
        
        I8080CGAMapperContextSetRegister(context, kI8080CGARegisterEnable, 0b10000001);
        r2max = (w < h) ? w/2 : h/2, r2max *= r2max;
        for ( y = 0; y < h; y++ ) {
            for ( x = 0; x < w; x++ ) {
                int     dx = x - (w / 2),
                        dy = y - (h / 2);
                double  r2 = dx * dx + dy * dy;
                uint8_t p;
                
                if ( r2 > 36 && r2 < r2max ) {
                    p = 1 + ((int)floor(sqrt(r2)) % I8080CGAPalettes[palette_id]->n_colors);
                } else {
                    p = 0;
                }
                I8080CGAMapperContextWriteXY(context, x, y, p);
            }
        }
        box(context->wndw, -1, -1);
        I8080CGAMapperContextSetRegister(context, kI8080CGARegisterEnable, 0b00000001);
        
        getch();
        
        I8080CGAMapperContextSetEnable(context, 0);
        printf("wait for it...\n");
        sleep(5);
        I8080CGAMapperContextSetEnable(context, 1);
        
        I8080CGAMapperContextSetRegister(context, kI8080CGARegisterEnable, 0b10000001);
        r2max = w / I8080CGAPalettes[palette_id]->n_colors;
        if ( r2max == 0 ) r2max = 1;
        for ( y = 0; y < h; y++ ) {
            c = 1;
            for ( x = 0; x < w && c <= I8080CGAPalettes[palette_id]->n_colors;  ) {
                int     n = r2max;
                while ( n-- ) I8080CGAMapperContextWriteXY(context, x++, y, c);
                c++;
            }
        }
        box(context->wndw, -1, -1);
        I8080CGAMapperContextSetRegister(context, kI8080CGARegisterEnable, 0b00000001);
        
        getch();
        
        I8080CGAMapperContextSetEnable(context, 0);
    }
    
    
    printf("%d %d \n", h, w);
    
    return 0;
}
