// Combate con barras de tiempo activo (port de Battle.gd y Battler.gd).
#include "world.h"
#include "ui.h"
#include "audio.h"
#include "scene.h"
#include "particles.h"
#include "dialogue.h"
#include <stdio.h>

#define ATB_RATE 1.15f
#define MAXB 10
#define VOFS 18          // desplazamiento vertical del fondo (320x180 -> 240x160)
#define HOFS 40

typedef struct {
	u8 used, is_enemy, frames_kind;   // frames_kind: 0 normal (2 fotogramas), 1 humanoide (9 poses)
	char id[20];
	const char* key;
	const char* name;
	int member;                       // -1 enemigo
	const EnemyDef* d;
	float st[ST_N];                   // max_stats
	s32 hp, mp;
	u8 status[S_COUNT];               // turnos restantes (0 = sin estado)
	u8 sorder[S_COUNT];
	u8 nstatus;
	float atb;
	float x, y, hx, hy;               // posición y casa (pantalla GBA)
	float anim_t, pose_t;
	int pose;
	int turns;
	u8 flag_enraged, flag_taunt, flag_weakened, flag_snack, flag_phase2, flag_stolen;
	u8 victory;
	// vídeo
	const MetaSprite* ms;
	const Composed* comp;
	int tile, pal, frame_up;
	int flash, flash_kind;            // fotogramas de destello
	float kb;                         // retroceso del sprite (x)
	float alpha, sx, sy;              // muerte: desvanecer y aplastar
	float shown_hp;                   // barra animada
	float grow;                       // Aleixolo engorda
} Battler;

EWRAM_BSS static Battler B[MAXB];
static Battler* party[4];
static int nparty;
static Battler* enemies[MAXB];
static int nen;
static int over, busy;
static float t_total;
static Battler* actor;
static char top_text[160];
static int top_on;
static int state;                     // 0 idle, 1 busy, 2 command, 3 sub, 4 target, 5 end
enum { ST_IDLE, ST_BUSY, ST_COMMAND, ST_SUB, ST_TARGET, ST_END };
EWRAM_BSS static MemberState snapshot[M_COUNT];
static float shake_amt;
static int shake_n;
static int enemy_slot_count;

// ------------------------------------------------------------------ utilidades
static int alive(Battler* b) { return b && b->used && b->hp > 0; }
static int max_hp(Battler* b) { return (int)b->st[ST_HP]; }
static int max_mp(Battler* b) { return (int)b->st[ST_MP]; }
static int has_st(Battler* b, int s) { return b->status[s] > 0; }
static void set_st(Battler* b, int s, int turns) {
	if (!b->status[s]) b->sorder[b->nstatus++] = s;
	b->status[s] = turns;
}
static void erase_st(Battler* b, int s) {
	if (!b->status[s]) return;
	b->status[s] = 0;
	int j = 0;
	for (int i = 0; i < b->nstatus; ++i) if (b->sorder[i] != s) b->sorder[j++] = b->sorder[i];
	b->nstatus = j;
}
static void clear_st(Battler* b) { memset(b->status, 0, sizeof b->status); b->nstatus = 0; }

static float stat(Battler* b, int k) {
	float v = b->st[k];
	if (k == ST_ATK) { if (has_st(b, S_RAGE)) v *= 1.35f; if (has_st(b, S_WEAK)) v *= 0.7f; }
	if (k == ST_DEF) { if (has_st(b, S_PROTECT)) v *= 1.5f; if (has_st(b, S_RAGE)) v *= 0.8f; }
	if (k == ST_RES && has_st(b, S_PROTECT)) v *= 1.3f;
	if (k == ST_MAG && has_st(b, S_WEAK)) v *= 0.8f;
	return v;
}

static int is_player(Battler* b) { return !b->is_enemy && b->member == M_PLAYER; }

static void b_wait(float s) { wait_sec(s); }

static int alive_list(Battler** src, int n, Battler** out) {
	int k = 0;
	for (int i = 0; i < n; ++i) if (alive(src[i])) out[k++] = src[i];
	return k;
}

// ------------------------------------------------------------------ números flotantes (sprites de texto)
#define NFLOAT 8
typedef struct { int on, t, tile; float x, y0; int big; u32 tiles[16 * 8]; } Float;
EWRAM_BSS static Float floats[NFLOAT];
static u16 float_pal[16];
static int float_palslot = -1;
enum { FC_WHITE = 1, FC_CRIT, FC_PDMG, FC_HEAL, FC_MP, FC_WEAK, FC_GRAY, FC_STATUS, FC_REV, FC_CHOCO, FC_POISON, FC_OUT };

static void float_init(void) {
	static const u8 C[13][3] = {{0, 0, 0}, {255, 255, 255}, {255, 242, 140}, {255, 140, 128}, {140, 255, 140}, {153, 191, 255},
		{255, 179, 77}, {204, 204, 204}, {255, 230, 128}, {255, 255, 153}, {217, 153, 102}, {153, 255, 102}, {26, 18, 26}};
	for (int i = 0; i < 13; ++i) float_pal[i] = (C[i][0] >> 3) | ((C[i][1] >> 3) << 5) | ((C[i][2] >> 3) << 10);
	memset(floats, 0, sizeof floats);
	for (int i = 0; i < NFLOAT; ++i) floats[i].tile = -1;
}

static void float_text(Battler* b, const char* text, int color, int big, int dy) {
	for (int i = 0; i < NFLOAT; ++i) {
		Float* f = &floats[i];
		if (f->on) continue;
		memset(f->tiles, 0, sizeof f->tiles);
		int w = text_to_tiles(f->tiles, 8, 2, 1, 2, text, color, FC_OUT);
		if (f->tile < 0) f->tile = objt_alloc(16);
		if (f->tile < 0) return;
		// 2 piezas de 32x16 en orden 1D
		for (int pc = 0; pc < 2; ++pc)
			for (int r = 0; r < 2; ++r)
				for (int c = 0; c < 4; ++c) {
					const u32* s = f->tiles + (r * 8 + pc * 4 + c) * 8;
					volatile u32* d = objt_ptr(f->tile + pc * 8 + r * 4 + c);
					for (int k = 0; k < 8; ++k) d[k] = s[k];
				}
		float h = b->ms ? b->ms->h : 32;
		f->x = b->x - (w + 2) / 2.0f;
		f->y0 = b->y - h * 0.5f - 6 + dy / 2.0f;
		f->t = 0;
		f->big = big;
		f->on = 1;
		return;
	}
}

static void floats_draw(void) {
	if (float_palslot < 0) float_palslot = objpal_get(float_pal, 256, 256, 256, 0);
	for (int i = 0; i < NFLOAT; ++i) {
		Float* f = &floats[i];
		if (!f->on) continue;
		f->t++;
		int t = f->t;
		float y;
		if (t < 10) y = f->y0 - 7 * t / 10.0f;
		else if (t < 18) y = f->y0 - 7 + 4 * (t - 10) / 8.0f;
		else if (t < 33) y = f->y0 - 3;
		else if (t < 63) y = f->y0 - 3 - 7 * (t - 33) / 30.0f;
		else { f->on = 0; continue; }
		if (t > 45 && (t & 1)) continue;       // parpadeo al desvanecer
		int x = (int)f->x, yy = (int)y;
		for (int pc = 0; pc < 2; ++pc)
			oam_push(0x00002000 + i * 2 + pc, obj_a0(yy, OBJ_WIDE, 0), obj_a1(x + pc * 32, 2, 0), obj_a2(f->tile + pc * 8, 0, float_palslot));
	}
}

// ------------------------------------------------------------------ interfaz
#define CMD_X 0
#define CMD_Y 12
static int cmd_index, sub_index, sub_scroll, sub_kind, target_index;
static int target_mode;
static Battler* target_list[MAXB];
static int ntarget;
static s16 sub_entries[24];
static int nsub;
static int pending_type, pending_id;      // 0 ataque, 1 habilidad, 2 objeto
static int cmd_dis[5];
static int bars_dirty, rows_dirty, ui_dirty = 1, cmd_visible, sub_visible, enemy_panel_visible = 1;
static int auto_battle;

static const char* const CMD_NAMES[5] = {"Atacar", "Habilidades", "Objetos", "Defender", "Huir"};
static const char* const CMD_ICONS[5] = {"cmd_attack", "cmd_skill", "cmd_item", "cmd_defend", "cmd_flee"};

static int win_bg(void) {
	const NineSrc* n = &g_nine[NP_WINDOW];
	return n->px[(n->h - 8) * n->w + n->w / 2];
}

static void draw_top(void) {
	ui_clear_rect(0, 0, 30, 3);
	if (!top_on) return;
	ui_panel(NP_WINDOW, PAL_UI_ALT, 0, 0, 30, 3);
	char buf[160];
	strncpy(buf, top_text, 159);
	buf[159] = 0;
	// recorte a una línea
	int n = strlen(buf);
	while (n > 0 && text_width_n(buf, n) > 224) { n--; while (n > 0 && (buf[n] & 0xC0) == 0x80) n--; }
	buf[n] = 0;
	ui_text_center(0, 6, 240, buf, UC_WIN_TEXT, UC_WIN_OUT);
}

static void show_top_now(const char* t) {
	strncpy(top_text, dfmt(t), 159);
	top_on = 1;
	draw_top();
}

static void show_top(const char* t, float hold) {
	show_top_now(t);
	b_wait(hold);
	if (state == ST_IDLE || state == ST_BUSY) { top_on = 0; draw_top(); }
}

static void hp_color_for(Battler* b, int* txt) {
	float r = (float)b->hp / max_hp(b);
	*txt = r <= 0.25f ? UC_WIN_LOW : (r <= 0.5f ? UC_WIN_HI : UC_WIN_TEXT);
}

static void draw_bar(int x, int y, int w, int h, float v, float mx, int col) {
	ui_fill_px(x, y, w, h, UC_WIN_OUT);
	int fw = mx > 0 ? (int)((w - 2) * (v < 0 ? 0 : v) / mx) : 0;
	if (fw > w - 2) fw = w - 2;
	if (fw > 0) ui_fill_px(x + 1, y + 1, fw, h - 2, col);
}

