// Probador automático (solo con -DAUTOTEST=n): recorre la historia llamando a los mismos ganchos
// que usaría el jugador, contesta las elecciones con respuestas prefijadas, gana los combates al
// instante y escribe un registro en la SRAM (0x0E008000) para leerlo desde el emulador.
#ifdef AUTOTEST
#include "story.h"
#include <stdarg.h>

extern int story_is_busy(void);
extern void story_on_interact(const char* id);
extern void story_on_npc(const char* id);
extern void story_on_trigger(const char* id);

#define LOG_BASE ((volatile u8*)0x0E008000)
#define LOG_SIZE 0x7F00
static int log_pos = -1;

void at_log(const char* fmt, ...) {
	if (log_pos < 0) {   // continúa donde se quedó (el registro sobrevive a los cambios de escena)
		log_pos = 0;
		while (log_pos < LOG_SIZE && LOG_BASE[log_pos] && LOG_BASE[log_pos] != 0xFF) log_pos++;
	}
	char buf[160];
	va_list ap;
	va_start(ap, fmt);
	int n = vsnprintf(buf, sizeof buf - 1, fmt, ap);
	va_end(ap);
	if (n > (int)sizeof buf - 2) n = sizeof buf - 2;
	buf[n++] = '\n';
	for (int i = 0; i < n && log_pos < LOG_SIZE - 1; ++i) LOG_BASE[log_pos++] = buf[i];
	LOG_BASE[log_pos] = 0;
}

void at_reset_log(void) { LOG_BASE[0] = 0; log_pos = 0; }

// ------------------------------------------------------------------ respuestas
static u8 answers[32];
static int nans, ians;
static int at_step;
static int fails;

int at_answer(int n, const char* first) {
	int a = 0;
	if (ians < nans) a = answers[ians++];
	else at_log("  ? eleccion sin respuesta prefijada (%s) -> 0", first ? first : "");
	if (a >= n) { at_log("  ! respuesta %d fuera de rango (%d opciones)", a, n); a = 0; }
	at_log("  > elige %d: %s", a, first ? first : "");
	return a;
}

// ------------------------------------------------------------------ guiones
enum { OP_END, OP_GOTO, OP_INT, OP_NPC, OP_TRIG, OP_ANS, OP_EXPECT, OP_NOT, OP_SET, OP_LEAVE, OP_OVW, OP_ITEM, OP_LEGEND, OP_LOG };
typedef struct { u8 op; const char* a; const char* b; } Step;
#define GOTO(m, s) {OP_GOTO, m, s}
#define INT(id) {OP_INT, id, 0}
#define NPCS(id) {OP_NPC, id, 0}
#define TRIG(id) {OP_TRIG, id, 0}
#define ANS(s) {OP_ANS, s, 0}
#define EXPECT(f) {OP_EXPECT, f, 0}
#define NOT(f) {OP_NOT, f, 0}
#define SET(f) {OP_SET, f, 0}
#define LEAVE(m) {OP_LEAVE, m, 0}
#define OVW {OP_OVW, 0, 0}
#define ITEM(i) {OP_ITEM, i, 0}
#define LEGEND {OP_LEGEND, 0, 0}
#define LOGS(s) {OP_LOG, s, 0}
#define STOP {OP_END, 0, 0}

