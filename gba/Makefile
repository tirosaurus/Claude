# Vaelmoor GBA. Requiere arm-none-eabi-gcc y python3 (Pillow, numpy).
CC      := arm-none-eabi-gcc
OBJCOPY := arm-none-eabi-objcopy
GBAFIX  ?= build/gbafix
ARCH    := -mcpu=arm7tdmi -mthumb -mthumb-interwork
CFLAGS  := $(ARCH) -O2 -ffreestanding -fno-strict-aliasing -Wall -Wno-unused-function
LDFLAGS := $(ARCH) -nostartfiles -T gba.ld -Wl,--gc-sections

SRC := src/main.c src/map.c src/text.c src/story.c src/battle.c
GEN := build/gen/maps.c build/gen/sprites.c build/gen/images.c build/gen/font.c
OBJ := $(SRC:src/%.c=build/obj/%.o) $(GEN:build/gen/%.c=build/obj/gen_%.o) build/obj/crt0.o

Vaelmoor.gba: build/vaelmoor.elf $(GBAFIX)
	$(OBJCOPY) -O binary $< $@
	$(GBAFIX) $@ -tVAELMOOR -cVAEL -m01

build/vaelmoor.elf: $(OBJ) gba.ld
	$(CC) $(LDFLAGS) -o $@ $(OBJ) -lc -lgcc -lnosys

build/gen/maps.c: tools/conv.py
	python3 tools/conv.py

build/obj/%.o: src/%.c src/*.h build/gen/maps.c
	@mkdir -p build/obj
	$(CC) $(CFLAGS) -c $< -o $@

build/obj/gen_%.o: build/gen/%.c
	@mkdir -p build/obj
	$(CC) $(CFLAGS) -c $< -o $@

build/obj/crt0.o: src/crt0.s
	@mkdir -p build/obj
	$(CC) $(ARCH) -c $< -o $@

build/gbafix: tools/gbafix.c
	@mkdir -p build
	gcc -O2 -o $@ $<

clean:
	rm -rf build/obj build/vaelmoor.elf Vaelmoor.gba
.PHONY: clean