static int party_bg = -1;
static void draw_party_row(int i) {
	{
		Battler* b = party[i];
		int y = 13 * 8 + 5 + i * 12;
		ui_fill_px(76, y - 1, 126, 13, party_bg);
		int nc = b == actor ? UC_WIN_HI : (!alive(b) ? UC_WIN_DIS : UC_WIN_TEXT);
		char nm[16];
		strncpy(nm, b->name, 15);
		nm[15] = 0;
		int n = strlen(nm);
		while (n > 0 && text_width_n(nm, n) > 44) n--;
		nm[n] = 0;
		ui_text(76, y, nm, nc, UC_WIN_OUT);
		int hc;
		hp_color_for(b, &hc);
		ui_text_right(120, y - 1, 44, sfmt("%ld/%d", (long)b->hp, max_hp(b)), hc, UC_WIN_OUT);
		float r = (float)b->shown_hp / max_hp(b);
		draw_bar(122, y + 9, 42, 3, b->shown_hp, max_hp(b), r <= 0.25f ? UC_WIN_LOW : (r <= 0.5f ? UC_WIN_HI : UC_WIN_HP));
		ui_text_right(166, y - 1, 34, sfmt("%ld/%d", (long)b->mp, max_mp(b)), UC_WIN_TEXT, UC_WIN_OUT);
		draw_bar(168, y + 9, 32, 3, b->mp, max_mp(b) > 0 ? max_mp(b) : 1, UC_WIN_MP);
	}
}

static void draw_party_panel(void) {
	ui_panel(NP_WINDOW, PAL_UI_ALT, 9, 13, 21, 7);
	if (party_bg < 0) party_bg = win_bg();
	for (int i = 0; i < nparty; ++i) draw_party_row(i);
}

static void draw_hp_bars(void) {
	for (int i = 0; i < nparty; ++i) {
		Battler* b = party[i];
		int y = 13 * 8 + 5 + i * 12;
		float r = (float)b->shown_hp / max_hp(b);
		draw_bar(122, y + 9, 42, 3, b->shown_hp, max_hp(b), r <= 0.25f ? UC_WIN_LOW : (r <= 0.5f ? UC_WIN_HI : UC_WIN_HP));
	}
}

static void draw_atb(void) {
	for (int i = 0; i < nparty; ++i) {
		Battler* b = party[i];
		int y = 13 * 8 + 5 + i * 12;
		int full = b->atb >= 100 && alive(b);
		int col = full ? (((int)(t_total * 10) & 1) ? UC_WIN_HI : UC_WIN_TEXT) : UC_WIN_HI;
		draw_bar(204, y + 3, 30, 5, alive(b) ? b->atb : 0, 100, col);
	}
}

static void draw_enemy_panel(void) {
	ui_panel(NP_WINDOW, PAL_UI_ALT, 0, 13, 9, 7);
	// nombres agrupados
	const char* names[8];
	int counts[8], nn = 0;
	for (int i = 0; i < nen; ++i) {
		if (!alive(enemies[i])) continue;
		int k;
		for (k = 0; k < nn; ++k) if (!strcmp(names[k], enemies[i]->name)) break;
		if (k == nn) { names[nn] = enemies[i]->name; counts[nn++] = 0; }
		counts[k]++;
	}
	for (int k = 0; k < nn && k < 4; ++k) {
		char nm[40];
		snprintf(nm, sizeof nm, "%s", names[k]);
		int lim = counts[k] > 1 ? 52 : 64;
		int n = strlen(nm);
		while (n > 0 && text_width_n(nm, n) > lim) n--;
		nm[n] = 0;
		ui_text(5, 13 * 8 + 5 + k * 12, nm, UC_WIN_TEXT, UC_WIN_OUT);
		if (counts[k] > 1) ui_text_right(0, 13 * 8 + 5 + k * 12, 68, sfmt("×%d", counts[k]), UC_WIN_HI, UC_WIN_OUT);
	}
}

static void draw_cmd_panel(void) {
	ui_panel(NP_WINDOW, PAL_UI_ALT, CMD_X, CMD_Y, 9, 8);
	for (int k = 0; k < 5; ++k) {
		int y = CMD_Y * 8 + 5 + k * 12;
		int col = cmd_dis[k] ? UC_WIN_DIS : (k == cmd_index ? UC_WIN_HI : UC_WIN_TEXT);
		ui_text(22, y, CMD_NAMES[k], col, UC_WIN_OUT);
	}
}

static void draw_sub_panel(void) {
	ui_panel(NP_WINDOW, PAL_UI_ALT, 8, 3, 22, 10);
	for (int k = 0; k < 6; ++k) {
		int idx = sub_scroll + k;
		if (idx >= nsub) break;
		int y = 3 * 8 + 6 + k * 12;
		int id = sub_entries[idx];
		int color = UC_WIN_TEXT;
		const char* name;
		const char* cost;
		if (sub_kind == 0) {
			const SkillDef* sk = &g_skills[id];
			name = sk->name;
			cost = sfmt("%d PM", sk->mp);
			if (sk->mp > actor->mp) color = UC_WIN_DIS;
		} else {
			name = g_items[id].name;
			cost = sfmt("×%d", item_count_k(id));
		}
		if (idx == sub_index && color != UC_WIN_DIS) color = UC_WIN_HI;
		ui_text(84, y, name, color, UC_WIN_OUT);
		ui_text_right(84, y, 148, cost, UC_WIN_MP, UC_WIN_OUT);
	}
}

static const char* el_icon(int el) {
	static const char* const E[8] = {"", "el_fire", "el_ice", "el_bolt", "el_light", "el_dark", "el_nature", "el_phys"};
	return E[el];
}

static void redraw_ui(void) {
	ui_clear_rect(0, 3, 30, 17);
	if (cmd_visible) draw_cmd_panel();
	else if (enemy_panel_visible) draw_enemy_panel();
	draw_party_panel();
	draw_atb();
	if (sub_visible) draw_sub_panel();
	ui_dirty = 0;
}

// ------------------------------------------------------------------ dibujo de combatientes
static int hand_x[6], hand_y[6], hand_flip[6], nhands;
static int bg_shake_x, bg_shake_y;

static void battler_draw(Battler* b, int idx) {
	if (!b->used) return;
	if (b->is_enemy && b->alpha <= 0.02f) return;
	int sx = bg_shake_x, sy = bg_shake_y;
	if (b->is_enemy) {
		const MetaSprite* ms = b->ms;
		int fr;
		if (b->frames_kind == 1) fr = b->pose_t > 0 ? b->pose : ((int)(b->anim_t * 1.6f) & 1);
		else fr = alive(b) && ms->nframes > 1 ? ((int)(b->anim_t * 2.0f) % ms->nframes) : 0;
		if (fr >= ms->nframes) fr = 0;
		if (b->tile < 0) b->tile = objt_alloc(ms->frame_tiles);
		if (b->frame_up != fr) { ms_upload(ms, fr, b->tile); b->frame_up = fr; }
		// paleta: destello blanco o tinte
		int tr = 256, tg = 256, tb = 256;
		if (b->d->has_tint) { tr = (int)(b->d->tint[0] * 256); tg = (int)(b->d->tint[1] * 256); tb = (int)(b->d->tint[2] * 256); }
		if (b->flag_weakened) { tr = tr * 205 / 256; tg = tg * 218 / 256; }
		if (b->flash && ((b->flash / 3) & 1)) { tr = tg = tb = 1024; }
		if (!alive(b)) { tr = tr * 256 / 256; tg = tg * 102 / 256; }
		if (b->pal >= 0) objpal_put(b->pal);
		b->pal = objpal_get(ms->pal, tr, tg, tb, 0);
		int semi = b->alpha < 0.99f;
		float scx = b->sx * b->grow, scy = b->sy * b->grow;
		ms_draw_scaled(ms, b->tile, b->pal, (int)b->x + sx, (int)b->y + sy, 2, 0x20000000 + 0x100 - idx, b->frames_kind == 1, semi,
			(int)(scx * 256), (int)(scy * 256));
		if (semi) alpha_set((int)(b->alpha * 16), 16 - (int)(b->alpha * 16));
		return;
	}
	// grupo: hoja de 9 poses
	const Composed* c = b->comp;
	int fr;
	if (b->pose_t > 0) fr = b->pose;
	else if (!alive(b)) fr = 7;
	else if (b->victory) fr = 8;
	else if ((float)b->hp / max_hp(b) <= 0.25f) fr = 6;
	else fr = (int)(b->anim_t * 1.6f) & 1;
	if (b->tile < 0) b->tile = objt_alloc(16);
	if (b->frame_up != fr) { dma_copy32(objt_ptr(b->tile), c->tiles + fr * 16 * 8, 128); b->frame_up = fr; }
	int tr = 256, tg = 256, tb = 256;
	if (!alive(b)) { tr = 192; tg = 179; tb = 205; }
	else if (has_st(b, S_CHOCO)) { tr = 141; tg = 90; tb = 51; }
	else if (is_player(b) && flag("player_dark")) { tr = 200; tg = 159; tb = 230; }
	if (b->flash && ((b->flash / 3) & 1)) {
		if (b->flash_kind == 1) { tr = 512; tg = 512; tb = 256; }       // curación/subida
		else if (b->flash_kind == 2) { tr = 384; tg = 384; tb = 768; }  // defensa
		else { tr = 768; tg = 154; tb = 154; }                           // daño
	}
	if (b->pal >= 0) objpal_put(b->pal);
	b->pal = objpal_get(c->pal, tr, tg, tb, 0);
	int x = (int)(b->x + b->kb) - 16 + sx, y = (int)b->y - 16 + sy;
	oam_push(0x20000000 + 0x200 + idx, obj_a0(y, OBJ_SQUARE, 0), obj_a1(x, 2, 0), obj_a2(b->tile, 2, b->pal));
}

static int enemy_half_w(Battler* b) { return b->ms ? b->ms->w / 2 : 16; }

// marcador del turno: triángulo dorado con contorno, 8x8
static int turn_tile = -1, turn_pal = -1;
static void turn_marker_init(void) {
	static const char* const ROWS[8] = {
		"1111111.", "1222221.", "1233321.", ".12321..", ".12221..", "..121...", "..111...", "...1....",
	};
	static u16 pal[16];
	pal[1] = (3) | (2 << 5) | (4 << 10);            // contorno
	pal[2] = (31) | (24 << 5) | (6 << 10);          // dorado
	pal[3] = (31) | (31 << 5) | (22 << 10);         // brillo
	turn_tile = objt_alloc(1);
	if (turn_tile < 0) return;
	volatile u32* d = objt_ptr(turn_tile);
	for (int y = 0; y < 8; ++y) {
		u32 row = 0;
		for (int x = 0; x < 8; ++x) {
			char c = ROWS[y][x];
			if (c >= '1' && c <= '3') row |= (u32)(c - '0') << (x * 4);
		}
		d[y] = row;
	}
	turn_pal = objpal_get(pal, 256, 256, 256, 0);
}

