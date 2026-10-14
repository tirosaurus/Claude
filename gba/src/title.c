// Título, creación de personaje, introducción, opciones y ranuras de guardado
// (port de Title.gd, CharacterCreation.gd, Intro.gd, OptionsUI.gd y SaveUI.gd).
#include "world.h"
#include "ui.h"
#include "audio.h"
#include "scene.h"
#include "particles.h"
#include "dialogue.h"

static void (*modal_render)(void);

// render genérico de las escenas de interfaz: sprites + partículas
static void ui_scene_render(void) {
	oam_begin();
	if (modal_render) modal_render();
	ui_frame_sprites();
	particles_render(0, 0, 0x10000000);
	oam_end();
}

static void ui_wait(void) {
	particles_update();
	ui_scene_render();
	frame_wait();
}

static void scene_fade_in(void) {
	while (fade_level() > 0) {
		fade_set(fade_level() - 2 < 0 ? 0 : fade_level() - 2, 0);
		ui_wait();
	}
}

static void scene_fade_out(void) {
	for (int i = fade_level(); i <= 16; i += 2) { fade_set(i, 0); ui_wait(); }
	fade_set(16, 0);
}

// ------------------------------------------------------------------ opciones
static const char* bar10(int v) {
	static char b[16];
	b[0] = '[';
	for (int i = 0; i < 10; ++i) b[1 + i] = i < v ? '|' : '.';
	b[11] = ']';
	b[12] = 0;
	return b;
}

void options_ui(void) {
	static const char* const names[5] = {"Música", "Efectos", "Texto", "Combate", "Volver"};
	static const char* const speeds[3] = {"Lenta", "Normal", "Rápida"};
	int idx = 0, redraw = 1;
	int ignore = 2;
	for (;;) {
		if (redraw) {
			ui_panel(NP_PANEL, PAL_UI_LIGHT, 4, 3, 22, 14);
			ui_text_center(32, 30, 176, "Opciones", UC_LIGHT_TITLE, -1);
			for (int k = 0; k < 5; ++k) {
				int col = k == idx ? UC_LIGHT_SEL : UC_LIGHT_TEXT;
				int y = 48 + k * 14;
				ui_text(44, y, sfmt("%s%s", k == idx ? "> " : "   ", names[k]), col, -1);
				const char* v = "";
				if (k == 0) v = bar10(g_music_level);
				else if (k == 1) v = bar10(g_sfx_level);
				else if (k == 2) v = speeds[g_text_speed];
				else if (k == 3) v = speeds[g_battle_speed];
				ui_text(120, y, v, col, -1);
			}
			ui_text_center(32, 120, 176, "Izq./Der.: cambiar  ·  B: volver", UC_LIGHT_HINT, -1);
			redraw = 0;
		}
		ui_wait();
		if (ignore) { ignore--; continue; }
		int d = 0;
		if (key_rep(KEY_UP)) { idx = (idx + 4) % 5; sfx("select", -14, 1); redraw = 1; }
		else if (key_rep(KEY_DOWN)) { idx = (idx + 1) % 5; sfx("select", -14, 1); redraw = 1; }
		else if (key_rep(KEY_LEFT)) d = -1;
		else if (key_rep(KEY_RIGHT)) d = 1;
		else if (key_hit(KEY_A)) { if (idx == 4) break; d = 1; }
		else if (key_hit(KEY_B)) break;
		if (d) {
			if (idx == 0) g_music_level = CLAMP(g_music_level + d, 0, 10);
			else if (idx == 1) g_sfx_level = CLAMP(g_sfx_level + d, 0, 10);
			else if (idx == 2) g_text_speed = CLAMP(g_text_speed + d, 0, 2);
			else if (idx == 3) g_battle_speed = CLAMP(g_battle_speed + d, 0, 2);
			audio_apply_settings();
			sfx("select", -8, 1);
			redraw = 1;
		}
	}
	settings_save();
	sfx("cancel", -8, 1);
	ui_clear_rect(4, 3, 22, 14);
	input_flush();
}

