#include "gfx.h"

u16 g_pal[512];
static u16 base_pal[512] EWRAM_BSS;
static u8 pal_world[32];          // por paleta (16 BG + 16 OBJ): ¿se tiñe?
static int tint_r = 256, tint_g = 256, tint_b = 256;
VideoRegs g_vid;
void (*gfx_vblank_hook)(void);
u8 g_trans_busy;

// ------------------------------------------------------------------ paletas
static inline u16 tint_color(u16 c, int r, int g, int b) {
	int cr = ((c & 31) * r) >> 8, cg = (((c >> 5) & 31) * g) >> 8, cb = (((c >> 10) & 31) * b) >> 8;
	if (cr > 31) cr = 31;
	if (cg > 31) cg = 31;
	if (cb > 31) cb = 31;
	return cr | (cg << 5) | (cb << 10);
}

static void apply_pal(int slot) {
	u16* dst = g_pal + slot * 16;
	const u16* src = base_pal + slot * 16;
	if (pal_world[slot] && (tint_r != 256 || tint_g != 256 || tint_b != 256)) {
		for (int i = 0; i < 16; ++i) dst[i] = tint_color(src[i], tint_r, tint_g, tint_b);
	} else {
		for (int i = 0; i < 16; ++i) dst[i] = src[i];
	}
}

void bgpal_load(int slot, const u16* pal16, int world) {
	memcpy(base_pal + slot * 16, pal16, 32);
	pal_world[slot] = world;
	apply_pal(slot);
}

void bgpal_load_n(int slot, const u16* pals, int n, int world) {
	for (int i = 0; i < n; ++i) bgpal_load(slot + i, pals + i * 16, world);
}

void world_tint(int r, int g, int b) {
	tint_r = r; tint_g = g; tint_b = b;
	for (int s = 0; s < 32; ++s) if (pal_world[s]) apply_pal(s);
}

void world_tint_get(int* r, int* g, int* b) { *r = tint_r; *g = tint_g; *b = tint_b; }

static struct { int from[3], to[3], t, n; } ttw;
void world_tint_tween(int r, int g, int b, int frames) {
	ttw.from[0] = tint_r; ttw.from[1] = tint_g; ttw.from[2] = tint_b;
	ttw.to[0] = r; ttw.to[1] = g; ttw.to[2] = b;
	ttw.t = 0; ttw.n = frames < 1 ? 1 : frames;
}
int world_tint_busy(void) { return ttw.t < ttw.n; }
static void tint_step(void) {
	if (ttw.t >= ttw.n) return;
	ttw.t++;
	int k = ttw.t * 256 / ttw.n;
	world_tint(ttw.from[0] + ((ttw.to[0] - ttw.from[0]) * k >> 8), ttw.from[1] + ((ttw.to[1] - ttw.from[1]) * k >> 8),
		ttw.from[2] + ((ttw.to[2] - ttw.from[2]) * k >> 8));
}

// ------------------------------------------------------------------ VRAM de OBJ: bloques de 8 tiles
#define OBJ_BLOCKS 128
static u8 blk_used[OBJ_BLOCKS];      // 0 libre; n = inicio de reserva de n bloques
static u8 blk_owner[OBJ_BLOCKS];
void objt_reset(void) { memset(blk_used, 0, sizeof blk_used); memset(blk_owner, 0, sizeof blk_owner); }
int objt_alloc(int ntiles) {
	int nb = (ntiles + 7) >> 3;
	if (nb <= 0) nb = 1;
	for (int i = 0; i + nb <= OBJ_BLOCKS; ++i) {
		int ok = 1;
		for (int j = 0; j < nb; ++j) if (blk_owner[i + j]) { ok = 0; i += j; break; }
		if (!ok) continue;
		for (int j = 0; j < nb; ++j) blk_owner[i + j] = 1;
		blk_used[i] = nb;
		return i * 8;
	}
	return -1;
}
void objt_free(int tile) {
	if (tile < 0) return;
	int b = tile >> 3;
	int nb = blk_used[b];
	for (int j = 0; j < nb; ++j) blk_owner[b + j] = 0;
	blk_used[b] = 0;
}

// ------------------------------------------------------------------ paletas de OBJ
typedef struct { const u16* src; s16 tr, tg, tb; u8 world, refs; } ObjPal;
static ObjPal opal[16];
void objpal_reset(void) { memset(opal, 0, sizeof opal); for (int i = 16; i < 32; ++i) pal_world[i] = 0; }
int objpal_get(const u16* pal16, int tr, int tg, int tb, int world) {
	int freei = -1;
	for (int i = 0; i < 16; ++i) {
		ObjPal* p = &opal[i];
		if (p->refs && p->src == pal16 && p->tr == tr && p->tg == tg && p->tb == tb && p->world == world) { p->refs++; return i; }
		if (!p->refs && freei < 0) freei = i;
	}
	if (freei < 0) return 15;
	ObjPal* p = &opal[freei];
	p->src = pal16; p->tr = tr; p->tg = tg; p->tb = tb; p->world = world; p->refs = 1;
	u16 tmp[16];
	for (int i = 0; i < 16; ++i) tmp[i] = (tr == 256 && tg == 256 && tb == 256) ? pal16[i] : tint_color(pal16[i], tr, tg, tb);
	memcpy(base_pal + 256 + freei * 16, tmp, 32);
	pal_world[16 + freei] = world;
	apply_pal(16 + freei);
	return freei;
}
void objpal_put(int slot) {
	if (slot >= 0 && slot < 16 && opal[slot].refs) opal[slot].refs--;
}

