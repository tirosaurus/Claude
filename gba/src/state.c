#include "state.h"
#include "audio.h"
#include <stdio.h>
#include <stdlib.h>

GameState G;

// ------------------------------------------------------------------ flags y metadatos
// Tabla de entradas clave -> entero o texto, con las cadenas en un montón compacto.
#define META_MAX 640
#define HEAP_SIZE 8192
typedef struct { u16 key; u16 sval; s32 ival; u8 is_str; u8 used; } MetaEnt;
static MetaEnt meta[META_MAX] EWRAM_BSS;
static int nmeta;
static char heap[HEAP_SIZE] EWRAM_BSS;
static int heap_top;

static void heap_compact(void);
static int heap_put(const char* s) {
	int n = strlen(s) + 1;
	if (heap_top + n > HEAP_SIZE) heap_compact();
	if (heap_top + n > HEAP_SIZE) return 0;
	int off = heap_top;
	memcpy(heap + off, s, n);
	heap_top += n;
	return off;
}
static void heap_compact(void) {
	extern u8 g_scratch[];
	char* tmp = (char*)g_scratch;
	int top = 1;
	for (int i = 0; i < nmeta; ++i) {
		if (!meta[i].used) continue;
		const char* k = heap + meta[i].key;
		int n = strlen(k) + 1;
		memcpy(tmp + top, k, n);
		meta[i].key = top;
		top += n;
		if (meta[i].is_str) {
			const char* v = heap + meta[i].sval;
			n = strlen(v) + 1;
			memcpy(tmp + top, v, n);
			meta[i].sval = top;
			top += n;
		}
	}
	memcpy(heap, tmp, top);
	heap_top = top;
}

void flags_clear_all(void) {
	nmeta = 0;
	heap_top = 1;
	heap[0] = 0;
}

static MetaEnt* meta_find(const char* k) {
	for (int i = 0; i < nmeta; ++i)
		if (meta[i].used && !strcmp(heap + meta[i].key, k)) return &meta[i];
	return 0;
}
static MetaEnt* meta_add(const char* k) {
	MetaEnt* e = meta_find(k);
	if (e) return e;
	int i;
	for (i = 0; i < nmeta; ++i) if (!meta[i].used) break;
	if (i == nmeta) { if (nmeta >= META_MAX) return 0; nmeta++; }
	e = &meta[i];
	e->used = 1;
	e->is_str = 0;
	e->ival = 0;
	e->key = heap_put(k);
	return e;
}

int flag(const char* f) { MetaEnt* e = meta_find(f); return e && !e->is_str && e->ival; }
void setf(const char* f) { setf_v(f, 1); }
void setf_v(const char* f, int v) { MetaEnt* e = meta_add(f); if (e) { e->is_str = 0; e->ival = v; } }
int meta_i(const char* k, int def) { MetaEnt* e = meta_find(k); return e && !e->is_str ? e->ival : def; }
void meta_set_i(const char* k, int v) { setf_v(k, v); }
int meta_has(const char* k) { return meta_find(k) != 0; }
void meta_erase(const char* k) { MetaEnt* e = meta_find(k); if (e) e->used = 0; }
const char* meta_s(const char* k, const char* def) { MetaEnt* e = meta_find(k); return e && e->is_str ? heap + e->sval : def; }
void meta_set_s(const char* k, const char* v) {
	MetaEnt* e = meta_add(k);
	if (!e) return;
	e->is_str = 1;
	int off = heap_put(v);
	e = meta_find(k);           // la compactación puede mover claves, no entradas
	if (e) e->sval = off;
}

// ------------------------------------------------------------------ partida
void default_appearance(Appearance* a) {
	a->race = RACE_HUMAN; a->female = 0; a->hair_style = 1; a->hair_color = 0; a->skin = 1; a->outfit = 0; a->beard = 0;
}

const char* race_id(void) { return g_races[G.race].id; }
const char* race_name(void) { return g_races[G.race].name; }
const char* player_name(void) { return G.name; }
int is_female(void) { return G.female; }
const char* gfm(const char* m, const char* f) { return G.female ? f : m; }

