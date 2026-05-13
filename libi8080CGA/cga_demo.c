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
    I8080CGAContextPtr  context;
    I8080CGAPaletteId_t palette_id = kI8080CGAPaletteIdDefault;
    
    if ( argc > 1 ) {
        long        id = strtol(argv[1], NULL, 0);
        if ( id >= kI8080CGAPaletteIdDefault && id < kI8080CGAPaletteIdMax ) palette_id = id;
    }
    
    I8080CGAInit();
    
    keypad(stdscr, TRUE);
    nodelay(stdscr, FALSE);
    clearok(stdscr, FALSE);
    leaveok(stdscr, TRUE);
    scrollok(stdscr, FALSE);
    immedok(stdscr, FALSE);
    curs_set(0);
    clear();
    
    int         w, h;
    
    getmaxyx(stdscr, h, w);
    
    WINDOW      *cga_display = newwin(h - 4, w - 4, 2, 2);
    
    context = I8080CGAContextCreate(
                    0x4000,
                    kI8080CGAModeClrGraphics,
                    cga_display);
    if ( context ) {
        keypad(cga_display, TRUE);
        nodelay(cga_display, FALSE);
        clearok(cga_display, FALSE);
        leaveok(cga_display, TRUE);
        scrollok(cga_display, FALSE);
        immedok(cga_display, FALSE);
        
        wrefresh(cga_display);
        refresh();
        
        I8080CGAContextLoadColorPalette(context, I8080CGAPalettes[palette_id]);
        
        int         x, y, c;
        int         r2max = (context->rgstrs.width < context->rgstrs.height) ?
                                context->rgstrs.width * context->rgstrs.width / 4 :
                                context->rgstrs.height * context->rgstrs.height / 4;
                                
        box(cga_display, -1, -1);
        for ( y = 0; y < context->rgstrs.height; y++ ) {
            for ( x = 0; x < context->rgstrs.width; x++ ) {
                int     dx = x - (context->rgstrs.width / 2),
                        dy = y - (context->rgstrs.height / 2);
                double  r2 = dx * dx + dy * dy;
                uint8_t p;
                
                if ( r2 > 36 && r2 < r2max ) {
                    p = 1 + ((int)floor(sqrt(r2)) % I8080CGAPalettes[palette_id]->n_colors);
                } else {
                    p = 0;
                }
                
                I8080CGAContextWriteXY(context, x, y, p);
            }
        }
        box(cga_display, -1, -1);
        wrefresh(cga_display);
        refresh();
        
        getch();
        wclear(cga_display);
        clear();
        wrefresh(cga_display);
        refresh();
        
        r2max = context->rgstrs.width / I8080CGAPalettes[palette_id]->n_colors;
        if ( r2max == 0 ) r2max = 1;
        for ( y = 0; y < context->rgstrs.height; y++ ) {
            c = 1;
            for ( x = 0; x < context->rgstrs.width && c <= I8080CGAPalettes[palette_id]->n_colors;  ) {
                int     n = r2max;
                while ( n-- ) I8080CGAContextWriteXY(context, x++, y, c);
                c++;
            }
        }
        box(cga_display, -1, -1);
        wrefresh(cga_display);
        refresh();
        
        getch();
        clear();
        wrefresh(cga_display);
        refresh();
        
        I8080CGAContextRestoreColors(context);
    }
    I8080CGAShutdown();
    
    printf("%d %d | %d %d \n", context->rgstrs.width, context->rgstrs.height, h, w);
    
    return 0;
}
