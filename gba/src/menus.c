// Menú de pausa (PauseMenu.gd) con Grupo, Objetos, Equipo (EquipUI.gd), Talentos (TalentUI.gd),
// Guardar, Opciones y Título; y la tienda (ShopUI.gd).
#include "world.h"
#include "ui.h"
#include "audio.h"
#include "scene.h"
#include "dialogue.h"
#include <stdio.h>
u32 g_dbg_t[4];

extern void options_ui(void);
extern int save_ui_run(int save_mode, int px, int py);

// ------------------------------------------------------------------ dibujo de sprites del menú
#define MAX_MSPR 12
static struct { int kind, x, y; const char* icon; int member, frame; } mspr[MAX_MSPR];
static int nmspr;
static int bat_tile[M_COUNT] = {-1, -1, -1, -1, -1};
static int bat_pal[M_COUNT] = {-1, -1, -1, -1, -1};
static int hand_on, hand_x, hand_y, hand_flip;

static void mspr_clear(void) { nmspr = 0; hand_on = 0; }
static void mspr_icon(int x, int y, const char* icon) {
	if (nmspr < MAX_MSPR) { mspr[nmspr].kind = 0; mspr[nmspr].x = x; mspr[nmspr].y = y; mspr[nmspr].icon = icon; nmspr++; }
}
static void mspr_member(int x, int y, int m, int frame) {
	if (nmspr < MAX_MSPR) { mspr[nmspr].kind = 1; mspr[nmspr].x = x; mspr[nmspr].y = y; mspr[nmspr].member = m; mspr[nmspr].frame = frame; nmspr++; }
}
static void menu_sprites_free(void) {
	for (int m = 0; m < M_COUNT; ++m) {
		if (bat_tile[m] >= 0) objt_free(bat_tile[m]);
		if (bat_pal[m] >= 0) objpal_put(bat_pal[m]);
		bat_tile[m] = bat_pal[m] = -1;
	}
}

static void menu_render(void) {
	oam_begin();
	for (int i = 0; i < nmspr; ++i) {
		if (mspr[i].kind == 0) ui_icon(mspr[i].x, mspr[i].y, mspr[i].icon, i);
		else {
			int m = mspr[i].member;
			const Composed* c = member_battle_sheet(m);
			if (bat_tile[m] < 0) bat_tile[m] = objt_alloc(16);
			if (bat_pal[m] >= 0) objpal_put(bat_pal[m]);
			bat_pal[m] = objpal_get(c->pal, 256, 256, 256, 0);
			if (bat_tile[m] >= 0) {
				dma_copy32(objt_ptr(bat_tile[m]), c->tiles + mspr[i].frame * 16 * 8, 128);
				oam_push(0x2000 + i, obj_a0(mspr[i].y, OBJ_SQUARE, 0), obj_a1(mspr[i].x, 2, 0), obj_a2(bat_tile[m], 0, bat_pal[m]));
			}
		}
	}
	if (hand_on) ui_cursor(hand_x, hand_y, CUR_HAND, hand_flip);
	ui_frame_sprites();
	oam_end();
}

static void mwait(void) {
	menu_render();
	frame_wait();
}

static int panel_bg(int np) {
	const NineSrc* n = &g_nine[np];
	return n->px[(n->h / 2) * n->w + n->w / 2];
}

// ------------------------------------------------------------------ pantalla de grupo
static void show_party(void) {
	mspr_clear();
	ui_panel(NP_PANEL, PAL_UI_LIGHT, 8, 0, 22, 20);
	int y = 4;
	for (int i = 0; i < G.nparty; ++i) {
		int m = G.party[i];
		Stats7 st = gs_stats(m);
		MemberState* ms = &G.members[m];
		mspr_member(66, y + 2, m, 0);
		int x = 98;
		ui_text(x, y, sfmt("%s · %s · Nv %ld", member_name(m), member_role(m), (long)G.level), UC_LIGHT_TEXT, -1);
		ui_text(x, y + 9, sfmt("Vida %ld/%ld  Maná %ld/%ld", (long)ms->hp, (long)st.v[ST_HP], (long)ms->mp, (long)st.v[ST_MP]), UC_LIGHT_TEXT, -1);
		ui_text(x, y + 18, sfmt("Atq %ld Def %ld Mag %ld Res %ld", (long)st.v[ST_ATK], (long)st.v[ST_DEF], (long)st.v[ST_MAG], (long)st.v[ST_RES]), UC_LIGHT_TEXT, -1);
		int w = ui_text(x, y + 27, sfmt("Vel %ld", (long)st.v[ST_SPD]), UC_LIGHT_TEXT, -1);
		const char* rel = m == M_KAELEN ? kaelen_bond_label() : m == M_YARA ? yara_bond_label() : m == M_PLAYER ? race_name() : "";
		if (rel[0]) ui_text_right(x + w + 6, y + 27, 232 - (x + w + 6), rel, UC_LIGHT_HI2, -1);
		y += 38;
	}
}

