#include "story.h"
#include "ui.h"
#include <stdio.h>

extern void story_call(void (*fn)(const char*), const char* arg);

// ------------------------------------------------------------------ atajos
void begin(void) { W.cutscene = 1; }
void end(void) { W.cutscene = 0; W.cam_focus = 0; }

void join(int m) {
	join_party(m);
	make_follower(m);
}

void walk_party_to(int x, int y, float speed) { walk_to(PLAYER, x, y, speed, 0); }

void flag_add(const char* key, int d) { meta_set_i(key, meta_i(key, 0) + d); }

const char* give_items_list(const ChestItem* items, int n) {
	char buf[300];
	int len = 0;
	buf[0] = 0;
	for (int i = 0; i < n; ++i) {
		const char* part;
		if (!strcmp(items[i].item, "gold")) {
			G.gold += items[i].qty;
			part = sfmt("%d coronas", items[i].qty);
		} else {
			add_item(items[i].item, items[i].qty);
			part = sfmt("%s ×%d", db_item_name(items[i].item), items[i].qty);
		}
		len += snprintf(buf + len, sizeof buf - len, "%s%s", i ? ", " : "", part);
	}
	return sfmt("%s", buf);
}

// ------------------------------------------------------------------ combate
static void battle_flash(void) {
	sfx("battle_start", -2.0f, 1.0f);
	for (int i = 0; i < 3; ++i) {
		for (int k = 0; k <= 4; ++k) { fade_set(k * 14 / 4, 1); frame_wait(); }
		for (int k = 4; k >= 0; --k) { fade_set(k * 14 / 4, 1); frame_wait(); }
	}
	fade_set(0, 0);
	fade_out(0.35f);
}

void battle_flash_and_go(void) {
	battle_flash();
	scene_request(SC_BATTLE);
}

void battle_ex(const char* id, const char* const* enemies, int n, const char* bg, const char* music, int boss, int vael, int weaken) {
	memset(&g_enc, 0, sizeof g_enc);
	strncpy(g_enc.id, id ? id : "", 23);
	g_enc.n = n;
	for (int i = 0; i < n && i < 6; ++i) strncpy(g_enc.enemies[i], enemies[i], 19);
	strncpy(g_enc.bg, bg, 23);
	if (music && music[0]) strncpy(g_enc.music, music, 15);
	g_enc.boss = boss;
	g_enc.can_flee = !boss;
	g_enc.vael = vael;
	g_enc.weaken_aleixolo = weaken;
	strcpy(g_enc.ret_map, W.m->id);
	g_enc.ret_has_pos = 1;
	g_enc.ret_x = ent_x(PLAYER);
	g_enc.ret_y = ent_y(PLAYER);
	W.cutscene = 1;
	g_trans_busy = 1;
	battle_flash();
	scene_request(SC_BATTLE);
	block_forever();
}

void battle(const char* id, const char* const* enemies, int n, const char* bg, const char* music, int boss) {
	battle_ex(id, enemies, n, bg, music, boss, 0, 0);
}

// ------------------------------------------------------------------ descanso, guardado y tienda
extern void save_ui(int save_mode, int x, int y);
extern void shop_ui(const char* id);

// los menús se abren desde la tarea del guion: no puede pausarse a sí misma
static int menu_begin(void) { g_menu_open = 1; g_paused = 1; return task_set_pausable(0); }
static void menu_end(int old) { g_paused = 0; g_menu_open = 0; task_set_pausable(old); }

void open_save(void) {
	int o = menu_begin();
	save_ui(1, ent_x(PLAYER), ent_y(PLAYER));
	menu_end(o);
}

void rest_and_save(const char* title) {
	begin();
	int i = choose_text(title ? title : "Descansáis junto al fuego.", OPTS("Descansar y guardar", "Solo descansar", "Seguir"));
	if (i <= 1) {
		set_tint_wait(77, 77, 102, 0.5f);
		heal_all();
		sfx("heal", -4.0f, 1.0f);
		wait(0.6f);
		int r, g, b;
		map_modulate(&r, &g, &b);
		set_tint(r, g, b, 0.5f);
		SAY(T("El grupo recupera todas sus fuerzas."));
		if (i == 0) open_save();
	}
	end();
}

