# I8080

One night while browsing the Internet regarding assembly language a page was among the search results that cited the Intel 8080 processor, from circa 1976.  It was a programmer's guide of the day — a paper manual that had clearly been scanned into a PDF.

Compared to the ARM64 ISA I was learning at the time, the 8080 was so simple — with some advantages over the 6502 but some clear disadvantages.  (I never properly-appreciated indirect and indexed addressing modes before I learned about the 8080!)  It had six general purpose registers that could be accessed separately or in 16-bit pairs.  The stack could be located anywhere in memory and was not limited to a single 256-byte page.  All branching instructions (jump, call, return) were conditional, taken only if the referenced status flag was set.

Another interesting aspect was the integration of device i/o into the ISA.  The `IN #` and `OUT #` instructions wrote the accumulator to or read the accumulator from an indexed device, respectively.  There could be up to 256 input and 256 output devices present on a system.

How easily could I construct an emulator for the 8080?

## The library

### Register set

The project started with the registers.  The registers are organized in a packed struct such that they can be accessed as 8-bit components or as pairs:

```
I8080Registers_t    rgstrs;

rgstrs.H = 0x20, rgstrs.L = 0xED;
// the value of rgstrs.HL == 0x20ED
```

Likewise, the 8-bit component and paired registers can be accessed by index:

```
// B = 0, C = 1
rgstrs.R[I8080RgstrIdx(0)] = 0xFA, rgstrs.R[I8080RgstrIdx(1)] = 0xCE;
// the value of rgstrs.BC == 0xFACE
```

The accumulator and flag registers are also organized to be accessible as components or a pair:

- 8-bit accumulator
- 8-bit flags
- individual 1-bit flags
- 16-bit PSW

Two non-standard fields are included:

- 8-bit interrupt enable
- 64-bit cycle counter

### System memory

The 8080 could be used with a variety of memory technologies:  RAM, ROM, etc.  Addresses were 16-bit, for a maximum of 64 KiB of directly-addressable memory.

It's tempting on a modern system to just use an array of 65536 bytes for this piece.  My demo program was a meager 59 bytes, so that would be massive overkill.  I could also imagine having a standard subroutine ROM that I'd like to map to a section of the address space, and having it be mutable would be very problematic!

The `I8080Mem` pseudo-class splits the system memory into 256-byte pages.  The object includes a 256-element array of page-pointers that starts empty.  Only when a byte is written to a page will that page be allocated.  An external callback mechanism facilitates _memory mappers_ that can intercept read/write calls or rewrite the target address.  Only one memory mapper can be associated with a page of memory, but can span an arbitrary number of pages.  This makes it straightforward to implement a ROM image:

- A read callback that accesses the external ROM image, not the memory pages themselves
- A write callback that is a no-op

The read/write callbacks indicate to `I8080Mem` whether or not they handled the operation; if not, `I8080Mem` can proceed as normal.  Another application is _mirroring_, where multiple ranges of address space all access the same underlying memory (the NES did a lot of this with the 6502).  The address-rewrite callback can modify the address and pass control back to `I8080Mem`, effectively filtering the address for operations.

The pseudo-class includes helper functions to move chunks of data into and out of the memory space.  This is useful for loading code and data, or making core dumps for post-mortem on failing code.

#### "CGA"

An additional library is present in the project that adds a _Curses Graphics Adapter_ memory mapper.  The **curses** screen display library is used to display text, black-and-white graphics, or 8-bit indexed color graphics on the terminal.  The black-and-white graphics mode uses normal space characters for black and reverse video space characters as white.  For color graphics mode, the terminal **must** support curses color functions; custom color palettes can be loaded (or created from within the emulated system itself) if the terminal supports changing the color.  Color pixels are drawn as a space character with the color pair attribute set to the desired color index.  In all three modes, a position on the screen is mapped to a byte in the memory space to which the adapter is mapped.  For the following sections, it will be assumed that the CGA device is mapped at address  `$4000` through `$5FFF`.

##### Registers

The first 16 bytes of the mapped memory space are the CGA _registers_ that software within the emulated system can use to control the CGA device.

| Index | Addr | Name | Description |
| :---: | :--: | :--- | :---------- |
| `0`   | `$4000` | `supmodes` | (Read-only) the display modes supported by this device (see below) |
| `1`   | `$4001` | `width` | (Read-only) the width of the display |
| `2`   | `$4002` | `height` |  (Read-only) the height of the display |
| `3`   | `$4003` | `ncolors` |  (Read-only) the number of colors supported |
| `4`   | `$4004` | `enable` |  Disable the display = `0`; enable = non-zero; deferred redraw = non-zero with MSb set (clear the MSb to trigger screen draw) |
| `5`   | `$4005` | `x` |  x-coordinate, used with triggered display operations |
| `6`   | `$4006` | `y` |  y-coordinate, used with triggered display operations |
| `7`   | `$4007` | `i` |  index, used with triggered display operations |
| `8`   | `$4008` | `r` |  red intensity, used with triggered display operations |
| `9`   | `$4009` | `g` |  green intensity, used with triggered display operations |
| `10`  | `$400A` | `b` |  blue intensity, used with triggered display operations |
| `11`  | `$400B` | `u16lo` | LSB of a 16-bit value, used with triggered display operations |
| `12`  | `$400C` | `u16hi` | MSB of a 16-bit value, used with triggered display operations |
| `13`  | `$400D` | `mode` | video mode: 0=text, 1=b&w graphics, 2=color graphics |
| `14`  | `$400E` | `op` | trigger a display operation |

The typical startup scenario is to enable the display by writing a non-zero value to the `enable` register; this will populate the read-only registers and put the display in the default initial mode.  The `mode` register can then be written with the desired display mode — the code can re-read the register to ensure that the mode has been entered (if mode 2 is requested but curses does not support color, the mode would remain unchanged).

##### Triggered operations

Writing the following values to the `op` register will trigger a display operation:

