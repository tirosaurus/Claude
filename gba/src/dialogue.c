#include "dialogue.h"
#include "ui.h"
#include "audio.h"

u8 g_dialog_active;
u32 g_dialog_close_frame;

typedef struct { const char* id; const char* name; const char* portrait; float pitch; } CharInfo;
static const CharInfo CHARS[] = {
	{"player", "", "player", 1.0f}, {"kaelen", "Kaelen", "kaelen", 0.85f}, {"yara", "Yara", "yara", 1.25f},
	{"mother", "Madre", "mother", 1.1f}, {"bartolo", "Bartolo", "bartolo", 0.7f}, {"roc", "Maese Roc", "roc", 0.65f},
	{"nil", "Nil", "nil", 1.45f}, {"remei", "Tía Remei", "remei", 1.05f}, {"aelis", "Aelis", "aelis", 1.3f},
	{"brom", "Brom", "brom", 0.6f}, {"ilvanis", "Anciana Ilvanis", "ilvanis", 0.95f},
	{"elf_guard", "Guardia élfico", "elf_guard", 1.0f}, {"emissary", "El Emisario", "emissary", 0.5f},
	{"thrall", "Siervo", "thrall", 0.45f}, {"durgan", "Capataz Durgan", "roc", 0.6f}, {"selen", "Selen", "selen", 1.2f},
	{"kraag", "General Kraag", "kraag", 0.4f},
};

static const CharInfo* char_info(const char* who) {
	if (!who) return 0;
	for (int i = 0; i < ARRLEN(CHARS); ++i) if (!strcmp(CHARS[i].id, who)) return &CHARS[i];
	return 0;
}

// ------------------------------------------------------------------ formato {name}, {o}...
static void repl(char* out, int* n, int max, const char* s) {
	while (*s && *n < max - 1) out[(*n)++] = *s++;
}
const char* dfmt(const char* s) {
	char out[600];
	int n = 0;
	int f = G.female;
	while (*s && n < (int)sizeof out - 1) {
		if (*s == '{') {
			const char* e = strchr(s, '}');
			if (e) {
				int len = e - s - 1;
				const char* k = s + 1;
				const char* v = 0;
				if (len == 4 && !strncmp(k, "name", 4)) v = player_name();
				else if (len == 1 && *k == 'o') v = f ? "a" : "o";
				else if (len == 2 && !strncmp(k, "os", 2)) v = f ? "as" : "os";
				else if (len == 4 && !strncmp(k, "hijo", 4)) v = f ? "hija" : "hijo";
				else if (len == 2 && !strncmp(k, "el", 2)) v = f ? "ella" : "él";
				else if (len == 1 && *k == 'e') v = f ? "a" : "e";
				else if (len == 4 && !strncmp(k, "raza", 4)) {
					static const char* R[3][2] = {{"humano", "humana"}, {"elfo", "elfa"}, {"enano", "enana"}};
					v = R[G.race][f];
				}
				if (v) { repl(out, &n, sizeof out, v); s = e + 1; continue; }
			}
		}
		out[n++] = *s++;
	}
	out[n] = 0;
	return sfmt("%s", out);
}

// ------------------------------------------------------------------ dibujo de la caja
#define BOX_Y 13
#define BOX_H 7
#define TEXT_X 10
#define TEXT_Y (BOX_Y * 8 + 7)
#define TEXT_W 220
#define LINE_H 12
static float pitch;
static int has_portrait;

