// Mapa: carga, capas de suelo y oclusión, cámara, dibujo y bucle de la escena del mundo.
#include "world.h"
#include "dialogue.h"
#include "scene.h"
#include "audio.h"
#include "ui.h"

World W;
Ent g_ent[MAX_ENT] EWRAM_BSS;
u8 g_scratch[65536] __attribute__((aligned(4))) EWRAM_BSS;

#define G_CB 1           // suelo: charblocks 1-2 (1024 huecos, uno por celda del screenblock)
#define G_SB 28
#define O_CB 3           // oclusión: charblock 3, tiles 0..255
#define O_SB 29
#define VIEW_W 31
#define VIEW_H 21
#define SCR_W 240
#define SCR_H 160

const MapData* map_by_id(const char* id) {
	for (int i = 0; i < g_nmaps; ++i) if (!strcmp(g_maps[i]->id, id)) return g_maps[i];
	return 0;
}

// ------------------------------------------------------------------ parches (objetos que cambian)
static const MapPatch* patch_for_cell(int cx, int cy, int* local) {
	const MapData* m = W.m;
	for (int i = 0; i < m->npatches; ++i) {
		const MapPatch* p = &m->patches[i];
		if (!W.pstate[i]) continue;
		if (cx < p->cx || cy < p->cy || cx >= p->cx + p->cw || cy >= p->cy + p->ch) continue;
		*local = (cy - p->cy) * p->cw + (cx - p->cx);
		return p;
	}
	return 0;
}

static void ground_cell(int cx, int cy, int* tile, int* pal) {
	int local;
	const MapPatch* p = patch_for_cell(cx, cy, &local);
	if (p) {
		const PatchState* s = &p->states[W.pstate[p - W.m->patches]];
		*tile = s->gtile[local];
		*pal = s->gpal[local];
		return;
	}
	int i = cy * W.m->wc + cx;
	*tile = W.m->gcells[i];
	*pal = W.m->gcpal[i];
}

static int overlay_list(int cx, int cy) {
	int local;
	const MapPatch* p = patch_for_cell(cx, cy, &local);
	if (p) return p->states[W.pstate[p - W.m->patches]].ov[local];
	return W.m->ocell[cy * W.m->wc + cx];
}

// ------------------------------------------------------------------ suelo con carga por columnas/filas
static int view_cx = -9999, view_cy = -9999;

static void load_cell(int cx, int cy) {
	int slot = ((cy & 31) << 5) | (cx & 31);
	volatile u32* dst = CHARBLOCK(G_CB) + slot * 8;
	if (cx < 0 || cy < 0 || cx >= W.m->wc || cy >= W.m->hc) {
		for (int k = 0; k < 8; ++k) dst[k] = 0;
		SCREENBLOCK(G_SB)[slot] = slot;
		return;
	}
	int t, p;
	ground_cell(cx, cy, &t, &p);
	const u32* src = W.m->gtiles + t * 8;
	for (int k = 0; k < 8; ++k) dst[k] = src[k];
	SCREENBLOCK(G_SB)[slot] = slot | (p << 12);
}

static void view_reload(int cx0, int cy0) {
	for (int y = 0; y < VIEW_H; ++y)
		for (int x = 0; x < VIEW_W; ++x) load_cell(cx0 + x, cy0 + y);
	view_cx = cx0;
	view_cy = cy0;
}

static void view_update(int camx, int camy) {
	int cx0 = camx >> 3, cy0 = camy >> 3;
	if (cx0 == view_cx && cy0 == view_cy) return;
	if (ABS(cx0 - view_cx) > 4 || ABS(cy0 - view_cy) > 4) { view_reload(cx0, cy0); return; }
	while (view_cx < cx0) { view_cx++; for (int y = 0; y < VIEW_H; ++y) load_cell(view_cx + VIEW_W - 1, view_cy + y); }
	while (view_cx > cx0) { view_cx--; for (int y = 0; y < VIEW_H; ++y) load_cell(view_cx, view_cy + y); }
	while (view_cy < cy0) { view_cy++; for (int x = 0; x < VIEW_W; ++x) load_cell(view_cx + x, view_cy + VIEW_H - 1); }
	while (view_cy > cy0) { view_cy--; for (int x = 0; x < VIEW_W; ++x) load_cell(view_cx + x, view_cy); }
}

