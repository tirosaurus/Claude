#include "ui.h"

// ------------------------------------------------------------------ gestión de tiles de BG0
// Celdas con contenido idéntico comparten tile (bordes y fondos de ventana); al escribir texto
// en una celda compartida se copia a un tile propio.
#define UI_TILES 512
static u16 refc[UI_TILES] EWRAM_BSS;
static u8 hashed[UI_TILES] EWRAM_BSS;
static u16 htab[1024] EWRAM_BSS;           // tabla de dispersión contenido -> tile (0 vacío)
static u16 cell_tile[32 * 20] EWRAM_BSS;
static u8 cell_pal[32 * 20] EWRAM_BSS;
// borrado diferido: la celda se vacía en el próximo volcado salvo que se vuelva a dibujar antes
static u8 stale[32 * 20] EWRAM_BSS;
static u8 any_stale;
static inline int cell_empty(int i) { return stale[i] || !cell_tile[i]; }
static int free_hint = 1;
// doble búfer: el mapa se escribe en una copia y se vuelca en el retrazado vertical; los tiles que se
// liberan o que ya están en pantalla no se reutilizan ni se reescriben hasta el siguiente volcado
static u16 sb_shadow[32 * 20] EWRAM_BSS;
static u8 sb_dirty;
static u8 held[UI_TILES] EWRAM_BSS;
static u16 held_list[UI_TILES] EWRAM_BSS;
static int nheld;
static int npend;
IWRAM_CODE void ui_commit(void);
static int last_cell = -1;
static volatile u32* last_ptr;

static inline volatile u32* tptr(int t) { return CHARBLOCK(0) + t * 8; }

IWRAM_CODE static u32 thash(const u32* d, int pal) {
	u32 h = 2166136261u ^ pal;
	for (int i = 0; i < 8; ++i) { h ^= d[i]; h *= 16777619u; }
	return h;
}

static u16 hslot[UI_TILES] EWRAM_BSS;
#define TOMB 0xFFFF
static int ntomb;
static void hash_rebuild(void);
// borrado perezoso: se deja una marca y la tabla se reconstruye cuando hay demasiadas
IWRAM_CODE static void hash_remove(int t) {
	if (!hashed[t]) return;
	htab[hslot[t]] = TOMB;
	hashed[t] = 0;
	if (++ntomb > 384) hash_rebuild();
}
static void hash_rebuild(void) {
	memset(htab, 0, sizeof htab);
	ntomb = 0;
	for (int t = 1; t < UI_TILES; ++t) {
		if (!hashed[t]) continue;
		volatile u32* p = CHARBLOCK(0) + t * 8;
		u32 d[8];
		for (int q = 0; q < 8; ++q) d[q] = p[q];
		int j = thash(d, hashed[t] - 1) & 1023;
		while (htab[j]) j = (j + 1) & 1023;
		htab[j] = t;
		hslot[t] = j;
	}
}

static void release_held(void) {
	for (int i = 0; i < nheld; ++i) {
		int t = held_list[i];
		held[t] = 0;
		if (!refc[t]) hash_remove(t);   // seguía en la tabla por si se reutilizaba este fotograma
	}
	nheld = 0;
}

// sin tiles libres: esperar al retrazado y volcar ya, para no pisar tiles que se ven en pantalla
static void resolve_stale(void);
static void flip_now(void) {
	resolve_stale();
	while (REG_VCOUNT >= 160) {}
	while (REG_VCOUNT < 160) {}
	dma_copy32(SCREENBLOCK(UI_SB), sb_shadow, sizeof sb_shadow / 4);
	sb_dirty = 0;
	release_held();
}

IWRAM_CODE static int tile_new(void) {
	for (int pass = 0; pass < 2; ++pass) {
		for (int k = 0; k < UI_TILES - 1; ++k) {
			int t = free_hint + k;
			if (t >= UI_TILES) t -= UI_TILES - 1;
			if (!refc[t] && !held[t]) {
				free_hint = t + 1 >= UI_TILES ? 1 : t + 1;
				return t;
			}
		}
		flip_now();
	}
	return 0;
}

void ui_vblank(void) {
	if (npend) ui_commit();
	resolve_stale();
	if (sb_dirty) {
		dma_copy32(SCREENBLOCK(UI_SB), sb_shadow, sizeof sb_shadow / 4);
		sb_dirty = 0;
	}
	release_held();
}

