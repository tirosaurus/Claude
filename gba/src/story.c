// Historia de la demo: Tortosa → Bosque Santo → el lobo. Escrita como funciones que bloquean
// (cada espera llama a frame_wait, que sigue moviendo y dibujando el mundo).
#include "game.h"
#include <string.h>

static int pending = -1;

// ---------------------------------------------------------------- utilidades
// Sustituye {name} y {o} (o/a según el personaje elegido).
static const char* fmt(const char* s) {
	static char out[400];
	char* d = out;
	while (*s && d < out + sizeof out - 20) {
		if (!strncmp(s, "{name}", 6)) { strcpy(d, g_name); d += strlen(g_name); s += 6; }
		else if (!strncmp(s, "{o}", 3)) { *d++ = FLAG(F_FEMALE) ? 'a' : 'o'; s += 3; }
		else *d++ = *s++;
	}
	*d = 0;
	return out;
}

static void say(const char* spk, const char* por, const char* msg) {
	text_say(spk, por, fmt(msg));
	while (text_busy()) frame_wait();
}
#define K(s) say("Kaelen", "kaelen", s)
#define Y(s) say("Yara", "yara", s)
#define M(s) say("Madre", "mother", s)
#define P(s) say(g_name, "player", s)
#define T(s) say(0, 0, s)

static int choose(const char* spk, const char* por, const char* q, const char* const* opts, int n) {
	text_say(spk, por, fmt(q));
	while (text_busy() && !text_done()) frame_wait();
	text_freeze();
	text_choice(opts, n);
	int r;
	while ((r = text_choice_result()) < 0) frame_wait();
	text_hide();
	return r;
}

static void wait(int frames) { while (frames-- > 0) frame_wait(); }

static void walk_to(Entity* e, int x, int y) {
	if (!e) return;
	e->tx = x; e->ty = y; e->moving = 1;
	while (e->moving) frame_wait();
}

static void begin(void) { g_cutscene = 1; }
static void end(void) { g_cutscene = 0; g_keys_prev = g_keys; }

static void join(int m, const char* id, int follow) {
	g_party[m].in_party = 1;
	Entity* e = ent_find(id);
	if (e) { e->follow = follow; e->moving = 0; }
	party_recalc();
}

// ---------------------------------------------------------------- llegada a mapas
void story_on_map(int map) {
	int x, y;
	switch (map) {
	case MAP_BEDROOM:
		if (!FLAG(F_WOKE_UP)) pending = map;
		break;
	case MAP_HOUSEMAIN:
		if (map_marker("mother", &x, &y)) ent_spawn("mother", &g_spr_mother, x, y, 0);
		if (!FLAG(F_MOTHER_TALKED)) pending = map;
		break;
	case MAP_VILLAGE:
		if (map_marker("bartolo", &x, &y)) ent_spawn("bartolo", &g_spr_bartolo, x, y, 0);
		if (map_marker("remei", &x, &y)) ent_spawn("remei", &g_spr_remei, x, y, 0);
		if (map_marker("roc", &x, &y)) ent_spawn("roc", &g_spr_roc, x, y, 0);
		if (map_marker("nil", &x, &y)) ent_spawn("nil", &g_spr_nil, x, y, 0);
		if (!FLAG(F_KAELEN_JOINED)) {
			if (map_marker("kaelen_start", &x, &y)) ent_spawn("kaelen", &g_spr_kaelen, x, y, 1);
			pending = map;
		}
		break;
	case MAP_FOREST:
		if (!FLAG(F_YARA_JOINED) && map_marker("yara", &x, &y)) ent_spawn("yara", &g_spr_yara, x, y, 0);
		if (FLAG(F_WON_WOLF) && !FLAG(F_DEMO_END)) pending = map;
		break;
	}
}

static void ev_wake(void) {
	begin();
	wait(30);
	T("La luz de la mañana se cuela entre las contraventanas y te da de lleno en la cara.");
	T("Desde abajo llega olor a pan recién hecho y el tintineo de los cacharros de tu madre.");
	T("Otro día más en Tortosa... o eso crees.");
	FLAG(F_WOKE_UP) = 1;
	end();
}