static void reload_cells(int cx, int cy, int cw, int ch) {
	for (int y = cy; y < cy + ch; ++y)
		for (int x = cx; x < cx + cw; ++x)
			if (x >= view_cx && x < view_cx + VIEW_W && y >= view_cy && y < view_cy + VIEW_H) load_cell(x, y);
}

// ------------------------------------------------------------------ capa de oclusión
// Para cada celda que pisa un personaje se elige la variante "objetos con y mayor que la suya".
static u16 ov_cur[1024] EWRAM_BSS;          // por hueco del screenblock: tile de oclusión mostrado (0 ninguno)
static u8 ov_curpal[1024] EWRAM_BSS;
static s16 ov_want_list[1024] EWRAM_BSS;    // índice de variante deseada (-1 ninguna) en este fotograma
static u16 ov_want_tile[1024] EWRAM_BSS;
static u8 ov_want_pal[1024] EWRAM_BSS;
static u16 ov_active[400] EWRAM_BSS;
static int ov_nactive;
static u16 vslot_tile[256] EWRAM_BSS;       // hueco de VRAM -> tile de oclusión
static u8 vslot_ref[256] EWRAM_BSS;
static u16 touched[400] EWRAM_BSS;
static int ntouched;

static void ov_reset(void) {
	memset(ov_cur, 0, sizeof ov_cur);
	memset(vslot_tile, 0, sizeof vslot_tile);
	memset(vslot_ref, 0, sizeof vslot_ref);
	ov_nactive = 0;
	for (int i = 0; i < 1024; ++i) { SCREENBLOCK(O_SB)[i] = 0; ov_want_list[i] = -1; }
	for (int k = 0; k < 8; ++k) CHARBLOCK(O_CB)[k] = 0;
	vslot_ref[0] = 255;
}

static int vslot_get(int otile) {
	int freei = -1;
	for (int i = 1; i < 256; ++i) {
		if (vslot_ref[i] && vslot_tile[i] == otile) { vslot_ref[i]++; return i; }
		if (!vslot_ref[i] && freei < 0) freei = i;
	}
	if (freei < 0) return 0;
	vslot_tile[freei] = otile;
	vslot_ref[freei] = 1;
	const u32* src = W.m->otiles + otile * 8;
	volatile u32* dst = CHARBLOCK(O_CB) + freei * 8;
	for (int k = 0; k < 8; ++k) dst[k] = src[k];
	return freei;
}
static void vslot_put(int s) { if (s > 0 && vslot_ref[s]) vslot_ref[s]--; }

static u16 ov_vslot[1024] EWRAM_BSS;

static void ov_want_cells(int x0, int y0, int x1, int y1, int depth) {
	int cx0 = x0 >> 3, cy0 = y0 >> 3, cx1 = x1 >> 3, cy1 = y1 >> 3;
	if (x0 < 0) cx0 = (x0 - 7) / 8;
	if (y0 < 0) cy0 = (y0 - 7) / 8;
	for (int cy = cy0; cy <= cy1; ++cy) {
		if (cy < 0 || cy >= W.m->hc) continue;
		for (int cx = cx0; cx <= cx1; ++cx) {
			if (cx < 0 || cx >= W.m->wc) continue;
			int off = overlay_list(cx, cy);
			if (!off) continue;
			const u16* L = W.m->olist + off;
			int n = L[0];
			int k;
			for (k = 0; k < n; ++k) if ((s16)L[1 + k * 3] > depth) break;
			if (k >= n) continue;
			int slot = ((cy & 31) << 5) | (cx & 31);
			if (ov_want_list[slot] < 0) {
				if (ntouched < 400) touched[ntouched++] = slot;
				ov_want_list[slot] = k;
			} else if (k > ov_want_list[slot]) ov_want_list[slot] = k;
			ov_want_tile[slot] = L[1 + ov_want_list[slot] * 3 + 1];
			ov_want_pal[slot] = L[1 + ov_want_list[slot] * 3 + 2];
		}
	}
}

