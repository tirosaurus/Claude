// Entidades del mapa: jugador, PNJ, seguidores y sprites de escena; colisiones, movimiento,
// interacción, salidas, zonas y encuentros (Player.gd, NPC.gd y parte de World.gd).
#include "world.h"
#include "dialogue.h"
#include "scene.h"
#include "audio.h"
#include "ui.h"

#define WALK_SPEED 72
#define RUN_SPEED 118
static const s8 DIRV[4][2] = {{0, 1}, {-1, 0}, {1, 0}, {0, -1}};

// rastro del jugador (para los seguidores)
#define TRAIL 120
static s32 trail_x[TRAIL] EWRAM_BSS, trail_y[TRAIL] EWRAM_BSS;
static int trail_n;
static s32 anim_t;                 // jugador
static int step_count;
static u8 player_locked;

// ------------------------------------------------------------------ colisiones
typedef struct { s16 x0, y0, x1, y1; } Box;
#define MAX_RECTS 600
static Box rects[MAX_RECTS] EWRAM_BSS;
static int nrects;
#define BUCKET 32
static u16 bucket_start[33 * 31] EWRAM_BSS;
static u16 bucket_items[3072] EWRAM_BSS;
static int bw, bh;

static void add_rect(int x, int y, int w, int h) {
	if (nrects >= MAX_RECTS) return;
	Box* b = &rects[nrects++];
	b->x0 = x; b->y0 = y; b->x1 = x + w; b->y1 = y + h;
}

static int prop_col_active(int i) {
	const MapProp* p = &W.m->props[i];
	if (!p->has_col || W.prop_removed[i]) return 0;
	if (p->patch >= 0 && W.m->patches[p->patch].kind == 2 && !W.pstate[p->patch]) return 0;
	return 1;
}

void coll_build(void) {
	nrects = 0;
	for (int i = 0; i < W.m->nsolids; ++i) {
		const Rect16* r = &W.m->solids[i];
		add_rect(r->x, r->y, r->w, r->h);
	}
	for (int i = 0; i < W.m->nprops; ++i) {
		if (!prop_col_active(i)) continue;
		const MapProp* p = &W.m->props[i];
		add_rect(p->x + p->col.x, p->y + p->col.y, p->col.w, p->col.h);
	}
	bw = (W.m->w + BUCKET - 1) / BUCKET;
	bh = (W.m->h + BUCKET - 1) / BUCKET;
	if (bw > 32) bw = 32;
	if (bh > 30) bh = 30;
	// recuento y llenado (dos pasadas)
	static u16 cnt[33 * 31] EWRAM_BSS;
	memset(cnt, 0, sizeof cnt);
	for (int pass = 0; pass < 2; ++pass) {
		if (pass == 1) {
			int acc = 0;
			for (int i = 0; i < bw * bh; ++i) { bucket_start[i] = acc; acc += cnt[i]; cnt[i] = 0; }
			bucket_start[bw * bh] = acc;
		}
		for (int r = 0; r < nrects; ++r) {
			int x0 = CLAMP(rects[r].x0 / BUCKET, 0, bw - 1), x1 = CLAMP((rects[r].x1 - 1) / BUCKET, 0, bw - 1);
			int y0 = CLAMP(rects[r].y0 / BUCKET, 0, bh - 1), y1 = CLAMP((rects[r].y1 - 1) / BUCKET, 0, bh - 1);
			for (int by = y0; by <= y1; ++by)
				for (int bx = x0; bx <= x1; ++bx) {
					int b = by * bw + bx;
					if (pass == 1) {
						int k = bucket_start[b] + cnt[b];
						if (k < 3072) bucket_items[k] = r;
					}
					cnt[b]++;
				}
		}
	}
}
void coll_rebuild(void) { coll_build(); }

// ¿choca la caja [x0,x1)x[y0,y1) (x256)? ignora la entidad self
static int overlaps(s32 x0, s32 y0, s32 x1, s32 y1, Ent* self) {
	int px0 = x0 >> FX, py0 = y0 >> FX, px1 = (x1 + 255) >> FX, py1 = (y1 + 255) >> FX;
	if (x0 < 0 || y0 < 0 || x1 > (W.m->w << FX) || y1 > (W.m->h << FX)) return 1;
	int bx0 = CLAMP(px0 / BUCKET, 0, bw - 1), bx1 = CLAMP((px1 - 1) / BUCKET, 0, bw - 1);
	int by0 = CLAMP(py0 / BUCKET, 0, bh - 1), by1 = CLAMP((py1 - 1) / BUCKET, 0, bh - 1);
	for (int by = by0; by <= by1; ++by)
		for (int bx = bx0; bx <= bx1; ++bx) {
			int b = by * bw + bx;
			for (int k = bucket_start[b]; k < bucket_start[b + 1] && k < 3072; ++k) {
				Box* r = &rects[bucket_items[k]];
				if ((r->x0 << FX) < x1 && (r->x1 << FX) > x0 && (r->y0 << FX) < y1 && (r->y1 << FX) > y0) return 1;
			}
		}
	// cuerpos de los PNJ quietos o paseando (10x6)
	for (int i = 1; i < MAX_ENT; ++i) {
		Ent* e = &g_ent[i];
		if (!e->used || e == self || e->kind != ENT_NPC || e->mode == MODE_FOLLOW) continue;
		s32 ex0 = e->x - (5 << FX), ey0 = e->y - (6 << FX), ex1 = e->x + (5 << FX), ey1 = e->y;
		if (ex0 < x1 && ex1 > x0 && ey0 < y1 && ey1 > y0) return 1;
	}
	if (self != PLAYER && self && self->kind == ENT_NPC && self->mode != MODE_FOLLOW) {
		Ent* p = PLAYER;
		s32 ex0 = p->x - (5 << FX), ey0 = p->y - (6 << FX), ex1 = p->x + (5 << FX), ey1 = p->y;
		if (ex0 < x1 && ex1 > x0 && ey0 < y1 && ey1 > y0) return 1;
	}
	return 0;
}

