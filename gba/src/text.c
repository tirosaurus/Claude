// Texto: fuente de ancho variable dibujada en tiles de BG0, cajas de diálogo, elecciones y carteles.
#include "game.h"
#include <string.h>

#define TXT_SB   30          // screenblock de BG0
#define PAL_UI   15
#define PAL_POR  14

enum { C_BG = 1, C_BORDER, C_TEXT, C_SHADOW, C_GOLD, C_NAME, C_HP, C_MP, C_RED, C_DIM };

static int pool_next = 1;    // asignador de tiles (0 = vacío)

static void ui_palette(void) {
	volatile u16* p = PAL_BG + PAL_UI * 16;
	p[0] = 0;
	p[C_BG] = rgb15(5, 3, 3);
	p[C_BORDER] = rgb15(25, 19, 11);
	p[C_TEXT] = rgb15(30, 28, 24);
	p[C_SHADOW] = rgb15(1, 1, 2);
	p[C_GOLD] = rgb15(31, 24, 8);
	p[C_NAME] = rgb15(29, 20, 12);
	p[C_HP] = rgb15(10, 26, 10);
	p[C_MP] = rgb15(10, 16, 30);
	p[C_RED] = rgb15(28, 8, 8);
	p[C_DIM] = rgb15(16, 13, 11);
}

void text_init(void) {
	ui_palette();
	for (int i = 0; i < 32 * 32; ++i) SCREENBLOCK(TXT_SB)[i] = 0;
	for (int i = 0; i < 8; ++i) CHARBLOCK(0)[i] = 0;
	pool_next = 1;
	REG_BG0CNT = (0 << 2) | (TXT_SB << 8) | 0;
	REG_BG0HOFS = 0;
	REG_BG0VOFS = 0;
}

// ---------------------------------------------------------------- píxeles
static inline volatile u32* cell_tile(int tx, int ty) {
	u16 e = SCREENBLOCK(TXT_SB)[ty * 32 + tx];
	return CHARBLOCK(0) + (e & 0x3FF) * 8;
}

static void pset(int x, int y, int c) {
	int tx = x >> 3, ty = y >> 3;
	if (tx < 0 || ty < 0 || tx >= 30 || ty >= 20) return;
	u16 e = SCREENBLOCK(TXT_SB)[ty * 32 + tx];
	if ((e & 0x3FF) == 0) return;
	volatile u32* t = CHARBLOCK(0) + (e & 0x3FF) * 8;
	int row = y & 7, sh = (x & 7) * 4;
	t[row] = (t[row] & ~(15u << sh)) | ((u32)c << sh);
}

void text_fill(int tx, int ty, int tw, int th, int color) {
	u32 v = color * 0x11111111u;
	for (int y = ty; y < ty + th; ++y)
		for (int x = tx; x < tx + tw; ++x) {
			volatile u32* t = cell_tile(x, y);
			if (t == CHARBLOCK(0)) continue;
			for (int i = 0; i < 8; ++i) t[i] = v;
		}
}

// Reserva tiles para una región de la pantalla y la convierte en ventana.
void text_window(int tx, int ty, int tw, int th) {
	for (int y = ty; y < ty + th; ++y)
		for (int x = tx; x < tx + tw; ++x) {
			u16 e = SCREENBLOCK(TXT_SB)[y * 32 + x];
			int idx = e & 0x3FF;
			if (idx == 0) {
				if (pool_next >= 511) pool_next = 1;
				idx = pool_next++;
			}
			SCREENBLOCK(TXT_SB)[y * 32 + x] = idx | (PAL_UI << 12);
		}
	text_fill(tx, ty, tw, th, C_BG);
	int x0 = tx * 8, y0 = ty * 8, x1 = (tx + tw) * 8 - 1, y1 = (ty + th) * 8 - 1;
	for (int x = x0 + 1; x < x1; ++x) { pset(x, y0, C_BORDER); pset(x, y1, C_BORDER); }
	for (int y = y0 + 1; y < y1; ++y) { pset(x0, y, C_BORDER); pset(x1, y, C_BORDER); }
}

static void region_clear(int tx, int ty, int tw, int th) {
	for (int y = ty; y < ty + th; ++y)
		for (int x = tx; x < tx + tw; ++x) SCREENBLOCK(TXT_SB)[y * 32 + x] = 0;
}

void text_clear_box(void) {
	for (int i = 0; i < 32 * 32; ++i) SCREENBLOCK(TXT_SB)[i] = 0;
	pool_next = 1;
}

// ---------------------------------------------------------------- glifos
static int utf8_next(const char** s) {
	const unsigned char* p = (const unsigned char*)*s;
	int c = *p;
	if (c < 0x80) { *s += 1; return c; }
	if ((c & 0xE0) == 0xC0) { *s += 2; return ((c & 0x1F) << 6) | (p[1] & 0x3F); }
	if ((c & 0xF0) == 0xE0) { *s += 3; return ((c & 0x0F) << 12) | ((p[1] & 0x3F) << 6) | (p[2] & 0x3F); }
	*s += 1;
	return '?';
}

