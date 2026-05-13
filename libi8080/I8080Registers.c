/*
 * I8080
 * An implementation of the I8080 processor.
 *
 * Dr. Jeffrey Frey
 * University of Delaware
 *
 * Implements the I8080 registers API.
 */

#include "I8080Registers.h"
#include <ctype.h>

void
I8080RegistersPrint(
    FILE                *stream,
    I8080Registers_t    *rgstrs
)
{
    fprintf(stream,
        " ____________________________________________________________________________________________\n"
        "| [B]=[0x%1$02hhX|%1$-3hhu|%1$-4hhd|%23$c] [C]=[0x%2$02hhX|%2$-3hhu|%2$-4hhd|%24$c]  "
        " [BC]=[0x%3$04hX|%3$-5hu|%3$-6hd]         A       C |\n"
        
        "| [D]=[0x%4$02hhX|%4$-3hhu|%4$-4hhd|%25$c] [E]=[0x%5$02hhX|%5$-3hhu|%5$-4hhd|%26$c]  "
        " [DE]=[0x%6$04hX|%6$-5hu|%6$-6hd]   S Z - C - P - Y |\n"
        
        "| [H]=[0x%7$02hhX|%7$-3hhu|%7$-4hhd|%27$c] [L]=[0x%8$02hhX|%8$-3hhu|%8$-4hhd|%28$c]  "
        " [HL]=[0x%9$04hX|%9$-5hu|%9$-6hd]   =============== |\n"
        
        "| [F]=[0x%10$02hhX|%10$-3hhu|%10$-4hhd|%29$c] [A]=[0x%11$02hhX|%11$-3hhu|%11$-4hhd|%30$c]  "
        "[PSW]=[0x%12$04hX|%12$-5hu|%12$-6hd]   %13$X %14$X 0 %15$X 0 %16$X 1 %17$X |\n"
        
        "|                                               [PC]=[0x%18$04hX|%18$-5hu|%18$-6hd]                   |\n"
        "| [CYCLS]=[0x%19$012llX][%22$10.0f µs]       [SP]=[0x%20$04hX|%20$-5hu|%20$-6hd]           [%21$c]INTE |\n"
        "|____________________________________________________________________________________________|\n",
        rgstrs->B, rgstrs->C, rgstrs->BC,
        rgstrs->D, rgstrs->E, rgstrs->DE,
        rgstrs->H, rgstrs->L, rgstrs->HL,
        rgstrs->F, rgstrs->A, rgstrs->PSW,
        rgstrs->S, rgstrs->Z, rgstrs->AC, rgstrs->P, rgstrs->CY,
        rgstrs->PC, rgstrs->CYCLS, rgstrs->SP,
        (rgstrs->INTE ? '*' : ' '),
        I8080CycleCountToMicroseconds(rgstrs->CYCLS),
        isprint(rgstrs->B) ? rgstrs->B : ' ', isprint(rgstrs->C) ? rgstrs->C : ' ',
        isprint(rgstrs->D) ? rgstrs->D : ' ', isprint(rgstrs->E) ? rgstrs->E : ' ',
        isprint(rgstrs->H) ? rgstrs->H : ' ', isprint(rgstrs->L) ? rgstrs->L : ' ',
        isprint(rgstrs->F) ? rgstrs->F : ' ', isprint(rgstrs->A) ? rgstrs->A : ' '
        );
    
}