static void draw_box(const char* who) {
	ui_clear();
	const CharInfo* ci = char_info(who);
	has_portrait = ci != 0;
	ui_panel(NP_PANEL, PAL_UI_LIGHT, 0, BOX_Y, 30, BOX_H);
	pitch = 0.9f;
	if (ci) {
		pitch = ci->pitch;
		ui_panel(NP_PANEL_DARK, PAL_UI_DARK, 0, BOX_Y - 8, 8, 8);
		int m = member_from_id(who);
		if (m >= 0) {
			const Composed* c = member_portrait(m);
			ui_portrait_buf(c->tiles, c->pal, 1, BOX_Y - 7);
		} else {
			const char* pn = ci->portrait;
			if (!strcmp(pn, "mother") && G.race != RACE_HUMAN) pn = G.race == RACE_ELF ? "mother_elf" : "mother_dwarf";
			const Portrait* p = portrait_by_name(pn);
			if (p) ui_portrait(p, 1, BOX_Y - 7);
		}
		const char* name = !strcmp(who, "player") ? player_name() : ci->name;
		int w = (text_width(name) + 16 + 7) / 8;
		if (w < 6) w = 6;
		ui_panel(NP_PANEL_DARK, PAL_UI_DARK, 8, BOX_Y - 2, w, 2);
		ui_text_center(64, (BOX_Y - 2) * 8 + 2, w * 8, name, UC_DARK_NAME, -1);
	} else if (who && who[0]) {
		int w = (text_width(who) + 16 + 7) / 8;
		ui_panel(NP_PANEL_DARK, PAL_UI_DARK, 1, BOX_Y - 2, w, 2);
		ui_text_center(8, (BOX_Y - 2) * 8 + 2, w * 8, who, UC_DARK_NAME, -1);
	}
}

static void clear_text_area(void) {
	ui_fill_px(TEXT_X - 2, TEXT_Y - 1, TEXT_W + 6, LINE_H * 3 + 2, 0);
	// el color de fondo del pergamino: rellenar con el color del centro del panel
}

static int panel_bg_color(void) {
	const NineSrc* n = &g_nine[NP_PANEL];
	return n->px[(n->h / 2) * n->w + n->w / 2];
}

static int line_wait_input;

// muestra una línea (con paginado si no cabe en tres renglones)
static void show_line(const Line* ln, int wait_input) {
	const char* who = ln->who;
	const char* text = dfmt(ln->text ? ln->text : "");
	draw_box(who);
	int italic = !char_info(who) && !(who && who[0]);
	char wrapped[640];
	wrap_text(text, TEXT_W, wrapped, sizeof wrapped);
	// páginas de 3 renglones
	char* pages[16];
	int npages = 0;
	char* p = wrapped;
	pages[npages++] = p;
	int lines_in_page = 0;
	for (char* q = wrapped; *q; ++q) {
		if (*q == '\n') {
			lines_in_page++;
			if (lines_in_page == 3 && npages < 16) { *q = 0; pages[npages++] = q + 1; lines_in_page = 0; }
		}
	}
	int bg = panel_bg_color();
	for (int pg = 0; pg < npages; ++pg) {
		ui_fill_px(TEXT_X - 2, TEXT_Y - 1, TEXT_W + 8, LINE_H * 3 + 2, bg);
		const char* s = pages[pg];
		int total = 0;
		for (const char* t = s; *t; ) { utf8_next(&t); total++; }
		float vis = 0;
		int shown = 0, blip = 0;
		int x = TEXT_X, y = TEXT_Y;
		const char* cur = s;
		int skip_frame = 1;
		while (shown < total) {
			frame_wait();
			float speed = 48.0f * text_speed_mult() * (key_held(KEY_B) ? 3.0f : 1.0f);
			vis += speed / 60.0f;
			int target = (int)vis;
			if (!skip_frame && key_hit(KEY_A)) target = total;
			skip_frame = 0;
			while (shown < target && shown < total) {
				int c = utf8_next(&cur);
				shown++;
				if (c == '\n') { x = TEXT_X; y += LINE_H; continue; }
				char tmp[5] = {0};
				int len = 0;
				if (c < 0x80) tmp[len++] = c;
				else if (c < 0x800) { tmp[len++] = 0xC0 | (c >> 6); tmp[len++] = 0x80 | (c & 63); }
				else { tmp[len++] = 0xE0 | (c >> 12); tmp[len++] = 0x80 | ((c >> 6) & 63); tmp[len++] = 0x80 | (c & 63); }
				x += ui_text_n(x, y, tmp, len, UC_LIGHT_TEXT, -1, italic);
				if (++blip % 3 == 0) sfx("blip", -18.0f, pitch + randf_range(-0.05f, 0.05f));
			}
		}
		input_flush();
		int last_page = pg == npages - 1;
		if (!wait_input && last_page) return;
		// indicador de continuar
		int t = 0;
		for (;;) {
			frame_wait();
			t++;
			extern int g_dlg_next_x, g_dlg_next_y, g_dlg_next_on;
			g_dlg_next_on = 1;
			g_dlg_next_x = 224;
			g_dlg_next_y = 148 + (fsin((t * 1024 * 6 / 60 / 6) & 1023) * 2 >> 12);
			if (key_hit(KEY_A)) {
				g_dlg_next_on = 0;
				sfx("blip", -14.0f, 1.4f);
				break;
			}
		}
	}
	(void)line_wait_input;
}