static void info_panel(void) {
	ui_panel(NP_PANEL_DARK, PAL_UI_DARK, 0, 16, 8, 4);
	ui_text(6, 16 * 8 + 4, sfmt("%ld coronas", (long)G.gold), UC_DARK_TEXT, -1);
	ui_text(6, 16 * 8 + 14, format_time(G.play_frames), UC_DARK_TEXT, -1);
	const char* xp = G.level >= MAX_LEVEL ? "Nv máximo" : sfmt("Exp %ld/%d", (long)G.xp, xp_to_next(G.level));
	ui_text(6, 16 * 8 + 24, xp, UC_DARK_GOLD, -1);
}

// ------------------------------------------------------------------ objetos
static int usable_list(int* keys, int max, int consumables_only) {
	int n = 0;
	for (int i = 0; i < G.ninv && n < max; ++i) {
		int k = G.inv[i].item;
		if (consumables_only && k >= N_ITEMS) continue;
		if (k < N_ITEMS && G.inv[i].count > 0) keys[n++] = k;
	}
	// ordenar por id (como inventory.keys().sort())
	for (int a = 1; a < n; ++a) {
		int t = keys[a], j = a - 1;
		while (j >= 0 && strcmp(g_items[keys[j]].id, g_items[t].id) > 0) { keys[j + 1] = keys[j]; --j; }
		keys[j + 1] = t;
	}
	return n;
}

static int apply_item(int key, int target) {
	const ItemDef* it = &g_items[key];
	int used = 0;
	for (int i = 0; i < G.nparty; ++i) {
		int m = G.party[i];
		if (target >= 0 && m != target) continue;
		MemberState* ms = &G.members[m];
		Stats7 st = gs_stats(m);
		if (it->has_revive) {
			if (ms->hp <= 0) { ms->hp = (s32)(st.v[ST_HP] * it->revive); used = 1; }
			continue;
		}
		if (ms->hp <= 0) continue;
		if (it->heal && ms->hp < st.v[ST_HP]) { ms->hp = MIN(st.v[ST_HP], ms->hp + it->heal); used = 1; }
		if (it->mp && ms->mp < st.v[ST_MP]) { ms->mp = MIN(st.v[ST_MP], ms->mp + it->mp); used = 1; }
		if (it->cure) used = 1;
	}
	if (used) { remove_item_k(key, 1); sfx("heal", -6, 1); }
	else sfx("cancel", -8, 1);
	return used;
}

static void pick_target(int key) {
	const ItemDef* it = &g_items[key];
	if (it->target == T_ENEMIES || it->target == T_ENEMY) return;
	int all = it->target == T_ALLIES;
	int n = all ? 1 : G.nparty;
	int sel = 0;
	for (;;) {
		mspr_clear();
		ui_panel(NP_PANEL, PAL_UI_LIGHT, 8, 0, 22, 20);
		ui_text(72, 8, sfmt("¿Sobre quién usas %s?", it->name), UC_LIGHT_TEXT, -1);
		for (int i = 0; i < n + 1; ++i) {
			const char* label;
			if (i == n) label = "Volver";
			else if (all) label = "Todo el grupo";
			else {
				int m = G.party[i];
				label = sfmt("%s   %ld/%ld vida", member_name(m), (long)G.members[m].hp, (long)gs_stats(m).v[ST_HP]);
			}
			ui_button(9, 3 + i * 3, i == n ? 8 : 20, 3, label, i == sel ? 1 : 0, 1, 0);
		}
		for (;;) {
			mwait();
			if (key_rep(KEY_UP)) { sel = (sel + n) % (n + 1); sfx("select", -16, 1); break; }
			if (key_rep(KEY_DOWN)) { sel = (sel + 1) % (n + 1); sfx("select", -16, 1); break; }
			if (key_hit(KEY_B)) return;
			if (key_hit(KEY_A)) {
				if (sel == n) return;
				apply_item(key, all ? -1 : G.party[sel]);
				return;
			}
		}
	}
}

