# Vaelmoor GBA. Requiere arm-none-eabi-gcc (con newlib) y python3 (Pillow, numpy, scipy, soundfile).
CC      := arm-none-eabi-gcc
OBJCOPY := arm-none-eabi-objcopy
GBAFIX  ?= build/gbafix
ARCH    := -mcpu=arm7tdmi -mthumb -mthumb-interwork
CFLAGS  := $(ARCH) $(DEFS) -O2 -ffreestanding -fno-strict-aliasing -Wall -Wno-unused-function -Wno-format-truncation -Wno-missing-braces -Wno-maybe-uninitialized -fdata-sections -ffunction-sections
LDFLAGS := $(ARCH) -nostartfiles -T gba.ld -Wl,--gc-sections -Wl,-Map=build/map.txt

SRC := $(wildcard src/*.c)
GENC := build/gen/db.c build/gen/gfx.c build/gen/audio_data.c build/gen/maps_index.c $(wildcard build/gen/map_*.c)
OBJ := $(SRC:src/%.c=build/obj/%.o) $(GENC:build/gen/%.c=build/obj/gen_%.o) build/obj/crt0.o build/obj/ctx.o build/obj/blobs.o

Vaelmoor.gba: build/vaelmoor.elf $(GBAFIX)
	$(OBJCOPY) -O binary $< $@
	$(GBAFIX) $@ -tVAELMOOR -cVAEL -m01 -r0
	@ls -l $@

build/vaelmoor.elf: $(OBJ) gba.ld
	$(CC) $(LDFLAGS) -o $@ $(OBJ) -lc -lgcc -lnosys -lm

assets:
	cd tools && python3 conv_db.py && python3 conv_gfx.py && python3 conv_audio.py && python3 conv_maps.py

build/obj/%.o: src/%.c src/*.h
	@mkdir -p build/obj
	$(CC) $(CFLAGS) -c $< -o $@

build/obj/gen_%.o: build/gen/%.c
	@mkdir -p build/obj
	$(CC) $(CFLAGS) -c $< -o $@

build/obj/crt0.o: src/crt0.s
	@mkdir -p build/obj
	$(CC) $(ARCH) -c $< -o $@

build/obj/ctx.o: src/ctx.s
	@mkdir -p build/obj
	$(CC) $(ARCH) -c $< -o $@

build/obj/blobs.o: build/gen/blobs.s $(wildcard build/gen/bin/*.bin)
	@mkdir -p build/obj
	cd build/gen && $(CC) $(ARCH) -c blobs.s -o ../obj/blobs.o

build/gbafix: tools/gbafix.c
	@mkdir -p build
	gcc -O2 -o $@ $<

clean:
	rm -rf build/obj build/vaelmoor.elf Vaelmoor.gba
.PHONY: clean assets