static const char* const MID[M_COUNT] = {"player", "kaelen", "yara", "aelis", "brom"};
const char* member_id(int m) { return m >= 0 && m < M_COUNT ? MID[m] : ""; }
int member_from_id(const char* id) {
	for (int i = 0; i < M_COUNT; ++i) if (!strcmp(MID[i], id)) return i;
	return -1;
}
static const MemberDef* mdef(int m) { return &g_members[m - 1]; }

void gs_new_game(const char* name, const Appearance* app, int diff) {
	flags_clear_all();
	memset(&G, 0, sizeof G);
	strncpy(G.name, name, 15);
	G.race = app->race;
	G.female = app->female;
	G.branch = -1;
	G.cls = -1;
	G.difficulty = diff;
	G.app = *app;
	G.level = 1;
	G.xp = 0;
	G.gold = 30;
	for (int m = 0; m < M_COUNT; ++m) for (int s = 0; s < SL_N; ++s) G.equipment[m][s] = -1;
	G.has_equip[M_PLAYER] = 1;
	add_item("pocion", 2);
	G.comp[M_KAELEN].alive = 1;
	G.comp[M_YARA].alive = 1;
	G.comp[M_YARA].kaelen_approval = 20;
	G.comp[M_AELIS].alive = 1;
	G.comp[M_BROM].alive = 1;
	G.party[0] = M_PLAYER;
	G.nparty = 1;
	heal_all();
	strcpy(G.current_map, "bedroom");
	strcpy(G.current_spawn, "start");
	G.has_load_pos = 0;
	G.play_frames = 0;
}

void join_party(int m) {
	if (m <= 0 || m >= M_COUNT) return;
	G.comp[m].joined = 1;
	if (!in_party(m)) G.party[G.nparty++] = m;
	if (!G.has_equip[m]) {
		G.has_equip[m] = 1;
		for (int i = 0; i < g_member_gear_n; ++i)
			if (!strcmp(g_member_gear[i].key, MID[m]))
				for (int k = 0; k < g_member_gear[i].n; ++k) {
					int e = db_equip(g_member_gear[i].items[k]);
					if (e >= 0) G.equipment[m][g_equip[e].slot] = e;
				}
	}
	Stats7 st = gs_stats(m);
	G.members[m].hp = st.v[ST_HP];
	G.members[m].mp = st.v[ST_MP];
}

void leave_party(int m) {
	int j = 0;
	for (int i = 0; i < G.nparty; ++i) if (G.party[i] != m) G.party[j++] = G.party[i];
	G.nparty = j;
	if (m > 0 && m < M_COUNT) G.comp[m].joined = 0;
}

int in_party(int m) {
	for (int i = 0; i < G.nparty; ++i) if (G.party[i] == m) return 1;
	return 0;
}

const char* member_name(int m) { return m == M_PLAYER ? player_name() : mdef(m)->name; }

int yara_is_witch(void) { return G.comp[M_YARA].corruption >= 40; }

const char* member_role(int m) {
	if (m == M_PLAYER) {
		if (G.cls >= 0) return g_classes[G.cls].name;
		return G.female ? "Aventurera" : "Aventurero";
	}
	if (m == M_YARA && yara_is_witch()) return "Bruja";
	if (m == M_KAELEN && flag("kaelen_dark")) return "Caído";
	return mdef(m)->role;
}

Stats7 gs_stats(int m) {
	Stats7 s;
	const Stats7* base;
	const StatsF* grow;
	if (m == M_PLAYER) { base = &g_races[G.race].base; grow = &g_races[G.race].grow; }
	else { base = &mdef(m)->base; grow = &mdef(m)->grow; }
	for (int k = 0; k < ST_N; ++k) s.v[k] = (s32)(base->v[k] + grow->v[k] * (G.level - 1));
	if (m == M_PLAYER) {
		if (G.branch >= 0)
			for (int k = 0; k < ST_N; ++k) s.v[k] += g_branches[G.branch].bonus.v[k];
		if (G.cls >= 0) {
			const ClassDef* c = &g_classes[G.cls];
			for (int k = 0; k < ST_N; ++k) s.v[k] += c->bonus.v[k];
			for (int n = 0; n < 9; ++n) {
				int r = G.tree[n];
				if (r > 0 && c->tree[n].has_stats)
					for (int k = 0; k < ST_N; ++k) s.v[k] += c->tree[n].stats.v[k] * r;
			}
		}
	}
	for (int sl = 0; sl < SL_N; ++sl) {
		int e = G.equipment[m][sl];
		if (e >= 0) for (int k = 0; k < ST_N; ++k) s.v[k] += g_equip[e].stats.v[k];
	}
	if (flag("selen_shattered")) s.v[ST_MAG] += 3;
	if (flag("kraag_spared") && m == M_PLAYER) s.v[ST_RES] += 2;
	if (m == M_PLAYER && flag("player_dark")) { s.v[ST_ATK] += 8; s.v[ST_MAG] += 8; s.v[ST_RES] -= 3; }
	for (int k = 0; k < ST_N; ++k) if (s.v[k] < 1) s.v[k] = 1;
	return s;
}