static int body_hits(Ent* e, s32 x, s32 y) { return overlaps(x - (5 << FX), y - (6 << FX), x + (5 << FX), y, e); }

// move_and_slide: por ejes, acercándose al contacto
static void move_slide(Ent* e, s32 dx, s32 dy) {
	if (dx) {
		if (!body_hits(e, e->x + dx, e->y)) e->x += dx;
		else {
			s32 step = dx > 0 ? 64 : -64;
			for (s32 m = step; ABS(m) <= ABS(dx); m += step) { if (body_hits(e, e->x + step, e->y)) break; e->x += step; }
		}
	}
	if (dy) {
		if (!body_hits(e, e->x, e->y + dy)) e->y += dy;
		else {
			s32 step = dy > 0 ? 64 : -64;
			for (s32 m = step; ABS(m) <= ABS(dy); m += step) { if (body_hits(e, e->x, e->y + step)) break; e->y += step; }
		}
	}
}

// ------------------------------------------------------------------ entidades
static Ent* ent_alloc(void) {
	for (int i = 1; i < MAX_ENT; ++i)
		if (!g_ent[i].used) {
			memset(&g_ent[i], 0, sizeof(Ent));
			g_ent[i].used = 1;
			g_ent[i].visible = 1;
			g_ent[i].tr = g_ent[i].tg = g_ent[i].tb = 255;
			g_ent[i].alpha = 16;
			g_ent[i].scale = 256;
			g_ent[i].tile = -1;
			g_ent[i].pal = -1;
			g_ent[i].up_frame = -1;
			g_ent[i].member = -1;
			g_ent[i].breath_off = rnd() & 1;
			return &g_ent[i];
		}
	return 0;
}

static void ent_free_video(Ent* e) {
	if (e->tile >= 0) objt_free(e->tile);
	if (e->pal >= 0) objpal_put(e->pal);
	e->tile = -1;
	e->pal = -1;
	e->up_frame = -1;
}

void ent_remove(Ent* e) {
	if (!e || e == PLAYER) return;
	ent_free_video(e);
	e->used = 0;
}

void ents_reset_keep_player(void) {
	for (int i = 1; i < MAX_ENT; ++i) g_ent[i].used = 0;
}

Ent* npc(const char* id) {
	for (int i = 1; i < MAX_ENT; ++i)
		if (g_ent[i].used && g_ent[i].kind == ENT_NPC && !strcmp(g_ent[i].id, id)) return &g_ent[i];
	return 0;
}

void remove_npc(const char* id) { Ent* e = npc(id); if (e) ent_remove(e); }

Ent* spawn_npc(const char* id, int x, int y, int face, int mode, const char* sheet_name) {
	Ent* old = npc(id);
	if (old) ent_remove(old);
	Ent* e = ent_alloc();
	if (!e) return 0;
	e->kind = ENT_NPC;
	strncpy(e->id, id, sizeof e->id - 1);
	e->x = x << FX; e->y = y << FX;
	e->home_x = e->x; e->home_y = e->y;
	e->facing = face;
	e->mode = mode;
	e->wradius = 40;
	e->wwait = 60;
	int m = member_from_id(id);
	if (sheet_name) e->sheet = char_sheet(sheet_name);
	else if (!strcmp(id, "mother") && G.race != RACE_HUMAN) e->sheet = char_sheet(G.race == RACE_ELF ? "mother_elf" : "mother_dwarf");
	else e->sheet = char_sheet(id);
	if (mode == MODE_FOLLOW && m > 0) { e->member = m; e->sheet = 0; }
	if (!e->sheet && e->member < 0) e->sheet = char_sheet("villager_m");
	return e;
}

static const u8 FOLLOW_GAPS[4] = {14, 18, 30, 34};
static const s8 FOLLOW_SIDES[4] = {7, -7, 4, -4};

static int follower_index(int m) {
	int k = 0;
	for (int i = 0; i < G.nparty; ++i) {
		if (G.party[i] == M_PLAYER) continue;
		if (G.party[i] == m) return k;
		k++;
	}
	return 0;
}

static void follow_point(Ent* e, s32* ox, s32* oy) {
	int idx = trail_n - 1 - e->follow_gap;
	if (idx < 0) idx = 0;
	s32 tx = trail_x[idx], ty = trail_y[idx];
	int ia = idx + 6 < trail_n - 1 ? idx + 6 : trail_n - 1;
	s32 dx = trail_x[ia] - tx, dy = trail_y[ia] - ty;
	u32 len = isqrt((u32)((dx >> 4) * (dx >> 4) + (dy >> 4) * (dy >> 4))) << 4;
	if (len < 128) { *ox = tx; *oy = ty; return; }
	// perpendicular (-dy, dx) normalizada * follow_side
	*ox = tx + (s32)((s64)(-dy) * e->follow_side / (s32)len);
	*oy = ty + (s32)((s64)dx * e->follow_side / (s32)len);
}