static void ov_commit(void) {
	// celdas activas que ya no se quieren
	int j = 0;
	for (int i = 0; i < ov_nactive; ++i) {
		int slot = ov_active[i];
		if (ov_want_list[slot] < 0) {
			vslot_put(ov_vslot[slot]);
			ov_cur[slot] = 0;
			ov_vslot[slot] = 0;
			SCREENBLOCK(O_SB)[slot] = 0;
		} else ov_active[j++] = slot;
	}
	ov_nactive = j;
	for (int i = 0; i < ntouched; ++i) {
		int slot = touched[i];
		int t = ov_want_tile[slot], p = ov_want_pal[slot];
		if (ov_cur[slot] != t || ov_curpal[slot] != p) {
			int was_active = ov_cur[slot] != 0;
			vslot_put(ov_vslot[slot]);
			int vs = vslot_get(t);
			ov_vslot[slot] = vs;
			ov_cur[slot] = t;
			ov_curpal[slot] = p;
			SCREENBLOCK(O_SB)[slot] = vs | (p << 12);
			if (!was_active && ov_nactive < 400) ov_active[ov_nactive++] = slot;
		}
	}
	for (int i = 0; i < ntouched; ++i) ov_want_list[touched[i]] = -1;
	ntouched = 0;
}

static void ov_refresh_all(void) {
	for (int i = 0; i < ov_nactive; ++i) {
		int slot = ov_active[i];
		vslot_put(ov_vslot[slot]);
		ov_cur[slot] = 0; ov_vslot[slot] = 0;
		SCREENBLOCK(O_SB)[slot] = 0;
	}
	ov_nactive = 0;
}

// ------------------------------------------------------------------ props y parches
static int prop_index(const char* pid) {
	for (int i = 0; i < W.m->nprops; ++i)
		if (W.m->props[i].id && !strcmp(W.m->props[i].id, pid)) return i;
	return -1;
}
int has_prop(const char* pid) {
	int i = prop_index(pid);
	if (i < 0 || W.prop_removed[i]) return 0;
	const MapProp* p = &W.m->props[i];
	if (p->patch >= 0 && W.m->patches[p->patch].kind == 2 && !W.pstate[p->patch]) return 0;
	return 1;
}

static void patch_set(int pidx, const char* state) {
	const MapPatch* p = &W.m->patches[pidx];
	for (int s = 0; s < p->nstates; ++s) {
		if (strcmp(p->states[s].name, state)) continue;
		if (W.pstate[pidx] == s) return;
		W.pstate[pidx] = s;
		reload_cells(p->cx, p->cy, p->cw, p->ch);
		ov_refresh_all();
		return;
	}
}

extern void coll_rebuild(void);
void set_prop_sprite(const char* pid, const char* sprite) {
	int i = prop_index(pid);
	if (i < 0 || W.m->props[i].patch < 0) return;
	patch_set(W.m->props[i].patch, sprite);
}
void remove_prop(const char* pid) {
	int i = prop_index(pid);
	if (i < 0) return;
	W.prop_removed[i] = 1;
	if (W.m->props[i].patch >= 0) patch_set(W.m->props[i].patch, "removed");
	coll_rebuild();
	nav_dirty();
}
void prop_dark(const char* pid) {
	int i = prop_index(pid);
	if (i < 0 || W.m->props[i].patch < 0) return;
	patch_set(W.m->props[i].patch, "dark");
}
void show_prop(const char* pid) {
	int i = prop_index(pid);
	if (i < 0 || W.m->props[i].patch < 0) return;
	patch_set(W.m->props[i].patch, "default");
	coll_rebuild();
	nav_dirty();
}

static void anim_props(void) {
	// objetos animados (hogueras, antorchas): 6 fps con fase por objeto
	for (int i = 0; i < W.m->npatches; ++i) {
		const MapPatch* p = &W.m->patches[i];
		if (p->kind != 1) continue;
		int f = ((g_frame / 10) + i * 7) % p->nstates;
		if (W.pstate[i] != f) {
			W.pstate[i] = f;
			reload_cells(p->cx, p->cy, p->cw, p->ch);
		}
	}
}

// ------------------------------------------------------------------ marcadores y luz del mapa
int marker(const char* name, int* x, int* y) {
	for (int i = 0; i < W.m->nmarkers; ++i)
		if (!strcmp(W.m->markers[i].name, name)) { *x = W.m->markers[i].x; *y = W.m->markers[i].y; return 1; }
	if (W.m->nmarkers) { *x = W.m->markers[0].x; *y = W.m->markers[0].y; }
	return 0;
}
int marker_x(const char* n) { int x, y; marker(n, &x, &y); return x; }
int marker_y(const char* n) { int x, y; marker(n, &x, &y); return y; }