// ------------------------------------------------------------------ equipo
const char* equip_rule_key(int m) {
	if (m == M_PLAYER) return G.cls >= 0 ? g_classes[G.cls].id : "none";
	return MID[m];
}

int equip_allowed(const char* rule, int e) {
	const EquipRule* r = 0;
	for (int i = 0; i < g_nequip_rules; ++i) if (!strcmp(g_equip_rules[i].key, rule)) { r = &g_equip_rules[i]; break; }
	if (!r) for (int i = 0; i < g_nequip_rules; ++i) if (!strcmp(g_equip_rules[i].key, "none")) { r = &g_equip_rules[i]; break; }
	const EquipDef* d = &g_equip[e];
	switch (d->slot) {
	case SL_WEAPON: return (r->weapons >> d->kind) & 1;
	case SL_SHIELD: return r->shield;
	case SL_HEAD: case SL_BODY: return (r->weights >> d->weight) & 1;
	}
	return 1;
}

int equipped(int m, int slot) { return G.equipment[m][slot]; }

void gs_equip(int m, int slot, int e) {
	Stats7 before = gs_stats(m);
	int cur = G.equipment[m][slot];
	if (cur >= 0) { add_item_k(N_ITEMS + cur, 1); G.equipment[m][slot] = -1; }
	if (e >= 0) { remove_item_k(N_ITEMS + e, 1); G.equipment[m][slot] = e; }
	G.has_equip[m] = 1;
	Stats7 after = gs_stats(m);
	MemberState* ms = &G.members[m];
	int lo = ms->hp > 0 ? 1 : 0;
	ms->hp = CLAMP(ms->hp + after.v[ST_HP] - before.v[ST_HP], lo, after.v[ST_HP]);
	ms->mp = CLAMP(ms->mp + after.v[ST_MP] - before.v[ST_MP], 0, after.v[ST_MP]);
	extern void appearance_dirty(void);
	appearance_dirty();
}

GearLook gear_look(int m) {
	GearLook g = {0, 1, "", "", "", ""};
	int w = G.equipment[m][SL_WEAPON];
	if (w >= 0) {
		g.weapon_kind = g_equip[w].kind;
		g.weapon_tier = g_equip[w].tier;
		g.orb = g_equip[w].orb ? g_equip[w].orb : "";
		if (!g.orb[0] && m == M_PLAYER && G.cls >= 0 && g_classes[G.cls].orb) g.orb = g_classes[G.cls].orb;
	}
	int e;
	if ((e = G.equipment[m][SL_HEAD]) >= 0 && g_equip[e].look) g.head = g_equip[e].look;
	if ((e = G.equipment[m][SL_BODY]) >= 0 && g_equip[e].look) g.body = g_equip[e].look;
	if ((e = G.equipment[m][SL_SHIELD]) >= 0 && g_equip[e].look) g.shield = g_equip[e].look;
	return g;
}