static void show_items(int interactive) {
	int keys[24];
	int sel = 0, scroll = 0;
	for (;;) {
		mspr_clear();
		int n = usable_list(keys, 24, 1);
		ui_panel(NP_PANEL, PAL_UI_LIGHT, 8, 0, 22, 20);
		if (!n) { ui_text(72, 140, "No llevas objetos.", UC_LIGHT_TEXT, -1); if (!interactive) return; }
		if (sel >= n) sel = n ? n - 1 : 0;
		if (sel < scroll) scroll = sel;
		if (sel >= scroll + 7) scroll = sel - 6;
		for (int r = 0; r < 7 && scroll + r < n; ++r) {
			int k = keys[scroll + r];
			ui_button(9, 1 + r * 2, 20, 2, sfmt("%s  ×%d", g_items[k].name, item_count_k(k)), interactive && scroll + r == sel ? 1 : 0, 1, g_items[k].icon);
			mspr_icon(9 * 8 + 4, (1 + r * 2) * 8, g_items[k].icon);
		}
		if (n && interactive) ui_text_wrapped(72, 124, 168, 10, g_items[keys[sel]].desc, UC_LIGHT_TEXT, -1, 3);
		if (!interactive) return;
		for (;;) {
			mwait();
			if (key_rep(KEY_UP) && n) { sel = (sel + n - 1) % n; sfx("select", -16, 1); break; }
			if (key_rep(KEY_DOWN) && n) { sel = (sel + 1) % n; sfx("select", -16, 1); break; }
			if (key_hit(KEY_B)) return;
			if (key_hit(KEY_A) && n) { pick_target(keys[sel]); break; }
		}
	}
}

// ------------------------------------------------------------------ equipo (pantalla completa)
static const char* const SLOT_ORDER_NAMES[5] = {"weapon", "shield", "head", "body", "ring"};

static int equip_score(int e) {
	int t = 0;
	for (int k = 0; k < ST_N; ++k) t += g_equip[e].stats.v[k] * (k == ST_HP || k == ST_MP ? 1 : 4);
	return t;
}

static int candidates(int m, int slot, int* out, int max) {
	int n = 0;
	const char* rule = equip_rule_key(m);
	for (int i = 0; i < G.ninv; ++i) {
		int k = G.inv[i].item;
		if (k < N_ITEMS || G.inv[i].count <= 0) continue;
		int e = k - N_ITEMS;
		if (g_equip[e].slot != slot || !equip_allowed(rule, e)) continue;
		if (n < max) out[n++] = e;
	}
	for (int a = 1; a < n; ++a) {
		int t = out[a], j = a - 1;
		while (j >= 0 && equip_score(out[j]) < equip_score(t)) { out[j + 1] = out[j]; --j; }
		out[j + 1] = t;
	}
	return n;
}

static const char* equip_icon(int e) {
	const EquipDef* d = &g_equip[e];
	switch (d->slot) {
	case SL_WEAPON: { static const char* const W[6] = {"eq_sword", "eq_sword", "eq_axe", "eq_daggers", "eq_staff", "eq_bow"}; return W[d->kind]; }
	case SL_SHIELD: return "eq_shield";
	case SL_HEAD: return d->weight == WT_CLOTH ? "eq_circlet" : (d->look && !strcmp(d->look, "hood") ? "eq_hood" : "eq_helm");
	case SL_BODY: return d->look && starts_with(d->look, "robe") ? "eq_robe" : "eq_armor";
	}
	return "eq_ring";
}

