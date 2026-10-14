// Vaelmoor GBA — demo: título, bucle de mapa y escenas.
#include "game.h"
#include <string.h>

u16 g_keys, g_keys_prev;
u8 g_flags[F_COUNT];
Member g_party[M_COUNT];
s16 g_level = 1;
s32 g_xp;
s16 g_potions = 1;
u8 g_class;
char g_name[16] = "Aleix";
int g_scene;
int g_cutscene;
void (*g_draw)(void);

void frame_wait(void) {
	vsync();
	if (g_draw) g_draw();
	else if (g_scene == SCENE_MAP) { map_update(); map_draw(); }
	keys_poll();
	text_update();
}

static int fading;
void fade_out(void) {
	int c = g_cutscene; g_cutscene = 1; fading = 1;
	REG_BLDCNT = 0x00FF;
	for (int i = 0; i <= 16; i += 2) { REG_BLDY = i; frame_wait(); }
	g_cutscene = c;
}
void fade_in(void) {
	int c = g_cutscene; g_cutscene = 1;
	REG_BLDCNT = 0x00FF;
	for (int i = 16; i >= 0; i -= 2) { REG_BLDY = i; frame_wait(); }
	REG_BLDCNT = 0;
	fading = 0;
	g_cutscene = c;
}

void party_recalc(void) {
	int L = g_level - 1;
	static const char* nk = "Kaelen";
	static const char* ny = "Yara";
	struct { s16 hp, mp, atk, def, mag, res, spd; } base[M_COUNT] = {
		{62, 16, 12, 6, 8, 5, 11}, {72, 12, 13, 7, 4, 4, 10}, {54, 28, 7, 5, 12, 8, 12}};
	if (g_class == 0) { base[0].hp += 10; base[0].atk += 3; base[0].def += 2; }
	if (g_class == 1) { base[0].mp += 8; base[0].mag += 5; base[0].hp -= 6; }
	if (g_class == 2) { base[0].mp += 6; base[0].mag += 3; base[0].res += 3; }
	for (int m = 0; m < M_COUNT; ++m) {
		Member* p = &g_party[m];
		int oh = p->maxhp, om = p->maxmp;
		p->maxhp = base[m].hp + L * 9; p->maxmp = base[m].mp + L * 3;
		p->atk = base[m].atk + L * 2; p->def = base[m].def + L; p->mag = base[m].mag + L * 2;
		p->res = base[m].res + L; p->spd = base[m].spd + L / 2;
		p->hp += p->maxhp - oh; p->mp += p->maxmp - om;
		if (p->hp > p->maxhp || oh == 0) p->hp = p->maxhp;
		if (p->mp > p->maxmp || om == 0) p->mp = p->maxmp;
	}
	g_party[M_PLAYER].name = g_name;
	g_party[M_KAELEN].name = nk;
	g_party[M_YARA].name = ny;
}

static void irq_init(void) {
	REG_DISPSTAT = 0x0008;   // IRQ de VBlank
	REG_IE = 0x0001;
	REG_IME = 1;
}

static void new_game(void) {
	memset(g_flags, 0, sizeof g_flags);
	memset(g_party, 0, sizeof g_party);
	g_level = 1; g_xp = 0; g_potions = 1; g_class = 0;
	g_party[M_PLAYER].in_party = 1;
	party_recalc();
}

static int title_screen(void) {
	g_scene = SCENE_TITLE;
	g_draw = 0;
	REG_DISPCNT = DCNT_BLANK;
	sprites_hide_all();
	text_init();
	bg_image_load(&g_img_battle_forest, 1);
	text_window(4, 3, 22, 5);
	text_draw_px(4 * 8 + 54, 3 * 8 + 8, "V A E L M O O R", 5);
	text_draw_px(4 * 8 + 34, 3 * 8 + 22, "Las Raíces del Pacto", 3);
	REG_DISPCNT = DCNT_MODE0 | DCNT_OBJ1D | DCNT_BG0 | DCNT_BG1;
	fade_in();
	u16 t = 0;
	for (;;) {
		frame_wait();
		t++;
		if ((t & 31) == 0) {
			text_window(9, 15, 12, 3);
			if (t & 32) text_draw_px(9 * 8 + 14, 15 * 8 + 6, "Pulsa START", 3);
		}
		if (key_hit(KEY_START) || key_hit(KEY_A)) break;
	}
	text_hide();
	static const char* const g[] = {"Chico", "Chica"};
	text_window(0, 0, 30, 3);
	text_draw_px(8, 6, "¿Quién eres?", 3);
	text_choice(g, 2);
	int r;
	while ((r = text_choice_result()) < 0) frame_wait();
	static const char* const nm[2][4] = {{"Aleix", "Aren", "Pau", "Joan"}, {"Nora", "Laia", "Aina", "Marta"}};
	text_hide();
	text_window(0, 0, 30, 3);
	text_draw_px(8, 6, "¿Cómo te llamas?", 3);
	text_choice(nm[r], 4);
	int n;
	while ((n = text_choice_result()) < 0) frame_wait();
	text_hide();
	new_game();
	FLAG(F_FEMALE) = r;
	strcpy(g_name, nm[r][n]);
	fade_out();
	return 1;
}

static void demo_end(void) {
	fade_out();
	REG_DISPCNT = DCNT_BLANK;
	sprites_hide_all();
	text_init();
	for (int i = 0; i < 32 * 32; ++i) SCREENBLOCK(28)[i] = 0;
	PAL_BG[0] = 0;
	text_window(2, 4, 26, 11);
	text_draw_px(2 * 8 + 62, 4 * 8 + 8, "FIN DE LA DEMO", 5);
	text_draw_px(2 * 8 + 10, 4 * 8 + 28, "El bosque está enfermando...", 3);
	text_draw_px(2 * 8 + 10, 4 * 8 + 42, "La aventura continuará en", 3);
	text_draw_px(2 * 8 + 10, 4 * 8 + 56, "la versión completa.", 3);
	text_draw_px(2 * 8 + 10, 4 * 8 + 72, "Gracias por jugar.", 6);
	REG_DISPCNT = DCNT_MODE0 | DCNT_BG0;
	g_scene = SCENE_END;
	fade_in();
	for (int i = 0; i < 60; ++i) frame_wait();
	while (!key_hit(KEY_START) && !key_hit(KEY_A)) frame_wait();
}

int main(void) {
	REG_WAITCNT = 0x4317;
	irq_init();
	REG_BLDCNT = 0x00FF; REG_BLDY = 16;
	for (;;) {
		title_screen();
		text_init();
		sprites_hide_all();
		memset(g_ent, 0, sizeof g_ent);
		g_ent[0].active = g_ent[0].visible = 1;
		g_ent[0].sheet = FLAG(F_FEMALE) ? &g_spr_player_f : &g_spr_player_m;
		g_ent[0].id = "player";
		g_scene = SCENE_MAP;
		map_load(MAP_BEDROOM, "start");
		fade_in();
		while (g_scene != SCENE_END) {
			frame_wait();
			story_run_pending();
		}
		demo_end();
	}
}
