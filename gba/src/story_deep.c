// Guiones de la cripta y el bosque profundo (port de crypt.gd y forest_deep.gd)
#include "story.h"

// ------------------------------------------------------------------ cripta
static const char* cr_music(void) { return "cathedral"; }
static int cr_no_enc(void) { return flag("seed_taken"); }

static void seed_scene(void) {
	begin();
	setf("seed_taken");
	wait(0.4f);
	walk_party_to(160, 440, 60);
	ent_face(PLAYER, 3);
	SAY(T("El Custodio se deshace en polvo brillante. El silencio vuelve a la cripta."),
		T("Sobre el pedestal flota una semilla de cristal del tamaño de un puño. Late. Respira."),
		T("Cuando te acercas, oyes una voz lejana dentro de tu cabeza: «...por fin...»"));
	Ent* em = spawn_npc("emissary", 260, 400, 1, MODE_IDLE, 0);
	em->alpha = 0;
	ent_fade(em, 16, 1.0f);
	sfx("dark", -2.0f, 1.0f);
	wait(1.0f);
	SAY(N("emissary", "Qué conmovedor. Tres niños de pueblo custodiando el mayor secreto del norte."),
		K("¿Quién eres tú? ¡Aléjate de ella!"),
		N("emissary", "Tranquilo, espadachín. No he venido a pelear. Solo a mirar quién la lleva."),
		N("emissary", "La Torre Negra se interesa por las cosas que duran. Y esa semilla ha durado trescientos años."),
		N("emissary", "Volveremos a hablar. Cuando la Madre Raíz os tenga contra las cuerdas... sabréis dónde encontrarme."));
	fade_npc("emissary", 0.8f);
	SAY(Y("¿La Madre Raíz?"), K("Ni idea. Pero no me ha gustado nada ese tipo."),
		Y("Alguien tiene que llevar la Semilla. Quema al tocarla... y susurra."));
	int i = CHOOSE(&K("¿Quién la lleva?"), "Yo la llevaré.", "Kaelen, llévala tú.", "Yara, tú la entiendes mejor que nadie.");
	if (i == 0) {
		setf("seed_player");
		SAY(T("Coges la Semilla. Está fría como el hielo y caliente como una brasa a la vez."),
			T("La voz vuelve, más clara: «...{name}...». Sabe tu nombre."), Y("¿Estás bien? Te has puesto pálid{o}."),
			P("Estoy bien. Vámonos."));
	} else if (i == 1) {
		setf("seed_kaelen");
		change_kaelen_rivalry(-4);
		change_approval(M_KAELEN, 6);
		SAY(K("¿Yo? ...Vale. Vale. No te fallaré."),
			T("Kaelen guarda la Semilla contra el pecho. Por un momento, sus ojos brillan con un reflejo violeta."),
			K("Susurra, es verdad. Dice que soy fuerte. Ja. Por fin alguien que lo nota."));
	} else {
		setf("seed_yara");
		change_approval(M_YARA, 5);
		change_yara_corruption(12);
		SAY(Y("Es... preciosa. Y está sufriendo. Puedo sentirlo."),
			T("Yara acuna la Semilla entre las manos. La luz azul se tiñe, apenas, de un matiz oscuro."),
			Y("Dice que puede curarlo todo. Que solo necesita... que le haga caso."));
	}
	setf("night_over");
	tod_set("dawn");
	heal_all();
	Line pr = has(M_KAELEN) ? K("La Semilla está a salvo. ¿Nos vamos?") : T("La Semilla está a salvo. ¿Qué hacéis?");
	int leave = choose_opts(OPTS("Volver a la superficie ahora.", "Explorar la cripta antes de salir."), &pr);
	if (leave == 1) {
		SAY(T("(Tómate tu tiempo: abre cofres y explora. Cuando quieras salir, sube por las escaleras del norte.)"));
		end();
		return;
	}
	SAY(T("Subís de nuevo a la Catedral. Fuera, el cielo empieza a clarear."));
	end();
	go_to_map("village_dawn", "cathedral_front", 0);
}

static void custodian(void) {
	begin();
	music_stop(0.6f);
	wait(0.3f);
	ent_face(PLAYER, 0);
	sfx("magic", 0.0f, 0.5f);
	SAY(T("Los cristales de la sala se encienden todos a la vez."),
		T("Del suelo, junto al pedestal, se levanta una figura enorme de piedra y cristal."),
		N("Custodio", "INTRUSOS. LA SEMILLA DUERME. NADIE LA DESPIERTA."),
		K("¡No somos ladrones! ¡Venimos a protegerla!"),
		N("Custodio", "TODOS DICEN LO MISMO."),
		Y("Tiene un núcleo de cristal en el pecho. Si le damos con rayos o luz..."));
	battle("custodian", ENEMIES("custodian"), "crypt", "boss", 1);
}

