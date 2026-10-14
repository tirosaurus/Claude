// Probador headless de ROMs de GBA usando el núcleo de mGBA.
// Uso: gbarun rom.gba guion.txt salida_prefijo
// guion: líneas "F TECLAS" (frame inicial, teclas mantenidas: A B S T R L U D r l, '-' = ninguna)
//        y "F shot nombre" para guardar captura PNG en ese frame. "F end" termina.
#include <mgba/core/core.h>
#include <mgba/core/config.h>
#include <png.h>
#include <stdio.h>
// Bits del registro KEYINPUT de la GBA
enum { K_A = 0, K_B, K_SELECT, K_START, K_RIGHT, K_LEFT, K_UP, K_DOWN, K_R, K_L };
#include <stdlib.h>
#include <string.h>

static void save_png(const char* path, const uint32_t* buf, int w, int h) {
	FILE* f = fopen(path, "wb");
	if (!f) return;
	png_structp p = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	png_infop info = png_create_info_struct(p);
	png_init_io(p, f);
	png_set_IHDR(p, info, w, h, 8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
	png_write_info(p, info);
	unsigned char* row = malloc(w * 3);
	for (int y = 0; y < h; ++y) {
		for (int x = 0; x < w; ++x) {
			uint32_t c = buf[y * w + x];
			row[x * 3 + 0] = c & 0xFF;
			row[x * 3 + 1] = (c >> 8) & 0xFF;
			row[x * 3 + 2] = (c >> 16) & 0xFF;
		}
		png_write_row(p, row);
	}
	png_write_end(p, NULL);
	png_destroy_write_struct(&p, &info);
	free(row);
	fclose(f);
}

static int keymask(const char* s) {
	int m = 0;
	for (; *s; ++s) {
		switch (*s) {
		case 'A': m |= 1 << K_A; break;
		case 'B': m |= 1 << K_B; break;
		case 'S': m |= 1 << K_SELECT; break;
		case 'T': m |= 1 << K_START; break;
		case 'R': m |= 1 << K_RIGHT; break;
		case 'L': m |= 1 << K_LEFT; break;
		case 'U': m |= 1 << K_UP; break;
		case 'D': m |= 1 << K_DOWN; break;
		case 'r': m |= 1 << K_R; break;
		case 'l': m |= 1 << K_L; break;
		}
	}
	return m;
}

int main(int argc, char** argv) {
	if (argc < 4) { fprintf(stderr, "uso: gbarun rom guion prefijo\n"); return 1; }
	struct mCore* core = mCoreFind(argv[1]);
	if (!core) { fprintf(stderr, "no se reconoce la ROM\n"); return 1; }
	core->init(core);
	mCoreInitConfig(core, NULL);
	unsigned w, h;
	core->desiredVideoDimensions(core, &w, &h);
	uint32_t* buf = calloc(w * h, 4);
	core->setVideoBuffer(core, (color_t*) buf, w);
	if (!mCoreLoadFile(core, argv[1])) { fprintf(stderr, "no se pudo cargar\n"); return 1; }
	core->reset(core);
	FILE* g = fopen(argv[2], "r");
	char line[256];
	int frame = 0, keys = 0;
	while (fgets(line, sizeof line, g)) {
		int f; char a[64] = "", b[128] = "";
		if (sscanf(line, "%d %63s %127s", &f, a, b) < 2) continue;
		while (frame < f) { core->setKeys(core, keys); core->runFrame(core); ++frame; }
		if (!strcmp(a, "shot")) {
			char path[512]; snprintf(path, sizeof path, "%s%s.png", argv[3], b);
			save_png(path, buf, w, h);
			printf("captura %s (frame %d)\n", path, frame);
		} else if (!strcmp(a, "end")) {
			break;
		} else {
			keys = strcmp(a, "-") ? keymask(a) : 0;
		}
	}
	fclose(g);
	core->deinit(core);
	return 0;
}
