#include "appearance.h"
#include "gfx.h"

// ------------------------------------------------------------------ memoria
#define MAP_TILES (32 * 12)
#define BAT_TILES (9 * 16)
static u32 map_buf[4][MAP_TILES * 8] EWRAM_BSS;
static u32 bat_buf[4][BAT_TILES * 8] EWRAM_BSS;
static u32 por_buf[4][36 * 8] EWRAM_BSS;
static Composed map_c[4], bat_c[4], por_c[4];
static u32 serial = 1;
extern u8 g_scratch[];            // zona compartida (world.c)

void appearance_dirty(void) {
	serial++;
}

// ------------------------------------------------------------------ capas
static const Layer* find_layer(const char* name) {
	int lo = 0, hi = g_nlayers - 1;
	while (lo <= hi) {
		int m = (lo + hi) >> 1;
		int c = strcmp(g_layers[m].name, name);
		if (!c) return &g_layers[m];
		if (c < 0) lo = m + 1; else hi = m - 1;
	}
	return 0;
}

typedef struct { float r, g, b; } Col;
static Col C8(int r, int g, int b) { Col c = {r / 255.0f, g / 255.0f, b / 255.0f}; return c; }
static Col lerp(Col a, Col b, float t) { Col c = {a.r + (b.r - a.r) * t, a.g + (b.g - a.g) * t, a.b + (b.b - a.b) * t}; return c; }
static Col shade(Col c, float t) {
	if (t > 0) return lerp(c, (Col){1, 0.98f, 0.92f}, t);
	return lerp(c, (Col){0.07f, 0.055f, 0.086f}, -t);
}
static u16 to555(Col c) {
	int r = (int)(c.r * 255 + 0.5f), g = (int)(c.g * 255 + 0.5f), b = (int)(c.b * 255 + 0.5f);
	r = CLAMP(r, 0, 255); g = CLAMP(g, 0, 255); b = CLAMP(b, 0, 255);
	return (r >> 3) | ((g >> 3) << 5) | ((b >> 3) << 10);
}

enum { RC_NONE, RC_BODY, RC_HAIR };
typedef struct { Col skin, hair, outfit; } Tint;

static u16 recolor(int r, int g, int b, int mode, const Tint* t) {
	if (mode != RC_NONE) {
		if (r == g && r == b) {
			if (mode == RC_BODY) {
				switch (r) {
				case 228: return to555(shade(t->skin, 0.12f));
				case 200: return to555(t->skin);
				case 184: return to555(lerp(t->skin, C8(230, 120, 120), 0.45f));
				case 160: return to555(shade(t->skin, -0.18f));
				case 120: return to555(lerp(shade(t->skin, -0.55f), C8(30, 24, 34), 0.3f));
				}
			} else {
				switch (r) {
				case 236: return to555(shade(t->hair, 0.3f));
				case 170: return to555(t->hair);
				case 110: return to555(shade(t->hair, -0.3f));
				case 70: return to555(shade(t->hair, -0.62f));
				}
			}
		} else if (mode == RC_BODY && r == 255 && g == 0 && b == 255) return to555(t->outfit);
		else if (mode == RC_BODY && r == 170 && g == 0 && b == 170) return to555(shade(t->outfit, -0.3f));
		else if (r == 110 && g == 0 && b == 110) return to555(shade(t->outfit, -0.62f));
		else if (mode == RC_BODY && r == 255 && g == 120 && b == 255) return to555(shade(t->outfit, 0.2f));
	}
	return (r >> 3) | ((g >> 3) << 5) | ((b >> 3) << 10);
}

// ------------------------------------------------------------------ composición
#define MAXCOL 250
static u16 ccol[MAXCOL + 1] EWRAM_BSS;
static int ncol;
static u8* cbuf;
static int cw, ch;

static int col_index(u16 c) {
	for (int i = 1; i <= ncol; ++i) if (ccol[i] == c) return i;
	if (ncol >= MAXCOL) return ncol;
	ccol[++ncol] = c;
	return ncol;
}