static void ev_mother(void) {
	begin();
	Entity* m = ent_find("mother");
	int x, y;
	if (m) { map_marker("from_upstairs", &x, &y); walk_to(m, x - 26, y); m->facing = 2; g_ent[0].facing = 1; }
	M("¡Mira quién se ha despertado! Ya pensaba que tendría que subir con el cubo de agua fría.");
	M("Hoy hace muy buen día, de esos que ya casi no quedan.");
	static const char* const o[] = {"Como un tronco, madre.", "He tenido un sueño muy raro...", "¿Eso es pan recién hecho?"};
	switch (choose("Madre", "mother", "¿Has descansado bien, cariño?", o, 3)) {
	case 0: M("Eso es que tienes la conciencia tranquila. Disfrútalo mientras dure."); break;
	case 1:
		M("¿Raro? ¿Raro cómo?");
		T("Recuerdas raíces negras creciendo bajo el bosque, latiendo como venas... y una voz que susurraba tu nombre.");
		P("Nada... tonterías. Raíces, oscuridad... ya se me está olvidando.");
		M("Bah, será la cena de anoche. Te dije que no repitieras queso.");
		T("Por un instante, a tu madre se le borra la sonrisa. Luego vuelve a remover el guiso como si nada.");
		break;
	case 2: M("De hogaza, con la harina del molino de los Ferrer. Coge un trozo, anda, que estás en los huesos."); break;
	}
	M("Por cierto, Kaelen ha pasado esta mañana preguntando por ti. Tres veces.");
	M("Anda, sal a que te dé el aire. Y volved antes de que anochezca, ¿eh? Últimamente el bosque... bueno, tú vuelve pronto.");
	if (m && map_marker("mother", &x, &y)) walk_to(m, x, y);
	FLAG(F_MOTHER_TALKED) = 1;
	end();
}

static void ev_kaelen(void) {
	begin();
	Entity* k = ent_find("kaelen");
	wait(20);
	K("¡{name}! ¡Eh, {name}! ¡Aquí!");
	if (k) {
		int px = g_ent[0].x >> 8, py = g_ent[0].y >> 8;
		walk_to(k, k->x >> 8, py);
		walk_to(k, px + 24, py);
		k->facing = 1; g_ent[0].facing = 2;
	}
	K("¡Por fin! Llevo media mañana tirando piedras al río para no morirme de aburrimiento.");
	K("En este pueblo nunca pasa nada. Nada. El viejo Bartolo dice que la Catedral está maldita, pero lleva cuarenta años diciendo lo mismo.");
	K("Yara se ha ido al Bosque Santo a por sus flores. Vamos a buscarla.");
	static const char* const o[] = {"¡Claro! Como en los viejos tiempos.", "¿Otra vez? Siempre acabas metiéndonos en líos.", "Vale, pero esta vez no te pierdas."};
	switch (choose("Kaelen", "kaelen", "¿Vienes o te vas a quedar ahí plantad{o}?", o, 3)) {
	case 0: K("¡Esa es la actitud! Sabía que no me fallarías."); break;
	case 1:
		K("Y siempre salimos de ellos, ¿no? Bueno... casi siempre.");
		K("No seas aguafiestas. Si pasa algo, yo te cubro. Como siempre.");
		T("Lo dice sonriendo, pero hay algo de pique en su voz.");
		break;
	case 2: K("¡Fue UNA vez! Y tenía diez años. Y la niebla era muy espesa."); K("...Y no se lo cuentes a Yara. Otra vez."); break;
	}
	K("El sendero del bosque sale por el este, pasado el cartel. ¡Vamos!");
	T("Kaelen se une al grupo.");
	FLAG(F_KAELEN_JOINED) = 1;
	join(M_KAELEN, "kaelen", 1);
	party_followers_spawn();
	end();
}

