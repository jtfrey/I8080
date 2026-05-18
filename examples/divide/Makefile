
OBJECTS		= divide.bin

default: $(OBJECTS)

clean::
	$(RM) $(OBJECTS)

run: $(OBJECTS)
	./i8e8e -L $(OBJECTS)@0x400 -P 0x400 

divide.bin: divide.asm params.inc

%.bin: %.asm
	asm8080 $< -l$*.lst

