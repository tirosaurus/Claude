// Guiones del Pantano de Selen, las Ruinas de Cristal y el Paso del Sur (port de pantano.gd, ruinas.gd y paso_sur.gd)
#include "story.h"

static void waystone_lit(void) {
	sfx("magic", -4.0f, 1.0f);
	place_waystone();
	SAY(T("Junto a ti, una piedra rúnica se enciende con luz azul. (Piedra de retorno: te lleva al Claro de la Savia cuando quieras.)"));
}

// ------------------------------------------------------------------ Pantano de Selen
static void pan_ready(void) {
	if (!flag("pantano_seen")) {
		setf("pantano_seen");
		begin();
		show_banner("Pantano de Selen");
		wait(0.5f);
		SAY(T("Agua negra, árboles muertos y una niebla que se te mete en los huesos."),
			T("Aquí se alzaba el Imperio de Cristal. Hoy solo quedan juncos... y lo que se esconde entre ellos."),
			Y("Al oeste se ven brillos entre la niebla. Y al este... humo. Mucho humo."),
			T("(Oeste: Ruinas de Cristal. Este: Paso del Sur.)"));
		end();
	}
}

static int pan_lines(const char* id, LineBuf* o) {
	if (IS(id, "sign_pantano"))
		LB(o, T("Un poste podrido. Una flecha al oeste: «Selen». Otra al este, tachada con sangre: «Tortosa por el Paso»."));
	else if (IS(id, "selen_statue")) {
		if (flag("tear_crystal")) {
			if (flag("selen_freed")) LB(o, T("La estatua de Selen parece sonreír. Juncos nuevos crecen a sus pies."));
			else LB(o, T("La estatua de Selen se ha agrietado de arriba abajo."));
		} else
			LB(o, T("Una mujer encapuchada, con dos dagas cruzadas. Su rostro está erosionado, pero sus mejillas tienen surcos, como de llanto."),
				T("«Selen, la Sombra. Mató a su emperador para salvar a su pueblo. Y lloró hasta convertirse en cristal.»"));
	}
	return o->n;
}

const StoryDef story_pantano = {.map = "pantano", .on_map_ready = pan_ready, .interact_lines = pan_lines};

// ------------------------------------------------------------------ Ruinas de Cristal
static int mirrors_lit(void) { return flag("mirror1_lit") + flag("mirror2_lit") + flag("mirror3_lit"); }
static void light(int k) { set_prop_sprite(sfmt("mirror%d", k), "mirror_on"); }

static void after_empress(void) {
	begin();
	SAY(T("La Emperatriz se agrieta. De sus ojos cae una sola lágrima que se endurece en el aire: cristal puro."),
		N("selen", "Gracias... El hambre se ha ido. Pero sigo atada a este cristal."),
		N("selen", "Puedes romperme y quedarte lo que queda de mi poder. O dejarme ir. Ambas cosas cuestan algo."));
	int i = CHOOSE(0, "Liberar su alma", "Romper el cristal y absorber su poder");
	if (i == 0) {
		setf("selen_freed");
		SAY(T("Tocas el cristal con suavidad. Se deshace en polvo de luz que sube hacia el techo."),
			N("selen", "Dile a Ysolde, si la ves, que al final también yo lloré por el mundo."),
			Y("...Qué triste. Y qué bonito."));
		change_approval(M_YARA, 3);
		change_yara_corruption(-3);
	} else {
		setf("selen_shattered");
		SAY(T("Descargas el golpe. El cristal estalla y una corriente helada te entra por los brazos."),
			T("Te sientes más fuerte. Y, por un instante, oyes un susurro que no es de Selen."),
			T("(Todo el grupo gana +3 de magia de forma permanente.)"));
		change_yara_corruption(5);
		change_approval(M_KAELEN, 2);
	}
	SAY(T(sfmt("(Obtienes la Lágrima de Cristal. %d de 3.)", 1 + flag("tear_stone") + flag("tear_blood"))));
	setf("tear_crystal");
	waystone_lit();
	end();
}

static void rui_ready(void) {
	for (int k = 1; k <= 3; ++k) if (flag(sfmt("mirror%d_lit", k))) light(k);
	if (flag("won_empress") && !flag("tear_crystal")) { after_empress(); return; }
	if (!flag("ruinas_seen")) {
		setf("ruinas_seen");
		begin();
		wait(0.4f);
		SAY(T("Salas de cristal verde agua, columnas rotas y un silencio de catedral."),
			T("En las paredes, reflejos que no son los vuestros: una mujer con dos dagas camina por dentro del cristal."),
			N("selen", "...Tres espejos apagados. Tres verdades que nadie quiso oír. Enciéndelos y te dejaré pasar."),
			T("(Mazmorra: Ruinas de Cristal. Enciende los tres espejos.)"));
		end();
	}
}