// ------------------------------------------------------------------ ranuras de guardado
static const struct { const char* id; const char* name; } MAP_NAMES[] = {
	{"bedroom", "Tu habitación"}, {"housemain", "Casa"}, {"village", "Tortosa"}, {"forest", "Bosque Santo"},
	{"village_night", "Tortosa en llamas"}, {"village_dawn", "Tortosa, al alba"}, {"cathedral", "La Catedral Vieja"},
	{"crypt", "La Cripta"}, {"forest_deep", "Bosque Profundo"}, {"elf_camp", "Claro de la Savia"}, {"heart", "Corazón del Bosque"},
};
static const char* map_name(const char* id) {
	for (int i = 0; i < ARRLEN(MAP_NAMES); ++i) if (!strcmp(MAP_NAMES[i].id, id)) return MAP_NAMES[i].name;
	const MapData* m = map_by_id(id);
	return m && m->display[0] ? m->display : id;
}

// devuelve 1 si se ha cargado una partida
int save_ui_run(int save_mode, int px, int py) {
	int sel = 0, redraw = 1;
	const char* msg = "";
	if (!save_mode) {
		for (sel = 0; sel < SAVE_SLOTS; ++sel) { SlotInfo si; if (slot_info(sel, &si)) break; }
		if (sel >= SAVE_SLOTS) sel = SAVE_SLOTS;
	}
	int prev_free = g_ui_free_pal;
	for (;;) {
		if (redraw) {
			ui_panel(NP_PANEL, PAL_UI_LIGHT, 1, 1, 28, 18);
			ui_text_center(8, 14, 224, save_mode ? "Guardar partida" : "Cargar partida", UC_LIGHT_TITLE, -1);
			for (int i = 0; i < SAVE_SLOTS; ++i) {
				SlotInfo si;
				int used = slot_info(i, &si);
				int state = i == sel ? 1 : (!save_mode && !used ? 2 : 0);
				ui_button(3, 4 + i * 4, 24, 4, "", state, 1, 0);
				int col = state == 2 ? UC_BTN_DIS : UC_BTN_TEXT;
				if (!used) ui_text(30, 4 * 8 + i * 32 + 9, sfmt("Ranura %d  —  vacía", i + 1), col, UC_BTN_OUT);
				else {
					ui_text(30, 4 * 8 + i * 32 + 4, sfmt("Ranura %d — %s · Nv %ld", i + 1, si.name, (long)si.level), col, UC_BTN_OUT);
					ui_text(30, 4 * 8 + i * 32 + 16, sfmt("%s · %s", map_name(si.map), format_time(si.play_frames)), col, UC_BTN_OUT);
				}
			}
			ui_button(3, 16, 8, 2, "Volver", sel == SAVE_SLOTS ? 1 : 0, 0, 0);
			ui_text(96, 130, msg, UC_LIGHT_GOOD, -1);
			redraw = 0;
		}
		ui_wait();
		if (key_rep(KEY_UP)) { sel = (sel + SAVE_SLOTS) % (SAVE_SLOTS + 1); sfx("select", -16, 1); redraw = 1; }
		else if (key_rep(KEY_DOWN)) { sel = (sel + 1) % (SAVE_SLOTS + 1); sfx("select", -16, 1); redraw = 1; }
		else if (key_hit(KEY_B)) break;
		else if (key_hit(KEY_A)) {
			if (sel == SAVE_SLOTS) break;
			if (save_mode) {
				if (save_game(sel, px, py)) { sfx("save", -4, 1); msg = sfmt("Partida guardada en la ranura %d.", sel + 1); }
				else msg = "No se pudo guardar.";
				redraw = 1;
			} else {
				SlotInfo si;
				if (!slot_info(sel, &si)) { sfx("cancel", -8, 1); continue; }
				if (load_game(sel)) {
					sfx("confirm", -4, 1);
					ui_clear_rect(1, 1, 28, 18);
					g_ui_free_pal = prev_free;
					return 1;
				}
			}
		}
	}
	ui_clear_rect(1, 1, 28, 18);
	g_ui_free_pal = prev_free;
	input_flush();
	return 0;
}

void save_ui(int save_mode, int x, int y) {
	dim_set(9);
	if (save_ui_run(save_mode, x, y) && !save_mode) { dim_set(0); scene_request(SC_WORLD); block_forever(); }
	dim_set(0);
}