// ------------------------------------------------------------------ OAM
typedef struct { u32 key; u16 a0, a1, a2; } OamEnt;
static OamEnt oent[128] EWRAM_BSS;
static int noent, naff;
static u16 oam_shadow[512] __attribute__((aligned(4)));
static u16 aff[32][4];
void oam_begin(void) { noent = 0; naff = 0; }
void oam_push(u32 key, u16 a0, u16 a1, u16 a2) {
	if (noent >= 128) return;
	OamEnt* e = &oent[noent++];
	e->key = key; e->a0 = a0; e->a1 = a1; e->a2 = a2;
}
int oam_affine(int pa, int pb, int pc, int pd) {
	if (naff >= 32) return -1;
	aff[naff][0] = pa; aff[naff][1] = pb; aff[naff][2] = pc; aff[naff][3] = pd;
	return naff++;
}
void oam_end(void) {
	// ordenación por inserción (pocos elementos, casi ordenados)
	for (int i = 1; i < noent; ++i) {
		OamEnt t = oent[i];
		int j = i - 1;
		while (j >= 0 && oent[j].key > t.key) { oent[j + 1] = oent[j]; --j; }
		oent[j + 1] = t;
	}
	for (int i = 0; i < 128; ++i) {
		if (i < noent) {
			oam_shadow[i * 4] = oent[i].a0;
			oam_shadow[i * 4 + 1] = oent[i].a1;
			oam_shadow[i * 4 + 2] = oent[i].a2;
		} else {
			oam_shadow[i * 4] = 0x0200;      // oculto
		}
	}
	for (int i = 0; i < 32; ++i)
		for (int k = 0; k < 4; ++k) oam_shadow[(i * 4 + k) * 4 + 3] = i < naff ? aff[i][k] : 0;
}

// ------------------------------------------------------------------ fundidos y fusiones
static int fade_lvl, fade_white, dim_lvl, alpha_eva, alpha_evb, flash_t, flash_n;
void fade_set(int level16, int white) { fade_lvl = level16; fade_white = white; }
int fade_level(void) { return fade_lvl; }
void dim_set(int level16) { dim_lvl = level16; }
void alpha_set(int eva, int evb) { alpha_eva = eva; alpha_evb = evb; }

void fade_out(float dur) {
	int n = (int)(dur * FPS);
	if (n < 1) n = 1;
	int start = fade_white ? 0 : fade_lvl;
	fade_white = 0;
	for (int i = 1; i <= n; ++i) { fade_lvl = start + (16 - start) * i / n; frame_wait(); }
	fade_lvl = 16;
}
void fade_in(float dur) {
	int n = (int)(dur * FPS);
	if (n < 1) n = 1;
	int start = fade_lvl;
	for (int i = 1; i <= n; ++i) { fade_lvl = start - start * i / n; frame_wait(); }
	fade_lvl = 0;
}
void flash_white(float t) { flash_n = (int)(t * FPS); if (flash_n < 1) flash_n = 1; flash_t = flash_n; }
void screen_flash(int r, int g, int b, int a16, float t) {
	(void)r; (void)g; (void)b;
	flash_n = (int)(t * FPS); if (flash_n < 1) flash_n = 1; flash_t = flash_n;
	(void)a16;
}

static void blend_regs(void) {
	if (fade_lvl > 0) {
		g_vid.bldcnt = 0x003F | (fade_white ? 0x80 : 0xC0);
		g_vid.bldy = fade_lvl > 16 ? 16 : fade_lvl;
	} else if (flash_t > 0) {
		g_vid.bldcnt = 0x003E | 0x80;     // aclarar mundo
		g_vid.bldy = 8 * flash_t / flash_n;
		flash_t--;
	} else if (dim_lvl > 0) {
		g_vid.bldcnt = 0x003E | 0xC0;     // BG1-3, OBJ y fondo; la interfaz (BG0) queda intacta
		g_vid.bldy = dim_lvl;
	} else {
		g_vid.bldcnt = 0x3E40;            // OBJ semitransparentes sobre BG1-3 y fondo
		g_vid.bldalpha = (alpha_eva & 31) | ((alpha_evb & 31) << 8);
		g_vid.bldy = 0;
	}
}

void gfx_init(void) {
	memset(&g_vid, 0, sizeof g_vid);
	memset(g_pal, 0, sizeof g_pal);
	memset(base_pal, 0, sizeof base_pal);
	objt_reset();
	objpal_reset();
	oam_begin();
	oam_end();
	alpha_eva = 16; alpha_evb = 0;
}

void gfx_vblank(void) {
	tint_step();
	blend_regs();
	dma_copy32(PAL_BG, g_pal, 256);
	dma_copy32(OAM, oam_shadow, 256);
	REG_DISPCNT = g_vid.dispcnt;
	volatile u16* cnt = &REG_BG0CNT;
	volatile u16* ofs = &REG_BG0HOFS;
	for (int i = 0; i < 4; ++i) { cnt[i] = g_vid.bgcnt[i]; ofs[i * 2] = g_vid.hofs[i]; ofs[i * 2 + 1] = g_vid.vofs[i]; }
	REG_BLDCNT = g_vid.bldcnt;
	REG_BLDALPHA = g_vid.bldalpha;
	REG_BLDY = g_vid.bldy;
	REG_WIN0H = g_vid.win0h; REG_WIN0V = g_vid.win0v; REG_WININ = g_vid.winin; REG_WINOUT = g_vid.winout;
	if (gfx_vblank_hook) gfx_vblank_hook();
}