static void battle_render(void) {
	t_total += 1.0f / 60;
	if (shake_n > 0) {
		shake_n--;
		bg_shake_x = shake_n ? randi(-(int)shake_amt, (int)shake_amt) : 0;
		bg_shake_y = shake_n ? randi(-(int)shake_amt, (int)shake_amt) : 0;
	}
	for (int i = 0; i < MAXB; ++i) {
		Battler* b = &B[i];
		if (!b->used) continue;
		b->anim_t += 1.0f / 60;
		if (b->pose_t > 0) { b->pose_t -= 1.0f / 60; if (b->pose_t <= 0) b->pose_t = 0; }
		if (b->flash) b->flash--;
		if (b->kb != 0) { b->kb *= 0.7f; if (b->kb < 0.3f && b->kb > -0.3f) b->kb = 0; }
		if (b->shown_hp != b->hp) {
			float d = b->hp - b->shown_hp;
			b->shown_hp += d * 0.2f;
			if (d < 1 && d > -1) b->shown_hp = b->hp;
			bars_dirty = 1;
		}
	}
	oam_begin();
	alpha_set(16, 0);
	for (int i = 0; i < MAXB; ++i) battler_draw(&B[i], i);
	// manos y flecha de turno
	int bob = (int)(sinf_fast(t_total * 8) * 2);
	for (int i = 0; i < nhands; ++i) ui_cursor(hand_x[i] + (hand_flip[i] ? -bob : bob), hand_y[i], CUR_HAND, hand_flip[i]);
	if (actor && (state == ST_COMMAND || state == ST_SUB || state == ST_TARGET))
		if (turn_tile >= 0) {
			int ay = (int)actor->y - 22 + (int)(sinf_fast(t_total * 5) * 1.5f);
			oam_push(0x00000100, obj_a0(ay, OBJ_SQUARE, 0), obj_a1((int)actor->x - 4, 0, 0), obj_a2(turn_tile, 0, turn_pal));
		}
	floats_draw();
	particles_update();
	particles_render(0, 0, 0x10000000);
	ui_frame_sprites();
	oam_end();
	g_vid.hofs[1] = HOFS - bg_shake_x;
	g_vid.vofs[1] = VOFS - bg_shake_y;
	if (ui_dirty) redraw_ui();
	else {
		if (rows_dirty) { for (int i = 0; i < nparty; ++i) draw_party_row(i); rows_dirty = 0; }
		if (bars_dirty) { draw_hp_bars(); bars_dirty = 0; }
		if ((g_frame & 3) == 0) draw_atb();
	}
}

static void hook(void) { battle_render(); }

// ------------------------------------------------------------------ efectos
static void flash(Battler* b, int kind) { b->flash = 12; b->flash_kind = kind; }
static void do_shake(float a) { shake_amt = a / 2; shake_n = 7; }

static void particles_at(float x, float y, int r, int g, int bcol, int amount, float radius, float gy, float speed, float life, float size) {
	ParticleDef d;
	memset(&d, 0, sizeof d);
	d.amount = amount / 2 > 2 ? amount / 2 : 2;
	d.one_shot = 1; d.explosiveness = 0.85f; d.lifetime = life; d.radius = radius / 2;
	d.dir_x = 0; d.dir_y = -1; d.spread = 180; d.gy = gy / 2; d.vmin = speed * 0.15f; d.vmax = speed / 2;
	d.size = size; d.r = r; d.g = g; d.b = bcol;
	particles_emit(&d, x, y, 1);
}

static void effect(Battler* t, int r, int g, int b) { particles_at(t->x, t->y, r, g, b, 24, 14, -40, 60, 0.7f, 2); }

static void ring(float x, float y, int r, int g, int b, float r1) {
	ParticleDef d;
	memset(&d, 0, sizeof d);
	d.amount = 10; d.one_shot = 1; d.explosiveness = 1; d.lifetime = 0.35f; d.radius = 1;
	d.dir_x = 1; d.dir_y = 0; d.spread = 180; d.vmin = r1 * 1.4f; d.vmax = r1 * 1.6f; d.size = 1; d.r = r; d.g = g; d.b = b;
	particles_emit(&d, x, y, 1);
}

static void spell_fx(Battler* t, int el) {
	float x = t->x, y = t->y;
	switch (el) {
	case EL_FIRE:
		particles_at(x, y + 5, 255, 140, 38, 40, 16, -120, 50, 0.7f, 3);
		particles_at(x, y, 255, 230, 102, 16, 8, -60, 30, 0.5f, 2);
		ring(x, y + 7, 255, 153, 51, 18);
		flash_white(0.18f);
		break;
	case EL_ICE:
		particles_at(x, y - 20, 191, 242, 255, 18, 22, 160, 20, 0.5f, 2);
		particles_at(x, y, 179, 242, 255, 24, 18, 30, 40, 0.8f, 2);
		flash_white(0.15f);
		break;
	case EL_BOLT:
		particles_at(x, y - 30, 255, 255, 191, 12, 4, 300, 10, 0.25f, 2);
		particles_at(x, y, 255, 255, 153, 20, 6, 0, 90, 0.4f, 2);
		flash_white(0.15f);
		break;
	case EL_LIGHT:
		particles_at(x, y - 24, 255, 247, 191, 20, 8, 120, 10, 0.45f, 3);
		particles_at(x, y, 255, 255, 204, 30, 16, -30, 40, 0.8f, 2);
		break;
	case EL_DARK:
		particles_at(x, y, 153, 64, 217, 40, 30, 0, 10, 0.6f, 3);
		ring(x, y, 179, 77, 230, 20);
		do_shake(3);
		break;
	case EL_NATURE:
		particles_at(x, y + 8, 128, 242, 102, 30, 20, -50, 40, 0.9f, 3);
		ring(x, y + 8, 128, 230, 102, 15);
		break;
	case 100:   // curación
		particles_at(x, y + 10, 153, 255, 166, 28, 16, -70, 20, 0.9f, 2);
		ring(x, y + 12, 153, 255, 153, 13);
		break;
	default:
		particles_at(x, y, 255, 255, 255, 18, 12, -30, 50, 0.7f, 2);
	}
}

static void cast_pose(Battler* b) {
	if (!b->is_enemy) { b->pose = 4; b->pose_t = 0.6f; effect(b, 255, 255, 204); }
}

static void slash(float x, float y) { particles_at(x, y, 255, 255, 230, 10, 10, 0, 60, 0.18f, 2); }

static void lunge(Battler* b) {
	float dir = b->is_enemy ? 1 : -1;
	int humanoid = !b->is_enemy || b->frames_kind == 1;
	if (humanoid) { b->pose = 2; b->pose_t = 0.2f; }
	float x0 = b->x, x1 = b->hx + dir * 22;
	for (int i = 1; i <= 8; ++i) { float k = i / 8.0f; k = k * (2 - k); b->x = x0 + (x1 - x0) * k; frame_wait(); }
	if (humanoid) { b->pose = 3; b->pose_t = 0.35f; slash(b->x + dir * 17, b->y); }
}

static void return_home(Battler* b) {
	float x0 = b->x, y0 = b->y;
	for (int i = 1; i <= 9; ++i) { float k = i / 9.0f; b->x = x0 + (b->hx - x0) * k; b->y = y0 + (b->hy - y0) * k; frame_wait(); }
	b->x = b->hx; b->y = b->hy;
}

static void on_death(Battler* t);
static void refresh(void) { rows_dirty = 1; }
static void relayout(void) { ui_dirty = 1; }

static void heal(Battler* t, int amount) {
	t->hp = MIN(max_hp(t), t->hp + amount);
	sfx("heal", -6, 1);
	flash(t, 1);
	float_text(t, sfmt("+%d", amount), FC_HEAL, 0, 0);
	relayout();
}

static void cure(Battler* t) { erase_st(t, S_POISON); erase_st(t, S_STUN); erase_st(t, S_WEAK); refresh(); }

static void damage(Battler* t, int amount, int crit, int weak) {
	t->hp = t->hp - amount < 0 ? 0 : t->hp - amount;
	if (!t->is_enemy) { t->pose = 5; t->pose_t = 0.4f; t->kb = 3; }
	flash(t, 0);
	do_shake(crit ? 6.0f : 3.0f);
	float_text(t, sfmt("%d", amount), crit ? FC_CRIT : (!t->is_enemy ? FC_PDMG : FC_WHITE), crit, 0);
	if (weak) float_text(t, "¡Débil!", FC_WEAK, 0, -22);
	if (has_st(t, S_STUN) && randf() < 0.3f) erase_st(t, S_STUN);
	refresh();
	if (!alive(t)) on_death(t);
}

static void on_death(Battler* t) {
	clear_st(t);
	t->atb = 0;
	if (t->is_enemy) {
		sfx("dark", -8, 1.4f);
		for (int i = 0; i < 4; ++i) {}
		// desvanecer y aplastar (0,6 s) sin bloquear
		t->alpha = 0.99f;
	}
	refresh();
}

static float el_mult(Battler* t, int el) {
	if (!t->is_enemy) return 1;
	int weak = t->d->weak, res = t->d->resist;
	if (el && (weak >> el & 1)) return 1.5f;
	if (el && (res >> el & 1)) return 0.5f;
	if (!el && (res >> EL_PHYS & 1)) return 0.5f;
	return 1;
}

static void try_inflict(Battler* t, int st, float chance) {
	if (!alive(t)) return;
	if (st == S_POISON && is_player(t) && G.race == RACE_DWARF) { float_text(t, "Inmune", FC_GRAY, 0, 0); return; }
	if (t->is_enemy && t->d->boss && st == S_STUN) chance *= 0.3f;
	if (randf() < chance) {
		set_st(t, st, st == S_STUN ? 1 : (st == S_CHOCO ? 2 : 3));
		float_text(t, g_status[st].name, FC_STATUS, 0, 0);
		refresh();
	} else float_text(t, "Resiste", FC_GRAY, 0, 0);
	b_wait(0.15f);
}