Ent* spawn_follower(int m) {
	int idx = follower_index(m);
	if (trail_n > 2 && ABS(trail_x[0] - trail_x[trail_n - 1]) < 256 && ABS(trail_y[0] - trail_y[trail_n - 1]) < 256)
		player_reset_trail();
	Ent* e = spawn_npc(member_id(m), PX(PLAYER->x), PX(PLAYER->y), PLAYER->facing, MODE_FOLLOW, 0);
	if (!e) return 0;
	e->member = m;
	e->sheet = 0;
	e->follow_gap = FOLLOW_GAPS[idx % 4];
	e->follow_side = FOLLOW_SIDES[idx % 4] << FX;
	follow_point(e, &e->x, &e->y);
	if (m == M_PLAYER) {}
	return e;
}

void make_follower(int m) {
	Ent* e = npc(member_id(m));
	if (!e) { spawn_follower(m); return; }
	int idx = follower_index(m);
	e->follow_gap = FOLLOW_GAPS[idx % 4];
	e->follow_side = FOLLOW_SIDES[idx % 4] << FX;
	e->mode = MODE_FOLLOW;
	e->member = m;
	e->sheet = 0;
	e->busy = 0;
	ent_free_video(e);
}

const SceneSprite* find_scene_sprite(const char* n) {
	for (int i = 0; i < g_nscene_sprites; ++i) if (!strcmp(g_scene_sprites[i].name, n)) return &g_scene_sprites[i];
	return 0;
}

Ent* scene_sprite(const char* name, int x, int y, int flip, int animate) {
	const SceneSprite* s = find_scene_sprite(name);
	if (!s) return 0;
	Ent* e = ent_alloc();
	if (!e) return 0;
	e->kind = ENT_SPRITE;
	strncpy(e->id, name, sizeof e->id - 1);
	e->ms = s->ms;
	e->x = x << FX; e->y = y << FX;
	e->flip = flip;
	e->ms_auto = animate;
	return e;
}
Ent* monster(const char* name, int x, int y, int flip) { return scene_sprite(name, x, y, flip, 1); }

void ent_face(Ent* e, int dir) { if (e) { e->facing = dir; e->walk_anim = 0; } }
static void face_dir(Ent* e, s32 dx, s32 dy) {
	if (!dx && !dy) return;
	if (ABS(dx) > ABS(dy)) e->facing = dx > 0 ? 2 : 1;
	else e->facing = dy > 0 ? 0 : 3;
}
void ent_face_towards(Ent* e, int x, int y) { if (e) face_dir(e, (x << FX) - e->x, (y << FX) - e->y); }
void ent_set_pos(Ent* e, int x, int y) { if (e) { e->x = x << FX; e->y = y << FX; } }
int player_facing(void) { return PLAYER->facing; }
void ent_set_tint(Ent* e, int r, int g, int b) {
	if (!e) return;
	if (e->tr != r || e->tg != g || e->tb != b) { e->tr = r; e->tg = g; e->tb = b; e->pal_key_dirty = 1; }
}

// ------------------------------------------------------------------ jugador y rastro
static void record_trail(void) {
	Ent* p = PLAYER;
	if (trail_n) {
		s32 dx = p->x - trail_x[trail_n - 1], dy = p->y - trail_y[trail_n - 1];
		// distancia >= 1,5 px
		if ((s64)dx * dx + (s64)dy * dy < (s64)384 * 384) return;
	}
	if (trail_n >= TRAIL) {
		memmove(trail_x, trail_x + 1, (TRAIL - 1) * 4);
		memmove(trail_y, trail_y + 1, (TRAIL - 1) * 4);
		trail_n = TRAIL - 1;
	}
	trail_x[trail_n] = p->x;
	trail_y[trail_n] = p->y;
	trail_n++;
}

void player_reset_trail(void) {
	Ent* p = PLAYER;
	static const s8 back[4][2] = {{0, -1}, {1, 0}, {-1, 0}, {0, 1}};
	int f = CLAMP(p->facing, 0, 3);
	trail_n = 0;
	for (int i = 80; i > 0; --i) {
		trail_x[trail_n] = p->x + back[f][0] * i * 384;
		trail_y[trail_n] = p->y + back[f][1] * i * 384;
		trail_n++;
	}
	trail_x[trail_n] = p->x;
	trail_y[trail_n] = p->y;
	trail_n++;
}

void player_setup(int x, int y, int face) {
	memset(g_ent, 0, sizeof g_ent);
	Ent* p = PLAYER;
	p->used = 1;
	p->kind = ENT_PLAYER;
	strcpy(p->id, "player");
	p->visible = 1;
	p->x = x << FX; p->y = y << FX;
	p->facing = face;
	p->member = M_PLAYER;
	p->tile = -1; p->pal = -1; p->up_frame = -1;
	p->alpha = 16;
	p->scale = 256;
	if (flag("player_dark")) { p->tr = 199; p->tg = 158; p->tb = 230; }
	else { p->tr = p->tg = p->tb = 255; }
	anim_t = 0;
	player_locked = 0;
	player_reset_trail();
}