static int is_tod_map(void) {
	const char* id = W.m->id;
	return !strcmp(id, "village") || !strcmp(id, "bedroom") || !strcmp(id, "housemain") || !strcmp(id, "forest");
}

void map_modulate(int* r, int* g, int* b) {
	int br = 256, bg = 256, bb = 256;
	if (W.m->has_modulate) { br = W.m->mod_r * 256 / 255; bg = W.m->mod_g * 256 / 255; bb = W.m->mod_b * 256 / 255; }
	if (is_tod_map()) {
		const char* tod = meta_s("tod", "day");
		if (!strcmp(tod, "dusk")) { br = br * 256 / 256; bg = bg * 205 / 256; bb = bb * 169 / 256; }
		else if (!strcmp(tod, "night")) { br = br * 118 / 256; bg = bg * 123 / 256; bb = bb * 184 / 256; }
		else if (!strcmp(tod, "dawn")) { br = br * 220 / 256; bg = bg * 205 / 256; bb = bb * 225 / 256; }
	}
	*r = br; *g = bg; *b = bb;
}

void set_tint(int r, int g, int b, float dur) { world_tint_tween(r, g, b, (int)(dur * FPS)); }
void set_tint_wait(int r, int g, int b, float dur) {
	set_tint(r, g, b, dur);
	while (world_tint_busy()) frame_wait();
}

// ------------------------------------------------------------------ cámara
static int cam_target_x(void) {
	int p = W.cam_focus ? W.focus_x : PX(PLAYER->x);
	int half = SCR_W / 2;
	if (W.m->w <= SCR_W) return W.m->w / 2;
	return CLAMP(p, half, W.m->w - half);
}
static int cam_target_y(void) {
	int p = (W.cam_focus ? W.focus_y : PX(PLAYER->y)) - 8;
	int half = SCR_H / 2;
	if (W.m->h <= SCR_H) return W.m->h / 2;
	return CLAMP(p, half, W.m->h - half);
}
static void camera_snap(void) { W.cam_x = cam_target_x() << FX; W.cam_y = cam_target_y() << FX; }
static void camera_update(void) {
	// lerp(pos, target, delta * speed) con speed 8 (2.2 al enfocar)
	int k = W.cam_focus ? 9 : 34;      // x256: 2.2/60 y 8/60
	W.cam_x += (((cam_target_x() << FX) - W.cam_x) * k) >> 8;
	W.cam_y += (((cam_target_y() << FX) - W.cam_y) * k) >> 8;
}

void world_camera_update(void) { camera_update(); }

void pan_to(int x, int y, float wait) {
	W.cam_focus = 1;
	W.focus_x = x; W.focus_y = y;
	wait_sec(wait);
}
void release_camera(float wait) { W.cam_focus = 0; wait_sec(wait); }

static int shake_n, shake_amt;
void shake(float amount, float t) { shake_amt = (int)amount; shake_n = (int)(t / 0.04f) * 2 + 3; }

int world_cam_left(void) { return (W.cam_x >> FX) - SCR_W / 2 + W.shake_x; }
int world_cam_top(void) { return (W.cam_y >> FX) - SCR_H / 2 + W.shake_y; }

// ------------------------------------------------------------------ banner con el nombre del mapa
extern void banner_show(const char* text);
void show_banner(const char* text) { banner_show(text); meta_set_s("last_banner", text); }
static void maybe_banner(const char* text) {
	if (!text || !text[0] || !strcmp(text, meta_s("last_banner", ""))) return;
	show_banner(text);
}

// ------------------------------------------------------------------ dibujo
extern void ents_draw(int camx, int camy);
extern void ents_overlay(void (*want)(int, int, int, int, int));
extern void hint_draw(int camx, int camy);
extern void particles_draw(int camx, int camy);