typedef struct { int kind; float power; float crit; int inflict; float chance; float vs_poison; int has_vs; } HitSpec;

static void hit(Battler* user, Battler* target, const HitSpec* sk, int el) {
	float dmg;
	if (sk->kind == K_PHYS) dmg = stat(user, ST_ATK) * sk->power * 1.25f - stat(target, ST_DEF) * 0.6f;
	else dmg = stat(user, ST_MAG) * sk->power * 1.35f - stat(target, ST_RES) * 0.5f;
	if (sk->has_vs && has_st(target, S_POISON)) dmg *= sk->vs_poison;
	float mult = el_mult(target, el);
	dmg *= mult;
	int crit = 0;
	if (sk->kind == K_PHYS) {
		float cc = 0.05f + sk->crit;
		if (is_player(user)) { if (G.race == RACE_HUMAN) cc += 0.1f; cc += perk(PK_CRIT); }
		if (randf() < cc) { crit = 1; dmg *= 1.7f; }
	}
	if (is_player(target) && G.race == RACE_DWARF && sk->kind == K_PHYS) dmg *= 0.9f;
	if (is_player(user)) dmg *= 1.0f + perk(sk->kind == K_PHYS ? PK_DMG_PHYS : PK_DMG_MAG);
	if (is_player(target)) { float r = 1.0f - perk(PK_DMG_RED); dmg *= r > 0.5f ? r : 0.5f; }
	if (has_st(target, S_WALL)) dmg *= 0.5f;
	dmg *= randf_range(0.9f, 1.1f);
	int amount = dmg < 1 ? 1 : (int)dmg;
	if (sk->kind == K_PHYS) sfx(crit ? "crit" : "hit", -2, 1);
	else spell_fx(target, el);
	damage(target, amount, crit, mult > 1.0f);
	if (sk->kind == K_DRAIN) heal(user, amount / 2);
	else if (sk->kind == K_PHYS && is_player(user) && perk(PK_LIFESTEAL) > 0) {
		int h = (int)(amount * perk(PK_LIFESTEAL));
		heal(user, h < 1 ? 1 : h);
	}
	if (sk->inflict >= 0 && alive(target)) try_inflict(target, sk->inflict, sk->chance);
	b_wait(0.22f);
}

static HitSpec spec_of(const SkillDef* s) {
	HitSpec h = {s->kind, s->power, s->crit, s->inflict, s->chance, s->vs_poison, s->has_vs_poison};
	return h;
}

static void apply_dead_look(Battler* b) { if (!b->is_enemy) { b->kb = 0; b->frame_up = -1; } }

// ------------------------------------------------------------------ creación de combatientes
static const EnemyGfx* enemy_gfx(const char* key) {
	for (int i = 0; i < g_nenemy_gfx; ++i) if (!strcmp(g_enemy_gfx[i].key, key)) return &g_enemy_gfx[i];
	return 0;
}

static float abyss_mult(void) {
	if (!starts_with(g_enc.ret_map, "abismo")) return 1.0f;
	return 1.0f + 0.25f * meta_i("abyss_floor", 1);
}

static float fsqrt(float v) { return v <= 0 ? 0 : isqrt((u32)(v * 65536)) / 256.0f; }

#define VAEL_HP 18500
#define VAEL_ATK 58

static Battler* new_battler(void) {
	for (int i = 0; i < MAXB; ++i) if (!B[i].used) {
		memset(&B[i], 0, sizeof B[i]);
		B[i].used = 1; B[i].tile = -1; B[i].pal = -1; B[i].frame_up = -1; B[i].alpha = 1; B[i].sx = B[i].sy = 1; B[i].grow = 1;
		return &B[i];
	}
	return 0;
}

static Battler* spawn_enemy(const char* key, float px, float py) {
	int ei = db_enemy(key);
	if (ei < 0) return 0;
	const EnemyDef* d = &g_enemies[ei];
	Battler* b = new_battler();
	if (!b) return 0;
	b->is_enemy = 1;
	b->member = -1;
	b->d = d;
	b->key = d->key;
	snprintf(b->id, sizeof b->id, "%s%d", key, nen);
	b->name = d->name;
	float dm[3];
	diff_mults(dm);
	float ab = abyss_mult();
	b->st[ST_HP] = (int)(d->hp * dm[0] * ab);
	b->st[ST_MP] = 99;
	b->st[ST_ATK] = ab > 1 ? (int)(d->atk * dm[1] * fsqrt(ab) * 1.15f) : d->atk * dm[1];
	b->st[ST_DEF] = (int)(d->def * fsqrt(ab));
	b->st[ST_MAG] = ab > 1 ? (int)(d->mag * dm[1] * fsqrt(ab) * 1.15f) : d->mag * dm[1];
	b->st[ST_RES] = (int)(d->res * fsqrt(ab));
	b->st[ST_SPD] = d->spd + (ab > 1 ? meta_i("abyss_floor", 0) : 0);
	if (g_enc.vael && !strcmp(key, "nhalzur")) {
		b->st[ST_HP] = (int)(VAEL_HP * dm[0]);
		b->st[ST_ATK] = (int)(VAEL_ATK * dm[1]);
		b->st[ST_MAG] = (int)(VAEL_ATK * 0.92f * dm[1]);
		b->st[ST_DEF] = 42; b->st[ST_RES] = 30; b->st[ST_SPD] = 20;
		b->name = "Nhal'Zur, el Hambre asustada";
	}
	b->hp = max_hp(b);
	b->shown_hp = b->hp;
	// coordenadas de Godot (640x360) -> pantalla GBA
	b->hx = b->x = px / 2 - HOFS;
	b->hy = b->y = py / 2 - VOFS;
	b->atb = randf_range(0, 40);
	const EnemyGfx* g = enemy_gfx(key);
	b->ms = g ? g->ms : 0;
	b->frames_kind = d->frames == -1 ? 1 : 0;
	enemies[nen++] = b;
	return b;
}

static void weaken_aleixolo(Battler* e) {
	e->flag_weakened = 1;
	e->st[ST_ATK] *= 0.6f; e->st[ST_MAG] *= 0.6f; e->st[ST_DEF] *= 0.6f; e->st[ST_RES] *= 0.6f;
	set_st(e, S_STUN, 2);
}

static void build_scene(void) {
	memset(B, 0, sizeof B);
	nparty = nen = 0;
	static const float SLOTS[5][5][2] = {
		{{180, 168}},
		{{140, 130}, {220, 200}},
		{{110, 118}, {220, 150}, {130, 214}},
		{{100, 112}, {214, 118}, {110, 210}, {224, 214}},
		{{90, 110}, {200, 106}, {150, 164}, {96, 218}, {214, 220}},
	};
	int n = g_enc.n;
	int si = CLAMP(n, 1, 5) - 1;
	for (int i = 0; i < n && i < 5; ++i) {
		float px = SLOTS[si][i][0], py = SLOTS[si][i][1];
		if (n == 1) { px = g_enc.boss ? 170 : 180; py = g_enc.boss ? 160 : 168; }
		spawn_enemy(g_enc.enemies[i], px, py);
	}
	enemy_slot_count = n;
	if (g_enc.weaken_aleixolo)
		for (int i = 0; i < nen; ++i) if (!strcmp(enemies[i]->key, "aleixolo")) weaken_aleixolo(enemies[i]);
	static const float PARTY_POS[4][2] = {{500, 84}, {530, 126}, {500, 168}, {530, 210}};
	for (int i = 0; i < G.nparty && i < 4; ++i) {
		int m = G.party[i];
		Battler* b = new_battler();
		b->member = m;
		b->key = member_id(m);
		strcpy(b->id, member_id(m));
		b->name = member_name(m);
		Stats7 st = gs_stats(m);
		for (int k = 0; k < ST_N; ++k) b->st[k] = st.v[k];
		b->hp = G.members[m].hp;
		b->mp = G.members[m].mp;
		b->shown_hp = b->hp;
		b->hx = b->x = PARTY_POS[i][0] / 2 - HOFS;
		b->hy = b->y = PARTY_POS[i][1] / 2 - VOFS;
		b->atb = randf_range(20, 60) + b->st[ST_SPD] + (m == M_PLAYER ? perk(PK_ATB_START) : 0);
		b->comp = member_battle_sheet(m);
		party[nparty++] = b;
	}
}

// ------------------------------------------------------------------ estados por turno
static void finish_actor_turn(void);
static int check_end(void);

static int tick_statuses(Battler* b) {
	int skip = 0;
	b->turns++;
	if (!b->is_enemy && alive(b) && max_mp(b) > 0) {
		int regen = roundf_i(max_mp(b) * 0.05f);
		if (regen < 1) regen = 1;
		if (is_player(b)) regen += (int)perk(PK_MP_REGEN);
		if (is_player(b) && G.race == RACE_ELF) regen += 2;
		if (b->mp < max_mp(b)) b->mp = MIN(max_mp(b), b->mp + regen);
	}
	if (has_st(b, S_POISON)) {
		int d = (int)(max_hp(b) * 0.07f);
		if (d < 1) d = 1;
		b->hp = b->hp - d < 0 ? 0 : b->hp - d;
		float_text(b, sfmt("%d", d), FC_POISON, 0, 0);
		flash(b, 1);
		b_wait(0.35f);
		if (!alive(b)) on_death(b);
	}
	if (has_st(b, S_REGEN) && alive(b)) {
		int h = (int)(max_hp(b) * 0.08f);
		if (h < 1) h = 1;
		b->hp = MIN(max_hp(b), b->hp + h);
		float_text(b, sfmt("+%d", h), FC_HEAL, 0, 0);
		b_wait(0.3f);
	}
	if (has_st(b, S_STUN)) { skip = 1; show_top(sfmt("%s está aturdido.", b->name), 0.7f); }
	else if (has_st(b, S_CHOCO)) { skip = 1; show_top(sfmt("%s es una estatua de chocolate y no puede moverse.", b->name), 0.8f); }
	for (int i = b->nstatus - 1; i >= 0; --i) {
		int s = b->sorder[i];
		if (--b->status[s] <= 0) { b->status[s] = 1; erase_st(b, s); }
	}
	refresh();
	return skip;
}

// ------------------------------------------------------------------ acciones
static void do_skill(Battler* user, int sid, Battler** targets, int nt);