int g_dlg_next_x, g_dlg_next_y, g_dlg_next_on;
int g_choice_cur_on, g_choice_cur_x, g_choice_cur_y;

void say_lines(const Line* lines, int n) {
	g_dialog_active = 1;
	for (int i = 0; i < n; ++i) show_line(&lines[i], 1);
	ui_clear();
	g_dialog_active = 0;
	g_dlg_next_on = 0;
	g_dialog_close_frame = g_frame;
}

// ------------------------------------------------------------------ elecciones
int choose_opts(const char* const* opts, int n, const Line* prompt) {
	g_dialog_active = 1;
	if (prompt) show_line(prompt, 0);
	else ui_clear();
	// medir opciones (con ajuste a dos líneas si no caben)
	const char* txt[8];
	int lines[8];
	int longest = 0;
	for (int i = 0; i < n && i < 8; ++i) {
		txt[i] = dfmt(opts[i]);
		int w = text_width(txt[i]);
		lines[i] = 1;
		if (w > 200) { lines[i] = 2; w = 200; }
		if (w > longest) longest = w;
	}
	int total_lines = 0;
	for (int i = 0; i < n; ++i) total_lines += lines[i];
	int tw = (longest + 28 + 7) / 8;
	if (tw > 30) tw = 30;
	int th = (total_lines * 12 + 10 + 7) / 8;
	int tx = 30 - tw, ty = BOX_Y - th;
	if (!prompt) ty = 20 - th - 3;
	if (ty < 0) ty = 0;
	ui_panel(NP_PANEL, PAL_UI_LIGHT, tx, ty, tw, th);
	int idx = 0;
	int armed = 0;
	int lock = 42;
	int redraw = 1;
	int ystart[8];
	int bg = panel_bg_color();
	for (;;) {
		if (redraw) {
			int y = ty * 8 + 5;
			ui_fill_px(tx * 8 + 4, ty * 8 + 4, tw * 8 - 8, th * 8 - 8, bg);
			for (int i = 0; i < n; ++i) {
				ystart[i] = y;
				int col = i == idx ? (armed ? UC_LIGHT_ARM : UC_LIGHT_HI2) : UC_LIGHT_TEXT;
				if (lines[i] == 1) ui_text(tx * 8 + 18, y, txt[i], col, -1);
				else ui_text_wrapped(tx * 8 + 18, y, 200, 12, txt[i], col, -1, 2);
				y += lines[i] * 12;
			}
			redraw = 0;
		}
		g_choice_cur_on = 1;
		g_choice_cur_x = tx * 8 + 6;
		g_choice_cur_y = ystart[idx] + 2;
		frame_wait();
		if (lock) lock--;
		if (key_rep(KEY_UP)) { idx = (idx + n - 1) % n; armed = 1; sfx("select", -14.0f, 1.0f); redraw = 1; }
		else if (key_rep(KEY_DOWN)) { idx = (idx + 1) % n; armed = 1; sfx("select", -14.0f, 1.0f); redraw = 1; }
		else if (key_hit(KEY_A) && !lock) {
			if (armed) break;
			armed = 1;
			sfx("select", -10.0f, 1.3f);
			redraw = 1;
		}
	}
	g_choice_cur_on = 0;
	ui_clear();
	g_dialog_active = 0;
	g_dialog_close_frame = g_frame;
	sfx("confirm", -8.0f, 1.0f);
	input_flush();
	return idx;
}

