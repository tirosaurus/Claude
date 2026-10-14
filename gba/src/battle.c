// Combate por turnos (orden por velocidad): Atacar / Habilidad / Defender / Objeto.
#include "game.h"
#include <string.h>

#define B_CB 1
#define B_SB 28
#define MAXF 3

typedef struct { const EnemyDef* d; s16 hp; u8 alive; u8 flash; } Foe;
static Foe foes[MAXF];
static int nfoes;
static u8 defending[M_COUNT];
static u8 hurt[M_COUNT];
static u8 pose[M_COUNT];
static u16 bt;
static char msgbuf[96];

static u32 rng = 0x1234567;
static int rnd(int n) { rng = rng * 1103515245 + 12345; return (rng >> 16) % n; }

static const SpriteSheet* bsheet(int m) {
	if (m == M_KAELEN) return &g_spr_b_kaelen;
	if (m == M_YARA) return &g_spr_b_yara;
	return &g_spr_b_player_m;
}

// Carga una imagen de fondo en BG1 (tile 0 queda vacío para las casillas sin imagen).
void bg_image_load(const BgImage* img, int yoff) {
	for (int p = 0; p < img->npal; ++p)
		for (int c = 0; c < 16; ++c) PAL_BG[p * 16 + c] = img->pals[p * 16 + c];
	PAL_BG[0] = 0;
	for (int i = 0; i < 8; ++i) CHARBLOCK(B_CB)[i] = 0;
	dma_copy32(CHARBLOCK(B_CB) + 8, img->tiles, img->ntiles * 8);
	for (int i = 0; i < 32 * 32; ++i) SCREENBLOCK(B_SB)[i] = 0;
	for (int y = 0; y < img->hc; ++y)
		for (int x = 0; x < img->wc; ++x) SCREENBLOCK(B_SB)[(y + yoff) * 32 + x] = img->cells[y * img->wc + x] + 1;
	REG_BG1CNT = (B_CB << 2) | (B_SB << 8) | 2;
	REG_BG1HOFS = 0; REG_BG1VOFS = 0;
}

static void battle_draw(void) {
	bt++;
	u16* o = (u16*)OAM;
	int used = 0;
	// grupo (a la derecha, mirando a la izquierda)
	for (int m = 0; m < M_COUNT; ++m) {
		if (!g_party[m].in_party) continue;
		const SpriteSheet* s = bsheet(m);
		int f = g_party[m].hp <= 0 ? 7 : (hurt[m] ? 5 : pose[m]);
		if (f >= s->frames) f = 0;
		dma_copy32(OBJ_TILES + m * 16 * 8, s->tiles + f * 16 * 8, 16 * 8);
		int x = 168 + m * 10, y = 24 + m * 26;
		int hide = hurt[m] && (bt & 2);
		o[used * 4] = hide ? 0x0200 : (y & 0xFF);
		o[used * 4 + 1] = (x & 0x1FF) | (2 << 14);
		o[used * 4 + 2] = (m * 16) | (1 << 10) | (m << 12);
		if (hurt[m]) hurt[m]--;
		++used;
	}
	// enemigos (de dos piezas de 64x64)
	for (int i = 0; i < nfoes; ++i) {
		Foe* e = &foes[i];
		int fr = (bt / 24) & 1;
		const SpriteSheet* s = e->d->sheet;
		for (int p = 0; p < e->d->parts; ++p) {
			int base = 64 + (i * 2 + p) * 64;
			if (base + 64 > 1024) break;
			dma_copy32(OBJ_TILES + base * 8, s->tiles + (fr * e->d->parts + p) * 64 * 8, 64 * 8);
			int x = 8 + p * 64, y = 32 + i * 30;
			int hide = !e->alive || (e->flash && (bt & 2));
			o[used * 4] = hide ? 0x0200 : (y & 0xFF);
			o[used * 4 + 1] = (x & 0x1FF) | (3 << 14);
			o[used * 4 + 2] = base | (1 << 10) | ((4 + i) << 12);
			++used;
		}
		if (e->flash) e->flash--;
	}
	for (int k = used; k < 32; ++k) o[k * 4] = 0x0200;
}