IWRAM_CODE static void tile_unref(int t) {
	last_cell = -1;
	if (t <= 0) return;
	if (refc[t] && --refc[t] == 0) {
		if (!held[t]) { held[t] = 1; held_list[nheld++] = t; }
		else if (!hashed[t]) {}
	}
}

// tile compartido para este contenido (crea si no existe)
IWRAM_CODE static int tile_shared(const u32* d, int pal) {
	u32 h = thash(d, pal);
	int idx = h & 1023;
	for (int probe = 0; probe < 1024; ++probe) {
		int i = (idx + probe) & 1023;
		int t = htab[i];
		if (!t) break;
		if (t == TOMB) continue;
		if (hashed[t] == pal + 1) {
			volatile u32* p = tptr(t);
			int same = 1;
			for (int k = 0; k < 8; ++k) if (p[k] != d[k]) { same = 0; break; }
			if (same) { refc[t]++; return t; }   // también resucita tiles retenidos con este contenido
		}
	}
	int t = tile_new();
	if (!t) return 0;
	volatile u32* p = tptr(t);
	for (int k = 0; k < 8; ++k) p[k] = d[k];
	refc[t] = 1;
	hashed[t] = pal + 1;
	for (int probe = 0; probe < 1024; ++probe) {
		int i = (idx + probe) & 1023;
		if (!htab[i] || htab[i] == TOMB) { if (htab[i] == TOMB) ntomb--; htab[i] = t; hslot[t] = i; break; }
	}
	return t;
}

IWRAM_CODE static void cell_set(int x, int y, int t, int pal) {
	last_cell = -1;
	if ((unsigned)x >= 32 || (unsigned)y >= 20) { tile_unref(t); return; }
	int i = y * 32 + x;
	stale[i] = 0;
	if (cell_tile[i] == t && cell_pal[i] == pal) { tile_unref(t); return; }   // mismo contenido: nada que hacer
	tile_unref(cell_tile[i]);
	cell_tile[i] = t;
	cell_pal[i] = pal;
	sb_shadow[i] = t | (pal << 12);
	sb_dirty = 1;
}

static void resolve_stale(void) {
	if (!any_stale) return;
	any_stale = 0;
	for (int i = 0; i < 32 * 20; ++i) if (stale[i]) cell_set(i & 31, i >> 5, 0, 0);
}

// Escritura de píxeles: se compone la celda en RAM y al terminar cada operación se busca un tile
// con el mismo contenido (o se crea). Así redibujar lo mismo no gasta tiles nuevos ni parpadea.
#define PEND 36
static u32 pend_data[PEND][8] EWRAM_BSS;
static s16 pend_cell[PEND];
static s8 pend_slot[32 * 20] EWRAM_BSS;

IWRAM_CODE void ui_commit(void) {
	int n = npend;
	npend = 0;
	last_cell = -1;
	for (int k = 0; k < n; ++k) {
		int i = pend_cell[k];
		pend_slot[i] = -1;
		int t = tile_shared(pend_data[k], cell_pal[i]);
		if (t) cell_set(i & 31, i >> 5, t, cell_pal[i]);
	}
	last_cell = -1;
}

IWRAM_CODE static volatile u32* cell_private(int x, int y) {
	int i = y * 32 + x;
	if (pend_slot[i] >= 0) return pend_data[pend_slot[i]];
	if (npend >= PEND) ui_commit();
	int k = npend++;
	pend_slot[i] = k;
	pend_cell[k] = i;
	int t = stale[i] ? 0 : cell_tile[i];
	u32* d = pend_data[k];
	if (t) { volatile u32* s = tptr(t); for (int q = 0; q < 8; ++q) d[q] = s[q]; }
	else for (int q = 0; q < 8; ++q) d[q] = 0;
	return d;
}

int ui_tiles_free(void) {
	int n = 0;
	for (int t = 1; t < UI_TILES; ++t) if (!refc[t]) n++;
	return n;
}

void ui_load_palettes(int battle) {
	bgpal_load(PAL_UI_LIGHT, uipal_light, 0);
	bgpal_load(PAL_UI_DARK, uipal_dark, 0);
	bgpal_load(PAL_UI_ALT, battle ? uipal_win : uipal_btn, 0);
	if (battle) bgpal_load(PAL_UI_ALT2, uipal_winred, 0);
}

