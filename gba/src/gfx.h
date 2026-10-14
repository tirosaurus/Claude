// Gráficos: sprites (OAM y VRAM de OBJ), paletas con tinte, fundidos y fusiones.
#pragma once
#include "core.h"
#include "gfxdata.h"
#include "../build/gen/gfx.h"

// ------------------------------------------------------------------ paletas (sombra en RAM, se copia en VBlank)
extern u16 g_pal[512];                       // 0..255 BG, 256..511 OBJ
void bgpal_load(int slot, const u16* pal16, int world);     // world: se tiñe con el tinte del mapa
void bgpal_load_n(int slot, const u16* pals, int n, int world);
void world_tint(int r, int g, int b);        // 0..256 por canal (256 = sin tinte)
void world_tint_get(int* r, int* g, int* b);
void world_tint_tween(int r, int g, int b, int frames);   // asíncrono
int world_tint_busy(void);

// ------------------------------------------------------------------ VRAM de OBJ (1024 tiles en bloques de 8)
int objt_alloc(int ntiles);                  // índice de tile o -1
void objt_free(int tile);
void objt_reset(void);
static inline volatile u32* objt_ptr(int tile) { return OBJ_TILES + tile * 8; }

// paletas de OBJ con recuento de referencias; tint 0..256 por canal; world = afectada por el tinte del mapa
int objpal_get(const u16* pal16, int tr, int tg, int tb, int world);
void objpal_put(int slot);
void objpal_reset(void);

// ------------------------------------------------------------------ OAM
// key: menor = delante. Capas: 0x0000xxxx interfaz, 0x1000xxxx efectos, 0x2000xxxx mundo
void oam_begin(void);
void oam_push(u32 key, u16 a0, u16 a1, u16 a2);
int oam_affine(int pa, int pb, int pc, int pd);  // matriz 8.8 -> índice (o -1)
void oam_end(void);                          // ordena y deja la sombra lista para VBlank
// atajos para OBJ normales
enum { OBJ_SQUARE = 0, OBJ_WIDE = 1, OBJ_TALL = 2 };
static inline u16 obj_a0(int y, int shape, int semi) { return (y & 0xFF) | (shape << 14) | (semi ? 0x0400 : 0); }
static inline u16 obj_a1(int x, int size, int hflip) { return (x & 0x1FF) | (size << 14) | (hflip ? 0x1000 : 0); }
static inline u16 obj_a2(int tile, int prio, int pal) { return (tile & 0x3FF) | (prio << 10) | (pal << 12); }

// ------------------------------------------------------------------ registros en sombra (se aplican en VBlank)
typedef struct {
	u16 dispcnt;
	u16 bgcnt[4];
	s16 hofs[4], vofs[4];
	u16 bldcnt, bldalpha, bldy;
	u16 win0h, win0v, winin, winout;
} VideoRegs;
extern VideoRegs g_vid;

// ------------------------------------------------------------------ fundidos (como Transition.gd) y atenuación
extern u8 g_trans_busy;
void fade_out(float dur);                    // a oscuro (bloquea la tarea)
void fade_in(float dur);
void fade_set(int level16, int white);       // nivel inmediato 0..16
int fade_level(void);
void dim_set(int level16);                   // atenúa el mundo (BG1-3 y OBJ) bajo la interfaz (menús)
void alpha_set(int eva, int evb);            // OBJ semitransparentes sobre el mundo
void flash_white(float t);                   // destello blanco breve (asíncrono)
void screen_flash(int r, int g, int b, int a16, float t);   // destello de color (aprox. con brillo)

void gfx_init(void);
void gfx_vblank(void);
extern void (*gfx_vblank_hook)(void);        // la escena activa puede subir tiles aquí
