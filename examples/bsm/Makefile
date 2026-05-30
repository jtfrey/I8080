#
#
#

AS		= /opt/AS/1.42.306/bin/asl
ASFLAGS		= -cpu 8080 -L -v 

LD		= /opt/AS/1.42.306/bin/p2bin
LDFLAGS		=

default: bsm-demo.bin

bsm-demo.bin: bsm-demo.asm

clean::
	$(RM) *.p *.lst *.bin

%.p: %.asm
	$(AS) $(ASFLAGS) -o $@ -olist $*.lst $<

%.bin: %.p
	$(LD) $(LDFLAGS) -o $@ $<


