// Mapas: carga por casillas al desplazar la cámara, colisiones, entidades y sprites.
#include "game.h"
#include <string.h>

#define MAP_CB   1           // los tiles del mapa ocupan los charblocks 1 y 2 (1024 huecos)
#define MAP_SB   28          // screenblock del mapa (32x32)
#define VIEW_W   31
#define VIEW_H   21
#define FP       8           // posiciones en punto fijo (x256)

int g_map = -1;
s32 g_camx, g_camy;
Entity g_ent[MAX_ENT];
static const MapData* M;
static int view_cx = -999, view_cy = -999;

// ---------------------------------------------------------------- casillas
static void load_cell(int cx, int cy) {
	int slot = ((cy & 31) << 5) | (cx & 31);
	volatile u32* dst = CHARBLOCK(MAP_CB) + slot * 8;
	if (cx < 0 || cy < 0 || cx >= M->wc || cy >= M->hc) {
		for (int i = 0; i < 8; ++i) dst[i] = 0;
		SCREENBLOCK(MAP_SB)[slot] = slot;
		return;
	}
	int ci = cy * M->wc + cx;
	const u32* src = M->tiles + M->cells[ci] * 8;
	for (int i = 0; i < 8; ++i) dst[i] = src[i];
	SCREENBLOCK(MAP_SB)[slot] = slot | (M->cpal[ci] << 12);
}

static void view_reload(int cx0, int cy0) {
	for (int y = 0; y < VIEW_H; ++y)
		for (int x = 0; x < VIEW_W; ++x)
			load_cell(cx0 + x, cy0 + y);
	view_cx = cx0;
	view_cy = cy0;
}

static void view_update(void) {
	int cx0 = g_camx >> 3, cy0 = g_camy >> 3;
	if (cx0 == view_cx && cy0 == view_cy) return;
	if (cx0 - view_cx > 2 || view_cx - cx0 > 2 || cy0 - view_cy > 2 || view_cy - cy0 > 2) {
		view_reload(cx0, cy0);
		return;
	}
	while (view_cx < cx0) { ++view_cx; for (int y = 0; y < VIEW_H; ++y) load_cell(view_cx + VIEW_W - 1, view_cy + y); }
	while (view_cx > cx0) { --view_cx; for (int y = 0; y < VIEW_H; ++y) load_cell(view_cx, view_cy + y); }
	while (view_cy < cy0) { ++view_cy; for (int x = 0; x < VIEW_W; ++x) load_cell(view_cx + x, view_cy + VIEW_H - 1); }
	while (view_cy > cy0) { --view_cy; for (int x = 0; x < VIEW_W; ++x) load_cell(view_cx + x, view_cy); }
}

// ---------------------------------------------------------------- colisión
int map_solid_at(int x, int y) {
	if (x < 0 || y < 0) return 1;
	int cx = x >> 3, cy = y >> 3;
	if (cx >= M->wc || cy >= M->hc) return 1;
	int i = cy * M->wc + cx;
	return (M->coll[i >> 3] >> (i & 7)) & 1;
}

static int blocked(int x, int y) {
	// caja de los pies: 8x4
	return map_solid_at(x - 4, y - 3) || map_solid_at(x + 3, y - 3) || map_solid_at(x - 4, y) || map_solid_at(x + 3, y);
}

int map_marker(const char* name, int* x, int* y) {
	for (int i = 0; i < M->nmarkers; ++i)
		if (!strcmp(M->markers[i].id, name)) { *x = M->markers[i].x; *y = M->markers[i].y; return 1; }
	return 0;
}

// ---------------------------------------------------------------- entidades
static int obj_pal_of(const SpriteSheet* s);

int ent_spawn(const char* id, const SpriteSheet* sheet, int x, int y, int facing) {
	for (int i = 1; i < MAX_ENT; ++i) {
		if (!g_ent[i].active) {
			Entity* e = &g_ent[i];
			memset(e, 0, sizeof *e);
			e->active = e->visible = 1;
			e->sheet = sheet;
			e->x = x << FP; e->y = y << FP;
			e->facing = facing;
			e->id = id;
			e->oam_slot = i;
			return i;
		}
	}
	return -1;
}