void open_shop(const char* id) {
	begin();
	int o = menu_begin();
	shop_ui(id);
	menu_end(o);
	end();
}

// ------------------------------------------------------------------ secretos del Soberano
void track_secret(const char* id) {
	// rastro de las últimas 6 interacciones "mapa:id", separado por '|'
	const char* old = meta_s("ov_seq", "");
	char items[6][40];
	int n = 0;
	const char* p = old;
	while (*p && n < 6) {
		const char* e = strchr(p, '|');
		int len = e ? e - p : (int)strlen(p);
		if (len > 39) len = 39;
		memcpy(items[n], p, len);
		items[n][len] = 0;
		n++;
		if (!e) break;
		p = e + 1;
	}
	if (n == 6) { for (int i = 0; i < 5; ++i) strcpy(items[i], items[i + 1]); n = 5; }
	snprintf(items[n], 40, "%s:%s", G.current_map, id);
	n++;
	char buf[260];
	int len = 0;
	buf[0] = 0;
	for (int i = 0; i < n; ++i) len += snprintf(buf + len, sizeof buf - len, "%s%s", i ? "|" : "", items[i]);
	meta_set_s("ov_seq", buf);
	if (!flag("ov1") && !flag("council_done") && n >= 4 && !strcmp(items[n - 4], "bedroom:bed") &&
		!strcmp(items[n - 3], "bedroom:wardrobe") && !strcmp(items[n - 2], "bedroom:bed") && !strcmp(items[n - 1], "bedroom:chest")) {
		setf("ov1");
		sfx("magic", -24.0f, 0.5f);
	} else if (flag("ov2") && !flag("ov3") && !flag("yara_joined") && n >= 3 && !strcmp(items[n - 3], "village:well") &&
		!strcmp(items[n - 2], "village:well") && !strcmp(items[n - 1], "village:well")) {
		setf("ov3");
		sfx("magic", -24.0f, 0.5f);
	}
}

void echo_lines(const Line* lines, int n) {
	begin();
	sfx("dark", -4.0f, 0.6f);
	set_tint_wait(154, 46, 64, 0.8f);
	say_lines(lines, n);
	int r, g, b;
	map_modulate(&r, &g, &b);
	set_tint_wait(r, g, b, 0.8f);
	end();
}

static int sovereign_echo(const char* id) {
	if (!flag("ov_done")) return 0;
	const char* map = G.current_map;
	if (!strcmp(map, "cathedral") && !strcmp(id, "altar") && !flag("ov_e1")) {
		setf("ov_e1");
		const Line l[] = {T("El arma del Soberano arde en tu mano. El altar se tiñe de rojo."),
			T("Ves a un hombre con una corona de cristal negro y a un monje con una túnica blanca, espalda contra espalda."),
			T("«Soy Vael, el Primer Soberano. Esta tierra lleva mi nombre, aunque nadie recuerda por qué.»"),
			T("«Bajo este altar, Ermengol y yo cerramos la puerta. Él puso la luz. Yo puse mi alma en el acero que ahora empuñas.»"),
			T("«Lo que encerramos no era un monstruo. Era un hambre. Se llama Nhal'Zur.»"),
			T("(Primer eco del Soberano: 1/3)")};
		echo_lines(l, ARRLEN(l));
		return 1;
	}
	if (!strcmp(map, "elf_camp") && !strcmp(id, "sacred_tree") && !flag("ov_e2")) {
		setf("ov_e2");
		const Line l[] = {T("Rozas el Árbol Sagrado con el arma. La corteza sangra luz roja."),
			T("«La druida Ysolde plantó la Semilla para que el bosque sujetara el Velo con sus raíces.»"),
			T("«Pero Nhal'Zur aprendió a hablar a través de ellas. La Madre Raíz es su voz. La Torre Negra, sus manos.»"),
			T("«Si la Madre Raíz cae, el Velo se abrirá del todo. Y él saldrá a comer.»"),
			T("«Por eso te elegí, heredero. Solo mi acero puede herir lo que no tiene cuerpo.»"),
			T("(Segundo eco del Soberano: 2/3)")};
		echo_lines(l, ARRLEN(l));
		return 1;
	}
	return 0;
}