static void cr_ready(void) {
	if (flag("seed_taken")) { set_prop_sprite("seed", "seed_empty"); return; }
	if (flag("won_custodian")) { seed_scene(); return; }
	if (!flag("crypt_seen")) {
		setf("crypt_seen");
		begin();
		wait(0.5f);
		SAY(T("Las escaleras bajan hasta una cripta helada. Sarcófagos sin nombre flanquean el pasillo."),
			T("Al fondo, una luz azul late despacio. Como un corazón."), Y("Es ella. La Semilla. Puedo sentirla."));
		end();
	}
}

static void cr_trigger(const char* id) {
	if (IS(id, "custodian_zone")) { if (!flag("won_custodian")) custodian(); }
	else if (IS(id, "seed_zone")) { if (flag("won_custodian") && !flag("seed_taken")) seed_scene(); }
}

static int cr_lines(const char* id, LineBuf* o) {
	if (IS(id, "seed") && flag("seed_taken")) LB(o, T("El pedestal está vacío. Todavía se nota el calor de la Semilla en la piedra."));
	return o->n;
}

const StoryDef story_crypt = {.map = "crypt", .music_override = cr_music, .encounters_disabled = cr_no_enc, .on_map_ready = cr_ready,
	.on_trigger = cr_trigger, .interact_lines = cr_lines};

// ------------------------------------------------------------------ bosque profundo
static Ent* root_spr;

// Brom ya está peleando cuando llegas: se le ve (y se le oye) desde lejos.
static void brom_idle_fight(void* a) {
	(void)a;
	while (!flag("brom_seen")) {
		wait(randf_range(0.6f, 1.1f));
		Ent* th = npc("thrall");
		Ent* brom = npc("brom");
		if (!th || !brom || flag("brom_seen")) return;
		ent_hop(th);
		if (flag("brom_seen")) return;
		ent_hop(brom);
		int dx = ent_x(PLAYER) - ent_x(brom), dy = ent_y(PLAYER) - ent_y(brom);
		if (dx * dx + dy * dy < 260 * 260) sfx("hit", -18.0f, randf_range(0.9f, 1.2f));
	}
}

static void stage_brom(void) {
	int mx = marker_x("brom"), my = marker_y("brom");
	spawn_npc("brom", mx, my, 1, MODE_IDLE, 0);
	spawn_npc("thrall", mx - 30, my + 14, 2, MODE_IDLE, 0);
	root_spr = monster("root_08", mx + 34, my + 10, 1);
	task_spawn(brom_idle_fight, 0, TASK_PAUSABLE);
}

static void brom_scene(void) {
	begin();
	setf("brom_seen");
	Ent* brom = npc("brom");
	Ent* th = npc("thrall");
	if (!brom || !th) {
		stage_brom();
		brom = npc("brom");
		th = npc("thrall");
	}
	wait(0.25f);   // deja terminar el salto de la pelea de fondo
	pan_to(marker_x("brom"), marker_y("brom"), 0.9f);
	for (int k = 0; k < 3; ++k) {
		move_node(th, ent_x(th) + 18, ent_y(th) - 8, 0.14f, 1);
		sfx("hit", -6.0f, 1.0f);
		shake(2.0f, 0.2f);
		ent_hop(brom);
		move_node(th, ent_x(th) - 18, ent_y(th) + 8, 0.2f, 1);
		sfx("crit", -8.0f, 1.2f);
		dust(ent_x(th), ent_y(th));
	}
	SAY(B("¡Atrás, bichos del demonio! ¡Atrás o os parto la cabeza!"),
		T("Un enano pelirrojo, atrapado de cintura para abajo entre raíces negras, reparte hachazos a un siervo morongul."),
		T("Detrás de él, una raíz reptante se enrosca buscando su cuello."),
		K("¡Hay alguien ahí!"), Y("¡Van a matarlo!"));
	release_camera(0.3f);
	walk_party_to(marker_x("brom") - 60, marker_y("brom"), 80);
	SAY(B("¡¿Humanos?! ¡Pues no os quedéis mirando y echad una mano!"));
	battle("brom_fight", ENEMIES("thrall", "root", "larva"), "deep", "battle", 0);
}

