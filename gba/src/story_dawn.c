// Guiones de Tortosa al alba y de la Catedral (port de village_dawn.gd y cathedral.gd)
#include "story.h"

// ------------------------------------------------------------------ Tortosa al alba
static int vd_no_enc(void) { return 1; }
static const char* vd_req(const char* to, const char* def) { return IS(to, "cathedral") && !flag("dawn_done") ? "dawn_done" : def; }
static const char* vd_blocked(const char* to, const char* def) {
	return IS(to, "cathedral") && !flag("dawn_done") ? "Ahora no. El pueblo te necesita aquí." : def;
}

static void marta(void) {
	sfx("hit", 0.0f, 0.5f);
	shake(5.0f, 0.6f);
	SAY(T("Un crujido enorme. La casa del sur, la de los Pujol, se viene abajo entre chispas."),
		T("«¡Marta! ¡Mi Marta sigue dentro!», grita una vecina."));
	pan_to(400, 470, 1.0f);
	dust(400, 450);
	SAY(K("{name}, no. Esa casa se está cayendo. Si entras, no sales. Y la Semilla no puede esperar."),
		K("Una vida contra la de todo el pueblo. Hay que ser práctico."),
		Y("¡Es una persona, Kaelen! ¡La conocemos desde críos! ¡Marta nos daba pan de higo a escondidas!"),
		T("Desde dentro se oye una voz débil: «¡Ayuda... por favor!»"));
	int i = CHOOSE(&T("Las llamas crecen. No hay tiempo para pensarlo dos veces."), "¡Voy a por ella!",
		"Kaelen tiene razón. La Semilla es lo primero.");
	release_camera(0.3f);
	if (i == 0) {
		setf("marta_saved");
		change_approval(M_YARA, 5);
		change_approval(M_KAELEN, 2);
		change_kaelen_rivalry(-2);
		fade_out(0.5f);
		sfx("hit", -4.0f, 0.7f);
		wait(0.6f);
		spawn_npc("marta", ent_x(PLAYER) + 18, ent_y(PLAYER) + 4, 1, MODE_IDLE, "villager_f");
		fade_in(0.5f);
		g_trans_busy = 0;
		SAY(T("Sales tosiendo, con la ropa humeante, cargando a una mujer joven. Detrás de ti, el techo se desploma."),
			N("Marta", "Gracias... gracias. Creía que... que era el final."),
			N("Marta", "Escuchad. Anoche, antes del ataque, vino al pueblo un hombre muy gordo con gorro de cocinero."),
			N("Marta", "Compró todo el chocolate de la tienda. TODO. Y con un cucharón enorme convirtió en chocolate al perro del molinero... y se lo comió."),
			N("Marta", "Luego fue al pozo viejo del Bosque Santo, el del claro del este, y tiró unos papeles dentro. Llorando."),
			N("Marta", "Decía: «¡Que nadie los lea jamás!». Si es lo que le da miedo... quizá os sirva."),
			K("...Vale. Ha merecido la pena. Pero la próxima vez que entres en una casa en llamas, avísame."),
			T("(Nuevo lugar: el pozo abandonado del claro del este, en el Bosque Santo. Queda de camino al bosque profundo.)"));
		remove_npc("marta");
	} else {
		setf("marta_lost");
		change_approval(M_YARA, -6);
		change_yara_corruption(5);
		change_approval(M_KAELEN, 2);
		SAY(T("Das la espalda a la casa. El techo se desploma con un rugido."),
			T("Nadie ve salir a Marta. Solo, entre las ruinas, una forma marrón y brillante... como una estatua de chocolate."),
			Y("¿Eso es... chocolate? ¿Qué clase de monstruo hace algo así?"), K("No lo sé. Y no quiero saberlo."));
	}
}