// ------------------------------------------------------------------ título
static int logo_tile = -1, logo_pal = -1;
static int title_t;
static void title_render(void) {
	if (logo_tile < 0) { logo_tile = ms_load(&ms_logo, 0); logo_pal = objpal_get(ms_logo.pal, 256, 256, 256, 0); }
	int bob = fsin((title_t * 1024 * 14 / 600) & 1023) * 3 / 4096;
	ms_draw(&ms_logo, logo_tile, logo_pal, 120, 24 + bob, 0, 0x100, 0, 0);
}

static ParticleDef fireflies(void) {
	ParticleDef d;
	memset(&d, 0, sizeof d);
	d.amount = 24; d.lifetime = 7; d.rect_w = 120; d.rect_h = 30; d.gy = -4; d.vmin = 2; d.vmax = 8; d.spread = 180;
	d.dir_x = 0; d.dir_y = -1; d.size = 2; d.r = 255; d.g = 230; d.b = 128; d.fade_in = 1;
	return d;
}

static void title_bg(int dim) {
	bg_image_load(&bg_title, 1, 28, 1);
	g_vid.bgcnt[1] = (1 << 2) | (28 << 8) | (1 << 14) | 3;
	g_vid.hofs[1] = 40;
	g_vid.vofs[1] = 10;
	if (dim) world_tint(128, 115, 141);
	else world_tint(256, 256, 256);
}

void title_scene(void* a) {
	(void)a;
#ifdef START_MAP
	{
		static int once;
		if (!once) {
			once = 1;
			Appearance ap;
			default_appearance(&ap);
			gs_new_game("Aleix", &ap, 1);
			strcpy(G.current_map, START_MAP);
			strcpy(G.current_spawn, START_SPAWN);
#ifdef START_PARTY
			join_party(M_KAELEN); join_party(M_YARA);
#endif
#ifdef START_FLAGS
			{ static const char* const FL[] = {START_FLAGS}; for (unsigned i = 0; i < sizeof FL / sizeof FL[0]; ++i) setf(FL[i]); }
#endif
#ifdef START_TOD
			meta_set_s("tod", START_TOD);
#endif
#ifdef START_ENDING
			setf("romance_yara"); setf("kaelen_oath"); G.cls = 0;
			scene_request(SC_ENDING);
			return;
#endif
#ifdef START_BATTLE
			memset(&g_enc, 0, sizeof g_enc);
			strcpy(g_enc.enemies[0], START_BATTLE); strcpy(g_enc.enemies[1], START_BATTLE); g_enc.n = 2;
			strcpy(g_enc.bg, "forest"); g_enc.can_flee = 1;
			strcpy(g_enc.ret_map, START_MAP); strcpy(g_enc.ret_spawn, START_SPAWN);
			scene_request(SC_BATTLE);
			return;
#endif
			scene_request(SC_WORLD);
			return;
		}
	}
#endif
	g_vid.dispcnt = DCNT_BLANK;
	fade_set(16, 0);
	objt_reset();
	objpal_reset();
	ui_init();
	ui_sprites_reset();
	particles_init();
	logo_tile = -1;
	title_bg(0);
	g_vid.dispcnt = DCNT_MODE0 | DCNT_OBJ1D | DCNT_BG0 | DCNT_BG1 | DCNT_OBJ;
	ParticleDef ff = fireflies();
	particles_emit(&ff, 120, 140, 1);
	music_play("title", 1.0f);
	modal_render = title_render;
	g_ui_free_pal = PAL_UI_DARK;
	int last = any_save();
	const char* items[4];
	int n = 0, ids[4];
	if (last >= 0) { items[n] = "Continuar"; ids[n++] = 0; }
	items[n] = "Nueva partida"; ids[n++] = 1;
	items[n] = "Cargar partida"; ids[n++] = 2;
	items[n] = "Opciones"; ids[n++] = 3;
	int sel = 0, redraw = 1;
	title_t = 0;
	for (;;) {
		if (redraw) {
			ui_clear();
			g_ui_free_pal = PAL_UI_DARK;
			ui_text_center(0, 44, 240, "Crónicas del Velo Roto", UC_DARK_TEXT, UC_DARK_OUT);
			for (int i = 0; i < n; ++i) {
				int dis = ids[i] == 2 && last < 0;
				ui_button(8, 8 + i * 3, 14, 3, items[i], dis ? 2 : (i == sel ? 1 : 0), 0, 0);
			}
			redraw = 0;
		}
		title_t++;
		ui_wait();
		if (fade_level() > 0) { fade_set(fade_level() - 1, 0); continue; }
		if (key_rep(KEY_UP) || key_rep(KEY_DOWN)) {
			int d = key_rep(KEY_UP) ? n - 1 : 1;
			do sel = (sel + d) % n; while (ids[sel] == 2 && last < 0);
			sfx("select", -16, 1);
			redraw = 1;
		}
		else if (key_hit(KEY_A) || key_hit(KEY_START)) {
			int id = ids[sel];
			if (id == 0) {
				if (load_game(last)) { sfx("confirm", -6, 1); scene_fade_out(); scene_request(SC_WORLD); return; }
			} else if (id == 1) {
				sfx("confirm", -6, 1);
				scene_fade_out();
				scene_request(SC_CREATION);
				return;
			} else if (id == 2 && last >= 0) {
				ui_clear();
				if (save_ui_run(0, 0, 0)) { scene_fade_out(); scene_request(SC_WORLD); return; }
				redraw = 1;
			} else if (id == 3) {
				ui_clear();
				options_ui();
				redraw = 1;
			}
		}
	}
}