int gs_skills_of(int m, s16* out) {
	int n = 0;
	if (m == M_PLAYER) {
		if (G.cls < 0) { out[n++] = db_skill("aux"); return n; }
		const ClassDef* c = &g_classes[G.cls];
		out[n++] = c->starter;
		if (c->starter != db_skill("aux") && strcmp(c->id, "cleric")) out[n++] = db_skill("aux");
		for (int i = 0; i < 9; ++i) {
			if (c->tree[i].skill < 0 || G.tree[i] <= 0) continue;
			int dup = 0;
			for (int k = 0; k < n; ++k) if (out[k] == c->tree[i].skill) dup = 1;
			if (!dup) out[n++] = c->tree[i].skill;
		}
		return n;
	}
	const MemberDef* d = mdef(m);
	const u8* lv = d->skill_lv;
	const s16* sk = d->skill;
	int cnt = d->nskills;
	if (m == M_YARA && yara_is_witch()) { lv = d->dark_lv; sk = d->dark; cnt = d->ndark; }
	for (int i = 0; i < cnt; ++i) if (G.level >= lv[i]) out[n++] = sk[i];
	if (m == M_KAELEN && flag("kaelen_redeemed")) out[n++] = db_skill("tajo_sombrio");
	return n;
}

// ------------------------------------------------------------------ talentos
int skill_points_total(void) { return G.level - 1 > 0 ? G.level - 1 : 0; }
int skill_points_spent(void) { int n = 0; for (int i = 0; i < 9; ++i) n += G.tree[i]; return n; }
int skill_points(void) { return G.cls < 0 ? 0 : skill_points_total() - skill_points_spent(); }

const char* talent_block_reason(int node) {
	if (G.cls < 0) return "?";
	const TreeNode* n = &g_classes[G.cls].tree[node];
	if (G.tree[node] >= n->max) return "Rango máximo";
	static const int need_t[4] = {0, 0, 3, 6};
	int need = need_t[n->tier];
	if (skill_points_spent() < need) return sfmt("Requiere %d puntos gastados", need);
	if (skill_points() <= 0) return "Sin puntos";
	return "";
}

int learn_talent(int node) {
	if (talent_block_reason(node)[0]) return 0;
	Stats7 before = gs_stats(M_PLAYER);
	G.tree[node]++;
	Stats7 after = gs_stats(M_PLAYER);
	G.members[M_PLAYER].hp += after.v[ST_HP] - before.v[ST_HP];
	G.members[M_PLAYER].mp += after.v[ST_MP] - before.v[ST_MP];
	return 1;
}

float perk(int key) {
	if (G.cls < 0) return 0;
	float v = 0;
	const ClassDef* c = &g_classes[G.cls];
	for (int i = 0; i < 9; ++i) if (c->tree[i].perk == key) v += c->tree[i].perk_val * G.tree[i];
	return v;
}
int perk_sum(int key) { return (int)(perk(key) * 1000); }

void level_restore(void) {
	for (int m = 0; m < M_COUNT; ++m) {
		Stats7 st = gs_stats(m);
		MemberState* ms = &G.members[m];
		int hp = (ms->hp > 1 ? ms->hp : 1) + (int)(st.v[ST_HP] * 0.5f);
		ms->hp = MIN(st.v[ST_HP], hp);
		int mpadd = (st.v[ST_MP] * 25 + 99) / 100;
		ms->mp = MIN(st.v[ST_MP], ms->mp + mpadd);
	}
}

int difficulty(void) { return G.difficulty; }
void diff_mults(float o[3]) {
	static const float t[3][3] = {{0.7f, 0.75f, 1.25f}, {0.85f, 0.87f, 1.1f}, {1.3f, 1.18f, 1.0f}};
	int d = CLAMP(G.difficulty, 0, 2);
	o[0] = t[d][0]; o[1] = t[d][1]; o[2] = t[d][2];
}

void heal_all(void) {
	for (int m = 0; m < M_COUNT; ++m) {
		Stats7 st = gs_stats(m);
		G.members[m].hp = st.v[ST_HP];
		G.members[m].mp = st.v[ST_MP];
	}
}

int xp_to_next(int level) { return 50 + level * level * 24; }

