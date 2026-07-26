# i8080mon

In the late 1970's Steve Wozniak created the Apple I computer.  In order to examine the memory address space and write bytes into it (notably, machine code) he wrote a program to fit into the last page that today is affectionately known as [wozmon](https://github.com/jefftranter/6502/blob/master/asm/wozmon/wozmon.s).

The *i8080mon* code is inspired by wozmon and implements a system monitor for the Intel 8080 *i8e8e* emulator.  The code is loaded into the first three pages of memory from `$0000` up to `$0300` and includes code for the `RST 0` vector the system boots to by default.  Where wozmon used a few zero-page addresses for variables and page 2 for the input buffer (page 1 is the fixed location of the stack), i8080mon has a stack situated at the top of the 64 KiB address space and situates its input buffer in the zero page following the reset vectors:

| Address range   | Use                            |
| :-------------- | :----------------------------- |
| `$0000 - $003F` | Reset vectors                  |
| `$0040`         | Monitor mode bitmask           |
| `$0041 - $0042` | Monitor parsed address slot 0  |
| `$0043 - $0044` | Monitor parsed address slot 1  |
| `$0045`         | Monitor saved character slot   |
| `$0046 - $00FF` | Monitor input buffer           |
| `$0100 - $02FF` | Monitor program (plus padding) |

The input buffer has a capacity of `$BA` (186) characters.

## Commands

The *i8080mon* displays an angle bracket (greater-than sign) as a prompt and fills its input buffer up to the capacity; when it reaches capacity or a newline character is received, the line is parsed.

All addresses must be in hexadecimal format with no leading prefixes; leading zeroes can be omitted.

Byte sequences to be set in memory must be in hexadecimal format with no leading prefixes.  A sequence of *N* continguous hexadecimal characters will be parsed as *N/2* bytes; if an odd number of characters are present, the final character will be parsed as the low nibble of a byte.  Non-matching characters can be used to separate sequences.  So the sequence:

```
01234 A B C DEF
```

would be parsed as the bytes

```
01 23 04 0A 0B 0C DE F0
```

### Print memory location

An bare address displays the byte at that memory address:

```
> FF00
FF00:00
> 0
0000:F3
> 100
0100:3E
```

The monitor will retain the address as it's last-parsed address.

### Print memory range

Two addresses separated by a dot displays the bytes in that range of memory addresses:

```
> 0.8
0000:F3 21 00 00 F9 C3 00 01
> 0100.120
0100:3E 3E D3 00 3E 20 D3 00
0108:21 46 00 0E BA DB 00 FE
0110:5F C2 1E 01 79 B7 CA 0D
0118:01 2B 0C C3 0D 01 FE 0A
```

The monitor will retain the starting address as it's last-parsed address.

### Set bytes in memory

A starting address followed by a colon is used to set a sequence of bytes in memory:

```
> FF00:0123456789ABCDEF
> FF08:FF EE DD C B A
```

The monitor will retain the starting address of the last set command as it's last-parsed address.  Subsequently displaying that range of memory shows the bytes that were set:

```
> FF00.FF10
FF00:01 23 45 67 89 AB CD EF
FF08:FF EE DD 0C 0B 0A 00 00
```

### Run code

Assuming machine code has been entered into memory (using set bytes commands), the program counter can be set and the program executed:

```
> 0100R
```

Since the *i8080mon* code is at `$0100` this command jumps back into the monitor entry point.  A full reset could be effected with:

```
> 0R
```

In the sections above it was noted that the print and set commands will have a last-parsed address retained by the monitor:  it is this address that will be set in the program counter if the address is omitted from the `R` command:

```
> 4000
4000:76
> R
{HALT}
```

Opcode `$76` at `$4000` in this session is `HLT`, and the bare `R` command executes the instruction at `$4000` and halts the machine.

### Halt the machine

An addition not contained in wozmon is the `X` command to exit the monitor and halt the system.

```
> X
{HALT}
```
