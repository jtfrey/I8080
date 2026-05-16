
OBJECTS		= kernel.bin hello_world.bin cga_demo.bin

default: $(OBJECTS)

clean::
	$(RM) $(OBJECTS)

%.bin: %.asm
	asm8080 $^

