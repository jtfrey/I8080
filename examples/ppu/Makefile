
OBJECTS		= main.bin

DEPENDS		= game_ball.asm \
		  palettes.inc palettes.asm \
		  tile_tables.inc tile_tables.asm \
		  ppu.inc math.inc

default: $(OBJECTS)

clean::
	$(RM) $(OBJECTS)

run: $(OBJECTS)
	./i8e8e -L $(OBJECTS)@0x400 -P 0x400

main.bin: main.asm $(DEPENDS)

%.bin: %.asm
	asm8080 $< -l$*.lst