// ------------------------------------------------------------------ caminar por guion
static void path_set(Ent* e, int x, int y, float speed, int direct) {
	int far = ABS(PX(e->x) - x) + ABS(PX(e->y) - y) > (e == PLAYER ? 20 : 0);
	if (!direct && far) e->pn = find_path(PX(e->x), PX(e->y), x, y, e->px, e->py, MAX_PATH);
	else { e->px[0] = x; e->py[0] = y; e->pn = 1; }
	if (!e->pn) { e->px[0] = x; e->py[0] = y; e->pn = 1; }
	e->pi = 0;
	e->speed = (s32)(speed * 256);
	e->moving = 1;
	if (e == PLAYER) player_locked = 1;
	else e->busy = 1;
}

static void path_step(Ent* e) {
	if (!e->moving) return;
	s32 gx = e->px[e->pi] << FX, gy = e->py[e->pi] << FX;
	s32 dx = gx - e->x, dy = gy - e->y;
	u32 dist = isqrt((u32)(((s64)dx * dx + (s64)dy * dy) >> 8)) << 4;    // x256
	if (dist <= 256) {
		if (++e->pi >= e->pn) {
			e->moving = 0;
			e->walk_anim = 0;
			if (e == PLAYER) player_locked = 0;
			else {
				int tx = e->px[e->pn - 1], ty = e->py[e->pn - 1];
				if (ABS(e->x - (tx << FX)) < 3 * 256 && ABS(e->y - (ty << FX)) < 3 * 256) { e->x = tx << FX; e->y = ty << FX; }
				e->busy = 0;
			}
			return;
		}
		return;
	}
	s32 step = e->speed / FPS;
	if ((u32)step > dist) step = dist;
	face_dir(e, dx, dy);
	e->x += (s32)((s64)dx * step / (s32)dist);
	e->y += (s32)((s64)dy * step / (s32)dist);
	e->walk_anim = 1;
	e->anim_t += 256 * 75 / 10 / FPS;
	if (e == PLAYER) record_trail();
}

void walk_to_async(Ent* e, int x, int y, float speed, int direct) { if (e) path_set(e, x, y, speed, direct); }
void walk_to(Ent* e, int x, int y, float speed, int direct) {
	if (!e) return;
	path_set(e, x, y, speed, direct);
	while (e->used && e->moving) frame_wait();
}
int ent_walking(Ent* e) { return e && e->moving; }

void ent_hop(Ent* e) {
	if (!e) return;
	for (int i = 1; i <= 6; ++i) { e->hop_y = -5 * i / 6; frame_wait(); }
	for (int i = 5; i >= 0; --i) { e->hop_y = -5 * i / 6; frame_wait(); }
}

// tween lineal por los puntos del camino (StoryBase.move_node)
static void node_set(Ent* e, int x, int y, float t, int direct) {
	s16 px[MAX_PATH], py[MAX_PATH];
	int n;
	int far = ABS(PX(e->x) - x) + ABS(PX(e->y) - y) > 24;
	if (!direct && far) n = find_path(PX(e->x), PX(e->y), x, y, px, py, MAX_PATH);
	else { px[0] = x; py[0] = y; n = 1; }
	if (!n) { px[0] = x; py[0] = y; n = 1; }
	// longitud total
	float total = 0;
	int ax = PX(e->x), ay = PX(e->y);
	for (int i = 0; i < n; ++i) {
		int dx = px[i] - ax, dy = py[i] - ay;
		total += isqrt(dx * dx + dy * dy);
		ax = px[i]; ay = py[i];
	}
	if (total < 1) total = 1;
	memcpy(e->px, px, n * 2);
	memcpy(e->py, py, n * 2);
	e->pn = n;
	e->pi = 0;
	e->speed = (s32)(total / (t > 0.05f ? t : 0.05f) * 256);
	e->moving = 1;
	e->busy = 1;
}
void move_node_async(Ent* e, int x, int y, float t, int direct) { if (e) node_set(e, x, y, t, direct); }
void move_node(Ent* e, int x, int y, float t, int direct) {
	if (!e) return;
	node_set(e, x, y, t, direct);
	while (e->used && e->moving) frame_wait();
}

void knock_down(Ent* e) {
	if (!e) return;
	sfx("hit", -6.0f, 1.0f);
	e->busy = 1;
	for (int i = 1; i <= 15; ++i) {
		e->rot = 256 * i / 15;
		ent_set_tint(e, 255 - (51 * i / 15), 255 - (102 * i / 15), 255 - (102 * i / 15));
		frame_wait();
	}
}

static struct { Ent* e; int from, to, t, n; } fades[4];
void ent_fade(Ent* e, int to16, float t) {
	for (int i = 0; i < 4; ++i)
		if (!fades[i].e || fades[i].e == e) {
			fades[i].e = e; fades[i].from = e->alpha; fades[i].to = to16; fades[i].t = 0;
			fades[i].n = (int)(t * FPS) < 1 ? 1 : (int)(t * FPS);
			return;
		}
}
static void fades_step(void) {
	for (int i = 0; i < 4; ++i) {
		if (!fades[i].e) continue;
		if (!fades[i].e->used) { fades[i].e = 0; continue; }
		fades[i].t++;
		fades[i].e->alpha = fades[i].from + (fades[i].to - fades[i].from) * fades[i].t / fades[i].n;
		if (fades[i].t >= fades[i].n) fades[i].e = 0;
	}
}

// ------------------------------------------------------------------ actualización de PNJ
static int breath(Ent* e) { return ((g_frame * 1000 / 60 / 700) + e->breath_off) & 1; }

