
OBJECTS		= timers.bin

default: $(OBJECTS)

clean::
	$(RM) $(OBJECTS)

run: $(OBJECTS)
	./i8e8e -L $(OBJECTS)@0x400 -T 0xDF00 -P 0x400

%.bin: %.asm
	asm8080 $<