int add_xp(int amount, LevelUp* out, int max_out) {
	int n = 0;
	if (G.level >= MAX_LEVEL) { G.xp = 0; return 0; }
	G.xp += amount;
	while (G.level < MAX_LEVEL && G.xp >= xp_to_next(G.level)) {
		G.xp -= xp_to_next(G.level);
		s16 before[M_COUNT][16];
		int nb[M_COUNT];
		for (int i = 0; i < G.nparty; ++i) nb[i] = gs_skills_of(G.party[i], before[i]);
		G.level++;
		LevelUp lu;
		memset(&lu, 0, sizeof lu);
		lu.level = G.level;
		for (int i = 0; i < G.nparty; ++i) {
			s16 now[16];
			int k = gs_skills_of(G.party[i], now);
			for (int a = 0; a < k; ++a) {
				int had = 0;
				for (int b = 0; b < nb[i]; ++b) if (before[i][b] == now[a]) had = 1;
				if (!had && lu.n < 8) { lu.who[lu.n] = G.party[i]; lu.skill[lu.n] = now[a]; lu.n++; }
			}
		}
		level_restore();
		if (n < max_out) out[n] = lu;
		n++;
	}
	if (G.level >= MAX_LEVEL) G.xp = 0;
	return n < max_out ? n : max_out;
}

int owns(const char* id) {
	if (item_count(id) > 0) return 1;
	int e = db_equip(id);
	if (e < 0) return 0;
	for (int m = 0; m < M_COUNT; ++m)
		for (int s = 0; s < SL_N; ++s) if (G.equipment[m][s] == e) return 1;
	return 0;
}
int missing_legendaries(void) {
	int n = 0;
	for (int i = 0; i < g_nlegendaries; ++i) if (!owns(g_legendaries[i])) n++;
	return n;
}
int vael_worthy(void) { return missing_legendaries() == 0 && !flag("ov_done"); }
int needs_branch_choice(void) { return G.level >= 2 && G.branch < 0; }
int needs_class_choice(void) { return G.level >= 2 && G.branch >= 0 && G.cls < 0; }
void choose_branch(int b) { G.branch = b; heal_all(); }

void choose_class(int c) {
	G.cls = c;
	for (int b = 0; b < N_BRANCHES; ++b)
		for (int k = 0; k < 3; ++k) if (g_branches[b].classes[k] == c) G.branch = b;
	for (int s = 0; s < SL_N; ++s) {
		int e = G.equipment[M_PLAYER][s];
		if (e >= 0 && !equip_allowed(g_classes[c].id, e)) gs_equip(M_PLAYER, s, -1);
	}
	for (int i = 0; i < g_class_gear_n; ++i) {
		if (strcmp(g_class_gear[i].key, g_classes[c].id)) continue;
		for (int k = 0; k < g_class_gear[i].n; ++k) {
			const char* id = g_class_gear[i].items[k];
			add_item(id, 1);
			int e = db_equip(id);
			if (e >= 0 && G.equipment[M_PLAYER][g_equip[e].slot] < 0) gs_equip(M_PLAYER, g_equip[e].slot, e);
		}
	}
	extern void appearance_dirty(void);
	appearance_dirty();
	heal_all();
}

const char* branch_name(void) { return G.branch < 0 ? "Sin senda" : g_branches[G.branch].name; }
const char* class_name_str(void) { return G.cls < 0 ? "Sin clase" : g_classes[G.cls].name; }

// ------------------------------------------------------------------ inventario
int item_key(const char* id) {
	int i = db_item(id);
	if (i >= 0) return i;
	int e = db_equip(id);
	return e >= 0 ? N_ITEMS + e : -1;
}
const char* item_key_id(int k) { return k < N_ITEMS ? g_items[k].id : g_equip[k - N_ITEMS].id; }

int item_count_k(int k) {
	for (int i = 0; i < G.ninv; ++i) if (G.inv[i].item == k) return G.inv[i].count;
	return 0;
}
void add_item_k(int k, int qty) {
	if (k < 0) return;
	for (int i = 0; i < G.ninv; ++i) if (G.inv[i].item == k) { G.inv[i].count += qty; return; }
	if (G.ninv < ARRLEN(G.inv)) { G.inv[G.ninv].item = k; G.inv[G.ninv].count = qty; G.ninv++; }
}
void remove_item_k(int k, int qty) {
	for (int i = 0; i < G.ninv; ++i) {
		if (G.inv[i].item != k) continue;
		G.inv[i].count -= qty;
		if (G.inv[i].count <= 0) {
			for (int j = i; j + 1 < G.ninv; ++j) G.inv[j] = G.inv[j + 1];
			G.ninv--;
		}
		return;
	}
}
void add_item(const char* id, int qty) { add_item_k(item_key(id), qty); }
void remove_item(const char* id, int qty) { remove_item_k(item_key(id), qty); }
int item_count(const char* id) { int k = item_key(id); return k < 0 ? 0 : item_count_k(k); }