static void aftermath(void) {
	begin();
	show_banner("Tortosa, al alba");
	wait(0.5f);
	walk_party_to(360, 300, 70);
	ent_face(PLAYER, 3);
	SAY(T("El sol sale sobre un pueblo herido."),
		T("Media Tortosa está negra de hollín. Los vecinos apagan las últimas brasas en silencio."));
	const char* lost = 0;
	if (flag("bartolo_dead") && flag("nil_dead")) lost = "Bartolo y Nil";
	else if (flag("bartolo_dead")) lost = "Bartolo";
	else if (flag("nil_dead")) lost = "Nil";
	if (lost)
		SAY(T(sfmt("Al pie de la Catedral, alguien ha cubierto con una manta a %s.", lost)),
			T("Tu madre deja una flor de luna sobre la manta. Luego te abraza muy fuerte, sin decir nada."));
	if (flag("nil_dead")) {
		SAY(K("Fue culpa mía. Si hubiera ido yo a por el carro..."), Y("Kaelen..."));
		int i = CHOOSE(&K("..."), "No fue culpa tuya. Fue mía. Yo elegí.", "Nadie tiene la culpa. Solo esas cosas.",
			"Llorar no le devolverá.");
		if (i == 0) {
			change_kaelen_rivalry(-6);
			change_approval(M_YARA, 3);
			SAY(K("...No lo cargues tú solo, ¿vale? Lo cargamos entre los dos."));
		} else if (i == 1) {
			change_kaelen_rivalry(-2);
			SAY(Y("Tiene razón. Los Moronguls hicieron esto. Nosotros no."));
		} else {
			change_kaelen_rivalry(8);
			change_approval(M_YARA, -5);
			change_yara_corruption(4);
			SAY(K("...Qué frío te has vuelto, {name}."), T("Yara te mira como si no te reconociera."));
		}
	} else if (flag("bartolo_dead"))
		SAY(Y("Bartolo nos contaba cuentos a los tres, ¿os acordáis? Del ciervo blanco. Del Archimago."),
			K("Y nunca le creímos. Ni una sola vez."));
	if (flag("mother_wounded")) SAY(M("Solo es un rasguño. No me mires así."), T("Tiene el brazo vendado hasta el hombro."));
	const char* sage = flag("bartolo_alive") ? "bartolo" : "remei";
	SAY(N(sage, "La Semilla sola no basta. Hay que plantarla en el Corazón del Bosque, donde nació el pacto."),
		N(sage, "Y solo los elfos de la Corte de la Savia conocen el ritual. Tienen un campamento más allá de la piedra santa, en lo profundo."),
		K("¿Los elfos? ¿Existen de verdad?"),
		Y("Existen. Mi abuela comerciaba con ellos: hierbas por miel."),
		N(sage, "El bosque profundo está corrompido. Id preparados. Roc os venderá lo que podáis pagar."));
	int j = CHOOSE(&K("¿Qué hacemos, {name}?"), "Partimos ya. No hay tiempo que perder.", "Primero me despido de mi madre.",
		"¿Y si nos quedamos a proteger el pueblo?");
	if (j == 0) change_approval(M_KAELEN, 3);
	else if (j == 1) {
		change_approval(M_YARA, 3);
		SAY(M("Ven aquí."), T("Tu madre te coloca bien la capa, como cuando eras pequeñ{o}."),
			M("Toma. Las flores de luna que me quedaban. Y vuelve, {name}. Vuelve."), T("(Flor de luna ×2)"));
		add_item("flor_luna_item", 2);
	} else {
		change_kaelen_rivalry(4);
		SAY(K("¿Y esperar a la próxima noche? Volverán. Y serán más."),
			Y("Si no plantamos la Semilla, no habrá pueblo que proteger, {name}."), P("...Tenéis razón. Vamos."));
	}
	if (j != 1) {
		add_item("flor_luna_item", 1);
		SAY(M("Llevaos esto, al menos. (Flor de luna ×1)"));
	}
	marta();
	add_item("pocion", 2);
	SAY(T("Los vecinos os dan lo poco que tienen: dos pociones y muchos abrazos."),
		T("(El camino al bosque profundo sigue desde el este del Bosque Santo. Puedes comprar en la herrería de Roc.)"));
	setf("dawn_done");
	setf("act3_started");
	tod_set("day");
	end();
}