static void ranged_attack(Battler* user, Battler* target, int wk, const char* orb) {
	int magic = wk == WK_STAFF;
	user->pose = magic ? 4 : 3;
	user->pose_t = 0.5f;
	sfx(magic ? "magic" : "select", -8, magic ? 1.3f : 0.7f);
	// proyectil: partículas que viajan
	int r = 140, g = 217, bcol = 255;
	if (!strcmp(orb, "dark")) { r = 179; g = 89; bcol = 242; }
	else if (!strcmp(orb, "light")) { r = 255; g = 242; bcol = 153; }
	if (!magic) { r = 217; g = 191; bcol = 140; }
	float fx = user->x - 9, fy = user->y - 3, tx = target->x, ty = target->y;
	int n = magic ? 13 : 8;
	for (int i = 1; i <= n; ++i) {
		float k = (float)i / n;
		particles_at(fx + (tx - fx) * k, fy + (ty - fy) * k, r, g, bcol, 4, 2, 0, 5, 0.15f, magic ? 3 : 2);
		frame_wait();
	}
	if (alive(target)) {
		if (magic) {
			particles_at(tx, ty, r, g, bcol, 14, 8, -30, 40, 0.4f, 2);
			HitSpec h = {K_MAG, 0.85f, 0, -1, 0, 0, 0};
			int el = !strcmp(orb, "dark") ? EL_DARK : (!strcmp(orb, "light") ? EL_LIGHT : EL_NONE);
			hit(user, target, &h, el);
		} else {
			HitSpec h = {K_PHYS, 0.95f, 0, -1, 0, 0, 0};
			hit(user, target, &h, 0);
		}
	}
	user->pose_t = 0;
	apply_dead_look(user);
}

static void do_attack(Battler* user, Battler* target) {
	if (!user->is_enemy) {
		GearLook look = gear_look(user->member);
		if (look.weapon_kind == WK_STAFF || look.weapon_kind == WK_BOW) {
			ranged_attack(user, target, look.weapon_kind, look.orb ? look.orb : "");
			return;
		}
	}
	lunge(user);
	if (alive(target)) { HitSpec h = {K_PHYS, 1.0f, 0, -1, 0, 0, 0}; hit(user, target, &h, 0); }
	return_home(user);
}

static void do_defend(Battler* b) {
	state = ST_BUSY;
	set_st(b, S_PROTECT, 1);
	sfx("defend", -4, 1);
	flash(b, 2);
	show_top(sfmt("%s se defiende.", b->name), 0.6f);
	finish_actor_turn();
}

static void write_back(void) {
	Battler* al[4];
	int any = alive_list(party, nparty, al);
	for (int i = 0; i < nparty; ++i) {
		Battler* b = party[i];
		G.members[b->member].hp = (over && any) ? MAX(b->hp, 1) : b->hp;
		G.members[b->member].mp = b->mp;
	}
}

static void return_to_world(void) {
	strcpy(G.current_map, g_enc.ret_map);
	if (g_enc.ret_has_pos) { G.has_load_pos = 1; G.load_x = g_enc.ret_x; G.load_y = g_enc.ret_y; G.current_spawn[0] = 0; }
	else strcpy(G.current_spawn, g_enc.ret_spawn);
	fade_out(0.5f);
	scene_request(SC_WORLD);
	block_forever();
}

static void do_flee(void) {
	state = ST_BUSY;
	float ps = 0, es = 0;
	for (int i = 0; i < nparty; ++i) ps += stat(party[i], ST_SPD);
	Battler* ae[MAXB];
	int na = alive_list(enemies, nen, ae);
	for (int i = 0; i < na; ++i) es += stat(ae[i], ST_SPD);
	ps /= nparty > 0 ? nparty : 1;
	es /= na > 0 ? na : 1;
	float chance = CLAMP(0.55f + (ps - es) * 0.02f, 0.25f, 0.9f);
	if (randf() < chance) {
		sfx("flee", -2, 1);
		show_top("¡Escapáis!", 0.8f);
		over = 1;
		write_back();
		return_to_world();
	}
	show_top("¡No podéis escapar!", 0.8f);
	finish_actor_turn();
}

static void steal(Battler* t) {
	if (t->flag_stolen) { show_top("No le queda nada que robar.", 0.7f); return; }
	if (randf() < 0.65f) {
		t->flag_stolen = 1;
		if (t->d->ndrops && randf() < 0.6f) {
			const char* it = t->d->drops[0].item;
			add_item(it, 1);
			show_top(sfmt("¡Robas %s!", db_item_name(it)), 0.9f);
		} else {
			int g = t->d->gold + randi(5, 20);
			G.gold += g;
			show_top(sfmt("¡Robas %d coronas!", g), 0.9f);
		}
		sfx("buy", -6, 1);
	} else show_top("Fallas el robo.", 0.7f);
}

static void eat(Battler* user, Battler* t) {
	if (!alive(t)) return;
	if (has_st(t, S_CHOCO)) {
		int dmg = (int)(max_hp(t) * 0.55f) + randi(0, 8);
		sfx("crit", -2, 0.7f);
		float_text(t, "¡ÑAM!", FC_CHOCO, 1, -24);
		erase_st(t, S_CHOCO);
		damage(t, dmg, 1, 0);
		int h = (int)(max_hp(user) * 0.12f);
		user->hp = MIN(max_hp(user), user->hp + h);
		user->st[ST_ATK] += 3;
		user->st[ST_MAG] += 2;
		float_text(user, sfmt("+%d", h), FC_HEAL, 0, 0);
		user->grow *= 1.06f;
		show_top(sfmt("%s se hace más fuerte... y más gordo.", user->name), 0.9f);
	} else {
		HitSpec h = {K_PHYS, 1.1f, 0, -1, 0, 0, 0};
		hit(user, t, &h, 0);
	}
	refresh();
}

static void summon(Battler* user) {
	Battler* al[MAXB];
	if (alive_list(enemies, nen, al) >= 4) return;
	static const float slots[3][2] = {{90, 110}, {96, 230}, {250, 240}};
	const char* key = user->d->summon ? user->d->summon : "larva";
	for (int k = 0; k < 2; ++k) {
		if (alive_list(enemies, nen, al) >= 3) break;
		const float* p = slots[(user->turns + k) % 3];
		Battler* e = spawn_enemy(key, p[0], p[1]);
		if (e) e->alpha = 0.05f;
	}
	sfx("dark", -4, 1);
	show_top(user->d->summon_msg ? user->d->summon_msg : "¡De la raíz brotan larvas!", 0.9f);
	relayout();
}

static int el_sfx_pick(int el, int kind, const char** out) {
	switch (el) {
	case EL_FIRE: *out = "fire"; return 1;
	case EL_ICE: *out = "ice"; return 1;
	case EL_BOLT: *out = "bolt"; return 1;
	case EL_DARK: *out = "dark"; return 1;
	}
	*out = (kind == K_HEAL || kind == K_REVIVE || kind == K_CURE) ? "heal" : "magic";
	return 1;
}

static void do_skill(Battler* user, int sid, Battler** targets, int nt) {
	const SkillDef* sk = &g_skills[sid];
	user->mp = user->mp - sk->mp < 0 ? 0 : user->mp - sk->mp;
	refresh();
	show_top(sk->name, 0.45f);
	int kind = sk->kind, el = sk->el;
	if (kind == K_PHYS) lunge(user);
	else {
		cast_pose(user);
		const char* s;
		el_sfx_pick(el, kind, &s);
		sfx(s, -4, 1);
		b_wait(0.2f);
	}
	int hits = sk->hits;
	HitSpec hs = spec_of(sk);
	if (sk->target == T_ENEMIES_RANDOM) {
		for (int h = 0; h < hits; ++h) {
			Battler* pool[MAXB];
			int np = user->is_enemy ? alive_list(party, nparty, pool) : alive_list(enemies, nen, pool);
			if (!np) break;
			hit(user, pool[rnd() % np], &hs, el);
		}
	} else {
		for (int i = 0; i < nt; ++i) {
			Battler* t = targets[i];
			if (kind == K_PHYS || kind == K_MAG || kind == K_DRAIN) {
				for (int h = 0; h < hits; ++h) if (alive(t)) hit(user, t, &hs, el);
			} else if (kind == K_HEAL) {
				if (alive(t)) {
					spell_fx(t, 100);
					int amt = (int)(stat(user, ST_MAG) * sk->power + sk->base);
					if (is_player(user)) amt = (int)(amt * (1.0f + perk(PK_HEAL_POW)));
					heal(t, amt);
					if (sk->cure) cure(t);
					for (int k = 0; k < sk->nadd; ++k) set_st(t, sk->add_status[k], 4);
				}
			} else if (kind == K_REVIVE) {
				if (!alive(t)) {
					int h = (int)(max_hp(t) * sk->ratio);
					t->hp = h < 1 ? 1 : h;
					apply_dead_look(t);
					effect(t, 255, 255, 179);
					sfx("heal", -4, 1);
					float_text(t, "¡Revive!", FC_REV, 0, 0);
				}
			} else if (kind == K_BUFF) {
				for (int k = 0; k < sk->nstatus; ++k) set_st(t, sk->status[k], sk->turns);
				int rage = 0;
				for (int k = 0; k < sk->nstatus; ++k) if (sk->status[k] == S_RAGE) rage = 1;
				if (rage) effect(t, 255, 128, 102); else effect(t, 153, 204, 255);
				if (sk->nstatus) float_text(t, g_status[sk->status[0]].name, FC_MP, 0, 0);
			} else if (kind == K_STATUS) {
				try_inflict(t, sk->inflict, sk->chance);
			} else if (kind == K_CURE) {
				cure(t);
				effect(t, 204, 255, 204);
				float_text(t, "Purificado", FC_HEAL, 0, 0);
			} else if (kind == K_STEAL) steal(t);
			else if (kind == K_SUMMON) summon(user);
			else if (kind == K_EAT) eat(user, t);
			b_wait(0.08f);
		}
	}
	for (int k = 0; k < sk->nself; ++k) set_st(user, sk->self_status[k], 3);
	if (sk->has_heal_party) {
		Battler** grp = user->is_enemy ? enemies : party;
		int ng = user->is_enemy ? nen : nparty;
		for (int i = 0; i < ng; ++i) if (alive(grp[i])) heal(grp[i], (int)(stat(user, ST_MAG) * sk->heal_party + 10));
	}
	b_wait(0.25f);
	if (kind == K_PHYS) return_home(user);
	user->pose_t = 0;
	refresh();
}