// ------------------------------------------------------------------ vínculos
static void recalc_yara(void) {
	Companion* y = &G.comp[M_YARA];
	Companion* k = &G.comp[M_KAELEN];
	int player_wants = y->approval >= 25 || flag("romance_yara");
	int kaelen_wants = y->kaelen_approval >= 45 && k->alive;
	int target = ROM_NONE;
	if (player_wants && kaelen_wants && rivalry() <= -30 && flag("trio_ok")) target = ROM_BOTH;
	else if (player_wants && flag("romance_yara")) target = ROM_PLAYER;
	else if (kaelen_wants && !flag("romance_yara")) target = ROM_KAELEN;
	y->romance_target = target;
}
void change_approval(int m, int d) {
	if (m <= 0 || m >= M_COUNT) return;
	G.comp[m].approval = CLAMP(G.comp[m].approval + d, -100, 100);
	if (m == M_YARA) recalc_yara();
}
void change_kaelen_rivalry(int d) { G.comp[M_KAELEN].rivalry = CLAMP(G.comp[M_KAELEN].rivalry + d, -100, 100); recalc_yara(); }
void change_yara_kaelen_approval(int d) { G.comp[M_YARA].kaelen_approval = CLAMP(G.comp[M_YARA].kaelen_approval + d, -100, 100); recalc_yara(); }
void change_yara_corruption(int d) { G.comp[M_YARA].corruption = CLAMP(G.comp[M_YARA].corruption + d, -100, 100); }
int rivalry(void) { return G.comp[M_KAELEN].rivalry; }
int corruption(void) { return G.comp[M_YARA].corruption; }

const char* kaelen_bond_label(void) {
	int r = rivalry();
	if (!G.comp[M_KAELEN].alive) return "Caído";
	if (r <= -40) return "Hermanos de alma";
	if (r <= -10) return "Mejores amigos";
	if (r < 10) return "Amigos de la infancia";
	if (r < 40) return "Pique constante";
	return "Rivales";
}
const char* yara_bond_label(void) {
	int a = G.comp[M_YARA].approval;
	if (flag("romance_yara")) return "Enamorada de ti";
	if (a >= 20) return "Muy unida a ti";
	if (a >= 8) return "Cómplice";
	if (a > -8) return "Amiga de siempre";
	return "Dolida contigo";
}

const char* format_time(u32 frames) {
	u32 s = frames / 60;
	return sfmt("%lu:%02lu:%02lu", (unsigned long)(s / 3600), (unsigned long)((s / 60) % 60), (unsigned long)(s % 60));
}

// ------------------------------------------------------------------ guardado en SRAM
// Disposición: 0..63 cabecera/opciones; cada ranura 10 KB a partir de 0x0100.
#define SLOT_BASE(i) (0x0100 + (i) * 0x2800)
#define SLOT_SIZE 0x2800
#define SAVE_MAGIC 0x564C4D32u     // "VLM2"

static void sram_write(int off, const void* src, int n) {
	const u8* s = src;
	for (int i = 0; i < n; ++i) SRAM[off + i] = s[i];
}
static void sram_read(int off, void* dst, int n) {
	u8* d = dst;
	for (int i = 0; i < n; ++i) d[i] = SRAM[off + i];
}
static u32 checksum(const u8* p, int n) {
	u32 h = 2166136261u;
	for (int i = 0; i < n; ++i) { h ^= p[i]; h *= 16777619u; }
	return h;
}

typedef struct { u32 magic, size, sum, serial; } SaveHdr;
extern u8 g_scratch[];
#define savebuf g_scratch

static u32 next_serial(void) {
	u32 best = 0;
	for (int i = 0; i < SAVE_SLOTS; ++i) {
		SaveHdr h;
		sram_read(SLOT_BASE(i), &h, sizeof h);
		if (h.magic == SAVE_MAGIC && h.serial > best) best = h.serial;
	}
	return best + 1;
}