// ------------------------------------------------------------------ teclado en pantalla para el nombre
static const char* const KEYS[5] = {"ABCDEFGHIJ", "KLMNÑOPQRS", "TUVWXYZ-'.", "abcdefghij", "klmnñopqrs"};
static const char* const KEYS2[2] = {"tuvwxyzáéí", "óú ¡!¿?    "};

static int key_cell(int row, int col, char* out) {
	const char* s = row < 5 ? KEYS[row] : KEYS2[row - 5];
	const char* p = s;
	for (int i = 0; *p; ++i) {
		const char* q = p;
		utf8_next(&q);
		if (i == col) { int n = q - p; memcpy(out, p, n); out[n] = 0; return n; }
		p = q;
	}
	out[0] = 0;
	return 0;
}

void name_entry(char* name, int maxlen) {
	char buf[32];
	strncpy(buf, name, sizeof buf - 1);
	buf[31] = 0;
	int r = 0, c = 0, redraw = 1;
	for (;;) {
		if (redraw) {
			ui_panel(NP_PANEL, PAL_UI_LIGHT, 2, 1, 26, 18);
			ui_text_center(16, 12, 208, "¿Cómo te llamas?", UC_LIGHT_TITLE, -1);
			ui_panel(NP_PANEL_DARK, PAL_UI_DARK, 6, 4, 18, 2);
			ui_text_center(48, 36, 144, sfmt("%s_", buf), UC_DARK_TEXT, -1);
			for (int row = 0; row < 7; ++row)
				for (int col = 0; col < 10; ++col) {
					char k[5];
					key_cell(row, col, k);
					int x = 40 + col * 16, y = 52 + row * 12;
					int col_c = (row == r && col == c) ? UC_LIGHT_ARM : UC_LIGHT_TEXT;
					if (k[0] == ' ' && !k[1]) ui_text(x, y, (row == r && col == c) ? "_" : "·", col_c, -1);
					else ui_text(x, y, k, col_c, -1);
				}
			int ok_sel = r == 7;
			ui_text(48, 138, ok_sel && c < 5 ? "> Borrar" : "  Borrar", ok_sel && c < 5 ? UC_LIGHT_ARM : UC_LIGHT_TEXT, -1);
			ui_text(132, 138, ok_sel && c >= 5 ? "> Aceptar" : "  Aceptar", ok_sel && c >= 5 ? UC_LIGHT_ARM : UC_LIGHT_TEXT, -1);
			redraw = 0;
		}
		ui_wait();
		if (key_rep(KEY_UP)) { r = (r + 7) % 8; redraw = 1; sfx("select", -14, 1); }
		else if (key_rep(KEY_DOWN)) { r = (r + 1) % 8; redraw = 1; sfx("select", -14, 1); }
		else if (key_rep(KEY_LEFT)) { c = (c + 9) % 10; redraw = 1; sfx("select", -14, 1); }
		else if (key_rep(KEY_RIGHT)) { c = (c + 1) % 10; redraw = 1; sfx("select", -14, 1); }
		else if (key_hit(KEY_B)) {
			int n = strlen(buf);
			while (n > 0 && (buf[n - 1] & 0xC0) == 0x80) n--;
			if (n > 0) buf[n - 1] = 0;
			redraw = 1;
		} else if (key_hit(KEY_START)) break;
		else if (key_hit(KEY_A)) {
			if (r == 7) {
				if (c < 5) { int n = strlen(buf); while (n > 0 && (buf[n - 1] & 0xC0) == 0x80) n--; if (n > 0) buf[n - 1] = 0; }
				else break;
			} else {
				char k[5];
				key_cell(r, c, k);
				int chars = 0;
				for (const char* p = buf; *p; ) { utf8_next(&p); chars++; }
				if (chars < maxlen && strlen(buf) + strlen(k) < 30) strcat(buf, k);
				sfx("confirm", -12, 1);
			}
			redraw = 1;
		}
	}
	// recortar espacios
	int n = strlen(buf);
	while (n > 0 && buf[n - 1] == ' ') buf[--n] = 0;
	char* s = buf;
	while (*s == ' ') s++;
	strncpy(name, s, 15);
	name[15] = 0;
	ui_clear();
	input_flush();
}