static int glyph_index(int code) {
	for (int i = 0; i < g_font.count; ++i) if (g_font.codes[i] == code) return i;
	return 31;  // '?'
}

static int draw_glyph(int x, int y, int code, int color) {
	int gi = glyph_index(code);
	const u16* rows = g_font.rows + gi * g_font.height;
	int w = g_font.widths[gi];
	for (int r = 0; r < g_font.height; ++r) {
		u16 bits = rows[r];
		for (int c = 0; bits; ++c, bits >>= 1)
			if (bits & 1) {
				pset(x + c + 1, y + r + 1, C_SHADOW);
				pset(x + c, y + r, color);
			}
	}
	return w;
}

static int text_width(const char* s) {
	int w = 0;
	while (*s) w += g_font.widths[glyph_index(utf8_next(&s))];
	return w;
}

void text_draw_string(int tile_x, int tile_y, int w_tiles, int h_tiles, const char* s, int color) {
	(void)w_tiles; (void)h_tiles;
	int x = tile_x * 8, y = tile_y * 8;
	while (*s) {
		int c = utf8_next(&s);
		if (c == '\n') { x = tile_x * 8; y += 12; continue; }
		x += draw_glyph(x, y, c, color ? color : C_TEXT);
	}
}

void text_draw_px(int x, int y, const char* s, int color) {
	while (*s) x += draw_glyph(x, y, utf8_next(&s), color ? color : C_TEXT);
}

// ---------------------------------------------------------------- diálogo
#define BOX_TX 0
#define BOX_TY 13
#define BOX_TW 30
#define BOX_TH 7

static struct {
	int active;
	char buf[512];
	int len, shown;
	int x0, y0, x, y, maxw;
	int done;
	u16 blink;
} dlg;

static const BgImage* portrait_by_name(const char* p) {
	if (!p) return 0;
	if (!strcmp(p, "kaelen")) return &g_img_por_kaelen;
	if (!strcmp(p, "yara")) return &g_img_por_yara;
	if (!strcmp(p, "mother")) return &g_img_por_mother;
	if (!strcmp(p, "bartolo")) return &g_img_por_bartolo;
	if (!strcmp(p, "roc")) return &g_img_por_roc;
	if (!strcmp(p, "nil")) return &g_img_por_nil;
	if (!strcmp(p, "remei")) return &g_img_por_remei;
	if (!strcmp(p, "player")) return FLAG(F_FEMALE) ? &g_img_por_player_f : &g_img_por_player_m;
	return 0;
}

static void draw_portrait(const BgImage* img, int tx, int ty) {
	for (int c = 0; c < 16; ++c) PAL_BG[PAL_POR * 16 + c] = img->pals[c];
	for (int y = 0; y < img->hc; ++y)
		for (int x = 0; x < img->wc; ++x) {
			int idx = SCREENBLOCK(TXT_SB)[(ty + y) * 32 + tx + x] & 0x3FF;
			const u32* src = img->tiles + img->cells[y * img->wc + x] * 8;
			volatile u32* dst = CHARBLOCK(0) + idx * 8;
			for (int i = 0; i < 8; ++i) dst[i] = src[i];
			SCREENBLOCK(TXT_SB)[(ty + y) * 32 + tx + x] = idx | (PAL_POR << 12);
		}
}

// Ajuste de línea: inserta '\n' para que quepa en maxw píxeles.
static void wrap(char* s, int maxw) {
	int w = 0;
	char* last_space = 0;
	int w_at_space = 0;
	for (char* p = s; *p;) {
		if (*p == '\n') { w = 0; last_space = 0; ++p; continue; }
		const char* q = p;
		int c = utf8_next(&q);
		if (c == ' ') { last_space = p; w_at_space = w; }
		w += g_font.widths[glyph_index(c)];
		if (w > maxw && last_space) {
			*last_space = '\n';
			w -= w_at_space + g_font.widths[glyph_index(' ')];
			last_space = 0;
		}
		p = (char*)q;
	}
}

void text_say(const char* speaker, const char* portrait, const char* msg) {
	const BgImage* por = portrait_by_name(portrait);
	region_clear(0, 0, 30, 20);
	pool_next = 1;
	text_window(BOX_TX, BOX_TY, BOX_TW, BOX_TH);
	int tx = 1;
	if (por) {
		draw_portrait(por, 1, BOX_TY);
		tx = 8;
	}
	dlg.x0 = tx * 8;
	dlg.y0 = BOX_TY * 8 + 4;
	if (speaker && speaker[0]) {
		int w = 0, x = dlg.x0;
		const char* s = speaker;
		while (*s) x += draw_glyph(x, dlg.y0, utf8_next(&s), C_NAME);
		(void)w;
		dlg.y0 += 13;
	}
	dlg.maxw = (BOX_TW * 8 - 8) - dlg.x0;
	strncpy(dlg.buf, msg, sizeof dlg.buf - 1);
	dlg.buf[sizeof dlg.buf - 1] = 0;
	wrap(dlg.buf, dlg.maxw);
	dlg.len = strlen(dlg.buf);
	dlg.shown = 0;
	dlg.x = dlg.x0;
	dlg.y = dlg.y0;
	dlg.done = 0;
	dlg.active = 1;
}