void ui_init(void) {
	memset(refc, 0, sizeof refc);
	memset(hashed, 0, sizeof hashed);
	memset(htab, 0, sizeof htab);
	ntomb = 0;
	memset(cell_tile, 0, sizeof cell_tile);
	memset(cell_pal, 0, sizeof cell_pal);
	for (int i = 0; i < 32 * 32; ++i) SCREENBLOCK(UI_SB)[i] = 0;
	memset(sb_shadow, 0, sizeof sb_shadow);
	memset(pend_slot, -1, sizeof pend_slot);
	memset(stale, 0, sizeof stale);
	any_stale = 0;
	npend = 0;
	memset(held, 0, sizeof held);
	nheld = 0;
	sb_dirty = 0;
	for (int k = 0; k < 8; ++k) tptr(0)[k] = 0;
	refc[0] = 0xFFFF;
	free_hint = 1;
	g_vid.bgcnt[0] = (0 << 2) | (UI_SB << 8) | 0;
	g_vid.hofs[0] = g_vid.vofs[0] = 0;
	ui_load_palettes(0);
}

void ui_clear_rect(int tx, int ty, int tw, int th) {
	ui_commit();
	for (int y = ty; y < ty + th; ++y)
		for (int x = tx; x < tx + tw; ++x)
			if ((unsigned)x < 32 && (unsigned)y < 20) {
				int i = y * 32 + x;
				if (cell_tile[i]) { stale[i] = 1; any_stale = 1; }
			}
}

void ui_clear(void) { ui_clear_rect(0, 0, 32, 20); }

// ------------------------------------------------------------------ ventanas
// Godot dibuja los NinePatchRect estirando bordes y centro; aquí igual, a nivel de píxel.
static int np_src(int p, int size, int srcsize, int margin) {
	if (p < margin) return p;
	if (p >= size - margin) return srcsize - (size - p);
	int inner = size - 2 * margin, sinner = srcsize - 2 * margin;
	if (inner <= 0 || sinner <= 0) return margin;
	return margin + (p - margin) * sinner / inner;
}

// tabla de np_src sin divisiones por píxel
static void np_table(u8* out, int size, int srcsize, int margin) {
	if (size > 256) size = 256;
	int inner = size - 2 * margin, sinner = srcsize - 2 * margin;
	int acc = 0, v = margin;
	for (int p = 0; p < size; ++p) {
		if (p < margin) out[p] = p;
		else if (p >= size - margin) out[p] = srcsize - (size - p);
		else if (inner <= 0 || sinner <= 0) out[p] = margin;
		else {
			out[p] = v;
			acc += sinner;
			while (acc >= inner) { acc -= inner; v++; }
		}
	}
}
IWRAM_CODE static void ui_panel_(int np, int pal, int tx, int ty, int tw, int th);
void ui_panel(int np, int pal, int tx, int ty, int tw, int th) { ui_panel_(np, pal, tx, ty, tw, th); }
IWRAM_CODE static void ui_panel_(int np, int pal, int tx, int ty, int tw, int th) {
	ui_commit();
	const NineSrc* n = &g_nine[np];
	int W = tw * 8, Hh = th * 8;
	static u8 sxs[256], sys_[256];
	np_table(sxs, W, n->w, n->margin);
	np_table(sys_, Hh, n->h, n->margin);
	// filas de la fuente uniformes en su zona central (relleno rápido)
	static s16 rowfill[64];
	for (int y = 0; y < n->h && y < 64; ++y) {
		const u8* rs = n->px + y * n->w;
		int v = rs[n->margin], uni = 1;
		for (int x = n->margin; x < n->w - n->margin; ++x) if (rs[x] != v) { uni = 0; break; }
		rowfill[y] = uni ? v : -1;
	}
	// copia de la fuente en RAM rápida (la ROM es lenta para lecturas sueltas)
	u8 src[48 * 48];
	int ssz = n->w * n->h;
	if (ssz > (int)sizeof src) ssz = sizeof src;
	for (int i = 0; i < ssz; ++i) src[i] = n->px[i];
	u32 prev[8];
	int prev_t = -1;
	for (int cy = 0; cy < th; ++cy) {
		for (int cx = 0; cx < tw; ++cx) {
			u32 d[8];
			int center = cx * 8 >= n->margin && cx * 8 + 8 <= W - n->margin;
			for (int r = 0; r < 8; ++r) {
				int sy = sys_[cy * 8 + r];
				if (center && rowfill[sy] >= 0) { d[r] = (u32)(rowfill[sy] & 15) * 0x11111111u; continue; }
				const u8* row_src = src + sy * n->w;
				const u8* sx = sxs + cx * 8;
				u32 row = 0;
				for (int c = 0; c < 8; ++c) row |= (u32)(row_src[sx[c]] & 15) << (c * 4);
				d[r] = row;
			}
			int t;
			if (prev_t > 0 && d[0] == prev[0] && d[1] == prev[1] && d[2] == prev[2] && d[3] == prev[3] && d[4] == prev[4] &&
				d[5] == prev[5] && d[6] == prev[6] && d[7] == prev[7]) {
				t = prev_t;
				refc[t]++;
			} else {
				t = tile_shared(d, pal);
				for (int k = 0; k < 8; ++k) prev[k] = d[k];
				prev_t = t;
			}
			cell_set(tx + cx, ty + cy, t, pal);
		}
	}
}