static const char* stats_text(const EquipDef* e) {
	static const char* const SN[7] = {"Vida", "PM", "Ataque", "Defensa", "Magia", "Resist.", "Velocidad"};
	char buf[160];
	int len = 0;
	buf[0] = 0;
	int first = 1;
	for (int k = 0; k < ST_N; ++k) {
		if (!(e->stat_mask & (1 << k))) continue;
		len += snprintf(buf + len, sizeof buf - len, "%s%s %+ld", first ? "" : ", ", SN[k], (long)e->stats.v[k]);
		first = 0;
	}
	return sfmt("%s", buf);
}

const char* item_desc(const char* id) {
	int i = db_item(id);
	if (i >= 0) return g_items[i].desc;
	int e = db_equip(id);
	if (e < 0) return "";
	const EquipDef* d = &g_equip[e];
	static const char* const SLN[5] = {"Arma", "Escudo", "Cabeza", "Cuerpo", "Anillo"};
	const char* s = sfmt("%s · %s · Nv %d · %s", g_rarity[d->rarity].name, SLN[d->slot], d->lvl, stats_text(d));
	if (d->lore) s = sfmt("%s\n%s", s, d->lore);
	return s;
}

static void equip_stats(int m, const Stats7* prev) {
	static const char* const SN[7] = {"Vida", "PM", "Ataque", "Defensa", "Magia", "Resist.", "Velocidad"};
	Stats7 cur = gs_stats(m);
	for (int k = 0; k < ST_N; ++k) {
		int y = 52 + k * 10;
		const char* t = sfmt("%s %ld", SN[k], (long)cur.v[k]);
		int col = UC_LIGHT_TEXT;
		if (prev) {
			int d = prev->v[k] - cur.v[k];
			if (d) { t = sfmt("%s → %ld", t, (long)prev->v[k]); col = d > 0 ? UC_LIGHT_GOOD : UC_LIGHT_SEL; }
		}
		ui_text(8, y, t, col, -1);
	}
}

static Stats7 preview_stats(int m, int slot, int e) {
	Stats7 st = gs_stats(m);
	int cur = equipped(m, slot);
	if (cur >= 0) for (int k = 0; k < ST_N; ++k) st.v[k] -= g_equip[cur].stats.v[k];
	if (e >= 0) for (int k = 0; k < ST_N; ++k) st.v[k] += g_equip[e].stats.v[k];
	return st;
}

static void auto_equip(int m) {
	for (int s = 0; s < SL_N; ++s) {
		int c[40];
		int n = candidates(m, s, c, 40);
		if (!n) continue;
		int cur = equipped(m, s), best = -1;
		for (int i = 0; i < n; ++i) if (g_equip[c[i]].lvl <= G.level) { best = c[i]; break; }
		if (best >= 0 && (cur < 0 || equip_score(best) > equip_score(cur))) gs_equip(m, s, best);
	}
	sfx("defend", -8, 1);
}

static int rarity_ok(int e) { return g_equip[e].rarity; }