int save_game(int slot, int x, int y) {
	u8* p = savebuf + sizeof(SaveHdr);
	G.has_load_pos = 1;
	G.load_x = x; G.load_y = y;
	memcpy(p, &G, sizeof G);
	p += sizeof G;
	// flags y metadatos: clave\0 tipo valor
	for (int i = 0; i < nmeta; ++i) {
		if (!meta[i].used) continue;
		const char* k = heap + meta[i].key;
		int kl = strlen(k) + 1;
		int vl = meta[i].is_str ? (int)strlen(heap + meta[i].sval) + 1 : 4;
		if (p + kl + 1 + vl + 2 >= savebuf + SLOT_SIZE) break;
		memcpy(p, k, kl); p += kl;
		*p++ = meta[i].is_str;
		if (meta[i].is_str) { memcpy(p, heap + meta[i].sval, vl); p += vl; }
		else { memcpy(p, &meta[i].ival, 4); p += 4; }
	}
	*p++ = 0;
	SaveHdr h;
	h.magic = SAVE_MAGIC;
	h.size = p - savebuf;
	h.sum = checksum(savebuf + sizeof h, h.size - sizeof h);
	h.serial = next_serial();
	memcpy(savebuf, &h, sizeof h);
	sram_write(SLOT_BASE(slot), savebuf, h.size);
	return 1;
}

int load_game(int slot) {
	SaveHdr h;
	sram_read(SLOT_BASE(slot), &h, sizeof h);
	if (h.magic != SAVE_MAGIC || h.size > SLOT_SIZE || h.size < sizeof h + sizeof G) return 0;
	sram_read(SLOT_BASE(slot), savebuf, h.size);
	if (checksum(savebuf + sizeof h, h.size - sizeof h) != h.sum) return 0;
	u8* p = savebuf + sizeof h;
	memcpy(&G, p, sizeof G);
	p += sizeof G;
	flags_clear_all();
	while (p < savebuf + h.size && *p) {
		const char* k = (const char*)p;
		p += strlen(k) + 1;
		int is_str = *p++;
		if (is_str) { meta_set_s(k, (const char*)p); p += strlen((const char*)p) + 1; }
		else { s32 v; memcpy(&v, p, 4); p += 4; setf_v(k, v); }
	}
	G.current_spawn[0] = 0;
	extern void appearance_dirty(void);
	appearance_dirty();
	return 1;
}

int slot_info(int slot, SlotInfo* out) {
	memset(out, 0, sizeof *out);
	SaveHdr h;
	sram_read(SLOT_BASE(slot), &h, sizeof h);
	if (h.magic != SAVE_MAGIC || h.size > SLOT_SIZE) return 0;
	static GameState tmp EWRAM_BSS;
	sram_read(SLOT_BASE(slot) + sizeof h, &tmp, sizeof tmp);
	out->used = 1;
	strncpy(out->name, tmp.name, 15);
	out->level = tmp.level;
	strncpy(out->map, tmp.current_map, 19);
	out->play_frames = tmp.play_frames;
	out->serial = h.serial;
	return 1;
}

int any_save(void) {
	int best = -1;
	u32 bs = 0;
	for (int i = 0; i < SAVE_SLOTS; ++i) {
		SlotInfo si;
		if (slot_info(i, &si) && (best < 0 || si.serial > bs)) { best = i; bs = si.serial; }
	}
	return best;
}

void settings_save(void) {
	u8 d[8] = {'V', 'O', 'P', 'T', g_music_level, g_sfx_level, g_text_speed, g_battle_speed};
	sram_write(0, d, 8);
}
void settings_load(void) {
	u8 d[8];
	sram_read(0, d, 8);
	if (d[0] == 'V' && d[1] == 'O' && d[2] == 'P' && d[3] == 'T') {
		g_music_level = d[4] > 10 ? 8 : d[4];
		g_sfx_level = d[5] > 10 ? 8 : d[5];
		g_text_speed = d[6] > 2 ? 1 : d[6];
		g_battle_speed = d[7] > 2 ? 1 : d[7];
	}
	audio_apply_settings();
}