void sovereign_reminder(void) {
	if (!flag("ov_done") || flag("ov_end_chosen") || (flag("ov_e1") && flag("ov_e2"))) return;
	const char* a = !flag("ov_e1") ? "el altar de la Catedral Vieja" : 0;
	const char* b = !flag("ov_e2") ? "el Árbol Sagrado del Claro de la Savia" : 0;
	const char* missing = a && b ? sfmt("%s y a %s", a, b) : (a ? a : b);
	begin();
	sfx("dark", -10.0f, 0.7f);
	SAY(T("El arma del Soberano vibra en tu mano. Una voz susurra:"),
		T(sfmt("«Aún no, heredero. Te faltan ecos. Llévame a %s antes de enfrentarte al Corazón.»", missing)),
		T("(Ruta del Soberano: si entras en el Corazón y vences a la Madre Raíz sin los dos primeros ecos, no verás a Nhal'Zur.)"));
	end();
}

// ------------------------------------------------------------------ piedras de retorno (Acto IV)
static const struct { const char* map; const char* flag; const char* name; } WAYSTONES[] = {
	{"minas", "tear_stone", "Minas de Khazgurim"}, {"ruinas", "tear_crystal", "Ruinas de Cristal"},
	{"paso_sur", "tear_blood", "Paso del Sur"},
};

void place_waystone(void) {
	if (has_prop("waystone")) return;
	const char* map = G.current_map;
	int show = 0;
	for (int i = 0; i < 3; ++i) if (!strcmp(map, WAYSTONES[i].map) && flag(WAYSTONES[i].flag)) show = 1;
	if (!strcmp(map, "elf_camp") && flag("act4_started")) show = 1;
	if (!strcmp(map, "village_dawn") && flag("act4_started")) show = 1;
	if (show) show_prop("waystone");
}

static void use_waystone(void) {
	begin();
	const char* map = G.current_map;
	const char* dests[8];
	const char* opts[8];
	int n = 0;
	if (strcmp(map, "elf_camp")) { dests[n] = "elf_camp"; opts[n++] = "Viajar a: Claro de la Savia"; }
	if (strcmp(map, "village_dawn")) { dests[n] = "village_dawn"; opts[n++] = "Viajar a: Tortosa"; }
	if (!strcmp(map, "elf_camp") || !strcmp(map, "village_dawn"))
		for (int i = 0; i < 3; ++i)
			if (flag(WAYSTONES[i].flag)) { dests[n] = WAYSTONES[i].map; opts[n++] = sfmt("Viajar a: %s", WAYSTONES[i].name); }
	opts[n] = "Quedarme aquí";
	Line pr = {0, "La piedra de retorno zumba con luz azul."};
	int i = choose_opts(opts, n + 1, &pr);
	end();
	if (i < n) {
		sfx("magic", -4.0f, 1.0f);
		go_to_map(dests[i], "from_waystone", 0);
	}
}

// ------------------------------------------------------------------ interacción por defecto
void base_open_chest(const char* id) {
	const char* fk = sfmt("opened_%s", id);
	if (flag(fk)) {
		begin();
		SAY(T("El cofre está vacío."));
		end();
		return;
	}
	setf(fk);
	sfx("chest", -4.0f, 1.0f);
	prop_dark(id);
	const ChestDef* c = 0;
	for (int i = 0; i < g_nchests; ++i) if (!strcmp(g_chests[i].id, id)) c = &g_chests[i];
	const char* txt = c ? give_items_list(c->items, c->n) : GIVE({"pocion", 1});
	begin();
	SAY(T(sfmt("Abres el cofre: %s.", txt)));
	end();
}