static void status_draw(void) {
	text_window(0, 14, 30, 6);
	for (int m = 0; m < M_COUNT; ++m) {
		Member* p = &g_party[m];
		if (!p->in_party) continue;
		char b[48];
		int y = 14 * 8 + 5 + m * 13;
		text_draw_px(8, y, p->name, p->hp > 0 ? 3 : 10);
		// texto con PV y PM
		int hp = p->hp, mp = p->mp;
		char* d = b;
		const char* pre = "PV ";
		while (*pre) *d++ = *pre++;
		if (hp >= 100) *d++ = '0' + hp / 100;
		if (hp >= 10) *d++ = '0' + hp / 10 % 10;
		*d++ = '0' + hp % 10;
		*d++ = '/';
		int mh = p->maxhp;
		if (mh >= 100) *d++ = '0' + mh / 100;
		if (mh >= 10) *d++ = '0' + mh / 10 % 10;
		*d++ = '0' + mh % 10;
		pre = "   PM ";
		while (*pre) *d++ = *pre++;
		if (mp >= 10) *d++ = '0' + mp / 10;
		*d++ = '0' + mp % 10;
		*d = 0;
		text_draw_px(84, y, b, p->hp * 4 < p->maxhp ? 9 : 3);
	}
}

static void redraw(const char* msg) {
	text_hide();
	status_draw();
	if (msg) {
		text_window(0, 0, 30, 3);
		text_draw_px(8, 6, msg, 3);
	}
}

static void msg(const char* s) {
	redraw(s);
	for (int i = 0; i < 70; ++i) { frame_wait(); if (i > 10 && key_hit(KEY_A)) break; }
}

static void numcat(char* d, int v) {
	char t[8]; int n = 0;
	if (v <= 0) { strcat(d, "0"); return; }
	while (v) { t[n++] = '0' + v % 10; v /= 10; }
	d += strlen(d);
	while (n) *d++ = t[--n];
	*d = 0;
}

static void say3(const char* a, const char* b, int v, const char* c) {
	strcpy(msgbuf, a); strcat(msgbuf, b);
	if (v >= 0) numcat(msgbuf, v);
	strcat(msgbuf, c);
	msg(msgbuf);
}

static int menu(const char* const* opts, int n, const char* title) {
	redraw(title);
	text_choice(opts, n);
	int r;
	while ((r = text_choice_result()) < 0) {
		frame_wait();
		if (key_hit(KEY_B)) { text_hide(); return -1; }
	}
	return r;
}

static int dmg(int atk, int def, int pct) {
	int d = (atk * 2 - def) * pct / 100;
	d = d * (90 + rnd(21)) / 100;
	return d < 1 ? 1 : d;
}

static int pick_ally(const char* title) {
	const char* opts[M_COUNT]; int map[M_COUNT], n = 0;
	for (int m = 0; m < M_COUNT; ++m) if (g_party[m].in_party) { opts[n] = g_party[m].name; map[n++] = m; }
	int r = menu(opts, n, title);
	return r < 0 ? -1 : map[r];
}

static void heal(int m, int amount, const char* who) {
	Member* t = &g_party[m];
	if (t->hp <= 0) { say3(t->name, " no responde...", -1, ""); return; }
	t->hp += amount;
	if (t->hp > t->maxhp) t->hp = t->maxhp;
	strcpy(msgbuf, who); strcat(msgbuf, " cura a "); strcat(msgbuf, t->name); strcat(msgbuf, ": +");
	numcat(msgbuf, amount); strcat(msgbuf, " PV.");
	msg(msgbuf);
}

static void hit_foe(Foe* e, int d, const char* who, const char* how) {
	e->hp -= d; e->flash = 24;
	strcpy(msgbuf, who); strcat(msgbuf, how); numcat(msgbuf, d); strcat(msgbuf, " de daño.");
	msg(msgbuf);
	if (e->hp <= 0) { e->hp = 0; e->alive = 0; say3(e->d->name, " cae derrotado.", -1, ""); }
}