void ui_fill(int tx, int ty, int tw, int th, int pal, int color) {
	ui_commit();
	u32 v = (color & 15) * 0x11111111u;
	u32 d[8] = {v, v, v, v, v, v, v, v};
	for (int y = ty; y < ty + th; ++y)
		for (int x = tx; x < tx + tw; ++x) cell_set(x, y, tile_shared(d, pal), pal);
}

int g_ui_free_pal = -1;     // >= 0: el texto puede escribirse en celdas vacías con esta paleta
void ui_pset(int x, int y, int color) {
	if ((unsigned)x >= 240 || (unsigned)y >= 160) return;
	int cx = x >> 3, cy = y >> 3;
	int ci = cy * 32 + cx;
	if (ci == last_cell) {
		int r = y & 7, c = x & 7;
		last_ptr[r] = (last_ptr[r] & ~(15u << (c * 4))) | ((u32)(color & 15) << (c * 4));
		return;
	}
	if (cell_empty(cy * 32 + cx)) {
		if (g_ui_free_pal < 0) return;          // solo sobre ventanas
		cell_pal[cy * 32 + cx] = g_ui_free_pal;
	}
	volatile u32* t = cell_private(cx, cy);
	if (!t) return;
	last_cell = ci;
	last_ptr = t;
	int r = y & 7, c = x & 7;
	t[r] = (t[r] & ~(15u << (c * 4))) | ((u32)(color & 15) << (c * 4));
}

IWRAM_CODE static void row_mask(int x, int y, u32 bits, int color);
void ui_fill_px(int x, int y, int w, int h, int color) {
	for (int j = 0; j < h; ++j)
		for (int i = 0; i < w; i += 24) {
			int n = w - i < 24 ? w - i : 24;
			row_mask(x + i, y + j, (1u << n) - 1, color);
		}
	ui_commit();
}

// ------------------------------------------------------------------ texto
static int glyph_index(int code) {
	int lo = 0, hi = g_font.count - 1;
	while (lo <= hi) {
		int m = (lo + hi) >> 1;
		int c = g_font.codes[m];
		if (c == code) return m;
		if (c < code) lo = m + 1; else hi = m - 1;
	}
	if (code == 0x2192) return glyph_index('>');     // flecha
	return code == '?' ? 0 : glyph_index('?');
}

static int char_adv(int code) { return g_font.adv[glyph_index(code)]; }

int text_width_n(const char* s, int nbytes) {
	const char* end = s + nbytes;
	int w = 0;
	while (*s && s < end) {
		int c = utf8_next(&s);
		if (c == '\n') break;
		w += char_adv(c);
	}
	return w;
}
int text_width(const char* s) { return text_width_n(s, 1 << 30); }

static void (*pset_fn)(int, int, int) = ui_pset;
static u32* sbuf;
static int sbuf_wc, sbuf_hc;
static void buf_pset(int x, int y, int color) {
	if (x < 0 || y < 0 || x >= sbuf_wc * 8 || y >= sbuf_hc * 8) return;
	u32* t = sbuf + ((y >> 3) * sbuf_wc + (x >> 3)) * 8 + (y & 7);
	int c = x & 7;
	*t = (*t & ~(15u << (c * 4))) | ((u32)(color & 15) << (c * 4));
}

