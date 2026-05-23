/*
 * I8080
 * An implementation of the I8080 processor.
 *
 * Dr. Jeffrey Frey
 * University of Delaware
 *
 * Header that defines the I8080 Game pad.
 */
 
#include "I8080DPad.h"

uint8_t
I8080DeviceDPadRead(
    I8080DevicePtr  dev,
    const void      *context
)
{
    I8080DPad_t     dpad_read;
    int             keycode;
    
    while ( (keycode = getch()) != ERR ) {
        switch ( keycode ) {
            case KEY_UP:
            case 'I':
            case 'i':
                dpad_read.bitmap |= kI8080DPadBitsUp;
                break;
            case KEY_LEFT:
            case 'J':
            case 'j':
                dpad_read.bitmap |= kI8080DPadBitsLeft;
                break;
            case KEY_DOWN:
            case 'K':
            case 'k':
                dpad_read.bitmap |= kI8080DPadBitsDown;
                break;
            case KEY_RIGHT:
            case 'L':
            case 'l':
                dpad_read.bitmap |= kI8080DPadBitsRight;
                break;
            case 'A':
            case 'a':
            case ' ':
                dpad_read.bitmap |= kI8080DPadBitsA;
                break;
            case 'S':
            case 's':
            case 'V':
            case 'v':
            case 'B':
            case 'b':
                dpad_read.bitmap |= kI8080DPadBitsB;
                break;
            case 'Q':
            case 'q':
                dpad_read.bitmap |= kI8080DPadBitsSelect;
                break;
            case 'W':
            case 'w':
            case '\n':
            case '\r':
            case KEY_ENTER:
                dpad_read.bitmap |= kI8080DPadBitsStart;
                break;
        }
    }
    return dpad_read.bitmap;
}

//

void
I8080DeviceDPadReset(
    I8080DevicePtr  dev,
    const void      *context
)
{
    nodelay(stdscr, TRUE);
    keypad(stdscr, TRUE);
    noecho();
}

//

void
I8080DeviceDPadShutdown(
    I8080DevicePtr  dev,
    const void      *context
)
{
    keypad(stdscr, FALSE);
    nodelay(stdscr, FALSE);
    echo();
}

//

I8080Device_t __I8080DeviceDPadIn = {
            .device_name = "game-dpad",
            .device_mode = kI8080DeviceModeInput,
            .input = {
                .read = I8080DeviceDPadRead },
            .output = {
                .write = NULL },
            .reset = I8080DeviceDPadReset,
            .shutdown = I8080DeviceDPadShutdown,
            .name = NULL
        };
const I8080DevicePtr I8080DeviceDPadIn = &__I8080DeviceDPadIn;