static void equip_ui(void) {
	int mi = 0;
	int mode = 0, slot = 0, sel = 0;
	int cands[40], nc = 0;
	for (;;) {
		int m = G.party[mi];
		mspr_clear();
		ui_clear();
		ui_panel(NP_PANEL, PAL_UI_LIGHT, 0, 0, 30, 20);
		ui_text_center(0, 4, 240, sfmt("<  %s · %s  >", member_name(m), member_role(m)), UC_LIGHT_TITLE, -1);
		ui_panel(NP_PANEL_DARK, PAL_UI_DARK, 1, 2, 6, 4);
		mspr_member(16, 18, m, 0);
		const char* desc = "";
		if (mode == 0) {
			equip_stats(m, 0);
			for (int s = 0; s < 6; ++s) {
				int y = 2 + s * 2;
				if (s < 5) {
					int e = equipped(m, s);
					const char* label = sfmt("%s:  %s", g_slot_names[s], e >= 0 ? g_equip[e].name : "—");
					ui_button(11, y, 18, 2, label, sel == s ? 1 : 0, 1, e >= 0 ? "x" : 0);
					if (e >= 0) mspr_icon(11 * 8 + 4, y * 8, equip_icon(e));
					if (sel == s) desc = sfmt("%s\nIzq./Der.: cambiar de personaje.", e >= 0 ? item_desc(g_equip[e].id) : "Hueco vacío.");
				} else {
					ui_button(11, y, 18, 2, "Equipar lo mejor", sel == 5 ? 1 : 0, 1, 0);
					if (sel == 5) desc = "Equipa automáticamente lo que más estadísticas dé.";
				}
			}
		} else {
			nc = candidates(m, slot, cands, 40);
			int shown = nc > 5 ? 5 : nc;
			Stats7 pv;
			if (sel == 0) { pv = preview_stats(m, slot, -1); desc = "Deja el hueco vacío."; }
			else { pv = preview_stats(m, slot, cands[sel - 1]); desc = item_desc(g_equip[cands[sel - 1]].id); }
			equip_stats(m, &pv);
			ui_button(11, 2, 18, 2, "(Quitar)", sel == 0 ? 1 : 0, 1, 0);
			for (int i = 0; i < shown; ++i) {
				int e = cands[i];
				int need = g_equip[e].lvl;
				const char* lvl = G.level >= need ? "" : sfmt("  (Nv %d)", need);
				int state = G.level < need ? 2 : (sel == i + 1 ? 1 : 0);
				ui_button(11, 4 + i * 2, 18, 2, sfmt("%s ×%d%s", g_equip[e].name, item_count(g_equip[e].id), lvl), state, 1, "x");
				mspr_icon(11 * 8 + 4, (4 + i * 2) * 8, equip_icon(e));
				(void)rarity_ok;
			}
			if (nc > 5 && sel == 0) desc = sfmt("(Se muestran las 5 mejores piezas de %d.)", nc);
			if (!nc) desc = sfmt("No tienes nada que %s pueda llevar ahí. Busca en tiendas y cofres.", member_name(m));
		}
		ui_text_wrapped(88, 116, 146, 9, desc, UC_LIGHT_HINT, -1, 4);
		for (;;) {
			mwait();
			int redraw = 0;
			int max = mode == 0 ? 6 : 1 + (nc > 5 ? 5 : nc);
			if (key_rep(KEY_UP)) { sel = (sel + max - 1) % max; sfx("select", -16, 1); redraw = 1; }
			else if (key_rep(KEY_DOWN)) { sel = (sel + 1) % max; sfx("select", -16, 1); redraw = 1; }
			else if (mode == 0 && G.nparty > 1 && (key_rep(KEY_LEFT) || key_rep(KEY_RIGHT))) {
				mi = (mi + (key_rep(KEY_LEFT) ? G.nparty - 1 : 1)) % G.nparty;
				sfx("select", -10, 1);
				redraw = 1;
			} else if (key_hit(KEY_B)) {
				sfx("cancel", -8, 1);
				if (mode == 1) { mode = 0; sel = slot; redraw = 1; }
				else { ui_clear(); return; }
			} else if (key_hit(KEY_A)) {
				if (mode == 0) {
					if (sel == 5) auto_equip(m);
					else { mode = 1; slot = sel; sel = 0; }
					redraw = 1;
				} else {
					if (sel == 0) { gs_equip(m, slot, -1); sfx("cancel", -8, 1); }
					else {
						int e = cands[sel - 1];
						if (G.level < g_equip[e].lvl) { sfx("cancel", -8, 1); continue; }
						gs_equip(m, slot, e);
						sfx("defend", -8, 1);
					}
					mode = 0; sel = slot;
					redraw = 1;
				}
			}
			if (redraw) break;
		}
	}
	(void)SLOT_ORDER_NAMES;
}

