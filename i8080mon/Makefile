#
#
#

AS		= asl
ASFLAGS		= -cpu 8080 -L -v 

LD		= p2bin
LDFLAGS		=

I8E8E_OPTS	= -2A --tty i-single-char,i-disable-echo,o-clear-on-reset

default: i8080mon.bin paste.bin

clean::
	$(RM) *.p *.lst *.bin

i8080mon.p: i8080mon.asm

paste.p: paste.asm

%.p: %.asm
	$(AS) $(ASFLAGS) -o $@ -olist $*.lst $<

%.bin: %.p
	$(LD) $(LDFLAGS) -o $@ $<

run: i8080mon.bin
	./i8e8e --load=$<@0x0000 $(I8E8E_OPTS)

debug: i8080mon.bin
	./i8e8e --load=$<@0x0000 $(I8E8E_OPTS) --disasm=run.dbg --core=run.core