static void draw_layer(const char* name, int mode, const Tint* t) {
	const Layer* L = find_layer(name);
	if (!L || L->w != cw || L->h != ch) return;
	u8 map[256];
	map[0] = 0;
	for (int i = 0; i < L->ncol; ++i)
		map[i + 1] = col_index(recolor(L->rgb[i * 3], L->rgb[i * 3 + 1], L->rgb[i * 3 + 2], mode, t));
	const u8* p = L->rle;
	int n = cw * ch, pos = 0;
	while (pos < n) {
		int b = *p++;
		if (b & 0x80) {
			int cnt = (b & 0x7F) + 1;
			u8 v = *p++;
			if (v) { u8 m = map[v]; for (int k = 0; k < cnt; ++k) cbuf[pos + k] = m; }
			pos += cnt;
		} else {
			int cnt = b + 1;
			for (int k = 0; k < cnt; ++k) { u8 v = p[k]; if (v) cbuf[pos + k] = map[v]; }
			p += cnt;
			pos += cnt;
		}
	}
}

static void begin(int w, int h) {
	cw = w; ch = h;
	cbuf = g_scratch;
	memset(cbuf, 0, w * h);
	ncol = 0;
}

// reduce la paleta compuesta a 15 colores uniendo los más parecidos (ponderado por píxeles)
static void reduce(u8 remap[256]) {
	static u32 cnt[MAXCOL + 1] EWRAM_BSS;
	memset(cnt, 0, sizeof cnt);
	for (int i = 0; i < cw * ch; ++i) cnt[cbuf[i]]++;
	for (int i = 0; i <= ncol; ++i) remap[i] = i;
	int alive[MAXCOL + 1];
	for (int i = 0; i <= ncol; ++i) alive[i] = i > 0 && cnt[i] > 0;
	int live = 0;
	for (int i = 1; i <= ncol; ++i) live += alive[i];
	int r[MAXCOL + 1], g[MAXCOL + 1], b[MAXCOL + 1];
	for (int i = 1; i <= ncol; ++i) { r[i] = ccol[i] & 31; g[i] = (ccol[i] >> 5) & 31; b[i] = (ccol[i] >> 10) & 31; }
	// prefusión barata: con muchos colores, unir los que coinciden a menor precisión
	for (int sh = 1; live > 40 && sh < 5; ++sh) {
		for (int i = 1; i <= ncol; ++i) {
			if (!alive[i]) continue;
			for (int j = i + 1; j <= ncol; ++j) {
				if (!alive[j]) continue;
				if ((r[i] >> sh) != (r[j] >> sh) || (g[i] >> sh) != (g[j] >> sh) || (b[i] >> sh) != (b[j] >> sh)) continue;
				u32 ci = cnt[i], cj = cnt[j];
				r[i] = (r[i] * ci + r[j] * cj) / (ci + cj);
				g[i] = (g[i] * ci + g[j] * cj) / (ci + cj);
				b[i] = (b[i] * ci + b[j] * cj) / (ci + cj);
				cnt[i] += cj;
				alive[j] = 0;
				remap[j] = i;
				live--;
			}
		}
	}
	for (int k = 1; k <= ncol; ++k) { int t = k; while (remap[t] != t) t = remap[t]; remap[k] = t; }
	int lst[MAXCOL + 1], nl = 0;
	for (int i = 1; i <= ncol; ++i) if (alive[i]) lst[nl++] = i;
	while (live > 15) {
		int bi = -1, bj = -1, bk = -1;
		u32 best = 0xFFFFFFFF;
		for (int a = 0; a < nl; ++a) {
			int i = lst[a];
			for (int c = a + 1; c < nl; ++c) {
				int j = lst[c];
				int dr = r[i] - r[j], dg = g[i] - g[j], db = b[i] - b[j];
				u32 d = (u32)(dr * dr * 3 + dg * dg * 4 + db * db * 2) * (cnt[i] < cnt[j] ? cnt[i] : cnt[j]);
				if (d < best) { best = d; bi = i; bj = j; bk = c; }
			}
		}
		u32 ci = cnt[bi], cj = cnt[bj];
		r[bi] = (r[bi] * ci + r[bj] * cj) / (ci + cj);
		g[bi] = (g[bi] * ci + g[bj] * cj) / (ci + cj);
		b[bi] = (b[bi] * ci + b[bj] * cj) / (ci + cj);
		cnt[bi] += cj;
		alive[bj] = 0;
		remap[bj] = bi;
		lst[bk] = lst[--nl];
		live--;
	}
	for (int k = 1; k <= ncol; ++k) { int t = k; while (remap[t] != t) t = remap[t]; remap[k] = t; }
	// compactar a 1..15
	u8 slot[MAXCOL + 1];
	int n = 0;
	for (int i = 1; i <= ncol; ++i) if (alive[i]) { slot[i] = ++n; ccol[n] = r[i] | (g[i] << 5) | (b[i] << 10); }
	for (int k = 1; k <= ncol; ++k) remap[k] = slot[remap[k]];
	remap[0] = 0;
	ncol = n;
}