int choose_text(const char* prompt, const char* const* opts, int n) {
	Line l = {0, prompt};
	return choose_opts(opts, n, prompt ? &l : 0);
}

int choose_list(const char* const* opts, int n, const Line* prompt) { return choose_opts(opts, n, prompt); }

// ------------------------------------------------------------------ listas de líneas
void lb_add(LineBuf* b, Line l) { if (b->n < 24) b->l[b->n++] = l; }

void cycle_lines(LineBuf* out, const char* key, const Line* const* opts, int n) {
	const char* mk = sfmt("talk_%s", key);
	int cnt = meta_i(mk, 0);
	meta_set_i(mk, cnt + 1);
	int idx = cnt;
	if (cnt >= n) idx = n > 1 ? 1 + (cnt - 1) % (n - 1) : 0;
	for (const Line* l = opts[idx]; l->text; ++l) lb_add(out, *l);
}

// ------------------------------------------------------------------ cartel con el nombre del mapa (OBJ semitransparente)
static u32 ban_tiles[28 * 2 * 8] EWRAM_BSS;
static int ban_t = -1, ban_tile = -1, ban_pal = -1, ban_w;
static u16 ban_palette[16];
void banner_show(const char* text) {
	memset(ban_tiles, 0, sizeof ban_tiles);
	ban_w = text_to_tiles(ban_tiles, 28, 2, 2, 2, text, 1, 2) + 6;
	ban_palette[0] = 0;
	ban_palette[1] = (31) | (29 << 5) | (23 << 10);          // (1, 0.93, 0.75)
	ban_palette[2] = (3) | (2 << 5) | (3 << 10);
	ban_t = 0;
}
static void banner_free(void) {
	if (ban_tile >= 0) objt_free(ban_tile);
	if (ban_pal >= 0) objpal_put(ban_pal);
	ban_tile = ban_pal = -1;
}
// devuelve el alfa (0..16) del cartel o -1 si no hay
int banner_draw(void) {
	if (ban_t < 0 || g_paused) return -1;
	ban_t++;
	int a;
	if (ban_t < 24) a = 0;
	else if (ban_t < 60) a = (ban_t - 24) * 16 / 36;
	else if (ban_t < 168) a = 16;
	else if (ban_t < 216) a = 16 - (ban_t - 168) * 16 / 48;
	else { ban_t = -1; banner_free(); return -1; }
	if (ban_tile < 0) {
		ban_tile = objt_alloc(56);
		if (ban_tile < 0) return -1;
		// 7 piezas de 32x16: reordenar a 1D por pieza
		for (int pc = 0; pc < 7; ++pc)
			for (int r = 0; r < 2; ++r)
				for (int c = 0; c < 4; ++c) {
					const u32* src = ban_tiles + (r * 28 + pc * 4 + c) * 8;
					volatile u32* dst = objt_ptr(ban_tile + pc * 8 + r * 4 + c);
					for (int k = 0; k < 8; ++k) dst[k] = src[k];
				}
	}
	if (ban_pal < 0) ban_pal = objpal_get(ban_palette, 256, 256, 256, 0);
	int x0 = (240 - ban_w) / 2;
	int np = (ban_w + 31) / 32;
	for (int pc = 0; pc < np && pc < 7; ++pc)
		oam_push(0x00000800 + pc, obj_a0(10, OBJ_WIDE, 1), obj_a1(x0 + pc * 32, 2, 0), obj_a2(ban_tile + pc * 8, 0, ban_pal));
	return a;
}

// sprites de interfaz de cada fotograma (indicador de continuar, cursor de elección, cartel)
void ui_frame_sprites(void) {
	if (g_dlg_next_on) ui_cursor_rot(g_dlg_next_x, g_dlg_next_y, CUR_CURSOR, 256);
	if (g_choice_cur_on) ui_cursor(g_choice_cur_x, g_choice_cur_y, CUR_CURSOR, 0);
	int a = banner_draw();
	if (a >= 0) alpha_set(a, 16 - a);
}