// ------------------------------------------------------------------ creación de personaje
static Appearance capp;
static int cdiff = 1;
static char cname[16];
static int prev_tile = -1, prev_pal = -1, prev_dirty = 1, crow;
static const Composed* prev_c;
static int ct;

static void creation_render(void) {
	if (prev_dirty) {
		prev_c = preview_sheet(&capp);
		if (prev_pal >= 0) objpal_put(prev_pal);
		prev_pal = objpal_get(prev_c->pal, 256, 256, 256, 0);
		prev_dirty = 0;
	}
	if (prev_tile < 0) prev_tile = objt_alloc(16);
	int dir = (ct / 96) % 4;
	static const int order[4] = {0, 2, 3, 1};
	int frame = order[dir] * 8 + 2 + (ct * 9 / 60) % 6;
	composed_upload_frame(prev_c, frame, prev_tile);
	// a doble tamaño (afín)
	int m = oam_affine(128, 0, 0, 128);
	if (m >= 0)
		oam_push(0x200, (24 & 0xFF) | 0x0300, (12 & 0x1FF) | (m << 9) | (2 << 14), obj_a2(prev_tile, 0, prev_pal));
	// cursor
	int y = crow < 10 ? 18 + crow * 10 : 0;
	ui_cursor(80, y + 1, CUR_HAND, 0);
}

static const char* const CROWS[10] = {"Nombre", "Sexo", "Raza", "Peinado", "Color de pelo", "Piel", "Ropa", "Barba", "Dificultad", ""};
static const char* const DIFFS[3] = {"Fácil", "Normal", "Difícil"};
static const char* const RACE_ORDER[3] = {"human", "elf", "dwarf"};

static const char* cvalue(int k) {
	switch (k) {
	case 0: return cname[0] ? cname : "(pulsa A)";
	case 1: return capp.female ? "Mujer" : "Hombre";
	case 2: return g_races[capp.race].name;
	case 3: return g_hair_styles[capp.hair_style].name;
	case 4: return g_hair_colors[capp.hair_color].name;
	case 5: return g_skin_tones[capp.skin].name;
	case 6: return g_outfits[capp.outfit].name;
	case 7: return capp.female ? "—" : (capp.beard ? "Sí" : "No");
	case 8: return DIFFS[cdiff];
	}
	return "";
}

static int panel_bg(void) {
	const NineSrc* n = &g_nine[NP_PANEL];
	return n->px[(n->h / 2) * n->w + n->w / 2];
}