// escribe un bloque (x0,y0, tw x th tiles) de la composición como tiles 4bpp
static void emit_tiles(u32* out, int x0, int y0, int tw, int th, const u8* remap) {
	for (int ty = 0; ty < th; ++ty)
		for (int tx = 0; tx < tw; ++tx) {
			for (int r = 0; r < 8; ++r) {
				u32 row = 0;
				int y = y0 + ty * 8 + r;
				for (int c = 0; c < 8; ++c) {
					int x = x0 + tx * 8 + c;
					int v = 0;
					if (x >= 0 && y >= 0 && x < cw && y < ch) v = remap[cbuf[y * cw + x]];
					row |= (u32)v << (c * 4);
				}
				*out++ = row;
			}
		}
}

static void finish_pal(Composed* c) {
	c->pal[0] = 0;
	for (int i = 1; i < 16; ++i) c->pal[i] = i <= ncol ? ccol[i] : 0;
}

// frames de mapa: 160x120 -> 32 frames de 3x4 tiles; el frame de 20x30 empieza en (2,2) del bloque
static void emit_map(Composed* c) {
	u8 remap[256];
	reduce(remap);
	finish_pal(c);
	for (int f = 0; f < 32; ++f) {
		int fx = (f & 7) * 20, fy = (f >> 3) * 30;
		emit_tiles(c->tiles + f * 12 * 8, fx - 2, fy - 2, 3, 4, remap);
	}
	// las celdas fuera del frame no deben tomar píxeles del vecino: limpiar márgenes
	for (int f = 0; f < 32; ++f) {
		u32* t = c->tiles + f * 12 * 8;
		// columna izquierda: x 0..1 del bloque; derecha: x 22..23; fila superior: y 0..1
		for (int ty = 0; ty < 4; ++ty)
			for (int r = 0; r < 8; ++r) {
				t[(ty * 3 + 0) * 8 + r] &= ~0xFFu;
				t[(ty * 3 + 2) * 8 + r] &= 0x00FFFFFFu;
			}
		for (int tx = 0; tx < 3; ++tx) { t[tx * 8 + 0] = 0; t[tx * 8 + 1] = 0; }
	}
}

static void emit_battle(Composed* c) {
	u8 remap[256];
	reduce(remap);
	finish_pal(c);
	for (int f = 0; f < 9; ++f) emit_tiles(c->tiles + f * 16 * 8, f * 32, 0, 4, 4, remap);
}

static void emit_portrait(Composed* c) {
	u8 remap[256];
	reduce(remap);
	finish_pal(c);
	emit_tiles(c->tiles, 0, 0, 6, 6, remap);
}

static const char* RACES[3] = {"human", "elf", "dwarf"};
static const char* const WK[6] = {"", "sword", "axe", "daggers", "staff", "bow"};

static void tint_of(const Appearance* a, Tint* t) {
	const u8* s = g_skin_tones[a->skin].rgb;
	const u8* h = g_hair_colors[a->hair_color].rgb;
	const u8* o = g_outfits[a->outfit].rgb;
	t->skin = C8(s[0], s[1], s[2]);
	t->hair = C8(h[0], h[1], h[2]);
	t->outfit = C8(o[0], o[1], o[2]);
}