static void npc_update(Ent* e) {
	if (e->moving) { path_step(e); return; }
	if (e->busy) return;
	switch (e->mode) {
	case MODE_FOLLOW: {
		s32 tx, ty;
		follow_point(e, &tx, &ty);
		s32 dx = tx - e->x, dy = ty - e->y;
		if (ABS(dx) > 128 || ABS(dy) > 128) {
			e->x += dx * 60 / 256;      // lerp 14/60
			e->y += dy * 60 / 256;
			face_dir(e, dx, dy);
			e->walk_anim = 1;
			e->anim_t += 256 * 75 / 10 / FPS;
		} else { e->walk_anim = 0; e->anim_t = 0; }
		break;
	}
	case MODE_WANDER:
		if (g_dialog_active) { e->walk_anim = 0; break; }
		if (e->wwait > 0) {
			e->wwait--;
			e->walk_anim = 0;
			if (e->wwait <= 0) {
				e->wtx = e->home_x + (s32)(randf_range(-1, 1) * e->wradius * 256);
				e->wty = e->home_y + (s32)(randf_range(-1, 1) * e->wradius * 256);
			}
			break;
		} else {
			s32 dx = e->wtx - e->x, dy = e->wty - e->y;
			u32 dist = isqrt((u32)(((s64)dx * dx + (s64)dy * dy) >> 8)) << 4;
			if (dist < 384) { e->wwait = randi(72, 210); break; }
			s32 step = 34 * 256 / FPS;
			s32 sx = (s32)((s64)dx * step / (s32)dist), sy = (s32)((s64)dy * step / (s32)dist);
			if (body_hits(e, e->x + sx, e->y + sy)) { e->wwait = randi(30, 90); break; }
			e->x += sx; e->y += sy;
			face_dir(e, dx, dy);
			e->walk_anim = 1;
			e->anim_t += 256 * 6 / FPS;
		}
		break;
	default:
		e->walk_anim = 0;
		break;
	}
}

// ------------------------------------------------------------------ dibujo de entidades
static int ent_frame(Ent* e) {
	if (e->kind == ENT_SPRITE) return e->ms_frame;
	int col;
	if (e->walk_anim) col = 2 + (((e->anim_t * 3 / 2) >> FX) % 6);
	else col = breath(e);
	return e->facing * 8 + col;
}

static const u16* ent_palette(Ent* e) {
	if (e->kind == ENT_SPRITE) return e->ms->pal;
	if (e->member >= 0) return member_sheet(e->member)->pal;
	return e->sheet ? e->sheet->pal : 0;
}

static void ent_ensure_video(Ent* e) {
	const u16* pal = ent_palette(e);
	if (!pal) return;
	if (e->pal < 0 || e->pal_key_dirty || e->up_src != (const void*)pal) {
		if (e->pal >= 0) objpal_put(e->pal);
		e->pal = objpal_get(pal, e->tr, e->tg, e->tb, 1);
		e->pal_key_dirty = 0;
		if (e->up_src != (const void*)pal) e->up_frame = -1;
		e->up_src = pal;
	}
	int need = e->kind == ENT_SPRITE ? e->ms->frame_tiles : 16;
	if (e->tile < 0 || e->ntiles != need) {
		if (e->tile >= 0) objt_free(e->tile);
		e->tile = objt_alloc(need);
		e->ntiles = need;
		e->up_frame = -1;
	}
}

static void ent_upload(Ent* e, int frame) {
	if (e->tile < 0 || e->up_frame == frame) return;
	if (e->kind == ENT_SPRITE) {
		dma_copy32(objt_ptr(e->tile), e->ms->tiles + frame * e->ms->frame_tiles * 8, e->ms->frame_tiles * 8);
	} else if (e->member >= 0) {
		composed_upload_frame(member_sheet(e->member), frame, e->tile);
	} else {
		dma_copy32(objt_ptr(e->tile), e->sheet->tiles + frame * 16 * 8, 16 * 8);
	}
	e->up_frame = frame;
}

static int semi_alpha = 16;

static void draw_one(Ent* e, int camx, int camy) {
	ent_ensure_video(e);
	if (e->tile < 0 || e->pal < 0) return;
	int frame = ent_frame(e);
	ent_upload(e, frame);
	int x = PX(e->x) + e->off_x - camx, y = PX(e->y) + e->hop_y - camy;
	u32 key = 0x20000000 + (u32)(0x8000 - (PX(e->y) + 0x4000)) * 32 + (e - g_ent);
	int semi = e->alpha < 16;
	if (semi && e->alpha < semi_alpha) semi_alpha = e->alpha;
	if (e->alpha == 0) return;
	if (e->kind == ENT_SPRITE) {
		const MetaSprite* ms = e->ms;
		for (int i = 0; i < ms->npieces; ++i) {
			const ObjPiece* p = &ms->pieces[i];
			static const u8 pw[3][4] = {{8, 16, 32, 64}, {16, 32, 32, 64}, {8, 8, 16, 32}};
			int w = pw[p->shape][p->size];
			int ox = e->flip ? -(p->x + w) : p->x;
			int sx = x + ox, sy = y + p->y;
			if (sx < -64 || sx > 240 || sy < -64 || sy > 160) continue;
			oam_push(key, obj_a0(sy, p->shape, semi), obj_a1(sx, p->size, e->flip), obj_a2(e->tile + p->tile, 2, e->pal));
		}
		return;
	}
	// personaje: frame 20x30 con pies en (x, y); bloque 32x32
	int bx = e->member >= 0 ? x - 12 : x - 16;
	int by = y - 31;
	if (e->rot) {
		// rotación alrededor de los pies (Godot gira el sprite sobre el origen del nodo)
		int c = fcos(e->rot), s = fsin(e->rot);
		// centro del bloque respecto a los pies
		int cx = bx + 16 - x, cy = by + 16 - y;
		int rx = (cx * c - cy * s) >> 12, ry = (cx * s + cy * c) >> 12;
		int m = oam_affine(c >> 4, s >> 4, -s >> 4, c >> 4);
		if (m >= 0) {
			int ox = x + rx - 32, oy = y + ry - 32;
			if (ox > -64 && ox < 240 && oy > -64 && oy < 160)
				oam_push(key, (oy & 0xFF) | 0x0300 | (semi ? 0x0400 : 0), (ox & 0x1FF) | (m << 9) | (2 << 14), obj_a2(e->tile, 2, e->pal));
			return;
		}
	}
	if (bx < -32 || bx > 240 || by < -32 || by > 160) return;
	oam_push(key, obj_a0(by, OBJ_SQUARE, semi), obj_a1(bx, 2, 0), obj_a2(e->tile, 2, e->pal));
}