static void vd_ready(void) {
	remove_prop("rubble");
	set_prop_sprite("cathedral", "cathedral_open");
	spawn_npc("remei", 398, 290, 1, MODE_IDLE, 0);
	spawn_npc("roc", 214, 262, 1, MODE_IDLE, 0);
	spawn_npc("mother", 330, 300, 2, MODE_IDLE, 0);
	if (flag("nil_alive") || flag("chose_home")) {
		Ent* nil = spawn_npc("nil", 300, 330, 0, MODE_WANDER, 0);
		nil->wradius = 30;
	}
	if (flag("bartolo_alive")) spawn_npc("bartolo", 368, 250, 0, MODE_IDLE, 0);
	if (flag("dawn_done")) return;
	aftermath();
}

static void vd_on_npc(const char* id) {
	if (IS(id, "roc")) {
		begin();
		Ent* n = npc("roc");
		if (n) ent_face_towards(n, ent_x(PLAYER), ent_y(PLAYER));
		const char* line = flag("roc_wounded") ? "Tengo el brazo hecho polvo, pero aún puedo venderos cosas."
			: "Lo prometido: os vendo lo que necesitéis. Casi a precio de coste.";
		int i = CHOOSE(&N("roc", line), "Comprar", "Nada");
		end();
		if (i == 0) open_shop("roc");
		return;
	}
	base_on_npc(id);
}

static int vd_npc(const char* id, LineBuf* o) {
	if (IS(id, "mother")) LB(o, M("Ve con cuidado. Y come algo por el camino, que te conozco."));
	else if (IS(id, "remei"))
		CYCLE(o, "remei_d", OPT(N("remei", "Mi gato ha vuelto. Con la cola chamuscada, pero ha vuelto. Como vosotros.")),
			OPT(N("remei", "Los elfos no son malos, pero no les gustan las prisas. Ni los enanos.")));
	else if (IS(id, "nil")) {
		if (flag("chose_home") && !flag("nil_alive"))
			LB(o, N("nil", "Roc me sacó... Tiene el brazo muy mal. Por mi culpa."),
				N("nil", "¡Cuando sea mayor seré herrero y le haré un brazo de hierro!"));
		else LB(o, N("nil", "¡Me salvasteis! ¡Cuando sea mayor seré un Guardián como vosotros!"));
	} else if (IS(id, "bartolo")) LB(o, N("bartolo", "Fe, Memoria, Sacrificio. La última es la que más cuesta, muchachos. Recordadlo."));
	return o->n;
}

static int vd_lines(const char* id, LineBuf* o) {
	if (IS(id, "cathedral")) LB(o, T("La Catedral, abierta por primera vez en siglos. Parece más pequeña a la luz del día."));
	else if (IS(id, "sign_forest")) LB(o, T("«→ Bosque Santo.» Alguien ha añadido con carbón: «Volved pronto»."));
	return o->n;
}

const StoryDef story_village_dawn = {.map = "village_dawn", .encounters_disabled = vd_no_enc, .exit_requirement = vd_req,
	.exit_blocked_message = vd_blocked, .on_map_ready = vd_ready, .on_npc = vd_on_npc, .npc_lines = vd_npc, .interact_lines = vd_lines};

// ------------------------------------------------------------------ Catedral
static const char* const RUNE_ID[3] = {"rune_1", "rune_2", "rune_3"};
static const char* const RUNE_NAME[3] = {"Fe", "Memoria", "Sacrificio"};
static const char* const RUNE_L1[3] = {"La runa de la Fe se enciende con una luz azul.",
	"La runa de la Memoria despierta. Las paredes susurran.", "La runa del Sacrificio arde, fría y blanca."};
static const char* const RUNE_L2[3] = {
	"Grabado en la piedra: «Los primeros de Tortosa juraron guardar lo que la discípula enterró, aunque nadie recordara por qué.»",
	"Grabado: «El Archimago Velmoth rasgó el Velo buscando ser dios. Su discípula, Ysolde, plantó la Semilla para coser la herida.»",
	"Grabado: «Toda semilla pide tierra. Y la tierra, a veces, pide un corazón.»"};