static void mirror(int k) {
	begin();
	if (flag(sfmt("mirror%d_lit", k))) {
		SAY(T("El espejo brilla con luz propia."));
		end();
		return;
	}
	static const char* const Q[3] = {"«Mi emperador bebió de la savia negra para vivir para siempre. ¿Qué hice?»",
		"«Después, el pueblo me llamó traidora. ¿Qué sentí?»", "«¿Qué hace falta para cerrar una herida en el mundo?»"};
	static const char* const OP[3][3] = {{"Lo maté para salvar a mi pueblo.", "Huí y lo dejé reinar.", "Bebí con él."},
		{"Orgullo.", "Culpa, aunque había hecho lo correcto.", "Nada."}, {"Poder.", "Olvido.", "Un corazón dispuesto a romperse."}};
	SAY(N("selen", Q[k - 1]));
	int i = choose_opts(OP[k - 1], 3, 0);
	if (i == k - 1) {
		setf(sfmt("mirror%d_lit", k));
		sfx("magic", -6.0f, 1.0f);
		light(k);
		SAY(T(sfmt("El espejo se enciende con una luz blanca y fría. (Espejos: %d/3)", mirrors_lit())));
		if (mirrors_lit() == 3) SAY(N("selen", "...Sí. Esa es la verdad. Pasa. Pero te advierto: ya no soy solo yo la que habita el trono."));
	} else {
		sfx("cancel", -4.0f, 1.0f);
		SAY(N("selen", "No. Eso es lo que dijeron ellos. Piénsalo otra vez."));
	}
	end();
}

static void rui_interact(const char* id) {
	if (starts_with(id, "mirror")) { mirror(id[6] - '0'); return; }
	if (IS(id, "chest_ruina3") && !flag("won_empress")) {
		begin();
		SAY(T("Un sello de cristal cubre el cofre. La figura del trono te observa."));
		end();
		return;
	}
	base_on_interact(id);
}

static void rui_trigger(const char* id) {
	if (!IS(id, "empress_zone") || flag("won_empress")) return;
	if (mirrors_lit() < 3) {
		begin();
		SAY(T(sfmt("Una barrera de cristal cierra el paso al trono. Los espejos siguen apagados. (%d/3)", mirrors_lit())));
		ent_set_pos(PLAYER, ent_x(PLAYER), ent_y(PLAYER) + 14);
		end();
		return;
	}
	begin();
	music_stop(0.8f);
	SAY(T("En el trono, una figura de cristal abre los ojos. Tiene el rostro de Selen, pero la savia negra le corre por dentro como sangre."),
		N("selen", "Huid... Nhal'Zur me ha encontrado. Lleva siglos susurrándome. Y yo... ya no sé decirle que no."),
		T("La figura se levanta. Su voz se vuelve doble, fría: «INTRUSOS EN MI IMPERIO.»"));
	battle("empress", ENEMIES("emperatriz"), "crypt", "boss", 1);
}

static int rui_lines(const char* id, LineBuf* o) {
	if (IS(id, "empress_throne")) {
		if (flag("won_empress")) LB(o, T("Un trono de cristal vacío. En el respaldo, grabado: «Aquí lloró Selen»."));
		else LB(o, T("Un trono de cristal. Algo te observa desde dentro."));
	}
	return o->n;
}

const StoryDef story_ruinas = {.map = "ruinas", .on_map_ready = rui_ready, .on_interact = rui_interact, .on_trigger = rui_trigger,
	.interact_lines = rui_lines};

// ------------------------------------------------------------------ Paso del Sur
static const char* paso_music(void) { return flag("tear_blood") ? "forest" : ""; }

static void empty_cages(void) {
	for (int k = 1; k <= 3; ++k) set_prop_sprite(sfmt("cage%d", k), "cage_empty");
}