#if AUTOTEST == 1
// Partida completa por la ruta del Soberano: arma Overlord, tres ecos, Lágrimas, Nhal'Zur y Velo sellado.
static const Step SCRIPT[] = {
	LOGS("== Acto I: secreto del Soberano =="),
	EXPECT("woke_up"),
	INT("bed"), INT("wardrobe"), INT("bed"), INT("chest"), EXPECT("ov1"),
	ANS("2"), GOTO("housemain", "from_upstairs"), EXPECT("mother_talked"), EXPECT("ov2"),
	ANS("0"), GOTO("village", "home_door"), EXPECT("kaelen_joined"),
	INT("well"), INT("well"), INT("well"), EXPECT("ov3"),
	ANS("0,0,0"), GOTO("forest", "from_village"), TRIG("meet_yara"), EXPECT("yara_joined"),
	INT("shrine"), EXPECT("ov_done"), OVW,
	ANS("0,0"), TRIG("wolf_zone"), EXPECT("won_wolf"), EXPECT("post_wolf_done"), EXPECT("council_done"),
	LOGS("== Acto II: la noche =="),
	ANS("0"), GOTO("bedroom", "start"), INT("bed"), EXPECT("night_attack"), EXPECT("night_woke"),
	ANS("0"), GOTO("housemain", "from_upstairs"), EXPECT("mother_hidden"),
	ANS("2"), GOTO("village_night", "home_door"), EXPECT("won_night1"), EXPECT("chose_cath"), EXPECT("night_post"),
	EXPECT("brute_defeated"),
	GOTO("cathedral", "from_village"), INT("altar"), EXPECT("ov_e1"),
	INT("rune_1"), INT("rune_2"), INT("rune_3"), EXPECT("runes_done"),
	ANS("0,0,0,0,0"), GOTO("crypt", "from_cathedral"), TRIG("custodian_zone"), EXPECT("seed_taken"), EXPECT("seed_player"),
	EXPECT("dawn_done"), EXPECT("act3_started"), EXPECT("marta_saved"),
	LOGS("== Acto III: el bosque =="),
	GOTO("forest", "from_village"), INT("old_well"), EXPECT("curriculum_found"), ITEM("curriculum"),
	ANS("1"), GOTO("forest_deep", "from_forest"), TRIG("brom_zone"), EXPECT("won_brom_fight"), EXPECT("aelis_joined"),
	GOTO("elf_camp", "from_deep"), EXPECT("camp_arrived"), EXPECT("ritual_ok"),
	INT("sacred_tree"), EXPECT("ov_e2"),
	ANS("0,0,0,0,0"), INT("campfire"), EXPECT("camp_night"), EXPECT("act4_started"),
	LOGS("== Acto IV: las Tres Lágrimas =="),
	ANS("0"), GOTO("minas", "from_montes"), TRIG("worm_zone"), EXPECT("won_worm"), EXPECT("tear_stone"), EXPECT("dwarves_saved"),
	ANS("0"), GOTO("ruinas", "from_pantano"), INT("mirror1"), ANS("1"), INT("mirror2"), ANS("2"), INT("mirror3"),
	EXPECT("mirror1_lit"), EXPECT("mirror2_lit"), EXPECT("mirror3_lit"),
	ANS("0"), TRIG("empress_zone"), EXPECT("won_empress"), EXPECT("tear_crystal"),
	ANS("0"), GOTO("paso_sur", "from_pantano"), TRIG("kraag_zone"), EXPECT("won_kraag"), EXPECT("tear_blood"),
	GOTO("elf_camp", "from_deep"), EXPECT("ready_for_heart"),
	LOGS("== El Corazón =="),
	GOTO("heart", "from_camp"), EXPECT("heart_seen"),
	ANS("0"), TRIG("aleixolo_zone"), EXPECT("won_aleixolo"), EXPECT("aleixolo_done"), ITEM("tableta_aleixolo"),
	ANS("0"), TRIG("kaelen_zone"), EXPECT("kaelen_resolved"),
	ANS("0,1"), TRIG("final_zone"), EXPECT("ov_e3"), EXPECT("ov_ready"), EXPECT("won_final"), EXPECT("won_nhalzur"),
	EXPECT("ending_sealed"),
	STOP,
};
#else
// Finales: se empieza en el Corazón con el grupo listo; START_FLAGS y estos pasos fijan las decisiones.
#define HEART_PRE GOTO("heart", "from_camp"), SET("heart_seen"), SET("aleixolo_event"), SET("kaelen_event"), SET("kaelen_resolved")
#if AUTOTEST == 2      // purificar con la Lágrima de la Savia
static const Step SCRIPT[] = {SET("ritual_ok"), HEART_PRE, ANS("0,0"), TRIG("final_zone"), EXPECT("ending_purify"), STOP};
#elif AUTOTEST == 3    // aceptar el pacto de la Torre
static const Step SCRIPT[] = {HEART_PRE, ANS("1"), TRIG("final_zone"), EXPECT("pact_accepted"), STOP};
#elif AUTOTEST == 4    // absorber la Semilla
static const Step SCRIPT[] = {HEART_PRE, ANS("0,1"), TRIG("final_zone"), EXPECT("ending_king"), STOP};
#elif AUTOTEST == 5    // fundirse con la Semilla
static const Step SCRIPT[] = {HEART_PRE, ANS("0,2"), TRIG("final_zone"), EXPECT("ending_sacrifice"), STOP};
#elif AUTOTEST == 6    // sin Lágrima ni Yara: la tierra rechaza la Semilla y no das tu corazón
static const Step SCRIPT[] = {LEAVE("yara"), HEART_PRE, ANS("0,0,1"), TRIG("final_zone"), EXPECT("ending_withered"), STOP};
#elif AUTOTEST == 7    // ruta de Vael: todos los legendarios sin el arma Overlord
static const Step SCRIPT[] = {LEGEND, HEART_PRE, ANS("0,0"), TRIG("final_zone"), EXPECT("vael_route"), EXPECT("ending_vael"), STOP};
#elif AUTOTEST == 8    // Soberano: ecos completos y quedarse el trono
static const Step SCRIPT[] = {SET("ov_done"), SET("ov_e1"), SET("ov_e2"), HEART_PRE, ANS("0,0"), TRIG("final_zone"),
	EXPECT("ov_ready"), EXPECT("ending_sovereign"), STOP};
#elif AUTOTEST == 9    // final amargo: muertes en el camino
static const Step SCRIPT[] = {SET("ritual_ok"), SET("bartolo_dead"), SET("nil_dead"), SET("kaelen_dead"), LEAVE("kaelen"), HEART_PRE,
	ANS("0,0"), TRIG("final_zone"), EXPECT("ending_purify"), STOP};