// --- dibujo rápido por filas con máscaras (BG0)
static u32 EXP[256];
static int exp_ready;
static void exp_init(void) {
	for (int b = 0; b < 256; ++b) { u32 m = 0; for (int i = 0; i < 8; ++i) if (b & (1 << i)) m |= 15u << (i * 4); EXP[b] = m; }
	exp_ready = 1;
}
IWRAM_CODE static volatile u32* cell_ptr(int cx, int cy) {
	int ci = cy * 32 + cx;
	if (ci == last_cell) return last_ptr;
	if (cell_empty(ci)) {
		if (g_ui_free_pal < 0) return 0;
		cell_pal[ci] = g_ui_free_pal;
	}
	volatile u32* t = cell_private(cx, cy);
	if (!t) return 0;
	last_cell = ci;
	last_ptr = t;
	return t;
}
IWRAM_CODE static void row_mask(int x, int y, u32 bits, int color) {
	if (!bits || y < 0 || y >= 160) return;
	if (!exp_ready) exp_init();
	if (x < 0) { if (x <= -24) return; bits >>= -x; x = 0; }
	u32 cv = (color & 15) * 0x11111111u;
	int cx = x >> 3, o = x & 7;
	u64 b = (u64)bits << o;
	int cy = y >> 3, r = y & 7;
	while (b) {
		u32 byte = (u32)b & 0xFF;
		if (byte && cx < 30) {
			volatile u32* t = cell_ptr(cx, cy);
			if (t) { u32 m = EXP[byte]; t[r] = (t[r] & ~m) | (cv & m); }
		}
		b >>= 8;
		cx++;
	}
}
// contorno y relleno de una fila en una sola pasada (bits relativos a x)
IWRAM_CODE static void row_mask2(int x, int y, u32 ob, int ocol, u32 fb, int fcol) {
	u32 all = ob | fb;
	if (!all || y < 0 || y >= 160) return;
	if (!exp_ready) exp_init();
	if (x < 0) { if (x <= -24) return; ob >>= -x; fb >>= -x; all >>= -x; x = 0; }
	u32 ov = (ocol & 15) * 0x11111111u, fv = (fcol & 15) * 0x11111111u;
	int cx = x >> 3, o = x & 7;
	u64 bo = (u64)ob << o, bf = (u64)fb << o;
	int cy = y >> 3, r = y & 7;
	while (bo | bf) {
		u32 by_o = (u32)bo & 0xFF, by_f = (u32)bf & 0xFF;
		if ((by_o | by_f) && cx < 30) {
			volatile u32* t = cell_ptr(cx, cy);
			if (t) {
				u32 mo = EXP[by_o], mf = EXP[by_f];
				t[r] = (t[r] & ~(mo | mf)) | (ov & mo) | (fv & mf);
			}
		}
		bo >>= 8; bf >>= 8;
		cx++;
	}
}
IWRAM_CODE static int draw_glyph_fast(int x, int y, int gi, int color, int outline, int italic) {
	if (!exp_ready) exp_init();
	const u16* rows = g_font.rows + gi * g_font.height;
	int H = g_font.height;
	if (outline >= 0) {
		for (int r = -1; r <= H; ++r) {
			u32 a = r - 1 >= 0 && r - 1 < H ? rows[r - 1] : 0;
			u32 c = r >= 0 && r < H ? rows[r] : 0;
			u32 d = r + 1 < H && r + 1 >= 0 ? rows[r + 1] : 0;
			u32 R = (a | c | d) << 1;
			u32 dil = R | (R << 1) | (R >> 1);
			u32 ol = dil & ~(c << 1);
			int sh = italic && r < 6 ? 1 : 0;
			row_mask2(x - 1 + sh, y + r, ol, outline, c << 1, color);
		}
		return g_font.adv[gi];
	}
	for (int r = 0; r < H; ++r) {
		int sh = italic && r < 6 ? 1 : 0;
		row_mask(x + sh, y + r, rows[r], color);
	}
	return g_font.adv[gi];
}

