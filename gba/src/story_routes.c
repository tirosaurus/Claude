// Rutas entre zonas y Abismo de los Susurros (port de route.gd, sus mapas y abyss.gd)
#include "story.h"
#include <stdio.h>

// ------------------------------------------------------------------ rutas
static void route_ready_lines(const char* map, const Line* lines, int n) {
	if (IS(map, "senda_corazon")) sovereign_reminder();
	const char* fk = sfmt("seen_%s", map);
	if (flag(fk)) return;
	setf(fk);
	if (!n) return;
	begin();
	wait(0.5f);
	say_lines(lines, n);
	end();
}
#define ROUTE_READY(...) do { const Line _l[] = {__VA_ARGS__}; route_ready_lines(G.current_map, _l, ARRLEN(_l)); } while (0)

static void cp_ready(void) {
	ROUTE_READY(T("El pantano se seca de golpe. Delante, tierra quemada, árboles negros y estandartes rojos clavados en el barro."),
		T("A lo lejos retumban tambores. Patrullas moronguls vigilan el camino."));
}
static int cp_lines(const char* id, LineBuf* o) {
	if (IS(id, "sign_route")) LB(o, T("Un poste tallado con runas moronguls. Debajo, en letra humana: «Tortosa resiste»."));
	return o->n;
}
const StoryDef story_camino_paso = {.map = "camino_paso", .on_map_ready = cp_ready, .interact_lines = cp_lines};

static void sc_ready(void) {
	ROUTE_READY(T("Ilvanis os acompaña hasta donde empieza la podredumbre. Luego, sin decir nada, se da la vuelta."),
		T("Los árboles se vuelven grises, después violetas. Las raíces se mueven cuando no las miras."),
		Y("Es como caminar dentro de algo vivo, {name}."),
		T("(Hay una hoguera a mitad de camino. Último lugar seguro antes del Corazón.)"));
}
static int sc_lines(const char* id, LineBuf* o) {
	if (IS(id, "sign_route")) LB(o, T("Una flecha élfica clavada en un tronco. Han tallado una sola palabra: «Volved»."));
	return o->n;
}
static int sc_enc(void) { return 0; }
const StoryDef story_senda_corazon = {.map = "senda_corazon", .on_map_ready = sc_ready, .interact_lines = sc_lines, .encounters_disabled = sc_enc};

static void sm_ready(void) {
	ROUTE_READY(T("El bosque élfico se abre poco a poco. Los robles dejan paso a pinos altos y el aire huele a nieve."),
		has(M_BROM) ? B("¡Eso es aire de montaña! Por fin algo que se puede respirar.") : Y("Hace frío... y se oyen cosas moviéndose entre las rocas."));
}
static int sm_lines(const char* id, LineBuf* o) {
	if (IS(id, "sign_route")) LB(o, T("«Oeste: Claro de la Savia. Este: Montes de Khazgurim. Cuidado con las salamandras: muerden y queman.»"));
	return o->n;
}
const StoryDef story_sendero_montes = {.map = "sendero_montes", .on_map_ready = sm_ready, .interact_lines = sm_lines};

static void sp_ready(void) {
	ROUTE_READY(T("El sendero baja y baja. Cada paso, el suelo es más blando y la luz más gris."),
		has(M_KAELEN) ? K("Si mis botas se llenan de barro, que conste que fue idea tuya.") : Y("Huele a agua quieta. No me gusta."));
}
static int sp_lines(const char* id, LineBuf* o) {
	if (IS(id, "sign_route")) LB(o, T("«Norte: Claro de la Savia. Sur: Pantano de Selen.» Alguien ha añadido debajo, con prisas: «No sigáis las luces»."));
	return o->n;
}
const StoryDef story_sendero_pantano = {.map = "sendero_pantano", .on_map_ready = sp_ready, .interact_lines = sp_lines};

// ------------------------------------------------------------------ Abismo de los Susurros
#define FLOORS 10
static const char* const VARIANTS[4] = {"abismo_a", "abismo_b", "abismo_c", "abismo_d"};
static const char* const VAEL_SET[4] = {"egida_vael", "yelmo_vael", "sello_vael", "manto_vael"};
static const u8 RARITY_W[3][5] = {   // pesos por rareza: [_, común, raro, muy raro, divino]
	{0, 5, 4, 1, 0}, {0, 0, 5, 3, 1}, {0, 0, 2, 4, 3}};

static int floor_n(void) { return meta_i("abyss_floor", 1); }

// lista "a,b,c" guardada en un metadato de texto
static int list_has(const char* list, const char* item) {
	int n = strlen(item);
	const char* p = list;
	while (*p) {
		const char* e = strchr(p, ',');
		int len = e ? e - p : (int)strlen(p);
		if (len == n && !strncmp(p, item, n)) return 1;
		if (!e) break;
		p = e + 1;
	}
	return 0;
}