static int creation_full = 1;
static void creation_draw(void) {
	if (creation_full) {
		ui_clear_rect(0, 0, 30, 20);
		ui_panel(NP_PANEL, PAL_UI_LIGHT, 0, 0, 30, 20);
		ui_text_center(0, 4, 240, "Crea tu personaje", UC_LIGHT_TITLE, -1);
		ui_panel(NP_PANEL_DARK, PAL_UI_DARK, 1, 2, 9, 10);
		creation_full = 0;
	} else {
		ui_fill_px(90, 14, 146, 102, panel_bg());
		ui_fill_px(6, 116, 228, 38, panel_bg());
	}
	for (int k = 0; k < 9; ++k) {
		int y = 18 + k * 10;
		int sel = k == crow;
		ui_text(94, y - 2, CROWS[k], sel ? UC_LIGHT_SEL : UC_LIGHT_TEXT, -1);
		if (k == 0) ui_text_center(156, y - 2, 80, cvalue(k), UC_LIGHT_HI2, -1);
		else {
			ui_text(160, y - 2, "<", UC_LIGHT_HINT, -1);
			ui_text_center(164, y - 2, 68, cvalue(k), UC_LIGHT_HI2, -1);
			ui_text(230, y - 2, ">", UC_LIGHT_HINT, -1);
		}
	}
	ui_text_center(84, 106, 156, "¡Comenzar la aventura!", crow == 9 ? UC_LIGHT_ARM : UC_LIGHT_HI2, -1);
	const char* desc;
	if (crow == 8) {
		static const char* const D[3] = {"Fácil: enemigos más débiles y más experiencia. Para disfrutar de la historia.",
			"Normal: equilibrado. Hay que usar habilidades y objetos con cabeza.",
			"Difícil: enemigos más duros y agresivos. Para veteranos de los JRPG."};
		desc = D[cdiff];
	} else desc = sfmt("%s %s", g_races[capp.race].desc, g_races[capp.race].trait);
	ui_text_wrapped(8, 116, 224, 9, desc, UC_LIGHT_TEXT, -1, 4);
}

static void creation_change(int k, int d) {
	switch (k) {
	case 1:
		capp.female = !capp.female;
		if (capp.female) { capp.beard = 0; if (capp.hair_style == 1 || capp.hair_style == 6) capp.hair_style = 2; }
		else if (capp.hair_style == 2 || capp.hair_style == 5) capp.hair_style = 1;
		break;
	case 2:
		capp.race = (capp.race + d + 3) % 3;
		if (capp.race == RACE_DWARF && !capp.female) capp.beard = 1;
		break;
	case 3: capp.hair_style = (capp.hair_style + d + N_HAIR_STYLES) % N_HAIR_STYLES; break;
	case 4: capp.hair_color = (capp.hair_color + d + N_HAIR_COLORS) % N_HAIR_COLORS; break;
	case 5: capp.skin = (capp.skin + d + N_SKIN_TONES) % N_SKIN_TONES; break;
	case 6: capp.outfit = (capp.outfit + d + N_OUTFITS) % N_OUTFITS; break;
	case 7: if (!capp.female) capp.beard = !capp.beard; break;
	case 8: cdiff = CLAMP(cdiff + d, 0, 2); break;
	}
	(void)RACE_ORDER;
	sfx("select", -12, 1);
	prev_dirty = 1;
}