|     `op`     | Name          | Description |
| :----------: | :------------ | :---------- |
|    `0x01`    | `clear`       | Clear the screen |
|    `0x02`    | `clearrow`    | Clear the row indicated by the `y` register |
|    `0x03`    | `clearcol`    | Clear the column indicated by the `x` register |
|    `0x04`    | `clearrows`   | Clear the `i` rows starting at the `y` register |
|    `0x05`    | `clearcols`   | Clear the `i` columns starting at the `x` register |
|    `0x06`    | `fillrow`     | Fill the row indicated by the `y` register with the byte in `i`|
|    `0x07`    | `fillcol`     | Fill the column indicated by the `x` register with the byte in `i`|
|    `0x10`    | `writenchars` | The `u16lo` and `u16hi` registers are set to the address of a sequence of bytes/characters; the `i` register is set to the number of bytes/characters; and the `x` and `y` registers indicate the starting coordinate:  write bytes/characters to the display |
|    `0x11`    | `writecstr`   | The `u16lo` and `u16hi` registers are set to the address of a NUL-terminated sequence of bytescharacters; and the `x` and `y` registers indicate the starting coordinate:  write bytes/characters to the display |
|    `0x7e`    | `getcolorrgb` | The `i` register is set to a color index; the `r`, `g`, and `b` registers will contain the component intensities for the color with that index |
|    `0x7f`    | `setcolorrgb` | The `i` register is set to a color index and the `r`, `g`, and `b` registers are set to the component intensities; the color at index `i` will be set to those values |
| `0b1XXXXXXX` | `fill`        | The lower 7-bits of the value will be written to every screen element of the display; for text, this will fill the screen with a single ASCII character; for graphics, the screen will be filled with the given color |

##### Addressing display elements

Each individual cell of the graphics array occupies a byte of memory starting at an offset of 16 bytes from the base mapped address:  in this example, that would put the upper-left corner at `$4010`.  For an 80x40 display, 3200 bytes are necessary (`$C80`) putting the total address space occupied at `$C90` bytes; the final display element at the bottom-right would reside at `$C8F`.

Display elements are ordered in row-major format:  display elements of the first row occupy a sequence of bytes matching the width of the display (which can be determined using the `width` register), followed by the second row, through the height of the display (which can be determined using the `height` register).

#### Realtime clock

I can recall going to a Hershey Apple Core user group auction when I was a teenager.  For a few dollars I bought a gigantic expansion card that I gathered would expand the memory in my Laser 128, provider better graphics, and even give the computer a realtime clock!  I dutifully slid the card into the expansion slot and was rewarded with...nothing.  Looking back, I'm pretty sure it was meant for a PC and someone got a little creative with their salesmanship (and I was a tad naive, too).

Today, having a clock built-in to your computer, TV, phone, microwave, refrigerator, etc. is just expected.  So why not expect to have on available as an add-on in an 8080 emulator?  Like the CGA library, the Timer library implements a memory mapper that occupies a single page of address space.

##### Registers

In the first 16 bytes of the mapped page the Timer provides a series of read-only registers.  This discussion assumes the mapper has been associated with the page at `$DF00`.

| Index | Addr | Name | Description |
| :---: | :--: | :--- | :---------- |
| `0`   | `$DF00` | `S` | (Read-only) current clock second |
| `1`   | `$DF01` | `M` | (Read-only) current clock minute |
| `2`   | `$DF02` | `H` | (Read-only) current clock hour (0-23) |
| `3`   | `$DF03` | `d` | (Read-only) current day of the month |
| `4`   | `$DF04` | `m` | (Read-only) current month (1-12) |
| `5`   | `$DF05` | `Y` | (Read-only) current 2-digit year (2026 = 26) |
| `6`   | `$DF06` | `Ycent` | (Read-only) current century (2026 = 20)  |
| `7`   | `$DF07` | `fired` | (Read-only) last timer index that elapsed ("fired") |

##### Timers

The main reason this mapper was created was to provide programs running in the emulator with realtime timer functionality.  There are eight distinct timers available, starting at offset `$10` from the base of the page.  A timer occupies 8 bytes of space, so timer 0 is found at offset `$10`, timer 1 at `$18`, timer 2 at `$20`, etc.

Each timer consists of a series of read/write registers:

| Index | Addr | Name | Description |
| :---: | :--: | :--- | :---------- |
| `0`   | `$DFXY` | `enable` | Enablement state of the timer |
| `1`   | `$DFXY` | `opcode` | Opcode to raise with the CPU when the timer elapses |
| `2`   | `$DFXY` | `msecFrac` | Fractional milliseconds (as an 8-bit, 8-digit mantissa fixed-point value) |
| `3`   | `$DFXY` | `msecWholeB0` | LSB of the 32-bit whole millisecond count |
| `4`   | `$DFXY` | `msecWholeB1` |    : |
| `5`   | `$DFXY` | `msecWholeB2` |    : |
| `6`   | `$DFXY` | `msecWholeB3` | MSB of the 32-bit whole millisecond count |

Where `X` is {1, 2, 3, 4} and `Y` is {0, 1, 2, 3, 4, 5, 6, 8, 9, A, B, C, D, E}, dependent on the timer index.

The timer interval is expressed as a 32-bit number of whole milliseconds and a fractional fixed-point component.  For example, an interval of 12000.5 milliseconds is:

`
12000 + 0.5 ms
= 0x00002EE0 + (0.5 * 256) ms
= 0x00002EE0 + 0x80
`

The `enable` register of each timer is initialized to `$00` at boot, corresponding to the unused state:  no resources are configured yet.  Incrementing to `$01` provisions the timer but does not start it; incrementing to `$02` starts the timer.  Once the timer elapses it "fires" and the `opcode` is delivered to the CPU.  The `RST #` opcodes are typically used since they are a single byte but actually accomplish something.  Another candidate would be `EI` to simply reenable interrupts.

With `enable` of `$02`, once the timer elapses it will return to `$01`.  For timers that should reenable automatically after elapsing, `$82` should be written to the `enable` register.