void world_render(void) {
	if (shake_n > 0) {
		shake_n--;
		W.shake_x = shake_n ? randi(-shake_amt, shake_amt) : 0;
		W.shake_y = shake_n ? randi(-shake_amt, shake_amt) : 0;
	}
	int camx = world_cam_left(), camy = world_cam_top();
	view_update(camx - W.shake_x, camy - W.shake_y);
	g_vid.hofs[1] = g_vid.hofs[2] = camx;
	g_vid.vofs[1] = g_vid.vofs[2] = camy;
	anim_props();
	oam_begin();
	ents_draw(camx, camy);
	hint_draw(camx, camy);
	particles_draw(camx, camy);
	extern void ui_frame_sprites(void);
	ui_frame_sprites();
	oam_end();
	ents_overlay(ov_want_cells);
	ov_commit();
}

// ------------------------------------------------------------------ carga del mapa
extern void ents_reset_keep_player(void);
extern void player_setup(int x, int y, int face);
extern void coll_build(void);
extern const StoryDef* story_for(const char* map);
extern void particles_setup(void);

static int spawn_facing(const char* sp) {
	const char* id = W.m->id;
	if (!strcmp(sp, "home_door") || !strcmp(sp, "from_upstairs") || !strcmp(sp, "from_downstairs") || !strcmp(sp, "from_crypt")) return 0;
	if (!strcmp(sp, "from_village")) return (!strcmp(id, "housemain") || !strcmp(id, "cathedral")) ? 3 : 2;
	if (!strcmp(sp, "from_forest")) return starts_with(id, "village") ? 1 : 2;
	if (!strcmp(sp, "from_deep") || !strcmp(sp, "from_camp") || !strcmp(sp, "from_cathedral"))
		return !strcmp(id, "heart") ? 3 : (!strcmp(id, "elf_camp") ? 2 : 1);
	return 0;
}

static void map_load(void) {
	const MapData* m = map_by_id(G.current_map);
	if (!m) m = map_by_id("bedroom");
	memset(&W, 0, sizeof W);
	W.m = m;
	bgpal_load_n(0, m->pals, 12, 1);
	map_modulate(&W.tint_r, &W.tint_g, &W.tint_b);
	world_tint(W.tint_r, W.tint_g, W.tint_b);
	ov_reset();
	view_cx = view_cy = -9999;
	coll_build();
	nav_dirty();
	int x = 40, y = 40, face = 0;
	if (G.has_load_pos) {
		x = G.load_x; y = G.load_y;
		G.has_load_pos = 0;
	} else {
		marker(G.current_spawn, &x, &y);
		face = spawn_facing(G.current_spawn);
	}
	player_setup(x, y, face);
	camera_snap();
	for (int i = 0; i < G.nparty; ++i) if (G.party[i] != M_PLAYER) spawn_follower(G.party[i]);
	W.story = story_for(m->id);
	g_vid.bgcnt[1] = (G_CB << 2) | (G_SB << 8) | 3;
	g_vid.bgcnt[2] = (O_CB << 2) | (O_SB << 8) | 1;
	g_vid.dispcnt = DCNT_MODE0 | DCNT_OBJ1D | DCNT_BG0 | DCNT_BG1 | DCNT_BG2 | DCNT_OBJ;
	particles_setup();
}

// ------------------------------------------------------------------ bucle
extern void world_tick(void);
extern void encounter_reset(void);
extern void story_start_map_ready(void);
extern const char* story_music(void);

void world_scene(void* arg) {
	(void)arg;
	g_vid.dispcnt = DCNT_BLANK;
	ui_init();
	ui_sprites_reset();
	objt_reset();
	objpal_reset();
	map_load();
	encounter_reset();
	music_play(story_music(), 0.8f);
	world_render();
	maybe_banner(W.m->display);
	extern void place_waystone(void);
	place_waystone();
	story_start_map_ready();
#ifdef AUTOTEST
	{ extern void at_world_ready(void); at_world_ready(); }
#endif
	g_trans_busy = 1;
	if (fade_level() > 0) {
		// fundido de entrada sin bloquear la lógica del mapa
		while (fade_level() > 0) {
			fade_set(fade_level() - 2 < 0 ? 0 : fade_level() - 2, 0);
			world_tick();
			world_render();
			frame_wait();
		}
	}
	g_trans_busy = 0;
	while (!scene_requested()) {
		if (g_menu_open) { frame_wait(); continue; }
		world_tick();
		world_render();
		frame_wait();
	}
}