// ------------------------------------------------------------------ talentos (pantalla completa)
static void talent_ui(void) {
	int c = G.cls;
	ui_clear();
	mspr_clear();
	ui_panel(NP_PANEL, PAL_UI_LIGHT, 0, 0, 30, 20);
	if (c < 0) {
		ui_text(8, 6, "Talentos", UC_LIGHT_TITLE, -1);
		ui_text_wrapped(8, 30, 224, 11, "Al llegar al nivel 2 elegirás tu senda y tu clase. Entonces podrás gastar aquí tus puntos de talento.",
			UC_LIGHT_TEXT, -1, 6);
		for (;;) { mwait(); if (key_hit(KEY_B) || key_hit(KEY_A)) break; }
		ui_clear();
		return;
	}
	const ClassDef* cl = &g_classes[c];
	// orden de la cuadrícula: filas = índice dentro de cada nivel, columnas = nivel
	int grid[3][3];
	int cnt[4] = {0, 0, 0, 0};
	for (int r = 0; r < 3; ++r) for (int t = 0; t < 3; ++t) grid[r][t] = -1;
	for (int i = 0; i < 9; ++i) {
		int t = cl->tree[i].tier;
		if (t >= 1 && t <= 3 && cnt[t] < 3) grid[cnt[t]++][t - 1] = i;
	}
	int r = 0, col = 0;
	for (;;) {
		mspr_clear();
		ui_clear();
		ui_panel(NP_PANEL, PAL_UI_LIGHT, 0, 0, 30, 20);
		ui_text(8, 4, sfmt("Talentos · %s", cl->name), UC_LIGHT_TITLE, -1);
		ui_text_right(8, 4, 224, sfmt("Puntos: %d", skill_points()), UC_LIGHT_HI2, -1);
		static const char* const HDR[3] = {"Nivel I", "Nivel II (3 pts)", "Nivel III (6 pts)"};
		for (int t = 0; t < 3; ++t) ui_text(8 + t * 76, 16, HDR[t], UC_LIGHT_HINT, -1);
		for (int rr = 0; rr < 3; ++rr)
			for (int t = 0; t < 3; ++t) {
				int ni = grid[rr][t];
				int tx = 1 + t * 9 + (t ? 1 : 0) * 0, ty = 4 + rr * 4;
				tx = 1 + t * 10 - (t == 2 ? 1 : 0);
				if (ni < 0) { ui_button(tx, ty, 9, 4, "", 2, 0, 0); continue; }
				const TreeNode* n = &cl->tree[ni];
				int rank = G.tree[ni];
				const char* reason = talent_block_reason(ni);
				int sel = rr == r && t == col;
				ui_button(tx, ty, 9, 4, "", sel ? 1 : 0, 0, 0);
				int tc = rank > 0 ? UC_BTN_GOLD : (!reason[0] ? UC_BTN_TEXT : UC_BTN_DIS);
				ui_text(tx * 8 + 20, ty * 8 + 4, n->name, tc, UC_BTN_OUT);
				ui_text(tx * 8 + 20, ty * 8 + 16, sfmt("%d/%d", rank, n->max), tc, UC_BTN_OUT);
				const char* ic;
				if (n->skill >= 0) {
					static const char* const EL[8] = {"", "el_fire", "el_ice", "el_bolt", "el_light", "el_dark", "el_nature", "el_phys"};
					int el = g_skills[n->skill].el;
					ic = el ? EL[el] : "cmd_skill";
				} else ic = n->perk ? "st_rage" : "cmd_defend";
				mspr_icon(tx * 8 + 3, ty * 8 + 8, ic);
			}
		int ni = grid[r][col];
		if (ni >= 0) {
			const TreeNode* n = &cl->tree[ni];
			const char* txt;
			if (n->skill >= 0) {
				const SkillDef* sk = &g_skills[n->skill];
				txt = sfmt("%s — %s (%d PM)", sk->name, sk->desc, sk->mp);
			} else txt = sfmt("%s — %s", n->name, n->desc);
			const char* reason = talent_block_reason(ni);
			txt = reason[0] ? sfmt("%s\n[%s]", txt, reason) : sfmt("%s\nPulsa A para aprender.", txt);
			ui_text_wrapped(8, 118, 224, 10, txt, UC_LIGHT_TEXT, -1, 4);
		}
		for (;;) {
			mwait();
			int redraw = 1;
			if (key_rep(KEY_UP)) r = (r + 2) % 3;
			else if (key_rep(KEY_DOWN)) r = (r + 1) % 3;
			else if (key_rep(KEY_LEFT)) col = (col + 2) % 3;
			else if (key_rep(KEY_RIGHT)) col = (col + 1) % 3;
			else if (key_hit(KEY_B)) { sfx("cancel", -8, 1); ui_clear(); return; }
			else if (key_hit(KEY_A) && grid[r][col] >= 0) {
				if (learn_talent(grid[r][col])) sfx("levelup", -8, 1);
				else sfx("cancel", -8, 1);
			} else redraw = 0;
			if (redraw) { if (!key_hit(KEY_A)) sfx("select", -16, 1); break; }
		}
	}
}