The following code sets timer `$02` for a 12000.5 ms interval, to elapse just once, and send the `EI` opcode:

```
        lxi     hl, 0DF26   ; start at the msecWholeB3 register
        mvi     m, 00h      ;
        dcr     l           ; decrement HL to write to msecWholeB2
        mvi     m, 00h      ;
        dcr     l           ; decrement HL to write to msecWholeB1
        mvi     m, 02eh     ;
        dcr     l           ; decrement HL to write to msecWholeB0
        mvi     m, 0e0h     ;
        dcr     l           ; decrement HL to write to msecFrac
        mvi     m, 080h     ;
        dcr     l           ; decrement HL to write to opcode
        mvi     m, 0fbh     ; EI = $FB
        dcr     l           ; decrement HL to write to enable
        mvi     m, 02h      ; enable <= 2 ("on")
```

A more-complex example is discussed in a later section.

#### Picture Processing Unit (PPU)

The CGA memory mapper device obscures a large region of memory since its 8 bits-per-pixel (BPP) direct backing of on-screen pixels can add up quickly:  an 80x40 display needs 3200 B for that VRAM, or 3.125 KiB or nearly 13 pages.  For higher resolution, like 120x80, the 9600 B occupies 37.5 pages.  Mapped at `$4000`, that's an ending address for the mapper of `$6590`!

In the early days of video gaming RAM was a precious resource, and having that much RAM used for pixel data was expensive.  Much of the pixel data is redundant:  screens full of graphical elements often repeat signifiant patterns.  One device I've tried to understand in-depth is the original NES:  its Picture Processing Unit (PPU) rendered to a 256x240 pixel display — 61440 pixels.  That would be 60 KiB for an 8 BPP format!  Instead, repeatable graphical units — tiles — were drawn to the display on-the-fly.  The 8x8 pixel tiles cut the memory required for a screen full of pixels by 64x:  a 32x30 tile map with an 8-bit tile index is just 960 B.  Tiles used a 2 BPP color index relative to a selected color palette:  16 bytes per tile.  Tile indices were 8-bits, meaning a table was comprised of 256 tiles (4 KiB).  With a tile map producing the background image, animated components were represented by sprites drawn at arbitrary (x,y) coordinates.

So why not take what I know of the NES PPU and use it to add a PPU to my 8080 emulator?  I settled on a fixed screen size of 128x64 pixels; 8x4 tiles (terminal windows pixels are rectangular, longer in the y-direction) with 2 BPP; 8 active color palettes (without restriction on some being for sprites and some for backgrounds); room for 32 of those 8x4 tiles; and room for 52 sprites.  By having just 32 tiles and 8 palettes, every tile reference can be an 8-bit value and there is no need for a separate attribute table for palette selection.  Likewise, sprites and backgrounds can make use of all 8 palettes, not just 4 each as in the NES.

#### Bank-Switched Memory expansion

What happens if a program needs more code or more data than the 64 KiB of RAM can hold?  The Apple IIe and IIc added a second bank of 64 KiB of RAM for a total of 128 KiB, but the 6502 was only capable of 16-bit addressing — so how did the other bank get accessed?  The short answer is that registers were altered to change which 64 KiB of RAM was attached to the address bus:  based on a "switch" the address `$2000` refers to two unique bytes of RAM.  The NES used bank-switching to, for example, switch which graphical tiles were visible in the system address space:  no need to copy 4 KiB of data around RAM, just alter what data in cartridge ROM is accessible at `$XXXX` on the system address bus.

The `lib8080BSM` library implements a multi-bank memory-mapper that user code running in the emulator can switch via the 8080 device bus.  There are two ways to provision a BSM object of size `$pp00` mapped at `$PP00`:

1. A single file is read into memory and divided into _N_ banks of `$pp` pages
2. _N_ files are read into memory and each acts as a bank of `$pp` pages

A device i/o channel is associated with the BSM:  reading the channel (e.g. `IN 04h`) sets the accumulator to the index of the in-scope bank, moving the desired bank index to the accumulator and writing to the channel (e.g. `OUT 04h`) will put that bank in-scope for subsequent memory access.  Consider the following example program:

```
                ORG 0400h
                
                LXI     D, 0B000h       ; DE <= BSM is mapped at $B000-$C000
                LXI     B, 00000h       ; BC <= $0000
-:              LDAX    D               ; A <= (DE)
                ADD     C               ; A <= A + C
                MOV     C, A            ; C <= A = A + C (C += (DE))
                MVI     A, 00h          ; A <= $00
                ADC     B               ; A <= A + B
                MOV     B, A            ; B <= A = A + B (B += CY)
                INX     D               ; DE <= DE + 1
                MOV     A, D            ; A <= D
                CPI     0C0h            ; Check if the high-byte in DE has hit $C0
                JZ      +               ; We've reached $C000, all done with the mapped region
                CPI     0B8h            ; Check if the high-byte in DE is half-way through
                JNZ     -               ; Not there yet, go add the next byte into BC
                MOV     A, E            ; Check if DE is $B800
                CPI     00h
                JNZ     -               ; Nope, do the next iteration of the summing
                MVI     A, 01h          ; DE is currently $B800, time to
                OUT     12h             ; switch to bank 1!
                JMP     -               ; Continue summing...
+:              HLT
```

This code sums 4096 8-bit integers into the 16-bit BC register.  The first 2048 bytes come from bank 0, the rest from bank 1 of the BSM mapped at `$B000` in the emulator.  Let me create the two banks:

```bash
$ for dummy in $(seq 0 4095); do printf "%b" '\0x01' >> ones; done
$ for dummy in $(seq 0 4095); do printf "%b" '\0x02' >> twos; done
```

If I create a BSM that combines these two files as two 4096-byte banks, the program above should add 1 2048 times then add 2 2048 times:  (2048 x 1) + (2048 x 2) = 6144 = `$1800`:

```bash
$ i8e8e -P 0x0400 -S 0x0000 --load=bsm-demo.bin@0x400 -A --bsm 0xB0+0x10#18:ones:twos

Resource usage in system runloop: u_cpu=0.001189, s_cpu=3e-06, swaps=0, io[i]=0, io[o]=0

 ____________________________________________________________________________________________
| [B]=[0x18|24 |24  | ] [C]=[0x00|0  |0   | ]   [BC]=[0x1800|6144 |6144  ]         A       C |
| [D]=[0xC0|192|-64 | ] [E]=[0x00|0  |0   | ]   [DE]=[0xC000|49152|-16384]   S Z - C - P - Y |
| [H]=[0x00|0  |0   | ] [L]=[0x00|0  |0   | ]   [HL]=[0x0000|0    |0     ]   =============== |
| [F]=[0x40|64 |64  |@] [A]=[0xC0|192|-64 | ]  [PSW]=[0x40C0|16576|16576 ]   0 1 0 0 0 0 1 0 |
|                                               [PC]=[0x0427|1063 |1063  ]                   |
| [CYCLS]=[0x00000004D625][    158482 µs]       [SP]=[0x0000|0    |0     ]           [*]INTE |
|____________________________________________________________________________________________|
I8080Device[$00] [←0x00000000|0x00000000→] BYTES  "tty"
I8080Device[$12] [←0x00000000|0x00000001→] BYTES  "bank-switched-memory"
I8080Device[$FF] [           |0x00000000→] BYTES  "stderr"
I8080Mem[$0000..$03FF] : unused
I8080Mem[$0400..$04FF] : allocated RAM
I8080Mem[$0500..$AFFF] : unused
I8080Mem[$B000..$BFFF] : mapper 0x6000013e0238 "bank-switched-memory"
I8080Mem[$C000..$FFFF] : unused
```

If the bank-switch `MVI A, 01h` is changed to `MVI A, 00h` we will remain in bank 0 ("ones") and can expect the sum to be (2048 x 1) + (2048 x 1) = 4096 = `$1000`:

```bash
$ ../../build/i8e8e/i8e8e -P 0x0400 -S 0x0000 --load=bsm-demo.bin@0x400 -A --bsm 0xB0+0x10#18:ones:twos
   :
| [B]=[0x10|16 |16  | ] [C]=[0x00|0  |0   | ]   [BC]=[0x1000|4096 |4096  ]         A       C |
```

The BSM library can create in-memory representations of banks by allocate-and-fill **or** by using `mmap()` to map the original files themselves into the emulator's virtual address space.  The `i8e8e` emulator uses the `-m`/`--rom-map-mode` mode _at the time the `-b`/`--bsm` option is used on the command line_ to dictate which method is used to move the file data into memory:

```bash
$ ../../build/i8e8e/i8e8e -P 0x0400 -S 0x0000 -m mmap --load=bsm-demo.bin@0x400 -A --bsm 0xB0+0x10#18:ones:twos
   :
| [B]=[0x10|16 |16  | ] [C]=[0x00|0  |0   | ]   [BC]=[0x1000|4096 |4096  ]         A       C |
```

##### Registers

The first 16 bytes of the mapped address region act as registers.  This discussion assumes the mapper has been associated with the page at `$8000`.

| Index | Addr | Name | Description |
| :---: | :--: | :--- | :---------- |
| `0`   | `$8000` | `ppu_mode` | Control the rendering functionality of the PPU |
| `1`   | `$8001` | `ppu_status` | (Read-only) bits set to reflect state of a rendering pass |
| `2`   | `$8002` | `sprite_offset` | Set to bias the selection of sprites for rendering; useful if the per-line limit has been hit (as reflected by the `ppu_status` from the last pass) |
| `3`   | `$8003` | `inte_opcode` | Set to a non-zero opcode and the renderer will start a cycle by raising an interrupt with the system, triggering the exection of that opcode (e.g. a `RST #`).  This can be used to trigger any non-graphical game update code to run while the renderer is building and displaying the screen. |
| `4`   | `$8004` | `is_rendering` | (Read-only) non-zero when the PPU is rendering, zero when it is not |
| `5`   | `$8005` | `dma_dst` | DMA copy destination |
| `6`   | `$8006` | `dma_src_offet` | DMA copy source offset within page |
| `7`   | `$8007` | `dma_src_page` | DMA copy source page and trigger |

The `ppu_mode` comprises a series of bits:

|  Bits   | Name | Usage |
| :-----: | :--- | :---- |
|   `0`   | `map_select` | when set, tile map 1 is drawn; when reset, tile map 0 is drawn (the default) |
|   `1`   | `render_enable` | master switch for rendering, set = on, reset = off (the default) |
|   `2`   | `render_background` | enable (1)/disable(0, the default) rendering of background tiles |
|   `3`   | `render_sprites` | enable (1)/disable(0, the default) rendering of sprites |
| `4`-`7` | `map_flip_freq_mask` | the integer value of these four bits is used to determine the frequency at which the PPU automatically switches between tile map 0 and 1; i=`0001` means every 2^i render cycles the map switches, in this case every other cycle |

When the system is reset the mode starts with a value of `$00`.  It is up to the user program to enable rendering, etc.

The `ppu_status` holds two bits:

|  Bits   | Name | Usage |
| :-----: | :--- | :---- |
|   `6`   | `sprite_overflow` | the renderer clears this bit as a cycle begins and sets it if any row of pixels had more than 16 sprites present and enabled |
|   `7`   | `sprite_0_hit` | the renderer clears this bit as a cycle begins; if the sprite at index 0 is drawn and an opaque pixel from the sprite overlaps an opaque pixel of the background, this bit is set |

The `sprite_0_hit` probably doesn't have much use in this emulator, since the renderer as currently implemented does not draw pixels on a fixed timing (it really can't, it's at the mercy of curses).  The overflow would typically influence the increment of the `sprite_offset` to bias the selection of higher-index sprites in subsequent rendering cycles.