static Foe* first_foe(void) { for (int i = 0; i < nfoes; ++i) if (foes[i].alive) return &foes[i]; return 0; }

// Devuelve 0 si el jugador cancela (para volver a mostrar el menú).
static int player_turn(int m) {
	Member* p = &g_party[m];
	defending[m] = 0;
	static const char* const top[] = {"Atacar", "Habilidad", "Defender", "Objeto"};
	strcpy(msgbuf, "Turno de "); strcat(msgbuf, p->name);
	int c = menu(top, 4, msgbuf);
	Foe* e = first_foe();
	if (c < 0) return 0;
	if (c == 0) {
		pose[m] = 3; hit_foe(e, dmg(p->atk, e->d->def, 100), p->name, " ataca: "); pose[m] = 0;
		return 1;
	}
	if (c == 1) {
		const char* name; int cost;
		if (m == M_KAELEN) { name = "Golpe doble (4 PM)"; cost = 4; }
		else if (m == M_YARA) { name = "Curar (5 PM)"; cost = 5; }
		else if (g_class == 0) { name = "Tajo fuerte (5 PM)"; cost = 5; }
		else if (g_class == 1) { name = "Llamarada (6 PM)"; cost = 6; }
		else { name = "Luz sanadora (5 PM)"; cost = 5; }
		const char* opts[1] = {name};
		if (menu(opts, 1, "Habilidad") < 0) return 0;
		if (p->mp < cost) { msg("No tienes maná suficiente."); return 0; }
		int healer = m == M_YARA || (m == M_PLAYER && g_class == 2);
		int t = -1;
		if (healer) { t = pick_ally("¿A quién curas?"); if (t < 0) return 0; }
		p->mp -= cost;
		pose[m] = healer || (m == M_PLAYER && g_class == 1) ? 4 : 3;
		if (healer) heal(t, p->mag * 3 + rnd(6), p->name);
		else if (m == M_KAELEN) {
			hit_foe(e, dmg(p->atk, e->d->def, 80), p->name, " golpea: ");
			if ((e = first_foe())) hit_foe(e, dmg(p->atk, e->d->def, 80), p->name, " vuelve a golpear: ");
		}
		else if (g_class == 0) hit_foe(e, dmg(p->atk, e->d->def, 165), p->name, " lanza un tajo fuerte: ");
		else hit_foe(e, dmg(p->mag, e->d->res, 220), p->name, " invoca una llamarada: ");
		pose[m] = 0;
		return 1;
	}
	if (c == 2) { defending[m] = 1; say3(p->name, " se pone en guardia.", -1, ""); return 1; }
	if (g_potions <= 0) { msg("No te quedan pociones."); return 0; }
	static const char* pot[1];
	static char potname[24];
	strcpy(potname, "Poción x"); potname[8] = 0; numcat(potname, g_potions);
	pot[0] = potname;
	if (menu(pot, 1, "Objeto") < 0) return 0;
	int t = pick_ally("¿A quién se la das?");
	if (t < 0) return 0;
	g_potions--;
	heal(t, 50, p->name);
	return 1;
}

static void foe_turn(Foe* e) {
	int alive[M_COUNT], n = 0;
	for (int m = 0; m < M_COUNT; ++m) if (g_party[m].in_party && g_party[m].hp > 0) alive[n++] = m;
	if (!n) return;
	int t = alive[rnd(n)];
	int pct = 100;
	const char* how = " muerde a ";
	if (rnd(4) == 0) { pct = 140; how = " se lanza a la yugular de "; }
	int d = dmg(e->d->atk, g_party[t].def, pct);
	if (defending[t]) d /= 2;
	g_party[t].hp -= d;
	if (g_party[t].hp < 0) g_party[t].hp = 0;
	hurt[t] = 24;
	strcpy(msgbuf, e->d->name); strcat(msgbuf, how); strcat(msgbuf, g_party[t].name);
	strcat(msgbuf, ": "); numcat(msgbuf, d); strcat(msgbuf, ".");
	msg(msgbuf);
	if (g_party[t].hp == 0) say3(g_party[t].name, " cae al suelo.", -1, "");
}