Entity* ent_find(const char* id) {
	for (int i = 0; i < MAX_ENT; ++i)
		if (g_ent[i].active && g_ent[i].id && !strcmp(g_ent[i].id, id)) return &g_ent[i];
	return 0;
}

void ent_remove(const char* id) {
	Entity* e = ent_find(id);
	if (e && e != &g_ent[ENT_PLAYER]) e->active = 0;
}

// rastro del jugador para que los compañeros le sigan
#define TRAIL 64
static s32 trail_x[TRAIL], trail_y[TRAIL];
static u8 trail_f[TRAIL];
static int trail_head;

static void trail_reset(void) {
	for (int i = 0; i < TRAIL; ++i) { trail_x[i] = g_ent[0].x; trail_y[i] = g_ent[0].y; trail_f[i] = g_ent[0].facing; }
	trail_head = 0;
}

void party_followers_spawn(void) {
	int px = g_ent[0].x >> FP, py = g_ent[0].y >> FP;
	if (g_party[M_KAELEN].in_party && !ent_find("kaelen")) {
		int i = ent_spawn("kaelen", &g_spr_kaelen, px, py, g_ent[0].facing);
		g_ent[i].follow = 1;
	}
	if (g_party[M_YARA].in_party && !ent_find("yara")) {
		int i = ent_spawn("yara", &g_spr_yara, px, py, g_ent[0].facing);
		g_ent[i].follow = 2;
	}
	trail_reset();
}

// ---------------------------------------------------------------- carga de mapa
void map_load(int map, const char* spawn) {
	REG_DISPCNT = DCNT_BLANK;
	g_map = map;
	M = &g_maps[map];
	for (int p = 0; p < 14; ++p)
		for (int c = 0; c < 16; ++c)
			PAL_BG[p * 16 + c] = M->pals[p * 16 + c];
	PAL_BG[0] = rgb15(2, 1, 3);
	// conservar al jugador; quitar el resto
	Entity pl = g_ent[0];
	memset(g_ent, 0, sizeof g_ent);
	g_ent[0] = pl;
	g_ent[0].active = g_ent[0].visible = 1;
	g_ent[0].moving = 0;
	int x = 40, y = 40;
	if (!spawn || !map_marker(spawn, &x, &y)) {
		if (M->nmarkers) { x = M->markers[0].x; y = M->markers[0].y; }
	}
	g_ent[0].x = x << FP;
	g_ent[0].y = y << FP;
	party_followers_spawn();
	g_camx = x - 120; g_camy = y - 90;
	map_update();
	view_reload(g_camx >> 3, g_camy >> 3);
	REG_BG1CNT = (MAP_CB << 2) | (MAP_SB << 8) | 2;
	REG_DISPCNT = DCNT_MODE0 | DCNT_OBJ1D | DCNT_BG0 | DCNT_BG1 | DCNT_OBJ;
	story_on_map(map);
}

// ---------------------------------------------------------------- actualización
static void camera_follow(void) {
	int tx = (g_ent[0].x >> FP) - 120, ty = (g_ent[0].y >> FP) - 92;
	int maxx = M->wc * 8 - 240, maxy = M->hc * 8 - 160;
	if (tx > maxx) tx = maxx;
	if (ty > maxy) ty = maxy;
	if (tx < 0) tx = 0;
	if (ty < 0) ty = 0;
	if (M->wc * 8 < 240) tx = (M->wc * 8 - 240) / 2;
	if (M->hc * 8 < 160) ty = (M->hc * 8 - 160) / 2;
	g_camx = tx;
	g_camy = ty;
}

