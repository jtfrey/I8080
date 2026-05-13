
OBJECTS		= kernel.bin hello_world.bin

default: $(OBJECTS)

clean::
	$(RM) $(OBJECTS)

%.bin: %.asm
	asm8080 $^