static void overlay_gear(const char* prefix, const char* race, const char* sex, const GearLook* g) {
	if (g->body[0]) draw_layer(sfmt("creator/%seqbody_%s_%s_%s", prefix, g->body, race, sex), RC_NONE, 0);
	if (g->head[0]) draw_layer(sfmt("creator/%seqhead_%s_%s_%s", prefix, g->head, race, sex), RC_NONE, 0);
	if (prefix[0] == 'b' && g->shield[0]) draw_layer(sfmt("creator/beqshield_%s_%s_%s", g->shield, race, sex), RC_NONE, 0);
}

static void weapon_layer(const char* race, const GearLook* g) {
	if (!g->weapon_kind) return;
	int tier = CLAMP(g->weapon_tier, 1, 3);
	const char* orb = g->orb ? g->orb : "";
	if (orb[0] && g->weapon_kind == WK_STAFF)
		draw_layer(sfmt("creator/weapon_%s_%s_%d_%s", race, WK[g->weapon_kind], tier, orb), RC_NONE, 0);
	else
		draw_layer(sfmt("creator/weapon_%s_%s_%d", race, WK[g->weapon_kind], tier), RC_NONE, 0);
}

static void compose_player(char prefix, const Appearance* a, const GearLook* g) {
	Tint t;
	tint_of(a, &t);
	const char* race = RACES[a->race];
	const char* sex = a->female ? "f" : "m";
	const char* style = g_hair_styles[a->hair_style].id;
	if ((!strcmp(g->head, "helm") || !strcmp(g->head, "helm_gold") || !strcmp(g->head, "cap") || !strcmp(g->head, "hood")) &&
		(!strcmp(style, "spiky") || !strcmp(style, "bun")))
		style = "short";
	const char* pf = prefix == 'b' ? "b" : prefix == 'p' ? "p" : "";
	if (prefix == 'b') weapon_layer(race, g);
	draw_layer(sfmt("creator/%sbody_%s_%s", pf, race, sex), RC_BODY, &t);
	draw_layer(sfmt("creator/%shair_%s_%s_%s", pf, race, sex, style), RC_HAIR, &t);
	if (a->beard && !a->female) draw_layer(sfmt("creator/%sbeard_%s_%s", pf, race, sex), RC_HAIR, &t);
	if (prefix != 'p') overlay_gear(pf, race, sex, g);
}

static const char* member_base(int m) {
	if (m == M_YARA && yara_is_witch()) return "yara_dark";
	if (m == M_KAELEN && flag("kaelen_dark")) return "kaelen_dark";
	return member_id(m);
}
static const char* member_race(int m) { return m == M_AELIS ? "elf" : m == M_BROM ? "dwarf" : "human"; }
static const char* member_sex(int m) { return (m == M_YARA || m == M_AELIS) ? "f" : "m"; }

static u32 look_key(const GearLook* g, const Appearance* a) {
	u32 h = 2166136261u;
	const char* parts[4] = {g->orb, g->head, g->body, g->shield};
	h = (h ^ g->weapon_kind) * 16777619u;
	h = (h ^ g->weapon_tier) * 16777619u;
	for (int i = 0; i < 4; ++i) for (const char* p = parts[i]; p && *p; ++p) h = (h ^ (u8)*p) * 16777619u;
	if (a) { const u8* p = (const u8*)a; for (int i = 0; i < (int)sizeof *a; ++i) h = (h ^ p[i]) * 16777619u; }
	return h;
}

static int slot_of(int m) {
	// el jugador siempre usa el hueco 0; los compañeros se reparten 1..3 según su orden en el grupo
	if (m == M_PLAYER) return 0;
	int k = 1;
	for (int i = 0; i < G.nparty; ++i) {
		if (G.party[i] == M_PLAYER) continue;
		if (G.party[i] == m) return k <= 3 ? k : 3;
		k++;
	}
	return m <= 3 ? m : 3;
}

