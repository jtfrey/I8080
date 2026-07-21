AS		= asl
ASFLAGS		= -cpu 8080 -L -v 

LD		= p2bin
LDFLAGS		=

default: engine.bin

clean::
	$(RM) *.p *.lst *.bin

engine.p: engine.asm apu.inc timers.inc museq.asm museq.inc soundtrack.asm

%.p: %.asm
	$(AS) $(ASFLAGS) -o $@ -olist $*.lst $<

%.bin: %.p
	$(LD) $(LDFLAGS) -o $@ $<

debug: engine.bin
	./i8e8e -L $<@0x100 -P 0x100 -2A --apu=0xF000 --timers=0xF800 --disasm=run.d

run: engine.bin
	./i8e8e -L $<@0x100 -P 0x100 -2A --apu=0xF000 --timers=0xF800