static const char* const RUNE_WHERE[3] = {"Fe (pared izquierda)", "Memoria (pared derecha)", "Sacrificio (abajo a la izquierda)"};

static int rune_index(const char* id) {
	for (int i = 0; i < 3; ++i) if (IS(id, RUNE_ID[i])) return i;
	return -1;
}

static void cat_ready(void) {
	for (int i = 0; i < 3; ++i) if (flag(sfmt("lit_%s", RUNE_ID[i]))) set_prop_sprite(RUNE_ID[i], "rune_on");
	if (flag("runes_done")) remove_prop("crypt_rubble");
	if (!flag("cathedral_seen")) {
		setf("cathedral_seen");
		begin();
		wait(0.6f);
		SAY(T("Por dentro, la Catedral es aún más grande de lo que parecía."),
			T("Vidrieras de colores imposibles filtran la luz de las llamas. Huele a piedra húmeda, a incienso antiguo... y a savia."),
			K("Nunca habíamos entrado tan lejos. De críos no pasábamos de la puerta."),
			Y("Algo se mueve ahí dentro. Con cuidado."),
			T("(Busca las tres runas del suelo, que brillan en azul: junto a la pared izquierda, junto a la derecha y en la esquina de abajo a la izquierda. Examínalas con A. Hay cofres escondidos.)"));
		end();
	}
}

static void rune(int r) {
	begin();
	const char* fk = sfmt("lit_%s", RUNE_ID[r]);
	if (flag(fk)) {
		SAY(T(sfmt("La runa de %s brilla con calma.", RUNE_NAME[r])));
		end();
		return;
	}
	setf(fk);
	set_prop_sprite(RUNE_ID[r], "rune_on");
	sfx("magic", -4.0f, 1.0f);
	SAY(T(RUNE_L1[r]), T(RUNE_L2[r]));
	if (r == 2 && has(M_YARA)) SAY(Y("«La tierra pide un corazón»... No me gusta cómo suena eso."));
	int count = 0;
	for (int i = 0; i < 3; ++i) if (flag(sfmt("lit_%s", RUNE_ID[i]))) count++;
	if (count == 3) {
		setf("runes_done");
		sfx("growl", -4.0f, 0.5f);
		wait(0.4f);
		remove_prop("crypt_rubble");
		SAY(T("Las tres runas brillan a la vez. Un temblor recorre la Catedral."),
			T("En la esquina del fondo, los escombros se deslizan y dejan al descubierto unas escaleras que bajan a la oscuridad."));
	}
	end();
}

static void cat_interact(const char* id) {
	int r = rune_index(id);
	if (r >= 0) { rune(r); return; }
	base_on_interact(id);
}

static int cat_lines(const char* id, LineBuf* o) {
	if (IS(id, "crypt_rubble")) {
		char buf[160];
		int len = 0;
		buf[0] = 0;
		for (int i = 0; i < 3; ++i)
			if (!flag(sfmt("lit_%s", RUNE_ID[i]))) len += snprintf(buf + len, sizeof buf - len, "%s%s", len ? ", " : "", RUNE_WHERE[i]);
		LB(o, T("Escombros que tapan unas escaleras. Tienen tallado el símbolo de tres runas."), T(sfmt("Faltan por encender: %s.", buf)));
	} else if (IS(id, "altar"))
		LB(o, T("Un altar de piedra cubierto con un paño rojo que el tiempo no ha logrado pudrir."),
			T("En el centro hay una marca con forma de semilla, como si algo hubiera descansado aquí durante siglos."));
	else if (IS(id, "statue"))
		LB(o, T("La estatua de una mujer con una semilla entre las manos. La placa está casi borrada: «Y...olde»."),
			T("A la otra estatua, rota, le falta la cabeza. Quizá era el Archimago."));
	return o->n;
}

const StoryDef story_cathedral = {.map = "cathedral", .on_map_ready = cat_ready, .on_interact = cat_interact, .interact_lines = cat_lines};