static void class_tutorial(void) {
	Y("Esperad. Si de verdad hay algo raro en el bosque, no quiero que vayamos a lo loco.");
	K("¿Otra de tus charlas de «seguridad ante todo»?");
	Y("Sí. Y esta vez me vais a escuchar. {name}, ¿cómo sueles pelear tú? Y no me digas «a palos».");
	static const char* const o[] = {"Guerrero: primera línea, con acero.", "Mago: hechizos desde lejos.", "Clérigo: curar y proteger."};
	g_class = choose("Yara", "yara", "¿Qué se te da mejor?", o, 3);
	switch (g_class) {
	case 0: K("Toma, la espada de práctica de mi padre. Está mellada, pero corta."); break;
	case 1: Y("El bastón de roble de mi abuela. Dicen que canaliza bien la magia... o eso decía ella."); break;
	default: Y("Toma mi maza de pastora. Y este colgante: la luz del bosque te escuchará."); break;
	}
	FLAG(F_CLASS_CHOSEN) = 1;
	party_recalc();
	Y("Escuchad, que esto es importante:");
	Y("En un combate actuamos por turnos, el más rápido primero. Atacar no gasta nada; las habilidades, maná.");
	Y("El maná se recupera poco a poco en cada turno. No lo malgastéis al principio.");
	Y("Si curo, puedo elegir a quién. Y las pociones también sirven, que para eso las llevamos.");
	K("Vale, vale, profesora. ¿Podemos ir ya a buscar al ciervo?");
}

static void ev_meet_yara(void) {
	begin();
	Entity* y = ent_find("yara");
	if (!y) { end(); return; }
	walk_to(&g_ent[0], 236, g_ent[0].y >> 8);
	wait(12);
	y->facing = 1;
	Y("¡Os estaba oyendo discutir desde la otra punta del bosque! Asustáis hasta a los ciervos.");
	K("Nosotros no discutimos. Debatimos. Con pasión.");
	Y("Ya.");
	walk_to(&g_ent[0], g_ent[0].x >> 8, (y->y >> 8) + 2);
	walk_to(&g_ent[0], (y->x >> 8) - 22, (y->y >> 8) + 2);
	g_ent[0].facing = 2; y->facing = 1;
	Y("Mirad: flor de luna. Solo crece donde la tierra está sana y el pacto sigue en pie.");
	K("La curandera oficial de Tortosa. ¿Sabes qué sería interesante? Encontrar la guarida del ciervo blanco.");
	Y("El ciervo blanco es un cuento para niños, Kaelen.");
	K("Eso dijiste de la cueva de los murciélagos, y mira.");
	Y("¡Casi nos morimos en la cueva de los murciélagos!");
	static const char* const o[] = {"Vamos a explorar. Será divertido.", "Yara tiene razón. Quedémonos cerca.", "Exploremos, pero sin dejar el sendero."};
	switch (choose("Yara", "yara", "{name}, tú decides. ¿Le seguimos la corriente o nos quedamos aquí tranquilos?", o, 3)) {
	case 0: K("¡Por fin alguien con sangre en las venas!"); Y("Siempre le das la razón a él..."); T("Yara lo dice en broma. Casi."); break;
	case 1: K("Qué par de abuelos."); Y("Gracias, {name}. Alguien sensato en este grupo."); break;
	case 2: Y("Diplomátic{o}, como siempre."); K("Me vale. Pero el que vea el ciervo primero invita a sidra."); break;
	}
	T("Yara guarda las flores en su zurrón. De repente, se queda muy quieta.");
	Y("Esperad... ¿lo oís?");
	K("No oigo nada.");
	Y("Exacto. Ni un pájaro. Ni un grillo. El bosque nunca está tan callado.");
	T("Yara se une al grupo.");
	FLAG(F_YARA_JOINED) = 1;
	join(M_YARA, "yara", 2);
	class_tutorial();
	party_followers_spawn();
	end();
}

static const EnemyDef WOLF = {"Lobo alfa", 170, 13, 4, 3, 3, 10, 40, &g_spr_e_wolf, 2};