static int draw_glyph(int x, int y, int code, int color, int outline, int italic) {
	if (pset_fn == ui_pset) return draw_glyph_fast(x, y, glyph_index(code), color, outline, italic);
	int gi = glyph_index(code);
	const u16* rows = g_font.rows + gi * g_font.height;
	if (outline >= 0) {
		for (int r = 0; r < g_font.height; ++r) {
			u32 v = rows[r];
			if (!v) continue;
			int sh = italic && r < 6 ? 1 : 0;
			for (int c = 0; c < 16; ++c) if (v & (1u << c)) {
				for (int dy = -1; dy <= 1; ++dy)
					for (int dx = -1; dx <= 1; ++dx) {
						if (!dx && !dy) continue;
						int rr = r + dy, cc = c + dx;
						if (rr >= 0 && rr < g_font.height && cc >= 0 && cc < 16 && (rows[rr] & (1u << cc))) continue;
						pset_fn(x + cc + sh, y + rr, outline);
					}
			}
		}
	}
	for (int r = 0; r < g_font.height; ++r) {
		u32 v = rows[r];
		int sh = italic && r < 6 ? 1 : 0;
		for (int c = 0; v; ++c, v >>= 1) if (v & 1) pset_fn(x + c + sh, y + r, color);
	}
	return g_font.adv[gi];
}

static int ui_text_n_(int x, int y, const char* s, int nbytes, int color, int outline, int italic);
int ui_text_n(int x, int y, const char* s, int nbytes, int color, int outline, int italic) {
	return ui_text_n_(x, y, s, nbytes, color, outline, italic);
}
static int ui_text_n_(int x, int y, const char* s, int nbytes, int color, int outline, int italic) {
	int x0 = x;
	const char* end = s + nbytes;
	while (*s && s < end) {
		int c = utf8_next(&s);
		if (c == '\n') { x = x0; y += 12; continue; }
		x += draw_glyph(x, y, c, color, outline, italic);
	}
	if (pset_fn == ui_pset) ui_commit();
	return x - x0;
}
int text_to_tiles(u32* tiles, int wc, int hc, int x, int y, const char* s, int color, int outline) {
	sbuf = tiles; sbuf_wc = wc; sbuf_hc = hc;
	pset_fn = buf_pset;
	int w = ui_text_n(x, y, s, 1 << 30, color, outline, 0);
	pset_fn = ui_pset;
	return w;
}
int ui_text(int x, int y, const char* s, int color, int outline) { return ui_text_n(x, y, s, 1 << 30, color, outline, 0); }
int ui_text_italic(int x, int y, const char* s, int color, int outline) { return ui_text_n(x, y, s, 1 << 30, color, outline, 1); }
int ui_text_center(int x, int y, int w, const char* s, int color, int outline) {
	int tw = text_width(s);
	return ui_text(x + (w - tw) / 2, y, s, color, outline);
}
int ui_text_right(int x, int y, int w, const char* s, int color, int outline) {
	int tw = text_width(s);
	return ui_text(x + w - tw, y, s, color, outline);
}

int wrap_text(const char* s, int maxw, char* out, int outsize) {
	int lines = 1, w = 0, n = 0;
	int last_space = -1, w_at_space = 0;
	const char* p = s;
	while (*p && n < outsize - 4) {
		const char* q = p;
		int c = utf8_next(&q);
		int len = q - p;
		if (c == '\n') { out[n++] = '\n'; lines++; w = 0; last_space = -1; p = q; continue; }
		if (c == ' ') { last_space = n; w_at_space = w; }
		memcpy(out + n, p, len);
		n += len;
		w += char_adv(c);
		if (w > maxw && last_space >= 0) {
			out[last_space] = '\n';
			lines++;
			w -= w_at_space + char_adv(' ');
			last_space = -1;
		}
		p = q;
	}
	out[n] = 0;
	return lines;
}

int ui_text_wrapped(int x, int y, int maxw, int line_h, const char* s, int color, int outline, int max_lines) {
	char buf[640];
	wrap_text(s, maxw, buf, sizeof buf);
	char* p = buf;
	int ln = 0;
	while (*p && ln < max_lines) {
		char* e = strchr(p, '\n');
		int n = e ? e - p : (int)strlen(p);
		ui_text_n(x, y + ln * line_h, p, n, color, outline, 0);
		ln++;
		if (!e) break;
		p = e + 1;
	}
	return ln;
}