The DMA feature allows the user program to setup bulk transfers from one region of memory to a region of the PPU.  The `dma_dst` can have the following values:

| Value | Meaning |
| :---: | :------ |
|  `0`  | Copy 208 bytes to the sprite table |
|  `1`  | Copy 256 bytes to the tile table |
|  `2`  | Copy 256 bytes to tile map 0 |
|  `3`  | Copy 256 bytes to tile map 1 |

The `dma_dst` and `dma_src_offset` registers should be set and then the `dma_src_page` register can be set, triggering the bulk copy from the page indicated by the byte written to the register.  So setting `dma_dst` to `$02`, `dma_src_offset` to `$00`, then setting `dma_src_page` to `$24` will copy 256 bytes from `$2400` through `$24FF` to tile map 0.  This feature is primarily useful if the program image or a ROM image has prepared data that can easily be loaded into the PPU with a DMA call.  The DMA operation does NOT reset the registers, so altering the data and again writing `$24` to `dma_src_page` will retrigger the same copy.

##### Direct Access

The PPU memory can also be directly accessed by a user program.  Again, assuming the mapper is situated at address `$8000`:

| Address | Bytes | Component |
| :------ | ----: | :-------- |
| `$8000` |    16 | The registers |
| `$8010` |    32 | The 8 color palettes |
| `$8030` |   208 | The 52 sprites |
| `$8100` |   256 | The 32 tile definitions |
| `$8200` |   256 | Tile map 0, upper-left at index 0, row major |
| `$8300` |   256 | Tile map 1, upper-left at index 0, row major |

Making changes while the renderer is active can have...interesting side effects.  The renderer runs in a separate thread, so the user program could make changes during a rendering cycle!

### Device bus

To allow the emulator to interface with devices, an API was created wherein a device is implemented as a data structure and callback functions:

- Byte read (an input callback)
- Byte write (an output callback)
- Reset (state changes on system boot or reset)
- Shutdown (state changes on system power off or device removal)

Devices can request a specific id when registering with the bus, or the bus can be allowed to assign an id.  The former is particularly useful since the ISA has the i/o id embedded as an operand (not taken from a register, by contrast):

```
LOOP:       IN  #0          ; accumulator <= byte from input device 0
            CPI #$04        ; was a CTRL-D read?
            JZ  EXIT        ; no more input, exit the program
            OUT #2          ; accumulator => line printer, output device 2
EXIT:       HLT             ; terminate the program
```

The code includes `I8080Device` wrappers for C stdin, stdout, and stderr and a `I8080DeviceTTY` that bundles stdin and stdout together as a single device.  The stdin wrapper has a few optional behaviors:

- Respond to input of a single character at a time, no buffering
- Disable echo of input characters

The stdout wrapper can be configured to automatically clear the terminal screen on reset.  On some systems, the `OLCUC` terminal output option can be set to force all output to uppercase alphabetic characters.

### Instruction decoding and dispatch

One thing that struck me about the 8080 documentation as I read it was that the opcodes were structured such that simple masked comparisons would easily extract categories:  all register decrement (`DCR`) instructions matched the pattern `0b00XXX101`, where the `XXX` bits selected the target register:

- `000` : B
- `001` : C
- `010` : D
- `011` : E
- `100` : H
- `101` : L
- `110` : memory ref (address in HL)
- `111` : A

So the comparison `(opcode & 0b11000111) == 0b00000101` is sufficient to establish that a `DCR` is being requested.  There were a few exceptions to the simplicity (among the jump, call, and return instructions).

Rather than create 250+ separate functions for each unique opcode, functions that collectively handle an instruction type were created.  The register increment (`INR`) instructions differ from `DCR` only in the set/reset state of bit 0, so a single `I8080InstrDispatchSingleRegIncDec()` function handles the dispatch of all 16 opcodes by extracting the operation from bit 0 and the register index from bits 3 through 5.  A `I8080InstrHandler_t` record provides the bitmask and comparator for matching opcodes to this dispatch function.

Each class of instruction gets its own `I8080InstrHandler_t` record, and the records can be linked together to form a dispatch chain.  In theory, this linked list could be used to match opcodes to handlers as a program is executed.

Then again, there are only 244 opcodes in the 8080 ISA, and an array of that many pointers is just under 2 KiB.  A linked list of `I8080InstrHandler_t` records can be _compiled_ into a `I8080InstrTable_t` that associates an array index with a dispatch function pointer.  At runtime, rather than walking the linked list to match an opcode to a function, the opcode is used as an index into the table:  much faster.  Another important benefit of compiling the `I8080InstrHandler_t` linked list is that collisions can be detected:  if two `I8080InstrHandler_t` records respond to the same opcode, that will be detected.

### The basic 8080 system

With the above subsystems implemented, they must be tied together to make a system.  The `I8080System_t` does just this.  It is implemented in such a way as to be extensible by the consumer:  when an instance is allocated, additional bytes can be requested as part of the data structure.  This allows the programmer to create their own system implementation that gets embedded in the baseline `I8080System_t`:

```
typedef struct {
    :
} SpaceInvaders_t;

  :
  
I8080SystemPtr  sys8080;
SpaceInvaders_t *space_invaders;

sys8080 = I8080SystemCreate(0, sizeof(SpaceInvaders_t));
space_invaders = (SpaceInvaders_t*)sys8080->aux_data;
```

When the baseline system is allocated:

- a zeroed register set is created
- a system memory space is allocated
- a device bus is allocated
    - the `I8080DeviceTTY` is registered as input and output device 0
- a `I8080InstrTable_t` is initialized using the standard ISA linked list
- the system state is set to "off"

The next step is to set the system into the "on" state.  This causes the system to invoke a reset on the memory and device bus.

After this step, data and program code can be loaded into the system memory.  The program counter can be set, and then the system can either be stepped one instruction at a time or put in a run loop.  On the first instruction, the system will be transitioned into the "running" state.

The system will continue processing instructions until:

- an illegel instruction is encountered
- the `HLT` instruction is encountered

In both cases, the system will transtion out of the "running" state and back to the "on" state.

## The example programs

There are a number of example 8080 programs included in the project.  They make use of the `i8e8e` CLI emulator.

### Random number generator

The [rng.asm](example/rng/rng.asm) example exploits the `i8e8e` emulator's ability to attach arbitrary files to an i/o channel.  The `/dev/urandom` file on Unix-like systems is an OS-provided random number generator.  Connecting that file to input channel 10, an 8080 program can fill the accumultor with a random byte quite easily:

```
    org     0400h
    
    mvi     h, '1'
    mvi     l, '0'
    in      10          ; A <- byte read from input 10
    mov     d, a
    in      10          ; A <- byte read from input 10
    mov     e, a
    dad     de
    hlt
```

The HL register pair is loaded with two ASCII characters, the DE pair is loaded with two random bytes.  DE is added to HL and the program halts with the resulting sum in HL.  To get `/dev/urandom` present as input channel 10:

```
i8e8e -L rng.bin@0x400 -f /dev/urandom:i#10 -P 0x400
```

The `i` indicates input-only, and `10` is the channel number (see `i8e8e --help` for more info).  Assembling and running the program:

```
 ____________________________________________________________________________________________
| [B]=[0x00|0  |0   | ] [C]=[0x00|0  |0   | ]   [BC]=[0x0000|0    |0     ]         A       C |
| [D]=[0xA2|162|-94 | ] [E]=[0x95|149|-107| ]   [DE]=[0xA295|41621|-23915]   S Z - C - P - Y |
| [H]=[0xD3|211|-45 | ] [L]=[0xC5|197|-59 | ]   [HL]=[0xD3C5|54213|-11323]   =============== |
| [F]=[0x00|0  |0   | ] [A]=[0x95|149|-107| ]  [PSW]=[0x0095|149  |149   ]   0 0 0 0 0 0 1 0 |
|                                               [PC]=[0x040C|1036 |1036  ]                   |
| [CYCLS]=[0x00000000003D][        30 µs]       [SP]=[0x0000|0    |0     ]           [*]INTE |
|____________________________________________________________________________________________|
I8080Device[$00] [←0x00000000|0x00000000→] BYTES  "tty"
I8080Device[$0A] [←0x00000002|           ] BYTES  "/dev/urandom"
I8080Device[$FF] [           |0x00000000→] BYTES  "stderr"
I8080Mem[$0000..$03FF] : unused
I8080Mem[$0400..$04FF] : allocated RAM
I8080Mem[$0500..$FFFF] : unused
```

The `IODevice` summary lines show input channels on the left, output on the right.  Excellent — two bytes were read from `/dev/urandom` which was on channel 10 (`$0A`).  Hopefully if the program is run multiple times different (randomized) results will be produced:

```
$ for dummy in `seq 0 10`; do make run; done | grep HL
| [H]=[0xDE|222|-34 | ] [L]=[0x2B|43 |43  |+]   [HL]=[0xDE2B|56875|-8661 ]   =============== |
| [H]=[0x3A|58 |58  |:] [L]=[0x56|86 |86  |V]   [HL]=[0x3A56|14934|14934 ]   =============== |
| [H]=[0x13|19 |19  | ] [L]=[0xE1|225|-31 | ]   [HL]=[0x13E1|5089 |5089  ]   =============== |
| [H]=[0xC5|197|-59 | ] [L]=[0x2C|44 |44  |,]   [HL]=[0xC52C|50476|-15060]   =============== |
| [H]=[0x4C|76 |76  |L] [L]=[0x32|50 |50  |2]   [HL]=[0x4C32|19506|19506 ]   =============== |
| [H]=[0xAD|173|-83 | ] [L]=[0x08|8  |8   | ]   [HL]=[0xAD08|44296|-21240]   =============== |
| [H]=[0x22|34 |34  |"] [L]=[0x83|131|-125| ]   [HL]=[0x2283|8835 |8835  ]   =============== |
| [H]=[0x22|34 |34  |"] [L]=[0x79|121|121 |y]   [HL]=[0x2279|8825 |8825  ]   =============== |
| [H]=[0xEA|234|-22 | ] [L]=[0xC6|198|-58 | ]   [HL]=[0xEAC6|60102|-5434 ]   =============== |
| [H]=[0x55|85 |85  |U] [L]=[0xC6|198|-58 | ]   [HL]=[0x55C6|21958|21958 ]   =============== |
| [H]=[0x2D|45 |45  |-] [L]=[0xF4|244|-12 | ]   [HL]=[0x2DF4|11764|11764 ]   =============== |
```

Hooray — our emulator has a good-quality RNG and no 8080 programming was necessary!

### Multiplication

Like many early CPUs the 8080 ISA does not include integer multiplication or division instructions.  The [multiply.asm](examples/multiply/multiply.asm) program contains three algorithms for multiplication of two 8-bit values.  The first is a naive repeated addition approach:

```
;
;  If E = (1 + 1 + … + 1), then
;  D * E = D * (1 + 1 + … + 1)
;        = D + D + … + D
;
; The 8-bit multiplier and multiplicand are in
; registers D and E.  On exit, the pair DE
; has the (possibly) 16-bit product.
;
mltalg1:        push        bc          ; we'll clobber BC and HL
                push        hl
                mvi         h, 0h       ; HL <- 0000h
                mvi         l, 0h
                mov         b, d        ; use B as our loop counter
                                        ; holding D...
                mvi         d, 0h       ; ..and zero-out the MSB of
                                        ; DE
                inr         b           ; we can only catch multiply
                                        ; by zero if we force a test
mltalg1_1:      dcr         b           ; of B == 0 by incrementing
                                        ; then decrementing
                jz          mltalg1_2   ; B == 0, all done
                dad         de          ; HL += DE
                jmp         mltalg1_1   ; next iteration
mltalg1_2:      xchg                    ; DE <=> HL
                pop         hl          ; restore clobbered registers
                pop         bc          ; and return with the product
                ret                     ; in DE
```