// ------------------------------------------------------------------ menú de pausa
static const char* const TABS[8] = {"Grupo", "Objetos", "Equipo", "Talentos", "Guardar", "Opciones", "Continuar", "Título"};

static void draw_side(int sel) {
	ui_panel(NP_PANEL, PAL_UI_LIGHT, 0, 0, 8, 16);
	for (int i = 0; i < 8; ++i) ui_button(0, i * 2, 8, 2, TABS[i], i == sel ? 1 : 0, 0, 0);
	info_panel();
}

void pause_open(void) {
	g_paused = 1;
	sfx("select", -8, 1);
	dim_set(9);
	ui_clear();
	int sel = 0;
	int opened = g_frame;
	draw_side(sel);
	show_party();
	for (;;) {
		mwait();
		if (g_frame <= (u32)opened + 1) continue;
		int redraw = 0;
		if (key_rep(KEY_UP)) { sel = (sel + 7) % 8; sfx("select", -16, 1); redraw = 1; }
		else if (key_rep(KEY_DOWN)) { sel = (sel + 1) % 8; sfx("select", -16, 1); redraw = 1; }
		else if (key_hit(KEY_START) || key_hit(KEY_B)) break;
		else if (key_hit(KEY_A)) {
			switch (sel) {
			case 0: show_party(); break;
			case 1: show_items(1); break;
			case 2: equip_ui(); break;
			case 3: talent_ui(); break;
			case 4:
				if (W.cutscene && difficulty() >= 2) {
					sfx("cancel", -6, 1);
					ui_panel(NP_PANEL, PAL_UI_LIGHT, 8, 0, 22, 8);
					ui_text_wrapped(72, 8, 160, 11, "Modo Difícil: no puedes guardar en un momento decisivo. Sí puedes usar objetos y cambiar el equipo.", UC_LIGHT_TEXT, -1, 4);
					for (;;) { mwait(); if (key_hit(KEY_A) || key_hit(KEY_B)) break; }
				} else {
					ui_clear();
					if (save_ui_run(1, ent_x(PLAYER), ent_y(PLAYER))) {}
				}
				break;
			case 5: ui_clear(); options_ui(); break;
			case 6: goto close;
			case 7:
				ui_clear();
				menu_sprites_free();
				g_paused = 0;
				dim_set(0);
				extern u8 g_trans_busy;
				g_trans_busy = 1;
				fade_out(0.4f);
				scene_request(SC_TITLE);
				return;
			}
			ui_clear();
			draw_side(sel);
			if (sel == 1) show_items(0); else show_party();
			continue;
		}
		if (redraw) {
			draw_side(sel);
			if (sel == 1) show_items(0);
			else if (sel == 0) show_party();
		}
	}
close:
	ui_clear();
	mspr_clear();
	menu_sprites_free();
	dim_set(0);
	g_paused = 0;
	input_flush();
}

// ------------------------------------------------------------------ tienda
static int upgrade_owned(const char* id) {
	for (int i = 0; i < g_nupgrades; ++i)
		if (!strcmp(g_upgrades[i].id, id)) {
			int cur = g_upgrades[i].weapon ? G.weapon_tier : G.armor_tier;
			return cur >= g_upgrades[i].tier;
		}
	return 0;
}

static int item_price(const char* id) {
	int i = db_item(id);
	if (i >= 0) return g_items[i].price;
	i = db_equip(id);
	if (i >= 0) return g_equip[i].price;
	for (int k = 0; k < g_nupgrades; ++k) if (!strcmp(g_upgrades[k].id, id)) return g_upgrades[k].price;
	return 0;
}

static const char* item_icon(const char* id) {
	int i = db_item(id);
	if (i >= 0) return g_items[i].icon;
	i = db_equip(id);
	if (i >= 0) return equip_icon(i);
	return 0;
}