static void do_item(Battler* user, int iid, Battler** targets, int nt) {
	const ItemDef* it = &g_items[iid];
	remove_item_k(iid, 1);
	show_top(it->name, 0.45f);
	cast_pose(user);
	if (it->special && !strcmp(it->special, "aleixolo")) {
		Battler* t0 = targets[0];
		if (t0->is_enemy && !strcmp(t0->key, "aleixolo") && !t0->flag_weakened) {
			show_top("Le enseñas el currículum a Aleixolo.", 1.0f);
			show_top("«Experiencia: tres meses de ayudante de repostería. Motivo del despido: comerse el género.»", 1.8f);
			show_top("Aleixolo se echa a llorar. «¡Nadie me quiso contratar! ¡Nadie!»", 1.4f);
			weaken_aleixolo(t0);
			float_text(t0, "¡Débil!", FC_WEAK, 1, 0);
			do_shake(4);
		} else {
			add_item_k(iid, 1);
			show_top("No parece interesarle lo más mínimo.", 0.9f);
		}
		user->pose_t = 0;
		refresh();
		return;
	}
	for (int i = 0; i < nt; ++i) {
		Battler* t = targets[i];
		if (it->has_revive) {
			if (!alive(t)) {
				int h = (int)(max_hp(t) * it->revive);
				t->hp = h < 1 ? 1 : h;
				apply_dead_look(t);
				float_text(t, "¡Revive!", FC_REV, 0, 0);
				sfx("heal", -4, 1);
			}
			continue;
		}
		if (it->damage) {
			if (alive(t)) {
				sfx("fire", -2, 1);
				spell_fx(t, EL_FIRE);
				float mult = el_mult(t, it->el);
				damage(t, (int)(it->damage * mult * randf_range(0.9f, 1.1f)), 0, mult > 1.0f);
			}
			continue;
		}
		if (!alive(t)) continue;
		if (it->heal) heal(t, it->heal);
		if (it->mp) {
			t->mp = MIN(max_mp(t), t->mp + it->mp);
			float_text(t, sfmt("+%d PM", it->mp), FC_MP, 0, 0);
			sfx("heal", -6, 1.3f);
		}
		if (it->cure) cure(t);
		if (it->nap) { set_st(t, S_STUN, 1); float_text(t, "¡Zzz… qué rico!", FC_CHOCO, 0, 0); }
	}
	b_wait(0.4f);
	user->pose_t = 0;
	refresh();
}

static void execute(Battler* user, int type, int id, Battler** targets, int nt) {
	if (type == 0) do_attack(user, targets[0]);
	else if (type == 1) do_skill(user, id, targets, nt);
	else do_item(user, id, targets, nt);
	finish_actor_turn();
}

// ------------------------------------------------------------------ IA de enemigos
static void enemy_act(Battler* e) {
	const EnemyDef* d = e->d;
	int sid = -1;
	float ratio = (float)e->hp / max_hp(e);
	const char* k = e->key;
	if (!strcmp(k, "wolf_alpha") || !strcmp(k, "brute")) {
		if (ratio < 0.5f && !e->flag_enraged) {
			e->flag_enraged = 1;
			sid = db_skill(!strcmp(k, "wolf_alpha") ? "e_aullido" : "e_rugido");
			sfx("growl", -2, 1);
			show_top(sfmt("¡%s se enfurece! Las venas negras palpitan...", e->name), 1.0f);
		}
	} else if (!strcmp(k, "custodian")) {
		if (ratio < 0.6f && !has_st(e, S_PROTECT) && randf() < 0.5f) sid = db_skill("e_escudo");
	} else if (!strcmp(k, "kaelen_dark")) {
		if (ratio < 0.5f && !e->flag_taunt) { e->flag_taunt = 1; show_top("Kaelen: «¿Por qué siempre tú? ¡¿Por qué nunca yo?!»", 1.4f); }
	} else if (!strcmp(k, "aleixolo")) {
		Battler* al[4];
		int na = alive_list(party, nparty, al);
		Battler* chocos[4];
		int nc = 0;
		for (int i = 0; i < na; ++i) if (has_st(al[i], S_CHOCO)) chocos[nc++] = al[i];
		if (e->flag_weakened) sid = db_skill(randi(0, 1) ? "e_panzazo" : "e_bombon");
		else if (nc && randf() < 0.8f) { Battler* t = chocos[rnd() % nc]; do_skill(e, db_skill("e_comer"), &t, 1); return; }
		else if (ratio < 0.4f && !e->flag_snack) {
			e->flag_snack = 1;
			e->hp = MIN(max_hp(e), e->hp + (int)(max_hp(e) * 0.2f));
			show_top("Aleixolo se zampa una tableta entera de chocolate. ¡Recupera fuerzas!", 1.2f);
			refresh();
			return;
		}
	} else if (!strcmp(k, "nhalzur")) {
		if (ratio < 0.5f && !e->flag_phase2) {
			e->flag_phase2 = 1;
			e->st[ST_ATK] = (int)e->st[ST_ATK] + 4;
			e->st[ST_MAG] = (int)e->st[ST_MAG] + 4;
			do_shake(10);
			flash_white(0.6f);
			sfx("growl", 0, 0.5f);
			show_top("Nhal'Zur: «MIL AÑOS DE HAMBRE. VAEL NO ESTÁ AQUÍ PARA SALVARTE.»", 1.8f);
			sid = db_skill("e_velo");
		}
	} else if (!strcmp(k, "mother_root")) {
		Battler* al[MAXB];
		if (ratio < 0.5f && !e->flag_phase2) {
			e->flag_phase2 = 1;
			e->st[ST_ATK] = (int)e->st[ST_ATK] + 6;
			e->st[ST_MAG] = (int)e->st[ST_MAG] + 6;
			do_shake(8);
			sfx("growl", 0, 0.7f);
			show_top("¡La Madre Raíz despierta del todo! El suelo late...", 1.4f);
			sid = db_skill("e_invocar");
		} else if (e->flag_phase2 && e->turns % 5 == 0 && alive_list(enemies, nen, al) < 2) sid = db_skill("e_invocar");
		else if (e->turns % 5 == 0) sid = db_skill("e_regenerar");
	}
	if (sid < 0) {
		int total = 0;
		for (int i = 0; i < d->nai; ++i) total += d->ai[i].weight;
		int r = randi(1, total);
		for (int i = 0; i < d->nai; ++i) { r -= d->ai[i].weight; if (r <= 0) { sid = d->ai[i].skill; break; } }
	}
	const SkillDef* sk = &g_skills[sid];
	Battler* ap[4];
	int na = alive_list(party, nparty, ap);
	if (!na) return;
	Battler* targets[MAXB];
	int nt = 0;
	if (sk->target == T_ENEMY) {
		Battler* tn[4];
		int ntn = 0;
		for (int i = 0; i < na; ++i) if (has_st(ap[i], S_TAUNT)) tn[ntn++] = ap[i];
		if (ntn && randf() < 0.8f) targets[nt++] = tn[rnd() % ntn];
		else targets[nt++] = ap[rnd() % na];
	} else if (sk->target == T_ENEMIES || sk->target == T_ENEMIES_RANDOM) {
		for (int i = 0; i < na; ++i) targets[nt++] = ap[i];
	} else targets[nt++] = e;
	do_skill(e, sid, targets, nt);
}

// ------------------------------------------------------------------ combate automático
static void auto_act(Battler* b) {
	state = ST_BUSY;
	s16 skills[16];
	int ns = gs_skills_of(b->member, skills);
	Battler* ap[4];
	int na = alive_list(party, nparty, ap);
	Battler* low = 0;
	float low_r = 1;
	for (int i = 0; i < na; ++i) { float r = (float)ap[i]->hp / max_hp(ap[i]); if (r < low_r) { low_r = r; low = ap[i]; } }
	Battler* dead[4];
	int nd = 0;
	for (int i = 0; i < nparty; ++i) if (!alive(party[i])) dead[nd++] = party[i];
	if (nd && item_count("pluma") > 0 && randf() < 0.7f) { execute(b, 2, db_item("pluma"), dead, 1); return; }
	for (int i = 0; i < ns; ++i) {
		const SkillDef* sk = &g_skills[skills[i]];
		if (sk->mp > b->mp) continue;
		if (sk->kind == K_REVIVE && nd) { execute(b, 1, skills[i], dead, 1); return; }
	}
	if (low && low_r < 0.45f) {
		for (int i = 0; i < ns; ++i) {
			const SkillDef* sk = &g_skills[skills[i]];
			if (sk->kind == K_HEAL && sk->mp <= b->mp) {
				if (sk->target == T_ALLY) execute(b, 1, skills[i], &low, 1);
				else execute(b, 1, skills[i], ap, na);
				return;
			}
		}
		if (item_count("pocion") > 0 && low_r < 0.3f) { execute(b, 2, db_item("pocion"), &low, 1); return; }
	}
	Battler* foes[MAXB];
	int nf = alive_list(enemies, nen, foes);
	if (!nf) { finish_actor_turn(); return; }
	for (int a = 1; a < nf; ++a) { Battler* t = foes[a]; int j = a - 1; while (j >= 0 && foes[j]->hp > t->hp) { foes[j + 1] = foes[j]; --j; } foes[j + 1] = t; }
	int best = -1;
	float best_score = 0;
	for (int i = 0; i < ns; ++i) {
		const SkillDef* sk = &g_skills[skills[i]];
		if (!(sk->kind == K_PHYS || sk->kind == K_MAG || sk->kind == K_DRAIN) || sk->mp > b->mp) continue;
		float score = sk->power * sk->hits;
		if (sk->target == T_ENEMIES || sk->target == T_ENEMIES_RANDOM) score *= nf * 0.8f;
		if (sk->el && (foes[0]->d->weak >> sk->el & 1)) score *= 1.5f;
		if (b->mp < max_mp(b) * 0.3f) score *= 0.6f;
		if (score > best_score && score > 1.2f) { best_score = score; best = skills[i]; }
	}
	if (best >= 0 && randf() < 0.7f) {
		const SkillDef* sk = &g_skills[best];
		if (sk->target == T_ENEMIES || sk->target == T_ENEMIES_RANDOM) execute(b, 1, best, foes, nf);
		else execute(b, 1, best, foes, 1);
		return;
	}
	execute(b, 0, 0, foes, 1);
}