const Composed* member_sheet(int m) {
	int s = slot_of(m);
	Composed* c = &map_c[s];
	GearLook g = gear_look(m);
	u32 key = look_key(&g, m == M_PLAYER ? &G.app : 0) ^ (m * 0x9E3779B1u) ^ serial * 31 ^ (flag("kaelen_dark") << 3) ^ (yara_is_witch() << 5);
	if (c->valid && c->key == key) return c;
	c->tiles = map_buf[s];
	begin(160, 120);
	if (m == M_PLAYER) compose_player(0, &G.app, &g);
	else {
		draw_layer(sfmt("chars/%s", member_base(m)), RC_NONE, 0);
		overlay_gear("", member_race(m), member_sex(m), &g);
	}
	emit_map(c);
	c->key = key;
	c->valid = 1;
	c->kind = 0;
	return c;
}

const Composed* member_battle_sheet(int m) {
	int s = slot_of(m);
	Composed* c = &bat_c[s];
	GearLook g = gear_look(m);
	if (m == M_YARA && yara_is_witch() && !g.orb[0]) g.orb = "dark";
	u32 key = look_key(&g, m == M_PLAYER ? &G.app : 0) ^ (m * 0x85EBCA6Bu) ^ serial * 31 ^ (flag("kaelen_dark") << 3) ^ (yara_is_witch() << 5);
	if (c->valid && c->key == key) return c;
	c->tiles = bat_buf[s];
	begin(288, 32);
	if (m == M_PLAYER) compose_player('b', &G.app, &g);
	else {
		weapon_layer(member_race(m), &g);
		draw_layer(sfmt("battle/%s_nw", member_base(m)), RC_NONE, 0);
		overlay_gear("b", member_race(m), member_sex(m), &g);
	}
	emit_battle(c);
	c->key = key;
	c->valid = 1;
	c->kind = 1;
	return c;
}

const Composed* member_portrait(int m) {
	int s = slot_of(m);
	Composed* c = &por_c[s];
	if (m != M_PLAYER) {
		const char* n = member_id(m);
		if (m == M_YARA && yara_is_witch()) n = "yara_dark";
		if (m == M_KAELEN && flag("kaelen_dark")) n = "kaelen_dark";
		const Portrait* p = portrait_by_name(n);
		if (p) {
			c->tiles = (u32*)p->tiles;
			memcpy(c->pal, p->pal, 32);
			c->valid = 0;
			return c;
		}
	}
	u32 key = look_key(&(GearLook){0}, &G.app) ^ serial * 31;
	if (c->valid && c->key == key && c->tiles == por_buf[s]) return c;
	c->tiles = por_buf[s];
	GearLook g = gear_look(M_PLAYER);
	begin(48, 48);
	compose_player('p', &G.app, &g);
	emit_portrait(c);
	c->key = key;
	c->valid = 1;
	c->kind = 2;
	return c;
}

const Composed* preview_sheet(const Appearance* a) {
	Composed* c = &map_c[0];
	c->tiles = map_buf[0];
	GearLook g = {0, 1, "", "", "", ""};
	begin(160, 120);
	compose_player(0, a, &g);
	emit_map(c);
	c->valid = 0;
	return c;
}

const Composed* preview_portrait(const Appearance* a) {
	Composed* c = &por_c[0];
	c->tiles = por_buf[0];
	GearLook g = {0, 1, "", "", "", ""};
	begin(48, 48);
	compose_player('p', a, &g);
	emit_portrait(c);
	c->valid = 0;
	return c;
}

void composed_upload_frame(const Composed* c, int frame, int tile) {
	const u32* src = c->tiles + frame * 12 * 8;
	volatile u32* dst = objt_ptr(tile);
	for (int r = 0; r < 4; ++r) {
		dma_copy32(dst + r * 4 * 8, src + r * 3 * 8, 24);
		for (int k = 0; k < 8; ++k) dst[(r * 4 + 3) * 8 + k] = 0;
	}
}

// ------------------------------------------------------------------ recursos fijos por nombre
#include "../build/gen/gfx_names.h"