static void reset_run(void) {
	meta_set_i("abyss_floor", 1);
	meta_set_s("abyss_opened", "");
	meta_set_i("abyss_banner_floor", 0);
	for (int k = 1; k <= FLOORS; ++k) meta_set_s(sfmt("abyss_chests_%d", k), "");
}

void abyss_enter(void) {
	reset_run();
	go_to_map(VARIANTS[rnd() % 4], "from_above", 1);
}

static void layout_chests(void) {
	const char* key = sfmt("abyss_chests_%d", floor_n());
	char keep[200];
	strncpy(keep, meta_s(key, ""), 199);
	keep[199] = 0;
	if (!keep[0]) {
		const char* ids[24];
		int n = 0;
		for (int i = 0; i < W.m->nprops && n < 24; ++i)
			if (W.m->props[i].id && starts_with(W.m->props[i].id, "chest_ab")) ids[n++] = W.m->props[i].id;
		for (int i = n - 1; i > 0; --i) { int j = rnd() % (i + 1); const char* t = ids[i]; ids[i] = ids[j]; ids[j] = t; }
		int cnt = randi(3, MIN(5, n));
		int len = 0;
		for (int i = 0; i < cnt && i < n; ++i) len += snprintf(keep + len, sizeof keep - len, "%s%s", i ? "," : "", ids[i]);
		meta_set_s(sfmt("abyss_chests_%d", floor_n()), keep);
	}
	const char* opened = meta_s("abyss_opened", "");
	for (int i = 0; i < W.m->nprops; ++i) {
		const char* pid = W.m->props[i].id;
		if (!pid || !starts_with(pid, "chest_ab")) continue;
		if (!list_has(keep, pid)) remove_prop(pid);
		else if (list_has(opened, sfmt("%d:%s", floor_n(), pid))) prop_dark(pid);
	}
}

static int random_equip(int f) {
	const u8* w = RARITY_W[CLAMP((f - 1) / 4, 0, 2)];
	int total = 0;
	for (int r = 0; r < 5; ++r) total += w[r];
	int roll = rnd() % total, rar = 0;
	for (int r = 0; r < 5; ++r) { roll -= w[r]; if (roll < 0) { rar = r; break; } }
	int pool[64], n = 0;
	for (int e = 0; e < N_EQUIP && n < 64; ++e) if (g_equip[e].rarity == rar) pool[n++] = e;
	return n ? pool[rnd() % n] : -1;
}

static int missing_list(const char** out) {
	int n = 0;
	for (int i = 0; i < g_nlegendaries; ++i) if (!owns(g_legendaries[i])) out[n++] = g_legendaries[i];
	return n;
}

static int is_legendary(const char* id) {
	for (int i = 0; i < g_nlegendaries; ++i) if (IS(id, g_legendaries[i])) return 1;
	return 0;
}

static void ab_open_chest(const char* id) {
	if (!starts_with(id, "chest_ab")) { base_open_chest(id); return; }
	char tag[40];
	snprintf(tag, sizeof tag, "%d:%s", floor_n(), id);
	char opened[400];
	strncpy(opened, meta_s("abyss_opened", ""), 399);
	opened[399] = 0;
	begin();
	if (list_has(opened, tag)) {
		SAY(T("El cofre está vacío."));
		end();
		return;
	}
	int len = strlen(opened);
	snprintf(opened + len, sizeof opened - len, "%s%s", len ? "," : "", tag);
	meta_set_s("abyss_opened", opened);
	sfx("chest", -4.0f, 1.0f);
	prop_dark(id);
	// botín
	int f = floor_n();
	ChestItem loot[4];
	int nl = 0;
	const char* miss[24];
	int nm = missing_list(miss);
	if (f >= 5 && nm && randf() < 0.035f * f) loot[nl++] = (ChestItem){miss[rnd() % nm], 1};
	else {
		if (randf() < 0.75f) {
			int e = random_equip(f);
			if (e >= 0) loot[nl++] = (ChestItem){g_equip[e].id, 1};
		}
		static const char* const CONS[5] = {"pocion_mayor", "eter", "pluma", "flor_luna_item", "bomba"};
		loot[nl++] = (ChestItem){CONS[rnd() % 5], randi(1, 2)};
		loot[nl++] = (ChestItem){"gold", randi(40, 90) * f};
	}
	const char* txt = give_items_list(loot, nl);
	SAY(T(sfmt("Abres el cofre: %s.", txt)));
	for (int i = 0; i < nl; ++i)
		if (is_legendary(loot[i].item)) {
			sfx("levelup", -2.0f, 0.8f);
			SAY(T(sfmt("¡Un objeto legendario! (%d de %d legendarios)", g_nlegendaries - missing_legendaries(), g_nlegendaries)));
		}
	end();
}