// ------------------------------------------------------------------ menús
static int usable_items(s16* out) {
	int keys[24];
	int n = 0;
	for (int i = 0; i < G.ninv; ++i) if (G.inv[i].item < N_ITEMS && G.inv[i].count > 0) keys[n++] = G.inv[i].item;
	for (int a = 1; a < n; ++a) { int t = keys[a], j = a - 1; while (j >= 0 && strcmp(g_items[keys[j]].id, g_items[t].id) > 0) { keys[j + 1] = keys[j]; --j; } keys[j + 1] = t; }
	for (int i = 0; i < n; ++i) out[i] = keys[i];
	return n;
}

static void close_menus(void) {
	cmd_visible = 0;
	sub_visible = 0;
	enemy_panel_visible = 1;
	top_on = 0;
	nhands = 0;
	draw_top();
	relayout();
}

static void open_command(Battler* b) {
	state = ST_COMMAND;
	cmd_index = 0;
	cmd_visible = 1;
	enemy_panel_visible = 0;
	s16 tmp[24];
	cmd_dis[0] = 0;
	cmd_dis[1] = gs_skills_of(b->member, tmp) == 0;
	cmd_dis[2] = usable_items(tmp) == 0;
	cmd_dis[3] = 0;
	cmd_dis[4] = !g_enc.can_flee;
	sfx("select", -12, 1);
	relayout();
	input_flush();
}

static void update_hands(void) {
	nhands = 0;
	if (state == ST_COMMAND) {
		hand_x[0] = CMD_X * 8 + 4; hand_y[0] = CMD_Y * 8 + 3 + cmd_index * 12; hand_flip[0] = 0; nhands = 1;
	} else if (state == ST_SUB) {
		hand_x[0] = 8 * 8 + 6; hand_y[0] = 3 * 8 + 4 + (sub_index - sub_scroll) * 12; hand_flip[0] = 0; nhands = 1;
	} else if (state == ST_TARGET) {
		int multi = target_mode == T_ENEMIES || target_mode == T_ALLIES || target_mode == T_ENEMIES_RANDOM;
		int n = multi ? ntarget : 1;
		for (int k = 0; k < n && k < 6; ++k) {
			Battler* b = multi ? target_list[k] : target_list[target_index];
			if (b->is_enemy) { hand_x[k] = (int)b->x - enemy_half_w(b) - 14; hand_y[k] = (int)b->y - 6; hand_flip[k] = 0; }
			else { hand_x[k] = (int)b->x + 14; hand_y[k] = (int)b->y - 4; hand_flip[k] = 1; }
			nhands++;
		}
	}
}

static void refresh_sub_top(void) {
	if (!nsub) return;
	int id = sub_entries[sub_index];
	show_top_now(sub_kind == 0 ? g_skills[id].desc : g_items[id].desc);
}

static void open_sub(int kind) {
	sub_kind = kind;
	state = ST_SUB;
	sub_index = sub_scroll = 0;
	if (kind == 0) nsub = gs_skills_of(actor->member, sub_entries);
	else nsub = usable_items(sub_entries);
	sub_visible = 1;
	relayout();
	refresh_sub_top();
	input_flush();
}

static void begin_target(int mode) {
	target_mode = mode;
	ntarget = 0;
	switch (mode) {
	case T_ENEMY: case T_ENEMIES: case T_ENEMIES_RANDOM:
		for (int i = 0; i < nen; ++i) if (alive(enemies[i])) target_list[ntarget++] = enemies[i];
		break;
	case T_ALLY: case T_ALLIES:
		for (int i = 0; i < nparty; ++i) if (alive(party[i])) target_list[ntarget++] = party[i];
		break;
	case T_ALLY_DEAD:
		for (int i = 0; i < nparty; ++i) if (!alive(party[i])) target_list[ntarget++] = party[i];
		break;
	case T_SELF:
		target_list[ntarget++] = actor;
		break;
	}
	if (!ntarget) { sfx("cancel", -8, 1); return; }
	target_index = 0;
	if (mode == T_ALLY || mode == T_ALLY_DEAD) {
		float low = 2;
		for (int k = 0; k < ntarget; ++k) { float r = (float)target_list[k]->hp / max_hp(target_list[k]); if (r < low) { low = r; target_index = k; } }
	}
	state = ST_TARGET;
	input_flush();
}

static void input_command(void) {
	if (key_rep(KEY_UP)) { cmd_index = (cmd_index + 4) % 5; sfx("select", -14, 1); relayout(); }
	else if (key_rep(KEY_DOWN)) { cmd_index = (cmd_index + 1) % 5; sfx("select", -14, 1); relayout(); }
	else if (key_hit(KEY_A)) {
		if (cmd_dis[cmd_index]) { sfx("cancel", -8, 1); return; }
		sfx("confirm", -10, 1);
		switch (cmd_index) {
		case 0: pending_type = 0; begin_target(T_ENEMY); break;
		case 1: open_sub(0); break;
		case 2: open_sub(1); break;
		case 3: close_menus(); do_defend(actor); break;
		case 4: close_menus(); do_flee(); break;
		}
	}
}

static void input_sub(void) {
	if ((key_rep(KEY_UP) || key_rep(KEY_DOWN)) && nsub) {
		sub_index = (sub_index + (key_rep(KEY_UP) ? nsub - 1 : 1)) % nsub;
		if (sub_index < sub_scroll) sub_scroll = sub_index;
		if (sub_index >= sub_scroll + 6) sub_scroll = sub_index - 5;
		sfx("select", -14, 1);
		relayout();
		refresh_sub_top();
	} else if (key_hit(KEY_B)) {
		sfx("cancel", -10, 1);
		sub_visible = 0;
		top_on = 0;
		draw_top();
		state = ST_COMMAND;
		relayout();
	} else if (key_hit(KEY_A) && nsub) {
		int id = sub_entries[sub_index];
		if (sub_kind == 0) {
			const SkillDef* sk = &g_skills[id];
			if (sk->mp > actor->mp) { sfx("cancel", -8, 1); return; }
			pending_type = 1; pending_id = id;
			begin_target(sk->target);
		} else {
			pending_type = 2; pending_id = id;
			begin_target(g_items[id].target);
		}
		sfx("confirm", -10, 1);
	}
}

static void input_target(void) {
	int multi = target_mode == T_ENEMIES || target_mode == T_ALLIES || target_mode == T_ENEMIES_RANDOM || target_mode == T_SELF;
	if (!multi) {
		int n = 0;
		if (key_rep(KEY_UP)) n = -1;
		if (key_rep(KEY_DOWN) || key_rep(KEY_LEFT) || key_rep(KEY_RIGHT)) n = 1;
		if (n) { target_index = (target_index + n + ntarget) % ntarget; sfx("select", -14, 1); }
	}
	Battler* t0 = multi ? target_list[0] : target_list[target_index];
	const char* label = !multi ? t0->name : (t0->is_enemy ? "Todos los enemigos" : "Todo el grupo");
	if (strcmp(top_text, label)) show_top_now(label);
	if (key_hit(KEY_B)) {
		sfx("cancel", -10, 1);
		state = pending_type == 0 ? ST_COMMAND : ST_SUB;
		if (state == ST_SUB) refresh_sub_top(); else { top_on = 0; draw_top(); }
		return;
	}
	if (key_hit(KEY_A)) {
		Battler* targets[MAXB];
		int nt = 0;
		if (multi) for (int i = 0; i < ntarget; ++i) targets[nt++] = target_list[i];
		else targets[nt++] = target_list[target_index];
		close_menus();
		state = ST_BUSY;
		execute(actor, pending_type, pending_id, targets, nt);
	}
}

// ------------------------------------------------------------------ turnos
static Battler* ready_q[MAXB];
static int nready;

static void finish_actor_turn(void) {
	if (actor) { actor->atb = 0; actor->pose_t = 0; }
	actor = 0;
	state = ST_IDLE;
	busy = 0;
	refresh();
	check_end();
}

static void start_turn(Battler* b) {
	busy = 1;
	int skip = tick_statuses(b);
	if (check_end()) return;
	if (skip || !alive(b)) { b->atb = 0; busy = 0; return; }
	if (b->is_enemy) {
		enemy_act(b);
		b->atb = 0;
		busy = 0;
		check_end();
		return;
	}
	actor = b;
	refresh();
	if (auto_battle) { auto_act(b); return; }
	open_command(b);
}

static void tick_atb(void) {
	for (int i = 0; i < MAXB; ++i) {
		Battler* b = &B[i];
		if (!alive(b)) continue;
		if (b->atb < 100) {
			b->atb += (1.0f / 60) * (20.0f + stat(b, ST_SPD) * 2.2f) * ATB_RATE * battle_speed_mult();
			if (b->atb > 100) b->atb = 100;
			if (b->atb >= 100) {
				int in = 0;
				for (int k = 0; k < nready; ++k) if (ready_q[k] == b) in = 1;
				if (!in && nready < MAXB) ready_q[nready++] = b;
			}
		}
	}
	if (!nready) return;
	Battler* b = ready_q[0];
	for (int k = 1; k < nready; ++k) ready_q[k - 1] = ready_q[k];
	nready--;
	if (!alive(b)) return;
	start_turn(b);
}

static int victory_pending, defeat_pending;
static int check_end(void) {
	if (over) return 1;
	Battler* tmp[MAXB];
	if (!alive_list(enemies, nen, tmp)) { over = 1; victory_pending = 1; return 1; }
	if (!alive_list(party, nparty, tmp)) { over = 1; defeat_pending = 1; return 1; }
	return 0;
}

// ------------------------------------------------------------------ paneles de elección (senda y clase)
static int choose_panel(const char* title, const char* sub, const char* const* names, const char* const* descs, int n) {
	int sel = 0;
	for (;;) {
		ui_clear();
		int h = 7 + n * 3;
		int ty = (20 - h) / 2;
		ui_panel(NP_PANEL, PAL_UI_LIGHT, 1, ty, 28, h);
		ui_text_center(8, ty * 8 + 5, 224, title, UC_LIGHT_TITLE, -1);
		ui_text_center(8, ty * 8 + 17, 224, sub, UC_LIGHT_TEXT, -1);
		for (int i = 0; i < n; ++i) ui_button(8, ty + 4 + i * 3, 14, 3, names[i], i == sel ? 1 : 0, 0, 0);
		ui_text_wrapped(16, (ty + 4 + n * 3) * 8 + 2, 208, 10, descs[sel], UC_LIGHT_TEXT, -1, 3);
		for (;;) {
			frame_wait();
			if (key_rep(KEY_UP)) { sel = (sel + n - 1) % n; sfx("select", -16, 1); break; }
			if (key_rep(KEY_DOWN)) { sel = (sel + 1) % n; sfx("select", -16, 1); break; }
			if (key_hit(KEY_A)) { ui_clear(); relayout(); return sel; }
		}
	}
}

