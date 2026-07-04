AS		= asl
ASFLAGS		= -cpu 8080 -L -v 

LD		= p2bin
LDFLAGS		=

default: apu-demo.bin

apu-demo.bin: apu-demo.p
	$(LD) -o $@ $^

clean::
	$(RM) *.p *.lst *.bin

apu-demo.p: apu-demo.asm apu.inc glass.dmc

%.p: %.asm
	$(AS) $(ASFLAGS) -o $@ -olist $*.lst $<

%.bin: %.p
	$(LD) $(LDFLAGS) -o $@ $<

run: apu-demo.bin
	./i8e8e -L apu-demo.bin@0x400 -P 0x400 -2A --apu=0x4000

%.dmc: %.raw
	python3 8bit_to_dpcm.py -o $@ $<