void creation_scene(void* a) {
	(void)a;
	g_vid.dispcnt = DCNT_BLANK;
	objt_reset();
	objpal_reset();
	ui_init();
	ui_sprites_reset();
	particles_init();
	title_bg(1);
	g_vid.dispcnt = DCNT_MODE0 | DCNT_OBJ1D | DCNT_BG0 | DCNT_BG1 | DCNT_OBJ;
	default_appearance(&capp);
	cdiff = 1;
	cname[0] = 0;
	crow = 0;
	creation_full = 1;
	prev_tile = -1; prev_pal = -1; prev_dirty = 1;
	modal_render = creation_render;
	g_ui_free_pal = -1;
	music_play("title", 0.5f);
	creation_draw();
	ct = 0;
	scene_fade_in();
	for (;;) {
		ct++;
		ui_wait();
		int redraw = 0;
		if (key_rep(KEY_UP)) { crow = (crow + 9) % 10; sfx("select", -14, 1); redraw = 1; }
		else if (key_rep(KEY_DOWN)) { crow = (crow + 1) % 10; sfx("select", -14, 1); redraw = 1; }
		else if (crow > 0 && crow < 9 && key_rep(KEY_LEFT)) { creation_change(crow, -1); redraw = 1; }
		else if (crow > 0 && crow < 9 && key_rep(KEY_RIGHT)) { creation_change(crow, 1); redraw = 1; }
		else if (key_hit(KEY_A)) {
			if (crow == 0) {
				modal_render = 0;
				name_entry(cname, 14);
				modal_render = creation_render;
				creation_full = 1;
				if (cname[0]) crow = 1;
				redraw = 1;
			} else if (crow == 9) break;
			else { creation_change(crow, 1); redraw = 1; }
		} else if (key_hit(KEY_START)) break;
		if (redraw) creation_draw();
	}
	const char* n = cname[0] ? cname : (capp.female ? "Alda" : "Arlen");
	gs_new_game(n, &capp, cdiff);
	sfx("confirm", -6, 1);
	music_stop(1.0f);
	modal_render = 0;
	scene_fade_out();
	scene_request(SC_INTRO);
}

// ------------------------------------------------------------------ introducción
static const char* const INTRO[4] = {
	"Hace trescientos años, el último Archimago del Imperio de Cristal quiso convertirse en dios.",
	"Fracasó. Y al caer, rasgó el velo que separa los mundos.",
	"Desde entonces, cosas antiguas despiertan en los rincones olvidados de Vaelmoor.",
	"Pero en los montes del norte, lejos de todo, hay un pueblo al que esas historias nunca han llegado.",
};

void intro_scene(void* a) {
	(void)a;
	g_vid.dispcnt = DCNT_BLANK;
	objt_reset();
	objpal_reset();
	ui_init();
	ui_sprites_reset();
	particles_init();
	modal_render = 0;
	g_pal[0] = (1) | (1 << 5) | (2 << 10);      // fondo oscuro
	g_vid.dispcnt = DCNT_MODE0 | DCNT_OBJ1D | DCNT_BG0 | DCNT_OBJ;
	g_ui_free_pal = PAL_UI_DARK;
	music_play("title", 1.0f);
	fade_set(0, 0);
	int skip = 0;
	for (int k = 0; k < 36; ++k) { ui_wait(); if (key_hit(KEY_B) || key_hit(KEY_START)) skip = 1; }
	for (int i = 0; i < 5 && !skip; ++i) {
		const char* s = i < 4 ? INTRO[i] : "Tortosa.";
		ui_clear();
		char wrapped[400];
		int lines = wrap_text(s, 200, wrapped, sizeof wrapped);
		int y = 80 - lines * 7;
		char* p = wrapped;
		for (int l = 0; l < lines; ++l) {
			char* e = strchr(p, '\n');
			int n = e ? e - p : (int)strlen(p);
			char tmp[200];
			memcpy(tmp, p, n);
			tmp[n] = 0;
			ui_text_center(0, y + l * 14, 240, tmp, UC_DARK_TEXT, UC_DARK_OUT);
			if (!e) break;
			p = e + 1;
		}
		// fundido de entrada del texto: brillo del BG0 (aprox. con BLDY sobre todo)
		for (int f = 16; f >= 0; f -= 1) { fade_set(f, 0); ui_wait(); if (key_hit(KEY_B) || key_hit(KEY_START)) skip = 1; }
		int t = 0;
		while (t < 204 && !skip) {
			ui_wait();
			t++;
			if (key_hit(KEY_A)) break;
			if (key_hit(KEY_B) || key_hit(KEY_START)) skip = 1;
		}
		for (int f = 0; f <= 16; f += 1) { fade_set(f, 0); ui_wait(); }
	}
	music_stop(1.2f);
	fade_set(16, 0);
	ui_clear();
	strcpy(G.current_map, "bedroom");
	strcpy(G.current_spawn, "start");
	scene_request(SC_WORLD);
}