void ui_portrait_buf(const u32* tiles, const u16* pal, int tx, int ty) {
	ui_commit();
	last_cell = -1;
	bgpal_load(PAL_PORTRAIT, pal, 0);
	for (int y = 0; y < 6; ++y)
		for (int x = 0; x < 6; ++x) {
			int t = tile_new();
			if (!t) return;
			volatile u32* d = tptr(t);
			const u32* s = tiles + (y * 6 + x) * 8;
			for (int k = 0; k < 8; ++k) d[k] = s[k];
			refc[t] = 1;
			hashed[t] = 0;
			int i = (ty + y) * 32 + tx + x;
			stale[i] = 0;
			tile_unref(cell_tile[i]);
			cell_tile[i] = t;
			cell_pal[i] = PAL_PORTRAIT;
			sb_shadow[i] = t | (PAL_PORTRAIT << 12);
			sb_dirty = 1;
		}
}
void ui_portrait(const Portrait* p, int tx, int ty) { ui_portrait_buf(p->tiles, p->pal, tx, ty); }

// ------------------------------------------------------------------ sprites de interfaz
static s16 icon_tile[160];
static s8 icon_pal_slot[8];
static int cursor_tile = -1, cursor_pal = -1;

void ui_sprites_reset(void) {
	for (int i = 0; i < 160; ++i) icon_tile[i] = -1;
	for (int i = 0; i < 8; ++i) icon_pal_slot[i] = -1;
	cursor_tile = -1;
	cursor_pal = -1;
}

int icon_index(const char* name) {
	for (int i = 0; i < g_nicons; ++i) if (!strcmp(g_icons[i].name, name)) return i;
	return -1;
}

void ui_icon(int x, int y, const char* name, int prio) {
	int i = icon_index(name);
	if (i < 0) return;
	if (icon_tile[i] < 0) {
		int t = objt_alloc(4);
		if (t < 0) return;
		dma_copy32(objt_ptr(t), icons_t + i * 32, 32);
		icon_tile[i] = t;
	}
	int pg = g_icons[i].pal;
	if (icon_pal_slot[pg] < 0) icon_pal_slot[pg] = objpal_get(g_icon_pals[pg], 256, 256, 256, 0);
	oam_push(0x00001000 + (u32)prio, obj_a0(y, OBJ_SQUARE, 0), obj_a1(x, 1, 0), obj_a2(icon_tile[i], 0, icon_pal_slot[pg]));
}

static void cursor_load(void) {
	if (cursor_tile < 0) {
		cursor_tile = objt_alloc(24);
		if (cursor_tile >= 0) dma_copy32(objt_ptr(cursor_tile), cursors_t, 24 * 8);
	}
	if (cursor_pal < 0) cursor_pal = objpal_get(cursors_p, 256, 256, 256, 0);
}

void ui_cursor(int x, int y, int which, int hflip) {
	cursor_load();
	if (cursor_tile < 0) return;
	if (which == CUR_ARROW2) {
		oam_push(0x00000100, obj_a0(y, OBJ_WIDE, 0), obj_a1(x, 2, 0), obj_a2(cursor_tile + CUR_ARROW2 * 4, 0, cursor_pal));
		return;
	}
	oam_push(0x00000100, obj_a0(y, OBJ_SQUARE, 0), obj_a1(x, 1, hflip), obj_a2(cursor_tile + which * 4, 0, cursor_pal));
}

void ui_cursor_rot(int x, int y, int which, int a) {
	cursor_load();
	if (cursor_tile < 0) return;
	int c = fcos(a) >> 4, s = fsin(a) >> 4;
	int m = oam_affine(c, s, -s, c);
	if (m < 0) { ui_cursor(x, y, which, 0); return; }
	oam_push(0x00000100, (y & 0xFF) | 0x0100, (x & 0x1FF) | (m << 9) | (1 << 14), obj_a2(cursor_tile + which * 4, 0, cursor_pal));
}

// ------------------------------------------------------------------ botones (UIKit.button)
void ui_button(int tx, int ty, int tw, int th, const char* text, int state, int align_left, const char* icon) {
	int np = state == 1 ? NP_BUTTON_HOVER : state == 2 ? NP_BUTTON_DISABLED : state == 3 ? NP_BUTTON_PRESSED : NP_BUTTON_NORMAL;
	ui_panel(np, PAL_UI_ALT, tx, ty, tw, th);
	int col = state == 2 ? UC_BTN_DIS : UC_BTN_TEXT;
	int x = tx * 8 + 6, y = ty * 8 + (th * 8 - 12) / 2;
	if (icon) x += 18;
	if (align_left) ui_text(x, y, text, col, UC_BTN_OUT);
	else ui_text_center(tx * 8, y, tw * 8, text, col, UC_BTN_OUT);
}