void ents_draw(int camx, int camy) {
	semi_alpha = 16;
	for (int i = 0; i < MAX_ENT; ++i) {
		Ent* e = &g_ent[i];
		if (!e->used || !e->visible) continue;
		if (e->kind == ENT_SPRITE && e->ms_auto && e->ms->nframes > 1) e->ms_frame = (g_frame / 13) % e->ms->nframes;
		draw_one(e, camx, camy);
	}
	alpha_set(semi_alpha, 16 - semi_alpha);
}

// celdas que pisa cada entidad (para la capa de oclusión)
void ents_overlay(void (*want)(int, int, int, int, int)) {
	for (int i = 0; i < MAX_ENT; ++i) {
		Ent* e = &g_ent[i];
		if (!e->used || !e->visible || e->alpha == 0) continue;
		int x = PX(e->x), y = PX(e->y);
		if (e->kind == ENT_SPRITE) {
			want(x - e->ms->w / 2, y - e->ms->h, x + e->ms->w / 2, y, y);
		} else {
			int top = e->rot ? y - 16 : y - 29 + e->hop_y;
			want(x - 12, top, x + 12, y + 1, y);
		}
	}
}

// ------------------------------------------------------------------ interacción
typedef struct { int is_npc; Ent* e; const char* id; int rx, ry, rw, rh; } Target;

static int probe_dist2(int px, int py, int x0, int y0, int x1, int y1) {
	int cx = CLAMP(px, x0, x1), cy = CLAMP(py, y0, y1);
	return (px - cx) * (px - cx) + (py - cy) * (py - cy);
}

static int interactable(const char* id) {
	if (W.story && W.story->is_interactable) return W.story->is_interactable(id);
	return !ends_with(id, "_zone");
}

static int find_target(Target* t) {
	Ent* p = PLAYER;
	int px = PX(p->x) + DIRV[p->facing][0] * 11, py = PX(p->y) + DIRV[p->facing][1] * 11 - 4;
	int best = 14 * 14;
	int found = 0;
	for (int i = 1; i < MAX_ENT; ++i) {
		Ent* e = &g_ent[i];
		if (!e->used || e->kind != ENT_NPC) continue;
		int dx = px - PX(e->x), dy = py - (PX(e->y) - 4);
		int d = dx * dx + dy * dy;
		if (d < best) { best = d; t->is_npc = 1; t->e = e; found = 1; }
	}
	for (int i = 0; i < W.m->nprops; ++i) {
		const MapProp* pr = &W.m->props[i];
		if (!pr->id || W.prop_removed[i]) continue;
		if (pr->patch >= 0 && W.m->patches[pr->patch].kind == 2 && !W.pstate[pr->patch]) continue;
		int x0, y0, w, h;
		if (pr->has_col) { x0 = pr->x + pr->col.x; y0 = pr->y + pr->col.y; w = pr->col.w; h = pr->col.h; }
		else { x0 = pr->x - 8; y0 = pr->y - 8; w = 16; h = 8; }
		x0 -= 4; y0 -= 4; w += 8; h += 8;
		int d = probe_dist2(px, py, x0, y0, x0 + w, y0 + h);
		if (d < best && interactable(pr->id)) {
			best = d; t->is_npc = 0; t->id = pr->id; t->rx = x0; t->ry = y0; t->rw = w; t->rh = h; found = 1;
		}
	}
	for (int i = 0; i < W.m->ntriggers; ++i) {
		const MapTrigger* tr = &W.m->triggers[i];
		int d = probe_dist2(px, py, tr->r.x, tr->r.y, tr->r.x + tr->r.w, tr->r.y + tr->r.h);
		if (d < best && interactable(tr->id)) {
			best = d; t->is_npc = 0; t->id = tr->id; t->rx = tr->r.x; t->ry = tr->r.y; t->rw = tr->r.w; t->rh = tr->r.h; found = 1;
		}
	}
	return found;
}

static int hint_on, hint_x, hint_y;
void hint_draw(int camx, int camy) {
	if (!hint_on) return;
	int bob = fsin(g_frame * 1024 * 5 / 60 / 6 & 1023) * 3 / 2 / 4096;
	ui_cursor(hint_x - camx - 5, hint_y - camy - 8 + bob, CUR_BUBBLE, 0);
}