void base_on_interact(const char* id) {
	if (!strcmp(id, "waystone")) { use_waystone(); return; }
	track_secret(id);
	if (sovereign_echo(id)) return;
	if (starts_with(id, "chest_")) {
		if (W.story && W.story->open_chest) W.story->open_chest(id);
		else base_open_chest(id);
		return;
	}
	if (!strcmp(id, "campfire")) { rest_and_save(0); return; }
	LineBuf lb;
	lb.n = 0;
	if (W.story && W.story->interact_lines) W.story->interact_lines(id, &lb);
	if (!lb.n) return;
	begin();
	say_lines(lb.l, lb.n);
	end();
}

int companion_banter(const char* id, LineBuf* out) {
	int act = 1;
	if (flag("act2_started")) act = 2;
	if (flag("act3_started")) act = 3;
	if (flag("ready_for_heart")) act = 4;
	const char* key = sfmt("%s_%d", id, act);
	char k[24];
	strncpy(k, key, 23);
	k[23] = 0;
	if (!strcmp(k, "kaelen_1")) {
		CYCLE(out, k, OPT(K("¿Te acuerdas de cuando nos colamos en la Catedral y casi nos aplasta una viga? Mejor día de mi vida.")),
			OPT(K("Algún día me iré de Tortosa. A Puerto Ceniza, al Gremio de Aventureros. Y tú te vienes conmigo.")));
	} else if (!strcmp(k, "kaelen_2")) {
		CYCLE(out, k, OPT(K("No dejo de pensar en esas cosas... los siervos. Algunos llevaban ropa de pastor. Eran personas, {name}.")),
			OPT(K("Si hubiera sido más rápido... No. No pienso en eso. Ahora no.")));
	} else if (!strcmp(k, "kaelen_3")) {
		if (rivalry() >= 30) CYCLE(out, k, OPT(K("Tú decides, tú eliges, tú, tú, tú. ¿Te has dado cuenta de que ya nunca me preguntas?")));
		else CYCLE(out, k, OPT(K("Los elfos me miran como si fuera un tronco con patas. Me caen bien, supongo.")),
			OPT(K("Pase lo que pase ahí dentro, cúbreme el flanco izquierdo. Siempre se me olvida.")));
	} else if (!strcmp(k, "kaelen_4")) {
		if (rivalry() < 20) LB(out, K("Ya casi estamos. Oye, {name}... gracias por no dejarme atrás."));
		else LB(out, K("Terminemos con esto."));
	} else if (!strcmp(k, "yara_1")) {
		CYCLE(out, k, OPT(Y("La flor de luna cura casi cualquier cosa si sabes prepararla. Casi.")),
			OPT(Y("A veces siento el bosque, ¿sabes? Como un latido. Hoy late... más despacio.")));
	} else if (!strcmp(k, "yara_2")) {
		CYCLE(out, k, OPT(Y("He curado tantas heridas esta noche que me tiemblan las manos.")),
			OPT(Y("¿Por qué la Semilla me mira así? ...Olvídalo, digo tonterías.")));
	} else if (!strcmp(k, "yara_3") || !strcmp(k, "yara_4")) {
		if (yara_is_witch())
			CYCLE(out, k, OPT(Y("La oscuridad no es tan fría como dicen, {name}. Es... cálida. Poderosa.")),
				OPT(Y("Mis flores ya no brillan. Ahora mis manos sí.")));
		else if (flag("romance_yara"))
			CYCLE(out, k, OPT(Y("Cuando esto acabe, quiero ver el mar contigo. ¿Me lo prometes?")),
				OPT(Y("Deja de mirarme así, que me desconcentras.")));
		else
			CYCLE(out, k, OPT(Y("Los elfos saben tanto de plantas... Ilvanis me ha enseñado tres ungüentos nuevos.")),
				OPT(Y("Pase lo que pase, seguimos siendo los Guardianes de Tortosa. ¿Vale?")));
	} else if (!strcmp(k, "aelis_3") || !strcmp(k, "aelis_4")) {
		CYCLE(out, k, OPT(A("No confío en los humanos. Pero confío en mis flechas, y te han visto luchar.")),
			OPT(A("El Corazón fue lo primero que plantaron mis ancestros. Verlo podrido... duele.")));
	} else if (!strcmp(k, "brom_3") || !strcmp(k, "brom_4")) {
		CYCLE(out, k, OPT(B("¡Ja! Otro bosque, otra pelea. En Khazgurim al menos los túneles no te muerden.")),
			OPT(B("Esas larvas salen de abajo, lo sé. De donde cavamos demasiado hondo. Nuestra culpa, en parte.")));
	}
	return out->n;
}

