
OBJECTS		= rng.bin

default: $(OBJECTS)

clean::
	$(RM) $(OBJECTS)

run: $(OBJECTS)
	./i8e8e -L $(OBJECTS)@0x400 -f /dev/urandom:i#10 -P 0x400

%.bin: %.asm
	asm8080 $<