static void after_kraag(void) {
	begin();
	SAY(T("Kraag cae de rodillas. El martillo se le escapa de las manos."),
		T("La gema roja de su cuello brilla más fuerte... y los ojos del morongul se aclaran por un momento."),
		N("kraag", "...Aldric. Me llamaba Aldric. Defendí este paso hace cien años, antes de que la raíz me encontrara."),
		N("kraag", "Mátame y la Lágrima es tuya. O déjame vivir lo poco que me quede siendo yo."));
	int i = CHOOSE(0, "Perdonarle la vida", "Acabar con él");
	if (i == 0) {
		setf("kraag_spared");
		SAY(N("kraag", "...Gracias. Toma. Esta sangre es mía, la del campeón que fui. No la del monstruo."),
			T("Aldric se corta la palma y deja caer una gota sobre la gema. Se vuelve de un rojo limpio, casi dorado."),
			N("kraag", "Un aviso: la Torre Negra guía a la Madre. No sois los primeros héroes que intentan el Corazón."),
			T("Aldric se aleja cojeando hacia las montañas. Los soldados moronguls que quedan huyen al verle."),
			T("(Tu resistencia aumenta en +2 de forma permanente.)"));
		change_approval(M_YARA, 3);
		change_yara_corruption(-3);
	} else {
		setf("kraag_killed");
		SAY(T("Un solo golpe. El general no se defiende."), T("La Lágrima de Sangre se tiñe de un rojo oscuro, espeso."));
		if (has(M_KAELEN)) SAY(K("Un monstruo menos. No me mires así, Yara."));
		change_approval(M_KAELEN, 2);
		change_yara_corruption(3);
	}
	const char* got = GIVE({"pocion_mayor", 3}, {"gold", 300});
	char gotb[120];
	strncpy(gotb, got, 119);
	gotb[119] = 0;
	SAY(T("Abrís las jaulas."),
		flag("bartolo_dead") ? T("Los vecinos de Tortosa salen a trompicones, llorando y riendo a la vez.")
			: N("bartolo", "¡Chaval! ¡Sabía que alguien vendría! ¡Os invito a una ronda en cuanto volvamos a Tortosa!"),
		N("remei", "Dios te bendiga, criatura. Toma, lo que les robamos a esos brutos antes de que nos encerraran."),
		T(sfmt("(Recibes: %s.)", gotb)),
		T(sfmt("(Obtienes la Lágrima de Sangre. %d de 3.)", 1 + flag("tear_stone") + flag("tear_crystal"))));
	setf("tear_blood");
	waystone_lit();
	empty_cages();
	SAY(T("(Vuelve al Claro de la Savia cuando tengas las tres lágrimas.)"));
	end();
}

static void paso_ready(void) {
	if (flag("won_kraag") && !flag("tear_blood")) { after_kraag(); return; }
	if (!flag("won_kraag")) {
		// el campamento tiene que verse vivo: Kraag junto a la hoguera y guardias en la empalizada
		monster("kraag_04", marker_x("kraag"), marker_y("kraag"), 1);
		static const s16 P[4][2] = {{470, 215}, {470, 300}, {560, 120}, {560, 380}};
		for (int k = 0; k < 4; ++k) monster("soldado_042", P[k][0], P[k][1], 1);
	}
	if (flag("tear_blood")) empty_cages();
	if (!flag("paso_seen")) {
		setf("paso_seen");
		begin();
		show_banner("Paso del Sur");
		wait(0.5f);
		SAY(T("Tierra quemada. Estandartes rojos. Tambores de guerra que retumban en el pecho."),
			T("Un campamento morongul entero cierra el paso. Y en jaulas, junto a la hoguera... gente."),
			flag("bartolo_dead") ? Y("{name}... ¡Esa es la tía Remei! ¡Y más gente de Tortosa!")
				: Y("{name}... ¡Esos son Bartolo y la tía Remei! ¡De Tortosa!"),
			has(M_KAELEN) ? K("Se los llevaron la noche del ataque. Creía que estaban muertos.")
				: Y("Se los llevaron la noche del ataque... y nadie vino a buscarlos."),
			T("(El general Kraag está al este, junto a la hoguera grande, tras la empalizada.)"));
		end();
	}
}

static void paso_trigger(const char* id) {
	if (IS(id, "kraag_zone") && !flag("won_kraag")) {
		begin();
		music_stop(0.8f);
		pan_to(marker_x("kraag"), marker_y("kraag"), 1.0f);
		SAY(T("Junto a la hoguera, un morongul enorme con armadura negra y un martillo del tamaño de una puerta."),
			T("Colgada al cuello lleva una gema roja que late despacio: la Lágrima de Sangre."),
			N("kraag", "Humanos. Y con la peste de la Semilla encima. La Madre dijo que vendríais."),
			N("kraag", "Soy Kraag. Antes de la raíz fui un hombre. Un campeón de este paso. Ya no recuerdo mi nombre de entonces."),
			N("kraag", "Recuerdo pelear. Eso sí."));
		release_camera(0.3f);
		battle("kraag", ENEMIES("kraag", "soldado"), "heart", "boss", 1);
	}
}

static int paso_lines(const char* id, LineBuf* o) {
	if (starts_with(id, "cage")) {
		if (flag("tear_blood")) LB(o, T("Una jaula vacía. La puerta cuelga de un gozne."));
		else LB(o, T("Una jaula de hierro. Dentro, alguien de Tortosa te mira con los ojos muy abiertos: «¡Sácanos de aquí!»"));
	}
	return o->n;
}

const StoryDef story_paso_sur = {.map = "paso_sur", .music_override = paso_music, .on_map_ready = paso_ready, .on_trigger = paso_trigger,
	.interact_lines = paso_lines};