// ------------------------------------------------------------------ imágenes de fondo (BG1, mapa de 64x32)
void bg_image_load(const BgImage* img, int cb, int sb, int world) {
	bgpal_load_n(0, img->pals, img->npal, world);
	dma_copy32(CHARBLOCK(cb), img->tiles, img->ntiles * 8);
	for (int i = 0; i < 2048; ++i) SCREENBLOCK(sb)[i] = 0;
	for (int y = 0; y < img->hc && y < 32; ++y)
		for (int x = 0; x < img->wc && x < 64; ++x) {
			int s = x < 32 ? sb : sb + 1;
			SCREENBLOCK(s)[y * 32 + (x & 31)] = img->cells[y * img->wc + x];
		}
}

// ------------------------------------------------------------------ metasprites
int ms_load(const MetaSprite* ms, int frame) {
	int t = objt_alloc(ms->frame_tiles);
	if (t >= 0) dma_copy32(objt_ptr(t), ms->tiles + frame * ms->frame_tiles * 8, ms->frame_tiles * 8);
	return t;
}
void ms_upload(const MetaSprite* ms, int frame, int tile) {
	if (tile >= 0) dma_copy32(objt_ptr(tile), ms->tiles + frame * ms->frame_tiles * 8, ms->frame_tiles * 8);
}
void ms_draw(const MetaSprite* ms, int tile, int pal, int x, int y, int prio, u32 key, int flip, int semi) {
	static const u8 pw[3][4] = {{8, 16, 32, 64}, {16, 32, 32, 64}, {8, 8, 16, 32}};
	for (int i = 0; i < ms->npieces; ++i) {
		const ObjPiece* p = &ms->pieces[i];
		int w = pw[p->shape][p->size];
		int ox = flip ? -(p->x + w) : p->x;
		int sx = x + ox, sy = y + p->y;
		if (sx < -64 || sx > 240 || sy < -64 || sy > 160) continue;
		oam_push(key, obj_a0(sy, p->shape, semi), obj_a1(sx, p->size, flip), obj_a2(tile + p->tile, prio, pal));
	}
}
// con escala (afín, doble tamaño): cada pieza se escala alrededor del ancla
void ms_draw_scaled(const MetaSprite* ms, int tile, int pal, int x, int y, int prio, u32 key, int flip, int semi, int sx256, int sy256) {
	static const u8 pw[3][4] = {{8, 16, 32, 64}, {16, 32, 32, 64}, {8, 8, 16, 32}};
	static const u8 ph[3][4] = {{8, 16, 32, 64}, {8, 8, 16, 32}, {16, 32, 32, 64}};
	if (sx256 == 256 && sy256 == 256) { ms_draw(ms, tile, pal, x, y, prio, key, flip, semi); return; }
	if (sx256 < 8) sx256 = 8;
	if (sy256 < 8) sy256 = 8;
	int pa = (256 * 256 / sx256) * (flip ? -1 : 1), pd = 256 * 256 / sy256;
	int m = oam_affine(pa, 0, 0, pd);
	if (m < 0) { ms_draw(ms, tile, pal, x, y, prio, key, flip, semi); return; }
	for (int i = 0; i < ms->npieces; ++i) {
		const ObjPiece* p = &ms->pieces[i];
		int w = pw[p->shape][p->size], h = ph[p->shape][p->size];
		int cx = (flip ? -(p->x + w / 2) : p->x + w / 2), cy = p->y + h / 2;
		int scx = x + cx * sx256 / 256, scy = y + cy * sy256 / 256;
		int ox = scx - w, oy = scy - h;
		if (ox < -128 || ox > 240 || oy < -128 || oy > 160) continue;
		oam_push(key, (oy & 0xFF) | 0x0300 | (p->shape << 14) | (semi ? 0x0400 : 0), (ox & 0x1FF) | (m << 9) | (p->size << 14),
			obj_a2(tile + p->tile, prio, pal));
	}
}