int text_busy(void) { return dlg.active; }
int text_done(void) { return dlg.shown >= dlg.len; }
void text_freeze(void) { dlg.active = 0; }

void text_hide(void) {
	dlg.active = 0;
	region_clear(0, 0, 30, 20);
	pool_next = 1;
}

void text_update(void) {
	if (!dlg.active) return;
	if (dlg.shown < dlg.len) {
		int n = key_held(KEY_A) || key_held(KEY_B) ? 6 : 2;
		while (n-- > 0 && dlg.shown < dlg.len) {
			const char* p = dlg.buf + dlg.shown;
			int c = utf8_next(&p);
			dlg.shown = p - dlg.buf;
			if (c == '\n') { dlg.x = dlg.x0; dlg.y += 12; continue; }
			dlg.x += draw_glyph(dlg.x, dlg.y, c, C_TEXT);
		}
		if (dlg.shown >= dlg.len) g_keys_prev |= KEY_A;   // no saltar al terminar
		return;
	}
	// indicador de continuar
	dlg.blink++;
	int bx = BOX_TW * 8 - 12, by = (BOX_TY + BOX_TH) * 8 - 9;
	int on = (dlg.blink / 16) & 1;
	for (int i = 0; i < 4; ++i) { pset(bx + i, by + i / 2, on ? C_GOLD : C_BG); pset(bx + 7 - i, by + i / 2, on ? C_GOLD : C_BG); }
	if (key_hit(KEY_A)) {
		dlg.active = 0;
		region_clear(0, 0, 30, 20);
		pool_next = 1;
	}
}

// ---------------------------------------------------------------- elecciones
static struct {
	int active, n, sel, result, tx, ty, tw, th;
	const char* const* opts;
	u16 lock;
	u8 armed;
} ch;

int g_choice_x = -1;

static void choice_draw(void) {
	text_fill(ch.tx, ch.ty, ch.tw, ch.th, C_BG);
	int x0 = ch.tx * 8, y0 = ch.ty * 8, x1 = (ch.tx + ch.tw) * 8 - 1, y1 = (ch.ty + ch.th) * 8 - 1;
	for (int x = x0 + 1; x < x1; ++x) { pset(x, y0, C_BORDER); pset(x, y1, C_BORDER); }
	for (int y = y0 + 1; y < y1; ++y) { pset(x0, y, C_BORDER); pset(x1, y, C_BORDER); }
	for (int i = 0; i < ch.n; ++i) {
		int x = x0 + 12, y = y0 + 4 + i * 12;
		int col = i == ch.sel ? (ch.armed ? C_GOLD : C_NAME) : C_TEXT;
		const char* s = ch.opts[i];
		if (i == ch.sel) draw_glyph(x0 + 4, y, '>', C_GOLD);
		while (*s) x += draw_glyph(x, y, utf8_next(&s), col);
	}
}

void text_choice(const char* const* opts, int n) {
	int w = 0;
	for (int i = 0; i < n; ++i) { int tw = text_width(opts[i]); if (tw > w) w = tw; }
	ch.tw = (w + 20 + 7) / 8;
	if (ch.tw > 30) ch.tw = 30;
	ch.th = (n * 12 + 8 + 7) / 8;
	ch.tx = g_choice_x >= 0 ? g_choice_x : 30 - ch.tw;
	ch.ty = BOX_TY - ch.th;
	if (ch.ty < 0) ch.ty = 0;
	ch.n = n;
	ch.opts = opts;
	ch.sel = 0;
	ch.result = -1;
	ch.active = 1;
	ch.lock = 24;     // ~0,4 s en los que A no cuenta (evita elegir sin querer)
	ch.armed = 0;
	text_window(ch.tx, ch.ty, ch.tw, ch.th);
	choice_draw();
}

int text_choice_result(void) {
	if (!ch.active) return ch.result;
	if (ch.lock) { ch.lock--; return -1; }
	int redraw = 0;
	if (key_hit(KEY_UP)) { ch.sel = (ch.sel + ch.n - 1) % ch.n; ch.armed = 1; redraw = 1; }
	if (key_hit(KEY_DOWN)) { ch.sel = (ch.sel + 1) % ch.n; ch.armed = 1; redraw = 1; }
	if (key_hit(KEY_A)) {
		if (!ch.armed) { ch.armed = 1; redraw = 1; }
		else {
			ch.active = 0;
			ch.result = ch.sel;
			region_clear(0, 0, 30, 20);
			pool_next = 1;
			return ch.result;
		}
	}
	if (redraw) choice_draw();
	return -1;
}

// ---------------------------------------------------------------- cartel con el nombre del mapa
void text_banner(const char* s) {
	int w = text_width(s);
	int tw = (w + 16 + 7) / 8;
	int tx = (30 - tw) / 2;
	text_window(tx, 1, tw, 3);
	int x = tx * 8 + (tw * 8 - w) / 2;
	while (*s) x += draw_glyph(x, 1 * 8 + 6, utf8_next(&s), C_GOLD);
}
