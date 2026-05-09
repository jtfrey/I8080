# I8080

One night while browsing the Internet regarding assembly language a page was among the search results that cited the Intel 8080 processor, from circa 1976.  It was a programmer's guide of the day — a paper manual that had clearly been scanned into a PDF.

Compared to the ARM64 ISA I was learning at the time, the 8080 was so simple, yet slightly more sophisticated than the 6502.  It had six general purpose registers that could be accessed separately or in 16-bit pairs.  The stack could be located anywhere in memory and was not limited to a single 256-byte page.  All branching instructions (jump, call, return) were conditional, taken only if the referenced status flag was set.

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

The 8080 could be used with a variety of memory technologies:  RAM, ROM, etc.  Addresses were 16-bit, for a maximum of 65 KiB of directly-addressable memory.

It's tempting on a modern system to just use an array of 65536 bytes for this piece.  My demo program was a meager 59 bytes, so that would be massive overkill.  I could also imagine having a standard subroutine ROM that I'd like to map to a section of the address space, and having it be mutable would be very problematic!

The `I8080Mem` pseudo-class splits the system memory into 256-byte pages.  The object includes a 256-element array of page-pointers that starts empty.  Only when a byte is written to a page will that page be allocated.  A simple external callback mechanism allows interception of read/write calls.  A ROM can then be implemented as:

- A read callback that accesses the external ROM image, not the memory pages themselves
- A write callback that is a no-op

The callbacks indicate to `I8080Mem` whether or not they handled the operation; if not, `I8080Mem` can proceed as normal.  Another application is _mirroring_, where multiple ranges of address space all access the same underlying memory (the NES did a lot of this with the 6502).  The read and write callbacks have the ability to modify the address and pass control back to `I8080Mem`, effectively filtering the address for operations.

The pseudo-class includes helper functions to move chunks of data into and out of the memory space.  This is useful for loading code and data, or making core dumps for post-mortem on failing code.

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

Then again, there are only 244 opcodes in the 8080 ISA, and an array of that many pointers is just under 2 KiB.  A linked list of `I8080InstrHandler_t` records can be _compiled_ into a `I8080InstrTable_t` that associates an array index with a dispatch function point.  At runtime, rather than walking the linked list to match an opcode to a function, the opcode is used as an index into the table:  much faster.  Another important benefit of compiling the `I8080InstrHandler_t` linked list is that collisions can be detected:  if two `I8080InstrHandler_t` records respond to the same opcode, that will be detected.

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

## The demo program

The demo program has the following 8080 assembly program contained within it:

```
                                ORG     $1000
                                    
1000:		0x3E  0x2C          MVI     A  #$2C     ; A <= $2C
1002:		0x27                DAA                 ; adjust to BCD form -> $32
1003:		0xCE  0x78          ACI     #$78        ; A <= A + $78
1005:		0x27                DAA                 ; adjust to BDC form -> $110 = $10
1006:		0xFE  0x10          CPI     #$10        ; did we get the right result?
1008:		0xCA  0x0C  0x10    JZ      +1          ; hooray, keep going with this program
100B:		0x76                HLT                 ; failure, halt now

100C:		0x26  0x01          MVI     H  #$01     ; H <= $01
100E:		0x2E  0xFF          MVI     L  #$FF     ; L <= $FF
1010:		0xF9                SPHL                ; SP <= HL=$01FF
1011:		0xC5                PUSH    BC          ; push registers to the stack
1012:		0xD5                PUSH    DE
1013:		0xE5                PUSH    HL
1014:		0xF5                PUSH    PSW

LOOP:
1015:		0xDB  0x00          IN      #0          ; Read from stdin
1017:		0xFE  0xFF          CPI     #$FF        ; EOF?
1019:		0xCA  0x33  0x10    JZ      DONE        ; Yep  exit the program
101C:		0xFE  0x04          CPI     #$04        ; Ctrl-D?
101E:		0xCA  0x33  0x10    JZ      DONE        ; Yep  exit the program
1021:		0xFE  0x61          CPI     #$61        ; Character == 0x61 ('a')
1023:		0xFA  0x2E  0x10    JM      OUTPUT      ; Character < 0x61
1026:		0xFE  0x7B          CPI     #$7B        ; Character == 0x7B ('z' + 1)
1028:		0xF2  0x2E  0x10    JP      OUTPUT      ; Character >= 0x7B
102B:		0xCD  0x38  0x10    CALL    UC          ; Call uppercase subroutine

OUTPUT:
102E:		0xD3  0x0A          OUT     #10         ; Write the char to stderr
1030:		0xC3  0x15  0x10    JMP     LOOP        ; Go back for another

DONE:
1033:		0xF1                POP      PSW        ; pop registers off the stack
1034:		0xE1                POP      HL
1035:		0xD1                POP      DE
1036:		0xC1                POP      BC
1037:		0x76                HLT                 ; Terminate the program

UC:
1038:		0xEE  0x20          XRI     #$20        ; Convert to uppercase...
103A:		0xC9                RET
```

This program expects the `I8080DeviceTTY` to be configured as device 0 and the `I8080DeviceStdError` as output device 10.  It tests a few things and then enters into a loop, reading characters from stdin one at a time, converting any lowercase letters to uppercase and echoing to stderr.  When end-of-file or Ctrl-D are read, the program exits.

```
THIS IS A TEST.

01234567788979  --____+++++====


^D
 ________________________________________________________________________________________
| [B]=[0x00|0  |0   ] [C]=[0x00|0  |0   ]   [BC]=[0x0000|0    |0     ]         A       C |
| [D]=[0x00|0  |0   ] [E]=[0x00|0  |0   ]   [DE]=[0x0000|0    |0     ]   S Z - C - P - Y |
| [H]=[0x01|1  |1   ] [L]=[0xFF|255|-1  ]   [HL]=[0x01FF|511  |511   ]   =============== |
| [F]=[0x81|129|-127] [A]=[0x0A|10 |10  ]  [PSW]=[0x810A|33034|-32502]   1 0 0 0 0 0 1 1 |
|                                           [PC]=[0x1038|4152 |4152  ]                   |
| [CYCLS]=[0x000000001F12][      3977 µs]   [SP]=[0x01FF|511  |511   ]           [*]INTE |
|________________________________________________________________________________________|
I8080Device[00] [←00000033|→00000000] BYTES  "tty"
I8080Device[0A] [         |→00000032] BYTES  "stderr"
```

When the input is terminated, a final summary of the registers and devices is written to the screen.  The total number of CPU cycles (and required CPU time) are present, as well as the total bytes that moved through each i/o device.