static void check_exits_and_zones(void) {
	int px = g_ent[0].x >> FP, py = g_ent[0].y >> FP;
	static int last_zone = -1;
	int inzone = -1;
	for (int i = 0; i < M->nzones; ++i) {
		const MapZone* z = &M->zones[i];
		if (px >= z->x && px < z->x + z->w && py >= z->y - 4 && py < z->y + z->h + 4) inzone = i;
	}
	if (inzone >= 0 && inzone != last_zone) {
		last_zone = inzone;
		story_on_zone(M->zones[inzone].id);
		return;
	}
	if (inzone < 0) last_zone = -1;
	for (int i = 0; i < M->nexits; ++i) {
		const MapExit* e = &M->exits[i];
		if (px >= e->x - 2 && px < e->x + e->w + 2 && py >= e->y - 2 && py < e->y + e->h + 4) {
			if (e->to == 255) continue;
			if (!story_exit_allowed(e->to)) {
				// empujar fuera de la salida
				static const s8 back[4][2] = {{0, -6}, {6, 0}, {-6, 0}, {0, 6}};
				g_ent[0].x += back[g_ent[0].facing][0] << FP;
				g_ent[0].y += back[g_ent[0].facing][1] << FP;
				return;
			}
			const char* sp = g_map_exit_spawns[g_map][i];
			fade_out();
			map_load(e->to, sp);
			fade_in();
			return;
		}
	}
}

static void interact(void) {
	int px = g_ent[0].x >> FP, py = (g_ent[0].y >> FP) - 6;
	static const s8 dir[4][2] = {{0, 1}, {-1, 0}, {1, 0}, {0, -1}};
	int fx = px + dir[g_ent[0].facing][0] * 14, fy = py + dir[g_ent[0].facing][1] * 14;
	// personajes
	for (int i = 1; i < MAX_ENT; ++i) {
		Entity* e = &g_ent[i];
		if (!e->active || e->follow) continue;
		int dx = (e->x >> FP) - fx, dy = ((e->y >> FP) - 6) - fy;
		if (dx * dx + dy * dy < 16 * 16) {
			static const u8 opp[4] = {3, 2, 1, 0};
			e->facing = opp[g_ent[0].facing];
			story_on_interact(e->id);
			return;
		}
	}
	// objetos del mapa
	int best = -1, bd = 22 * 22;
	for (int i = 0; i < M->nobjs; ++i) {
		int dx = M->objs[i].x - fx, dy = (M->objs[i].y - 6) - fy;
		int d = dx * dx + dy * dy;
		if (d < bd) { bd = d; best = i; }
	}
	if (best >= 0) story_on_interact(M->objs[best].id);
}

static void move_entity_towards(Entity* e, int speed) {
	int dx = (e->tx << FP) - e->x, dy = (e->ty << FP) - e->y;
	int adx = dx < 0 ? -dx : dx, ady = dy < 0 ? -dy : dy;
	if (adx <= speed && ady <= speed) {
		e->x = e->tx << FP; e->y = e->ty << FP; e->moving = 0; e->walking = 0;
		return;
	}
	if (adx > ady) { e->x += dx > 0 ? speed : -speed; e->facing = dx > 0 ? 2 : 1; }
	else { e->y += dy > 0 ? speed : -speed; e->facing = dy > 0 ? 0 : 3; }
	e->walking = 1;
	e->anim++;
}

void map_update(void) {
	Entity* p = &g_ent[0];
	if (!g_cutscene && !text_busy()) {
		int sp = key_held(KEY_B) ? 500 : 307;   // correr con B
		int dx = 0, dy = 0;
		if (key_held(KEY_LEFT)) dx = -1;
		else if (key_held(KEY_RIGHT)) dx = 1;
		if (key_held(KEY_UP)) dy = -1;
		else if (key_held(KEY_DOWN)) dy = 1;
		if (dx || dy) {
			if (dx && dy) sp = sp * 181 >> 8;
			if (dx) p->facing = dx > 0 ? 2 : 1;
			else p->facing = dy > 0 ? 0 : 3;
			int nx = p->x + dx * sp, ny = p->y + dy * sp;
			if (!blocked(nx >> FP, p->y >> FP)) p->x = nx;
			if (!blocked(p->x >> FP, ny >> FP)) p->y = ny;
			p->walking = 1;
			p->anim += key_held(KEY_B) ? 2 : 1;
			// rastro
			trail_head = (trail_head + 1) % TRAIL;
			trail_x[trail_head] = p->x; trail_y[trail_head] = p->y; trail_f[trail_head] = p->facing;
		} else {
			p->walking = 0;
		}
		if (key_hit(KEY_A)) interact();
		else check_exits_and_zones();
	} else if (p->moving) {
		move_entity_towards(p, 256);
		trail_head = (trail_head + 1) % TRAIL;
		trail_x[trail_head] = p->x; trail_y[trail_head] = p->y; trail_f[trail_head] = p->facing;
	} else {
		p->walking = 0;
	}
	for (int i = 1; i < MAX_ENT; ++i) {
		Entity* e = &g_ent[i];
		if (!e->active) continue;
		if (e->moving) { move_entity_towards(e, 256); continue; }
		if (e->follow) {
			int k = (trail_head - e->follow * 14 + TRAIL * 4) % TRAIL;
			int was_x = e->x, was_y = e->y;
			e->x = trail_x[k]; e->y = trail_y[k]; e->facing = trail_f[k];
			e->walking = (was_x != e->x || was_y != e->y);
			if (e->walking) e->anim++;
		}
	}
	camera_follow();
}