void shop_ui(const char* id) {
	const ShopDef* shop = 0;
	for (int i = 0; i < g_nshops; ++i) if (!strcmp(g_shops[i].id, id)) shop = &g_shops[i];
	if (!shop) return;
	dim_set(9);
	int n = shop->n;
	int sel = 0, scroll = 0;
	const char* msg = 0;
	for (;;) {
		mspr_clear();
		ui_clear();
		ui_panel(NP_PANEL, PAL_UI_LIGHT, 0, 0, 30, 20);
		ui_text(8, 4, shop->name, UC_LIGHT_TITLE, -1);
		ui_text_right(8, 4, 224, sfmt("%ld coronas", (long)G.gold), UC_LIGHT_HI2, -1);
		if (sel < scroll) scroll = sel;
		if (sel >= scroll + 6) scroll = sel - 5;
		for (int r = 0; r < 6 && scroll + r <= n; ++r) {
			int i = scroll + r;
			int y = 2 + r * 2;
			if (i == n) { ui_button(1, y, 12, 2, "Salir de la tienda", sel == n ? 1 : 0, 0, 0); continue; }
			const char* it = shop->stock[i];
			int owned = upgrade_owned(it);
			const char* label = owned ? sfmt("%s (ya lo tienes)", db_item_name(it)) : db_item_name(it);
			ui_button(1, y, 28, 2, "", owned ? 2 : (sel == i ? 1 : 0), 1, 0);
			ui_text(30, y * 8 + 3, label, owned ? UC_BTN_DIS : UC_BTN_TEXT, UC_BTN_OUT);
			if (!owned) ui_text_right(8, y * 8 + 3, 222, sfmt("%d cor.", item_price(it)), UC_BTN_GOLD, UC_BTN_OUT);
			const char* ic = item_icon(it);
			if (ic) mspr_icon(12, y * 8, ic);
		}
		// detalle del seleccionado: descripción, cuántos tienes y quién puede usarlo
		if (sel < n) {
			const char* it = shop->stock[sel];
			const char* extra = "";
			int e = db_equip(it);
			if (e >= 0) {
				char users[80];
				int len = 0;
				users[0] = 0;
				for (int i = 0; i < G.nparty; ++i)
					if (equip_allowed(equip_rule_key(G.party[i]), e))
						len += snprintf(users + len, sizeof users - len, "%s%s", len ? ", " : "", member_name(G.party[i]));
				extra = sfmt(" · %s", len ? users : "nadie puede usarlo");
			}
			const char* line1 = msg ? msg : sfmt("Tienes %d%s", item_count(it), extra);
			ui_text(8, 116, line1, UC_LIGHT_HI2, -1);
			ui_text_wrapped(8, 127, 224, 9, item_desc(it), UC_LIGHT_TEXT, -1, 3);
		}
		for (;;) {
			mwait();
			if (key_rep(KEY_UP)) { sel = (sel + n) % (n + 1); sfx("select", -16, 1); msg = 0; break; }
			if (key_rep(KEY_DOWN)) { sel = (sel + 1) % (n + 1); sfx("select", -16, 1); msg = 0; break; }
			if (key_hit(KEY_B)) goto out;
			if (key_hit(KEY_A)) {
				if (sel == n) goto out;
				const char* it = shop->stock[sel];
				if (upgrade_owned(it)) { sfx("cancel", -6, 1); break; }
				int price = item_price(it);
				if (G.gold < price) { sfx("cancel", -6, 1); msg = "No tienes suficientes coronas."; break; }
				G.gold -= price;
				int up = -1;
				for (int k = 0; k < g_nupgrades; ++k) if (!strcmp(g_upgrades[k].id, it)) up = k;
				if (up >= 0) {
					if (g_upgrades[up].weapon) G.weapon_tier = g_upgrades[up].tier; else G.armor_tier = g_upgrades[up].tier;
					msg = "¡Equipo mejorado!";
				} else { add_item(it, 1); msg = sfmt("Compras %s.", db_item_name(it)); }
				sfx("buy", -4, 1);
				break;
			}
		}
	}
out:
	ui_clear();
	mspr_clear();
	menu_sprites_free();
	dim_set(0);
	input_flush();
}