// ------------------------------------------------------------------ tareas de historia
typedef struct { void (*fn)(const char*); char arg[32]; } StoryCall;
static StoryCall calls[6];
static void story_task(void* a) {
	StoryCall* c = a;
	c->fn(c->arg);
	c->fn = 0;
}
void story_call(void (*fn)(const char*), const char* arg) {
	for (int i = 0; i < 6; ++i)
		if (!calls[i].fn) {
			calls[i].fn = fn;
			strncpy(calls[i].arg, arg ? arg : "", 31);
			calls[i].arg[31] = 0;
			if (task_spawn(story_task, &calls[i], TASK_PAUSABLE | TASK_BIG) < 0) task_spawn(story_task, &calls[i], TASK_PAUSABLE);
			return;
		}
}
void story_calls_reset(void) { memset(calls, 0, sizeof calls); }

extern void story_on_interact(const char* id);
extern void story_on_npc(const char* id);
extern void story_on_trigger(const char* id);
extern const char* story_exit_requirement(const char* to, const char* def);
extern const char* story_exit_blocked(const char* to, const char* def);
extern int story_before_exit(const char* to);
extern int story_encounters_disabled(void);

static int story_busy(void) {
	for (int i = 0; i < 6; ++i) if (calls[i].fn) return 1;
	return 0;
}

// ------------------------------------------------------------------ salidas y zonas
static u8 exit_inside[16], trig_inside[32], trig_wait[32];
static u32 last_exit_block, exit_poll;

static int body_in_rect(const Rect16* r) {
	s32 x0 = PLAYER->x - (5 << FX), y0 = PLAYER->y - (6 << FX), x1 = PLAYER->x + (5 << FX), y1 = PLAYER->y;
	return (r->x << FX) < x1 && ((r->x + r->w) << FX) > x0 && (r->y << FX) < y1 && ((r->y + r->h) << FX) > y0;
}

const char* resolve_target(const char* to) {
	if (!strcmp(to, "village_any")) {
		if (flag("act3_started") || flag("night_over")) return "village_dawn";
		if (flag("night_attack")) return "village_night";
		return "village";
	}
	if (!strcmp(to, "village_night_or_dawn")) return flag("night_over") ? "village_dawn" : "village_night";
	if (!strcmp(to, "village")) {
		if (flag("night_over")) return "village_dawn";
		if (flag("night_attack")) return "village_night";
	}
	return to;
}

static char tr_map[24], tr_spawn[24];
static u8 tr_door;
static void transition_task(void* a) {
	(void)a;
	if (tr_door) sfx("door", -6.0f, 1.0f);
	fade_out(0.3f);
	strcpy(G.current_map, tr_map);
	strcpy(G.current_spawn, tr_spawn);
	G.has_load_pos = 0;
	scene_request(SC_WORLD);
}
void go_to_map(const char* map, const char* spawn, int door) {
	if (g_trans_busy) { block_forever(); return; }
	g_trans_busy = 1;
	strncpy(tr_map, map, 23); tr_map[23] = 0;
	strncpy(tr_spawn, spawn, 23); tr_spawn[23] = 0;
	tr_door = door;
	if (task_current() >= 0 && task_spawn(transition_task, 0, 0) < 0) transition_task(0);
	block_forever();
}
static void go_to_map_async(const char* map, const char* spawn, int door) {
	if (g_trans_busy) return;
	g_trans_busy = 1;
	strncpy(tr_map, map, 23); tr_map[23] = 0;
	strncpy(tr_spawn, spawn, 23); tr_spawn[23] = 0;
	tr_door = door;
	task_spawn(transition_task, 0, 0);
}

static const char* blocked_msg;
static void blocked_task(const char* a) {
	(void)a;
	W.cutscene = 1;
	Line l = {0, blocked_msg};
	say_lines(&l, 1);
	W.cutscene = 0;
}

static void on_exit(int i) {
	const MapExit* e = &W.m->exits[i];
	if (W.cutscene || g_trans_busy) return;
	const char* to = resolve_target(e->to);
	const char* req = story_exit_requirement(e->to, e->requires ? e->requires : "");
	if (req[0] && !flag(req)) {
		if (g_frame - last_exit_block < 30) return;
		last_exit_block = g_frame;
		PLAYER->x -= DIRV[PLAYER->facing][0] * 6 * 256;
		PLAYER->y -= DIRV[PLAYER->facing][1] * 6 * 256;
		blocked_msg = story_exit_blocked(e->to, e->blocked ? e->blocked : "");
		story_call(blocked_task, "");
		return;
	}
	if (!story_before_exit(to)) return;
	int door = W.m->interior || !strcmp(to, "housemain") || !strcmp(to, "bedroom") || !strcmp(to, "cathedral") || !strcmp(to, "crypt");
	go_to_map_async(to, e->spawn, door);
}

static void check_exits(void) {
	for (int i = 0; i < W.m->nexits && i < 16; ++i) {
		int in = body_in_rect(&W.m->exits[i].r);
		if (in && !exit_inside[i]) on_exit(i);
		exit_inside[i] = in;
	}
	// sondeo: si ya estaba dentro al acabar una escena
	if (!W.cutscene && !g_trans_busy && !g_dialog_active && g_frame - exit_poll >= 24) {
		exit_poll = g_frame;
		for (int i = 0; i < W.m->nexits && i < 16; ++i) {
			if (!exit_inside[i]) continue;
			const MapExit* e = &W.m->exits[i];
			const char* req = story_exit_requirement(e->to, e->requires ? e->requires : "");
			if (req[0] && !flag(req)) continue;
			on_exit(i);
			return;
		}
	}
}

