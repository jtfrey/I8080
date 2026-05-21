
OBJECTS		= cga_demo.bin cga_world.bin

default: $(OBJECTS)

clean::
	$(RM) $(OBJECTS)

run: $(OBJECTS)
	./i8e8e -L $(OBJECTS)@0x400 -P 0x400

cga.bin: cga.asm

cga_world.bin: cga_world.asm cga.inc

%.bin: %.asm
	asm8080 $< -l$*.lst