#elif AUTOTEST == 10   // final muy bueno: juramento y romance
static const Step SCRIPT[] = {SET("ritual_ok"), SET("kaelen_oath"), SET("romance_yara"), HEART_PRE, ANS("0,0"), TRIG("final_zone"),
	EXPECT("ending_purify"), STOP};
#elif AUTOTEST == 11   // oscuridad: Yara devorada por Aleixolo
static const Step SCRIPT[] = {SET("romance_yara"), GOTO("heart", "from_camp"), SET("heart_seen"), TRIG("aleixolo_zone"),
	EXPECT("yara_eaten"), EXPECT("player_dark"), EXPECT("kaelen_corrupt"), ANS("0"), TRIG("kaelen_zone"), EXPECT("kaelen_resolved"),
	TRIG("final_zone"), EXPECT("ending_king"), STOP};
#endif
#endif

static int step_done(const Step* s) {
	switch (s->op) {
	case OP_EXPECT:
		if (flag(s->a)) at_log("  OK  %s", s->a);
		else { at_log("  FALLO falta %s (mapa %s)", s->a, G.current_map); fails++; }
		return 1;
	case OP_NOT:
		if (!flag(s->a)) at_log("  OK  no %s", s->a);
		else { at_log("  FALLO sobra %s", s->a); fails++; }
		return 1;
	case OP_SET: setf(s->a); return 1;
	case OP_LEAVE: {
		int m = member_from_id(s->a);
		leave_party(m);
		remove_npc(s->a);
		return 1;
	}
	case OP_ITEM:
		if (item_count(s->a) > 0) at_log("  OK  objeto %s", s->a);
		else { at_log("  FALLO sin objeto %s", s->a); fails++; }
		return 1;
	case OP_OVW: {
		int eq = equipped(M_PLAYER, SL_WEAPON);
		if (eq >= 0 && starts_with(g_equip[eq].id, "overlord_")) at_log("  OK  arma equipada %s", g_equip[eq].id);
		else { at_log("  FALLO arma Overlord no equipada"); fails++; }
		return 1;
	}
	case OP_LEGEND:
		for (int i = 0; i < g_nlegendaries; ++i) if (!owns(g_legendaries[i])) add_item(g_legendaries[i], 1);
		at_log("  legendarios que faltan: %d", missing_legendaries());
		return 1;
	case OP_LOG: at_log("%s", s->a); return 1;
	case OP_ANS: {
		nans = ians = 0;
		for (const char* p = s->a; *p; ++p) if (*p >= '0' && *p <= '9' && nans < 32) answers[nans++] = *p - '0';
		return 1;
	}
	}
	return 0;
}

static void wait_idle(void) {
	int idle = 0;
	for (int t = 0; t < 60 * 120; ++t) {
		frame_wait();
		if (!story_is_busy() && !g_trans_busy && !W.cutscene && !g_dialog_active) { if (++idle > 15) return; }
		else idle = 0;
	}
	at_log("  ATASCADO en el paso %d (mapa %s)", at_step, G.current_map);
	fails++;
}

static void driver(void* a) {
	(void)a;
	for (;;) {
		wait_idle();
		const Step* s = &SCRIPT[at_step];
		if (s->op == OP_END) {
			at_log("== FIN: %d fallo(s) ==", fails);
			at_log("#DONE");
			block_forever();
		}
		at_step++;
		if (step_done(s)) continue;
		switch (s->op) {
		case OP_GOTO:
			at_log("-> %s (%s)", s->a, s->b);
			go_to_map(s->a, s->b, 0);
			break;
		case OP_INT: at_log("  usar %s", s->a); story_call(story_on_interact, s->a); break;
		case OP_NPC: at_log("  hablar %s", s->a); story_call(story_on_npc, s->a); break;
		case OP_TRIG: at_log("  zona %s", s->a); story_call(story_on_trigger, s->a); break;
		}
	}
}

void at_world_ready(void) {
	static int started;
	if (!started) { started = 1; at_reset_log(); at_log("#AUTOTEST %d", AUTOTEST); }
	task_spawn(driver, 0, 0);
}

void at_battle(const char* id) {
	at_log("  combate %s: victoria", id);
	if (id && id[0]) setf(sfmt("won_%s", id));
}

// la escena del final no vuelve al mundo: se comprueban aquí los pasos que quedan
void at_ending(const char* id) {
	at_log("  FINAL: %s", id);
	for (;;) {
		const Step* s = &SCRIPT[at_step];
		if (s->op == OP_END) break;
		at_step++;
		if (!step_done(s)) at_log("  ! paso %d no se puede hacer tras el final", at_step - 1);
	}
	at_log("== FIN: %d fallo(s) ==", fails);
	at_log("#DONE");
}
#endif