void base_on_npc(const char* id) {
	Ent* n = npc(id);
	if (n && n->mode != MODE_FOLLOW) ent_face_towards(n, ent_x(PLAYER), ent_y(PLAYER));
	LineBuf lb;
	lb.n = 0;
	if (W.story && W.story->npc_lines) W.story->npc_lines(id, &lb);
	if (!lb.n) companion_banter(id, &lb);
	if (!lb.n) return;
	begin();
	say_lines(lb.l, lb.n);
	end();
}

// ------------------------------------------------------------------ despacho desde el mundo
void story_on_interact(const char* id) {
	if (W.story && W.story->on_interact) W.story->on_interact(id);
	else base_on_interact(id);
}
void story_on_npc(const char* id) {
	if (W.story && W.story->on_npc) W.story->on_npc(id);
	else base_on_npc(id);
}
void story_on_trigger(const char* id) { if (W.story && W.story->on_trigger) W.story->on_trigger(id); }
const char* story_exit_requirement(const char* to, const char* def) {
	return W.story && W.story->exit_requirement ? W.story->exit_requirement(to, def) : def;
}
const char* story_exit_blocked(const char* to, const char* def) {
	return W.story && W.story->exit_blocked_message ? W.story->exit_blocked_message(to, def) : def;
}
int story_before_exit(const char* to) { return W.story && W.story->before_exit ? W.story->before_exit(to) : 1; }
int story_encounters_disabled(void) { return W.story && W.story->encounters_disabled ? W.story->encounters_disabled() : 0; }
const char* story_music(void) {
	const char* o = W.story && W.story->music_override ? W.story->music_override() : "";
	return o && o[0] ? o : W.m->music;
}
static void map_ready_task(const char* a) {
	(void)a;
	if (W.story && W.story->on_map_ready) W.story->on_map_ready();
}
void story_start_map_ready(void) {
	extern void story_calls_reset(void);
	story_calls_reset();
	story_call(map_ready_task, "");
}

void goto_world(const char* map, const char* spawn, float fade) {
	strcpy(G.current_map, map);
	strcpy(G.current_spawn, spawn);
	G.has_load_pos = 0;
	g_trans_busy = 0;
	if (fade > 0) fade_out(fade);
	scene_request(SC_WORLD);
	block_forever();
}

void fade_npc(const char* id, float t) {
	Ent* e = npc(id);
	if (!e) return;
	ent_fade(e, 0, t);
	wait(t);
	remove_npc(id);
}

void tint_tween(Ent* e, int r, int g, int b, float t) {
	if (!e) return;
	int r0 = e->tr, g0 = e->tg, b0 = e->tb;
	int n = (int)(t * 60);
	if (n < 1) n = 1;
	for (int i = 1; i <= n; ++i) {
		ent_set_tint(e, r0 + (r - r0) * i / n, g0 + (g - g0) * i / n, b0 + (b - b0) * i / n);
		frame_wait();
	}
}

void go_ending(void) {
	music_stop(1.5f);
	wait(0.5f);
	g_trans_busy = 1;
	fade_out(1.4f);
	scene_request(SC_ENDING);
	block_forever();
}