static void ev_wolf(void) {
	begin();
	wait(20);
	Y("¿Qué ha sido eso?");
	int px = g_ent[0].x >> 8, py = g_ent[0].y >> 8;
	int i = ent_spawn("wolf", &g_spr_villager_m, px + 120, py, 1);
	g_ent[i].visible = 0;     // el lobo grande solo cabe en el combate: aquí se oye entre la maleza
	wait(30);
	ent_remove("wolf");
	K("¿Un lobo? ¿Tan cerca del pueblo?");
	Y("Mirad su lomo... esas venas negras. Y los ojos. Eso no es normal.");
	K("Sea lo que sea, viene a por nosotros. ¡{name}, a mi lado!");
	Y("¡Yo os curo! ¡Elegid bien a quién!");
	FLAG(F_WOLF_SEEN) = 1;
	for (;;) {
		if (battle_run(&WOLF, 1, 1)) break;
		g_scene = SCENE_MAP;
		sprites_hide_all();
		map_load(MAP_FOREST, "party_vs_wolf");
		fade_in();
		T("Todo se vuelve negro... pero Yara te sacude del hombro. Aún respiras.");
		Y("¡Arriba! Lo intentamos otra vez. ¡Esta vez con cabeza!");
		for (int m = 0; m < M_COUNT; ++m) { g_party[m].hp = g_party[m].maxhp; g_party[m].mp = g_party[m].maxmp; }
	}
	FLAG(F_WON_WOLF) = 1;
	g_scene = SCENE_MAP;
	sprites_hide_all();
	map_load(MAP_FOREST, "party_vs_wolf");
	fade_in();
	end();
}

static void ev_post_wolf(void) {
	begin();
	g_ent[0].facing = 2;
	wait(40);
	K("Uf... ¿estáis todos bien?");
	T("El lobo ha huido entre la maleza, herido. En el suelo solo queda un charco espeso y negro.");
	Y("Esto no es sangre. Es... como savia. Savia podrida.");
	Y("Huele igual que la raíz muerta que vi ayer junto a la piedra santa.");
	K("¿Me estás diciendo que el bosque está... enfermando?");
	static const char* const o[] = {"No lo sé, pero lo averiguaremos juntos.", "Deberíamos volver ya. Esto nos viene grande.", "Si algo amenaza Tortosa, lo pararé."};
	switch (choose("Kaelen", "kaelen", "{name}... ¿tú qué crees que está pasando?", o, 3)) {
	case 0: K("Juntos. Como siempre."); Y("Como siempre."); break;
	case 1: Y("Es lo más sensato."); K("...Sí. Supongo que sí."); T("Kaelen mira hacia el sur del bosque un rato más de la cuenta."); break;
	case 2: K("¡Esa es la actitud! Cueste lo que cueste."); Y("No digas eso tan a la ligera, {name}..."); break;
	}
	FLAG(F_DEMO_END) = 1;
	end();
	g_scene = SCENE_END;
}

void story_run_pending(void) {
	int p = pending;
	pending = -1;
	switch (p) {
	case MAP_BEDROOM: ev_wake(); break;
	case MAP_HOUSEMAIN: ev_mother(); break;
	case MAP_VILLAGE: ev_kaelen(); break;
	case MAP_FOREST: ev_post_wolf(); break;
	}
}

// ---------------------------------------------------------------- zonas
void story_on_zone(const char* id) {
	if (!strcmp(id, "window")) { begin(); T("Desde la ventana se ve la plaza de Tortosa y la Catedral Vieja. Más allá, el verde oscuro del Bosque Santo."); end(); return; }
	if (!strcmp(id, "sword")) { begin(); T("Tu espada de madera de cuando eras pequeñ{o}. Tiene más muescas que filo."); end(); return; }
	if (!strcmp(id, "meet_yara")) { if (!FLAG(F_YARA_JOINED)) ev_meet_yara(); return; }
	if (!strcmp(id, "wolf_zone")) {
		if (FLAG(F_WON_WOLF)) return;
		if (!FLAG(F_YARA_JOINED)) {
			begin();
			g_ent[0].x -= 12 << 8;
			K("Eh, ¿adónde vas? Yara estará en el claro de las flores, junto a la piedra santa.");
			end();
			return;
		}
		ev_wolf();
	}
}