// ---------------------------------------------------------------- dibujo (OAM)
static u16 oam_shadow[128 * 4];
static const SpriteSheet* loaded_sheet[MAX_ENT];
static int loaded_frame[MAX_ENT];
static const SpriteSheet* pal_sheets[16];

static int obj_pal_of(const SpriteSheet* s) {
	for (int i = 0; i < 16; ++i) if (pal_sheets[i] == s) return i;
	for (int i = 0; i < 16; ++i) if (!pal_sheets[i]) {
		pal_sheets[i] = s;
		for (int c = 0; c < 16; ++c) PAL_OBJ[i * 16 + c] = s->pal[c];
		return i;
	}
	return 0;
}

void sprites_hide_all(void) {
	for (int i = 0; i < 128; ++i) { oam_shadow[i * 4] = 0x0200; oam_shadow[i * 4 + 1] = 0; oam_shadow[i * 4 + 2] = 0; }
	for (int i = 0; i < 128 * 4; ++i) OAM[i] = oam_shadow[i];
	memset(loaded_sheet, 0, sizeof loaded_sheet);
	memset(pal_sheets, 0, sizeof pal_sheets);
}

void map_draw(void) {
	// orden por profundidad: el más bajo en pantalla se dibuja delante (índice OAM menor)
	int order[MAX_ENT], n = 0;
	for (int i = 0; i < MAX_ENT; ++i) if (g_ent[i].active && g_ent[i].visible) order[n++] = i;
	for (int a = 0; a < n; ++a)
		for (int b = a + 1; b < n; ++b)
			if (g_ent[order[b]].y > g_ent[order[a]].y) { int t = order[a]; order[a] = order[b]; order[b] = t; }
	int used = 0;
	for (int k = 0; k < n; ++k) {
		int i = order[k];
		Entity* e = &g_ent[i];
		int col = e->walking ? 2 + (e->anim / 6) % 6 : (e->anim / 40) % 2;
		if (!e->walking) e->anim++;
		int frame = e->facing * 8 + col;
		if (frame >= e->sheet->frames) frame = 0;
		if (loaded_sheet[i] != e->sheet || loaded_frame[i] != frame) {
			dma_copy32(OBJ_TILES + i * 16 * 8, e->sheet->tiles + frame * 16 * 8, 16 * 8);
			loaded_sheet[i] = e->sheet;
			loaded_frame[i] = frame;
		}
		int sx = (e->x >> FP) - 16 - g_camx, sy = (e->y >> FP) - 31 - g_camy;
		u16* o = &oam_shadow[used * 4];
		if (sx < -32 || sx > 240 || sy < -32 || sy > 160) { o[0] = 0x0200; ++used; continue; }
		o[0] = (sy & 0xFF) | 0x0000;                 // forma cuadrada, 4bpp
		o[1] = (sx & 0x1FF) | (2 << 14);             // 32x32
		o[2] = (i * 16) | (1 << 10) | (obj_pal_of(e->sheet) << 12);   // prioridad 1 (sobre el mapa)
		++used;
	}
	for (int k = used; k < 32; ++k) oam_shadow[k * 4] = 0x0200;
	for (int k = 0; k < 32 * 4; ++k) OAM[k] = oam_shadow[k];
	REG_BG1HOFS = g_camx;
	REG_BG1VOFS = g_camy;
	view_update();
}