static int party_alive(void) {
	for (int m = 0; m < M_COUNT; ++m) if (g_party[m].in_party && g_party[m].hp > 0) return 1;
	return 0;
}

int battle_run(const EnemyDef* defs, int count, int boss) {
	(void)boss;
	fade_out();
	g_scene = SCENE_BATTLE;
	REG_DISPCNT = DCNT_BLANK;
	text_hide();
	sprites_hide_all();
	bg_image_load(&g_img_battle_forest, 0);
	for (int m = 0; m < M_COUNT; ++m) {
		const u16* pal = bsheet(m)->pal;
		for (int c = 0; c < 16; ++c) PAL_OBJ[m * 16 + c] = pal[c];
		defending[m] = hurt[m] = pose[m] = 0;
	}
	nfoes = count > MAXF ? MAXF : count;
	for (int i = 0; i < nfoes; ++i) {
		foes[i].d = &defs[i]; foes[i].hp = defs[i].hp; foes[i].alive = 1; foes[i].flash = 0;
		for (int c = 0; c < 16; ++c) PAL_OBJ[(4 + i) * 16 + c] = defs[i].sheet->pal[c];
	}
	rng ^= bt * 2654435761u;
	g_draw = battle_draw;
	g_choice_x = 0;
	battle_draw();
	REG_DISPCNT = DCNT_MODE0 | DCNT_OBJ1D | DCNT_BG0 | DCNT_BG1 | DCNT_OBJ;
	redraw(0);
	fade_in();
	say3("¡Aparece ", defs[0].name, -1, "!");
	int result = 0;
	for (;;) {
		// orden de la ronda por velocidad (con algo de azar)
		int order[M_COUNT + MAXF], key[M_COUNT + MAXF], n = 0;
		for (int m = 0; m < M_COUNT; ++m) if (g_party[m].in_party && g_party[m].hp > 0) { order[n] = m; key[n++] = g_party[m].spd * 4 + rnd(12); }
		for (int i = 0; i < nfoes; ++i) if (foes[i].alive) { order[n] = 100 + i; key[n++] = foes[i].d->spd * 4 + rnd(12); }
		for (int a = 0; a < n; ++a) for (int b = a + 1; b < n; ++b)
			if (key[b] > key[a]) { int t = key[a]; key[a] = key[b]; key[b] = t; t = order[a]; order[a] = order[b]; order[b] = t; }
		for (int k = 0; k < n; ++k) {
			int who = order[k];
			if (who >= 100) { if (foes[who - 100].alive) foe_turn(&foes[who - 100]); }
			else if (g_party[who].hp > 0) {
				while (!player_turn(who)) {}
				Member* p = &g_party[who];
				p->mp += 2; if (p->mp > p->maxmp) p->mp = p->maxmp;
			}
			if (!first_foe()) { result = 1; goto done; }
			if (!party_alive()) { result = 0; goto done; }
		}
	}
done:
	if (result) {
		int xp = 0;
		for (int i = 0; i < nfoes; ++i) xp += foes[i].d->xp;
		g_xp += xp;
		say3("¡Victoria! Ganáis ", "", xp, " de experiencia.");
		while (g_xp >= g_level * 30) {
			g_xp -= g_level * 30;
			g_level++;
			party_recalc();
			say3("¡Subís a nivel ", "", g_level, "!");
		}
		for (int m = 0; m < M_COUNT; ++m) if (g_party[m].hp <= 0) g_party[m].hp = 1;
	} else {
		msg("El grupo ha caído...");
	}
	fade_out();
	text_hide();
	g_draw = 0;
	g_choice_x = -1;
	return result;
}