static void check_triggers(void) {
	for (int i = 0; i < W.m->ntriggers && i < 32; ++i) {
		int in = body_in_rect(&W.m->triggers[i].r);
		if (in && !trig_inside[i]) trig_wait[i] = 3;
		if (!in) trig_wait[i] = 0;
		trig_inside[i] = in;
		if (trig_wait[i] && !g_trans_busy) {
			if (--trig_wait[i] == 0 && in && !W.cutscene) story_call(story_on_trigger, W.m->triggers[i].id);
		}
	}
}

// ------------------------------------------------------------------ encuentros
static s32 steps, next_enc;
void encounter_reset(void) {
	steps = 0;
	if (W.m->enc_rate) next_enc = (s32)(randf_range(W.m->enc_rate * 0.6f, W.m->enc_rate * 1.4f) * 16.0f * 256);
	else next_enc = 0x7FFFFFFF;
}
extern void battle_flash_and_go(void);
static void on_player_moved(s32 dist) {
	if (!W.m->enc_rate || W.cutscene || g_trans_busy || story_encounters_disabled()) return;
	steps += dist;
	if (steps >= next_enc) {
		encounter_reset();
		const Formation* f = &W.m->enc[rnd() % W.m->nenc];
		memset(&g_enc, 0, sizeof g_enc);
		g_enc.n = f->n;
		for (int i = 0; i < f->n && i < 6; ++i) strncpy(g_enc.enemies[i], f->enemies[i], 19);
		strncpy(g_enc.bg, W.m->battle_bg, 23);
		g_enc.can_flee = 1;
		strcpy(g_enc.ret_map, W.m->id);
		g_enc.ret_has_pos = 1;
		g_enc.ret_x = PX(PLAYER->x);
		g_enc.ret_y = PX(PLAYER->y);
		W.cutscene = 1;
		g_trans_busy = 1;
		task_spawn((TaskFn)battle_flash_and_go, 0, 0);
	}
}

// ------------------------------------------------------------------ bucle del jugador
extern void pause_open(void);
extern void particles_tick(void);

void world_tick(void) {
	Ent* p = PLAYER;
	G.play_frames++;
	fades_step();
	int can_move = !player_locked && !g_dialog_active && !g_trans_busy && !W.cutscene;
	int dx = 0, dy = 0;
	if (can_move) {
		if (key_held(KEY_LEFT)) dx = -1; else if (key_held(KEY_RIGHT)) dx = 1;
		if (key_held(KEY_UP)) dy = -1; else if (key_held(KEY_DOWN)) dy = 1;
	}
	if (p->moving) path_step(p);
	else if (dx || dy) {
		if (dx && !dy) p->facing = dx > 0 ? 2 : 1;
		else p->facing = dy > 0 ? 0 : 3;
		int running = key_held(KEY_B);
		s32 sp = (running ? RUN_SPEED : WALK_SPEED) * 256 / FPS;
		s32 mx = dx * sp, my = dy * sp;
		if (dx && dy) { mx = mx * 181 / 256; my = my * 181 / 256; }
		s32 bx = p->x, by = p->y;
		move_slide(p, mx, my);
		s32 mdx = p->x - bx, mdy = p->y - by;
		s32 moved = (s32)(isqrt((u32)(((s64)mdx * mdx + (s64)mdy * mdy) >> 8)) << 4);
		if (moved > 12) {
			on_player_moved(moved);
			anim_t += 256 * (running ? 11 : 75 / 10) / FPS;
			record_trail();
		}
		p->walk_anim = 1;
		p->anim_t = anim_t;
		int step_now = (anim_t * 3 / 2 / 3) >> FX;
		if (step_now != step_count && moved > 12) { step_count = step_now; sfx("step", -20.0f, randf_range(0.9f, 1.1f)); }
	} else if (!p->moving) {
		p->walk_anim = 0;
		anim_t = 0;
	}
	for (int i = 1; i < MAX_ENT; ++i) {
		Ent* e = &g_ent[i];
		if (!e->used) continue;
		if (e->kind == ENT_NPC) npc_update(e);
		else if (e->kind == ENT_SPRITE && e->moving) path_step(e);
	}
	extern void world_camera_update(void);
	world_camera_update();
	// interacción
	int can_hint = !W.cutscene && !g_dialog_active && !g_trans_busy;
	Target t;
	hint_on = 0;
	if (can_hint && find_target(&t)) {
		hint_on = 1;
		if (t.is_npc) { hint_x = PX(t.e->x); hint_y = PX(t.e->y) - 36; }
		else { hint_x = t.rx + t.rw / 2; hint_y = t.ry - 6; }
	}
	int fresh = g_frame - g_dialog_close_frame > 8;
	if (can_move && fresh && key_hit(KEY_A) && !story_busy()) {
		if (hint_on) {
			if (t.is_npc) story_call(story_on_npc, t.e->id);
			else story_call(story_on_interact, t.id);
		}
	}
	if (!g_trans_busy) { check_exits(); check_triggers(); }
	particles_tick();
	if (key_hit(KEY_START) && !g_dialog_active && !g_trans_busy) pause_open();
}
