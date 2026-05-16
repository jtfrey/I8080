
OBJECTS		= multiply.bin

default: $(OBJECTS)

clean::
	$(RM) $(OBJECTS)

run: $(OBJECTS)
	./i8e8e -L $(OBJECTS)@0x400 -P 0x400

multiply.bin: multiply.asm params.inc

%.bin: %.asm
	asm8080 $<