Register D will _always_ be the multiplier, so even if E < D the loop will be performed D times.  Even worse, if E is `$00` and D is `$FF` the code will loop 255 times.  The cycle count output that `asm8080` produces shows that the body of the loop comprises 35 clock cycles.

The second subroutine uses bit shifts to effect a sum of factors-of-two times the multiplicand:

```
;
;  D * E = D * Sum(i=0…7, E_i * 2^i)
;        = Sum(i=0…7, E_i * D * 2^i)
;        = Sum(i=0…7, E_i * (D << i))
; Since E_i will be 0/1, the multiplication
; is a sum of shifted copies of the multiplier.
;
; The 8-bit multiplier and multiplicand are in
; registers D and E.  On exit, the pair DE
; has the (possibly) 16-bit product.
;
mltalg2:        push        psw         ; we'll clobber all registers
                push        bc
                push        hl
                mvi         h, 0h       ; HL <- 0000h
                mvi         l, 0h
                mvi         b, 0h       ; load the multiplicand, E, into BC
                mov         c, e
                mov         a, d        ; move the multiplier, D, to A
                mvi         d, 8        ; now use D as our loop counter
mltalg2_1:      rar                     ; A >> 1 -> carry
                jnc         mltalg2_2   ; do not add this factor
                dad         bc          ; HL += BC
mltalg2_2:      push        psw         ; save A
                xra         a           ; clear carry
                mov         a, c
                ral                     ; carry <- c << 1
                mov         c, a
                mov         a, b
                ral                     ; b << 1 <- carry
                mov         b, a        ; thus, BC = BC * 2
                pop         psw         ; restore A (the shifted multiplier)
                dcr         d           ; decrement counter and
                jp          mltalg2_1   ; go do another round
                xchg                    ; DE <=> HL
                pop         hl          ; restore clobbered registers
                pop         bc          ; and return with the product
                pop         psw         ; in DE
                ret
```

The repeated addition algorithm requires N iterations, where N is the value of the multiplier; by comparison, the second algorithm takes a constant 8 iterations.  So for circa 3.125% of the possible multipliers the second algorithm may be slower.  This algorithm also does not catch any special case like with D or E is `$00`.  The cycle count from `asm8080` shows that the body of the loop comprises 82/92 clock cycles — circa 2 times that of the repeated addition loop.

Let's start with the worst case:  0xFF times 0xFF.  Repeated addition (`mltalg1`):

```
 ____________________________________________________________________________________________
| [B]=[0x00|0  |0   | ] [C]=[0x00|0  |0   | ]   [BC]=[0x0000|0    |0     ]         A       C |
| [D]=[0xFE|254|-2  | ] [E]=[0x01|1  |1   | ]   [DE]=[0xFE01|65025|-511  ]   S Z - C - P - Y |
| [H]=[0x00|0  |0   | ] [L]=[0x00|0  |0   | ]   [HL]=[0x0000|0    |0     ]   =============== |
| [F]=[0x40|64 |64  |@] [A]=[0x00|0  |0   | ]  [PSW]=[0x4000|16384|16384 ]   0 1 0 0 0 0 1 0 |
|                                               [PC]=[0x0408|1032 |1032  ]                   |
| [CYCLS]=[0x000000002369][      4532 µs]       [SP]=[0x0000|0    |0     ]           [*]INTE |
|____________________________________________________________________________________________|
```

versus the factors-of-two algorithm (`mltalg2`):

```
 ____________________________________________________________________________________________
| [B]=[0x00|0  |0   | ] [C]=[0x00|0  |0   | ]   [BC]=[0x0000|0    |0     ]         A       C |
| [D]=[0xFE|254|-2  | ] [E]=[0x01|1  |1   | ]   [DE]=[0xFE01|65025|-511  ]   S Z - C - P - Y |
| [H]=[0x00|0  |0   | ] [L]=[0x00|0  |0   | ]   [HL]=[0x0000|0    |0     ]   =============== |
| [F]=[0x00|0  |0   | ] [A]=[0x00|0  |0   | ]  [PSW]=[0x0000|0    |0     ]   0 0 0 0 0 0 1 0 |
|                                               [PC]=[0x0408|1032 |1032  ]                   |
| [CYCLS]=[0x0000000003CB][       486 µs]       [SP]=[0x0000|0    |0     ]           [*]INTE |
|____________________________________________________________________________________________|
```

As expected:  255 * 35 cycles = 8925, versus 8 * [82…92] = [656…736].  If we zero-out bit 7 of both factors (0x7F * 0x7F) the repeated addition algorithm predictably improves by around a factor of two:

```
| [CYCLS]=[0x0000000011E9][      2292 µs]       [SP]=[0x0000|0    |0     ]           [*]INTE |
```

We observe a minimal time-savings of 10 cycles in the second algorithm (thanks to a skipped sum):

```
| [CYCLS]=[0x0000000003C1][       480 µs]       [SP]=[0x0000|0    |0     ]           [*]INTE |
```

The best possible performance for repeated addition is a multiplier of zero.  When that's the case, the timings are 332 µs versus 466 µs.  The loop in `mltalg1` consists of fewer cycles than the `mltalg2` loop, so for lower multiplier values it will be more optimal.  This suggests a combined algorithm (`mltalg3`) that:

- selects the smallest of the pair to be the multiplier,
- and opts for repeated addition when the chosen multiplier is below some threshold.

The [benchmark.sh](examples/multiply/benchmark.sh) script automates the test of every combination of multiplicand and multiplier:  the results for a multiplier threshold of 16, for example:

```
$ ./benchmark.sh 16 > benchmark-16.log
$ cat benchmark-16.log | awk -F , '{cyc+=$4;s+=$5;n++;}END{printf("%g cyc, %g µs\n",cyc/n,s/n);}'
971.583 cyc, 485.786 µs
```

Producing an entire series of averages:

```
$ cat benchmark-19.log| awk -F , '{cyc+=$4;s+=$5;n++;}END{printf("%g cyc, %g µs\n",cyc/n,s/n);}'
968.355 cyc, 484.164 µs
$ cat benchmark-20.log| awk -F , '{cyc+=$4;s+=$5;n++;}END{printf("%g cyc, %g µs\n",cyc/n,s/n);}'
965.073 cyc, 482.535 µs
$ cat benchmark-21.log| awk -F , '{cyc+=$4;s+=$5;n++;}END{printf("%g cyc, %g µs\n",cyc/n,s/n);}'
964.99 cyc, 482.491 µs
$ cat benchmark-22.log| awk -F , '{cyc+=$4;s+=$5;n++;}END{printf("%g cyc, %g µs\n",cyc/n,s/n);}'
967.815 cyc, 483.908 µs
$ cat benchmark-23.log| awk -F , '{cyc+=$4;s+=$5;n++;}END{printf("%g cyc, %g µs\n",cyc/n,s/n);}'
967.676 cyc, 483.852 µs
$ cat benchmark-24.log| awk -F , '{cyc+=$4;s+=$5;n++;}END{printf("%g cyc, %g µs\n",cyc/n,s/n);}'
969.779 cyc, 484.889 µs
$ cat benchmark-25.log| awk -F , '{cyc+=$4;s+=$5;n++;}END{printf("%g cyc, %g µs\n",cyc/n,s/n);}'
971.298 cyc, 485.634 µs
$ cat benchmark-26.log| awk -F , '{cyc+=$4;s+=$5;n++;}END{printf("%g cyc, %g µs\n",cyc/n,s/n);}'
968.869 cyc, 484.424 µs
$ cat benchmark-27.log| awk -F , '{cyc+=$4;s+=$5;n++;}END{printf("%g cyc, %g µs\n",cyc/n,s/n);}'
967.885 cyc, 483.934 µs
$ cat benchmark-28.log| awk -F , '{cyc+=$4;s+=$5;n++;}END{printf("%g cyc, %g µs\n",cyc/n,s/n);}'
970.521 cyc, 485.249 µs
$ cat benchmark-32.log| awk -F , '{cyc+=$4;s+=$5;n++;}END{printf("%g cyc, %g µs\n",cyc/n,s/n);}'
977.598 cyc, 488.796 µs
```

With the data [graphed](examples/multiply/multiply-threshold.png), a threshold of 21 appears to have the best overall performance.

### More-complex timer usage

A [more-complex example](examples/timers/timers.asm) sets the timer to auto-reenable and uses a `RST 1` opcode to execute a subroutine when the timer elapses.  It allows for three timer intervals and then exits:

```
$ time ./i8e8e -L timers.bin@0x400 -T 0xDF00 -t i-all -P 0x400 --2mhz

Please wait for three timer intervals to elapse...
12000.5 ms has elapsed
12000.5 ms has elapsed
12000.5 ms has elapsed

 ____________________________________________________________________________________________
| [B]=[0x00|0  |0   | ] [C]=[0x00|0  |0   | ]   [BC]=[0x0000|0    |0     ]         A       C |
| [D]=[0x00|0  |0   | ] [E]=[0x00|0  |0   | ]   [DE]=[0x0000|0    |0     ]   S Z - C - P - Y |
| [H]=[0x04|4  |4   | ] [L]=[0x74|116|116 |t]   [HL]=[0x0474|1140 |1140  ]   =============== |
| [F]=[0x40|64 |64  |@] [A]=[0x00|0  |0   | ]  [PSW]=[0x4000|16384|16384 ]   0 1 0 0 0 0 1 0 |
|                                               [PC]=[0x0440|1088 |1088  ]                   |
| [CYCLS]=[0x00000440E08D][     35680 ms]       [SP]=[0x0000|0    |0     ]           [*]INTE |
|____________________________________________________________________________________________|
I8080Device[$00] [←0x00000000|0x00000079→] BYTES  "tty"
I8080Device[$FF] [           |0x00000000→] BYTES  "stderr"
I8080Mem[$0000..$00FF] : allocated RAM
I8080Mem[$0100..$03FF] : unused
I8080Mem[$0400..$04FF] : allocated RAM
I8080Mem[$0500..$DEFF] : unused
I8080Mem[$DF00..$DFFF] : mapper 0x6000020c8058 "realtime-timers"
I8080Mem[$E000..$FEFF] : unused
I8080Mem[$FF00..$FFFF] : allocated RAM
I8080Timer:
  [$00] |- 0x15 (21)
  [$01] |- 0x0F (15)
  [$02] |- 0x00 (0)
  [$03] |- 0x11 (17)
  [$04] |- 0x05 (5)
  [$05] |- 0x1A (26)
  [$06] |- 0x14 (20)
  [$07] |- 0x02 (2)
        |- TIMERS:
  [$10]     |- 0: unused
  [$18]     |- 1: unused
  [$20]     |- 2: 12000.500 ms, auto-reenable (opcode: 0xCF)
  [$28]     |- 3: unused
  [$30]     |- 4: unused
  [$38]     |- 5: unused
  [$40]     |- 6: unused
  [$48]     |- 7: unused

./i8e8e -L timers.bin@0x400 -T 0xDF00 -t i-all -P 0x400 --2mhz  2.00s user 7.47s system 26% cpu 36.013 total
```

The command line shows the `--2mhz` flag's being used to limit the emulator to the 2 MHz clock speed of which the 8080 was capable.  The total elapsed time of 36 seconds matches well with the 35680 ms of emulated CPU time.

One caveat to emulation of interrupts is when an `IN #` instruction is blocking, waiting on i/o.  If a timer elapses and fires while the i/o is blocking the interrupt will be raised but will wait until the currently-executing `IN #` instruction completes.  This didn't occur to me until I at first put a "please press any key" pause in the program and didn't understand why interrupts weren't being processed — until I *did* press a key!