static void after_boss(void) {
	begin();
	meta_set_i("abyss_boss_pending", 0);
	const char* miss[24];
	int nm = missing_list(miss);
	const char* vm[4];
	int nv = 0;
	for (int i = 0; i < nm; ++i)
		for (int k = 0; k < 4; ++k) if (IS(miss[i], VAEL_SET[k])) vm[nv++] = miss[i];
	SAY(T("El Heraldo se arrodilla y se deshace en polvo de oro."), T("«El legado de Vael es tuyo... por ahora.»"));
	ChestItem loot[3] = {{"gold", 1500}, {"pocion_mayor", 2}};
	int nl = 2;
	if (nm) loot[nl++] = (ChestItem){nv ? vm[rnd() % nv] : miss[rnd() % nm], 1};
	const char* txt = give_items_list(loot, nl);
	SAY(T(sfmt("Recibes: %s.", txt)));
	nm = missing_legendaries();
	if (!nm) {
		sfx("levelup", 0.0f, 0.7f);
		SAY(T("Los catorce legendarios vibran a la vez. Por un instante, ves tu reflejo en el cristal... con una corona de luz."),
			flag("ov_done") ? T("(Tienes todos los objetos legendarios.)")
				: T("(Tienes todos los objetos legendarios. Algo ha cambiado en ti. Quizá al final del camino lo descubras.)"));
	} else SAY(T(sfmt("(Legendarios: %d de %d. Vuelve a bajar para buscar el resto.)", g_nlegendaries - nm, g_nlegendaries)));
	reset_run();
	end();
	go_to_map("cave", "from_abyss", 1);
}

static void ab_ready(void) {
	layout_chests();
	if (flag("won_abyss_boss") && meta_i("abyss_boss_pending", 0)) { after_boss(); return; }
	if (meta_i("abyss_banner_floor", 0) != floor_n()) {
		meta_set_i("abyss_banner_floor", floor_n());
		show_banner(sfmt("Abismo de los Susurros · Piso %d/%d", floor_n(), FLOORS));
		if (floor_n() == 1) {
			begin();
			wait(0.4f);
			SAY(T("Las escaleras bajan y bajan. El aire huele a hierro y a tormenta."),
				T("Los susurros de la cueva vienen de aquí abajo. Dicen tu nombre. Dicen otro nombre: «Vael»."),
				T("(Abismo de los Susurros: 10 pisos que cambian en cada visita. Cuanto más bajes, más duros los enemigos y mejor el botín."),
				T("Salir por donde has entrado te devuelve a la superficie, pero el descenso empezará de nuevo.)"));
			end();
		} else if (floor_n() == FLOORS) {
			begin();
			SAY(T("Último piso. Al fondo, junto a la escalera, algo dorado respira despacio."));
			end();
		}
	}
}

static void boss(const char* a) {
	(void)a;
	begin();
	music_stop(0.8f);
	SAY(T("Junto a la escalera se alza una figura de cristal dorado, con una corona rota sobre la frente."),
		T("«Soy el Heraldo de Vael. Guardo lo que el Primer Soberano dejó atrás antes de entregar su alma al acero.»"),
		T("«Demuéstrame que eres digno de su legado.»"));
	setf_v("won_abyss_boss", 0);
	meta_set_i("abyss_boss_pending", 1);
	battle("abyss_boss", ENEMIES("heraldo", "wisp", "wisp"), "crypt", "boss", 1);
}

static void leave_prompt(const char* a) {
	(void)a;
	begin();
	int i = CHOOSET("¿Abandonas el Abismo?", "Seguir explorando", "Volver a la superficie (el descenso se reinicia)");
	end();
	if (i == 1) {
		reset_run();
		go_to_map("cave", "from_abyss", 1);
	}
}

static void next_floor(const char* a) {
	(void)a;
	meta_set_i("abyss_floor", floor_n() + 1);
	meta_set_s(sfmt("abyss_chests_%d", floor_n()), "");
	go_to_map(VARIANTS[rnd() % 4], "from_above", 1);
}

static int ab_before_exit(const char* to) {
	if (IS(to, "abismo_next")) {
		if (floor_n() >= FLOORS) {
			ent_set_pos(PLAYER, ent_x(PLAYER), ent_y(PLAYER) + 14);
			story_call(boss, "");
			return 0;
		}
		story_call(next_floor, "");
		return 0;
	}
	if (IS(to, "cave")) {
		ent_set_pos(PLAYER, ent_x(PLAYER), ent_y(PLAYER) - 16);
		story_call(leave_prompt, "");
		return 0;
	}
	return 1;
}

#define ABYSS(name) {.map = name, .on_map_ready = ab_ready, .open_chest = ab_open_chest, .before_exit = ab_before_exit}
const StoryDef story_abismo_a = ABYSS("abismo_a");
const StoryDef story_abismo_b = ABYSS("abismo_b");
const StoryDef story_abismo_c = ABYSS("abismo_c");
const StoryDef story_abismo_d = ABYSS("abismo_d");
