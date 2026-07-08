AS		= asl
ASFLAGS		= -cpu 8080 -L -v 

LD		= p2bin
LDFLAGS		=

default: synth.bin

clean::
	$(RM) *.p *.lst *.bin

synth.p: synth.asm apu.inc timers.inc

%.p: %.asm
	$(AS) $(ASFLAGS) -o $@ -olist $*.lst $<

%.bin: %.p
	$(LD) $(LDFLAGS) -o $@ $<

run: synth.bin
	./i8e8e -L $<@0x100 -P 0x100 -2A --apu=0xF000 --timers=0xF800 --disasm=run.d