int story_exit_allowed(int to) {
	if (g_map == MAP_VILLAGE && to == MAP_FOREST && !FLAG(F_KAELEN_JOINED)) return 0;
	if (g_map == MAP_FOREST && to == MAP_VILLAGE && FLAG(F_KAELEN_JOINED) && !FLAG(F_WON_WOLF)) {
		begin(); K("¿Ya te rajas? Primero encontremos a Yara."); end();
		return 0;
	}
	return 1;
}

// ---------------------------------------------------------------- examinar / hablar
void story_on_interact(const char* id) {
	begin();
	if (!strcmp(id, "bed")) T("Ya has dormido bastante. ¡Arriba, que el día no espera!");
	else if (!strcmp(id, "nightstand")) T("Anoche te quedaste leyendo hasta tarde. Otra vez.");
	else if (!strcmp(id, "wardrobe")) T("Ropa, más ropa y un calcetín sin pareja desde hace dos inviernos.");
	else if (!strcmp(id, "chest")) {
		T("Una piedra con forma de corazón, una pluma de cuervo y un dibujo hecho por Yara:");
		T("vosotros tres delante de la Catedral Vieja, y debajo, en letras torcidas: «LOS GUARDIANES DE TORTOSA».");
		T("Sonríes sin querer.");
	}
	else if (!strcmp(id, "desk")) T("Un mapa de Vaelmoor. El resto del mundo parece enorme... y lleno de nombres que suenan a aventura.");
	else if (!strcmp(id, "fireplace")) T("El fuego crepita. Huele a leña de encina y a guiso.");
	else if (!strcmp(id, "kitchen_table")) {
		static u8 bread;
		if (!bread) { bread = 1; g_potions += 2; T("Comes algo de pan y encuentras 2 pociones junto a la hogaza. (Pociones ×2)"); }
		else T("Migas. Tu madre te mira con cara de «ni se te ocurra».");
	}
	else if (!strcmp(id, "cupboard")) T("Un libro de recetas viejísimo. En la última página: «Si el bosque calla, cerrad las puertas».");
	else if (!strcmp(id, "mother")) M(FLAG(F_KAELEN_JOINED) ? "Y dile a Yara que me guarde unas flores de luna, que me quedan pocas." : "¿Todavía aquí? Kaelen va a echar la puerta abajo.");
	else if (!strcmp(id, "cathedral")) T("La Catedral Vieja. La gran puerta está sellada por los escombros desde antes de que naciera nadie del pueblo.");
	else if (!strcmp(id, "well")) T("El pozo de la plaza. Dicen que si tiras una moneda, Remei te la cobra.");
	else if (!strcmp(id, "stall")) T("El puesto de fruta. Hoy solo quedan manzanas arrugadas.");
	else if (!strcmp(id, "sign_forest")) T("«Bosque Santo — Respetad el pacto».");
	else if (!strcmp(id, "anvil") || !strcmp(id, "smithy")) T("La herrería de Maese Roc. Huele a hierro y a carbón.");
	else if (!strcmp(id, "bartolo")) say("Bartolo", "bartolo", "La Catedral está maldita, ¿me oyes? Cuarenta años llevo diciéndolo y nadie me escucha.");
	else if (!strcmp(id, "remei")) say("Remei", "remei", "Si vas al bosque, vuelve con flores de luna. Y sin heridas, que las vendas no crecen en los árboles.");
	else if (!strcmp(id, "roc")) say("Maese Roc", "roc", "Los mercaderes del sur hablaban de pueblos vacíos. Bah. ...Aunque les compré un hacha, por si acaso.");
	else if (!strcmp(id, "nil")) say("Nil", "nil", "¡Kaelen dice que en el bosque hay un ciervo blanco! ¿Me traerás una foto? ...¿Qué es una foto?");
	else if (!strcmp(id, "shrine")) T("La piedra santa. Las runas brillan débilmente, como un corazón cansado.");
	else if (!strcmp(id, "old_well")) T("Un pozo viejo, seco y tapado con tablas podridas. Por alguna razón, huele a chocolate.");
	else if (!strncmp(id, "house", 5)) T("La puerta está cerrada.");
	end();
}