static void elves_arrive(void) {
	begin();
	int mx = marker_x("brom"), my = marker_y("brom");
	spawn_npc("brom", mx, my, 1, MODE_IDLE, 0);
	SAY(B("¡Ja! ¡Así se hace! Brom, hijo de Thrain, de Khazgurim. Os debo una, humanos."),
		T("Antes de que podáis responder, tres flechas se clavan en el suelo, a un palmo de vuestros pies."));
	spawn_npc("aelis", mx + 40, my - 30, 1, MODE_IDLE, 0);
	spawn_npc("elf_guard", mx + 56, my, 1, MODE_IDLE, 0);
	SAY(A("Ni un paso más."),
		T("Una elfa de pelo claro os apunta con un arco. Detrás, un guardia élfico hace lo mismo."),
		A("Soy Aelis, de la Corte de la Savia. Ese enano taló árboles sagrados en la linde de nuestras tierras."),
		B("¡Talamos UN árbol! ¡Uno! Y estaba muerto, podrido por esa porquería negra."),
		N("elf_guard", "La ley es la ley. El enano viene con nosotros. La Anciana decidirá. Probablemente, la horca."),
		A("Y vosotros... ¿qué hacen unos humanos tan dentro del bosque?"));
	if (G.race == RACE_ELF)
		SAY(A("Un momento. Tú... tienes sangre de los nuestros. ¿De qué rama vienes, herman{o}?"),
			P("De ninguna que conozcas. Crecí en Tortosa."), A("Curioso. Muy curioso."));
	else if (G.race == RACE_DWARF)
		SAY(A("Y otr{o} enan{o}. Genial. ¿Venís en manada ahora?"), B("¡Eh! ¡Un paisano! ¡Por las barbas de mi abuelo!"));
	if (flag("seed_kaelen") || flag("seed_player") || flag("seed_yara"))
		SAY(Y("Llevamos la Semilla del Pacto. Necesitamos a vuestra Anciana."),
			T("Aelis baja el arco un palmo. Solo un palmo."), A("...La Semilla. Así que es cierto. Se ha despertado."));
	const char* opts[3] = {"Brom viene con nosotros. Nos ha ayudado.", "Lleváoslo. Vuestra ley, vuestras normas."};
	int n = 2;
	if (G.race == RACE_ELF) opts[n++] = "(Elf{o}) Como sangre de la Savia, respondo por él.";
	if (G.race == RACE_DWARF) opts[n++] = "(Enan{o}) Brom, cuéntales qué viste bajo las minas.";
	Line pr = A("¿Y bien? El enano. ¿Vais a interponeros?");
	int i = choose_opts(opts, n, &pr);
	setf("brom_event_done");
	if (i == 0) {
		setf("brom_joined");
		setf("elves_hostile");
		change_approval(M_KAELEN, 3);
		SAY(A("Como queráis. Pero la Anciana sabrá esto."), T("Aelis y el guardia se retiran entre los árboles."),
			B("¡Por mi hacha! Humanos con agallas. Mi hacha es vuestra hasta que la deuda esté pagada."), T("Brom se une al grupo."));
		remove_npc("aelis");
		remove_npc("elf_guard");
		join(M_BROM);
	} else if (i == 1) {
		setf("aelis_joined");
		setf("brom_prisoner");
		change_approval(M_YARA, -3);
		SAY(B("¡¿Qué?! ¡Traidores! ¡Os salvé el pellejo!"), T("El guardia ata a Brom y se lo lleva hacia el este."),
			A("Sabia decisión. La Anciana os recibirá. Y yo os acompañaré: la Semilla es asunto de mi pueblo."), T("Aelis se une al grupo."));
		remove_npc("brom");
		remove_npc("elf_guard");
		join(M_AELIS);
	} else {
		setf("brom_joined");
		setf("elves_friendly");
		SAY(A("...Está bien. Por esta vez."), A("Pero si el enano toca una sola rama, responderás tú."),
			G.race == RACE_ELF ? B("¡Ja! ¡Trato hecho!")
				: B("Bajo las minas de Khazgurim hay grietas que no deberían existir. De ahí salió la savia negra. Cavamos demasiado hondo."),
			A("...Eso cambia las cosas. La Anciana debe oírlo."), T("Brom se une al grupo. Aelis os guiará hasta el campamento."));
		remove_npc("aelis");
		remove_npc("elf_guard");
		join(M_BROM);
		change_approval(M_AELIS, 10);
	}
	end();
}

static void fd_ready(void) {
	root_spr = 0;
	if (flag("won_brom_fight") && !flag("brom_event_done")) { elves_arrive(); return; }
	if (!flag("won_brom_fight")) stage_brom();
	if (!flag("deep_seen")) {
		setf("deep_seen");
		begin();
		wait(0.5f);
		SAY(T("Más allá de la piedra santa, el Bosque Santo deja de ser santo."),
			T("Los árboles tienen la corteza violácea y blanda como carne. El suelo late bajo los pies."),
			Y("Duele... El bosque está sufriendo. Lo noto aquí, en el pecho."),
			K("Mantened los ojos abiertos. Aquí todo quiere comernos."));
		if (flag("seed_yara")) SAY(T("La Semilla, en las manos de Yara, brilla un poco más fuerte. Ella no parece notarlo."));
		else if (flag("seed_kaelen")) SAY(T("Kaelen se lleva la mano al pecho, donde guarda la Semilla, y sonríe sin motivo."));
		else if (flag("seed_player")) SAY(T("La Semilla te susurra al oído: «...aquí... más adentro... el Corazón...»"));
		end();
	}
}

static void fd_trigger(const char* id) {
	if (IS(id, "brom_zone") && !flag("won_brom_fight")) brom_scene();
}

const StoryDef story_forest_deep = {.map = "forest_deep", .on_map_ready = fd_ready, .on_trigger = fd_trigger};