// ------------------------------------------------------------------ victoria y derrota
static void victory(void) {
	close_menus();
	state = ST_END;
	b_wait(0.6f);
	music_play("victory", 0.2f);
	float dm[3];
	diff_mults(dm);
	int xp_total = 0, gold_total = 0;
	const char* drops[8];
	int nd = 0;
	for (int i = 0; i < nen; ++i) {
		Battler* e = enemies[i];
		xp_total += (int)(e->d->xp * dm[2] * abyss_mult());
		gold_total += (int)(e->d->gold * dm[2] * abyss_mult());
		for (int k = 0; k < e->d->ndrops; ++k) if (randf() < e->d->drops[k].chance && nd < 8) drops[nd++] = e->d->drops[k].item;
	}
	for (int i = 0; i < nparty; ++i) {
		Battler* b = party[i];
		if (!alive(b)) { b->hp = 1; apply_dead_look(b); }
		b->victory = 1;
		b->frame_up = -1;
	}
	write_back();
	G.gold += gold_total;
	for (int i = 0; i < nd; ++i) add_item(drops[i], 1);
	char txt[300];
	int len = snprintf(txt, sizeof txt, "Experiencia: +%d\nCoronas: +%d", xp_total, gold_total);
	for (int i = 0; i < nd; ++i) len += snprintf(txt + len, sizeof txt - len, "\nObtienes: %s", db_item_name(drops[i]));
	ui_panel(NP_PANEL, PAL_UI_LIGHT, 5, 3, 20, 11);
	ui_text_center(40, 30, 160, "¡Victoria!", UC_LIGHT_TITLE, -1);
	ui_text_wrapped(52, 46, 140, 11, txt, UC_LIGHT_TEXT, -1, 6);
	b_wait(1.6f);
	LevelUp lu[6];
	int nl = add_xp(xp_total, lu, 6);
	for (int r = 0; r < nl; ++r) {
		sfx("levelup", -2, 1);
		len = snprintf(txt, sizeof txt, "¡Nivel %ld! Recuperáis parte de las fuerzas.", (long)lu[r].level);
		for (int k = 0; k < lu[r].n; ++k)
			len += snprintf(txt + len, sizeof txt - len, "\n%s aprende %s.", member_name(lu[r].who[k]), g_skills[lu[r].skill[k]].name);
		ui_panel(NP_PANEL, PAL_UI_LIGHT, 5, 3, 20, 11);
		ui_text_center(40, 30, 160, "¡Victoria!", UC_LIGHT_TITLE, -1);
		ui_text_wrapped(52, 46, 140, 11, txt, UC_LIGHT_TEXT, -1, 6);
		for (int i = 0; i < nparty; ++i) {
			Battler* b = party[i];
			Stats7 st = gs_stats(b->member);
			for (int k = 0; k < ST_N; ++k) b->st[k] = st.v[k];
			b->hp = G.members[b->member].hp;
			b->mp = G.members[b->member].mp;
			flash(b, 1);
		}
		relayout();
		b_wait(2.0f);
	}
	ui_clear_rect(5, 3, 20, 11);
	relayout();
	if (needs_branch_choice() || needs_class_choice()) {
		const char* names[3];
		const char* descs[3];
		for (int bi = 0; bi < 3; ++bi) {
			const BranchDef* bd = &g_branches[bi];
			names[bi] = bd->name;
			descs[bi] = sfmt("%s\nClases: %s, %s, %s", bd->desc, g_classes[bd->classes[0]].name, g_classes[bd->classes[1]].name,
				g_classes[bd->classes[2]].name);
		}
		int bidx = choose_panel("Elige tu senda", "Cómo vas a luchar. Luego elegirás tu clase.", names, descs, 3);
		for (int c = 0; c < 3; ++c) {
			const ClassDef* cd = &g_classes[g_branches[bidx].classes[c]];
			names[c] = cd->name;
			descs[c] = cd->desc;
		}
		int cidx = choose_panel("Elige tu clase", "Cada clase tiene su árbol de talentos.", names, descs, 3);
		choose_class(g_branches[bidx].classes[cidx]);
		sfx("levelup", -4, 1);
	}
	if (skill_points() > 0) show_top(sfmt("Tienes %d punto(s) de talento. Ábrelos en el menú (START) > Talentos.", skill_points()), 1.8f);
	if (g_enc.id[0]) setf(sfmt("won_%s", g_enc.id));
	return_to_world();
}

static void defeat(void) {
	close_menus();
	state = ST_END;
	music_stop(0.8f);
	show_top("Todo se vuelve negro...", 1.6f);
	dim_set(11);
	int slot = any_save();
	int sel = 0;
	for (;;) {
		ui_panel(NP_PANEL, PAL_UI_LIGHT, 6, 4, 18, 12);
		ui_text_center(48, 40, 144, "Habéis caído", UC_LIGHT_TITLE, -1);
		ui_button(8, 7, 14, 2, "Reintentar el combate", sel == 0 ? 1 : 0, 0, 0);
		ui_button(8, 10, 14, 2, "Cargar partida", slot < 0 ? 2 : (sel == 1 ? 1 : 0), 0, 0);
		ui_button(8, 13, 14, 2, "Volver al título", sel == 2 ? 1 : 0, 0, 0);
		for (;;) {
			frame_wait();
			if (key_rep(KEY_UP) || key_rep(KEY_DOWN)) {
				int d = key_rep(KEY_UP) ? 2 : 1;
				do sel = (sel + d) % 3; while (sel == 1 && slot < 0);
				sfx("select", -16, 1);
				break;
			}
			if (key_hit(KEY_A)) {
				dim_set(0);
				if (sel == 0) {
					memcpy(G.members, snapshot, sizeof snapshot);
					heal_all();
					fade_out(0.4f);
					scene_request(SC_BATTLE);
					block_forever();
				} else if (sel == 1) {
					if (load_game(slot)) { fade_out(0.5f); scene_request(SC_WORLD); block_forever(); }
				} else { fade_out(0.5f); scene_request(SC_TITLE); block_forever(); }
			}
		}
	}
}

// ------------------------------------------------------------------ escena
static const BgImage* battle_bg(const char* name) {
	if (!strcmp(name, "cathedral")) return &bg_battle_cathedral;
	if (!strcmp(name, "crypt")) return &bg_battle_crypt;
	if (!strcmp(name, "deep")) return &bg_battle_deep;
	if (!strcmp(name, "heart")) return &bg_battle_heart;
	if (!strcmp(name, "village_night")) return &bg_battle_village_night;
	return &bg_battle_forest;
}

static void intro(void) {
	busy = 1;
	const char* names[8];
	int counts[8], nn = 0;
	for (int i = 0; i < nen; ++i) {
		int k;
		for (k = 0; k < nn; ++k) if (!strcmp(names[k], enemies[i]->name)) break;
		if (k == nn) { names[nn] = enemies[i]->name; counts[nn++] = 0; }
		counts[k]++;
	}
	char buf[160];
	int len = 0;
	buf[0] = 0;
	for (int k = 0; k < nn; ++k)
		len += counts[k] == 1 ? snprintf(buf + len, sizeof buf - len, "%s%s", k ? ", " : "", names[k])
			: snprintf(buf + len, sizeof buf - len, "%s%d × %s", k ? ", " : "", counts[k], names[k]);
	show_top(buf, 1.1f);
	busy = 0;
}

void battle_scene(void* arg) {
	(void)arg;
	g_vid.dispcnt = DCNT_BLANK;
	objt_reset();
	objpal_reset();
	ui_init();
	ui_load_palettes(1);
	ui_sprites_reset();
	particles_init();
	float_init();
	float_palslot = -1;
	turn_marker_init();
	memcpy(snapshot, G.members, sizeof snapshot);
	over = busy = 0;
	victory_pending = defeat_pending = 0;
	actor = 0;
	nready = 0;
	state = ST_IDLE;
	top_on = 0;
	cmd_visible = sub_visible = 0;
	enemy_panel_visible = 1;
	nhands = 0;
	t_total = 0;
	auto_battle = G.auto_battle;
	bg_image_load(battle_bg(g_enc.bg), 1, 28, 0);
	g_vid.bgcnt[1] = (1 << 2) | (28 << 8) | (1 << 14) | 3;
	g_vid.hofs[1] = HOFS;
	g_vid.vofs[1] = VOFS;
	build_scene();
	g_frame_hook = hook;
	g_vid.dispcnt = DCNT_MODE0 | DCNT_OBJ1D | DCNT_BG0 | DCNT_BG1 | DCNT_OBJ;
	music_play(g_enc.music[0] ? g_enc.music : (g_enc.boss ? "boss" : "battle"), 0.3f);
	g_trans_busy = 0;
	refresh();
	fade_in(0.4f);
	intro();
	for (;;) {
		// enemigos que mueren: desvanecer y aplastar
		for (int i = 0; i < nen; ++i) {
			Battler* e = enemies[i];
			if (!alive(e) && e->alpha > 0) {
				e->alpha -= 1.0f / 36;
				e->sx += 0.2f / 36;
				e->sy -= 0.4f / 36;
				if (e->alpha < 0) e->alpha = 0;
			} else if (alive(e) && e->alpha < 1) { e->alpha += 1.0f / 24; if (e->alpha > 1) e->alpha = 1; }
		}
		if (key_hit(KEY_R) && !over) {
			auto_battle = !auto_battle;
			G.auto_battle = auto_battle;
			sfx("select", -8, 1);
			if (auto_battle && (state == ST_COMMAND || state == ST_SUB || state == ST_TARGET)) { close_menus(); auto_act(actor); }
		}
		if (victory_pending) { victory_pending = 0; victory(); }
		if (defeat_pending) { defeat_pending = 0; defeat(); }
		if (!over) {
			switch (state) {
			case ST_IDLE: if (!busy) tick_atb(); break;
			case ST_COMMAND: input_command(); break;
			case ST_SUB: input_sub(); break;
			case ST_TARGET: input_target(); break;
			}
		}
		update_hands();
		frame_wait();
	}
}
