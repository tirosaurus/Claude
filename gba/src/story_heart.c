// Guion del Corazón del Bosque (port de heart.gd): Aleixolo, Kaelen, la Madre Raíz y las rutas del Soberano y de Vael
#include "story.h"

#define AL(s) N("Aleixolo", (s))
#define EM(s) N("emissary", (s))
#define KD(s) N("Kaelen", (s))

static const char* h_music(void) { return "heart"; }
static int h_no_enc(void) { return flag("final_started"); }

static Ent* spawn_emissary(int dx, int dy, int alpha16, float t) {
	Ent* em = spawn_npc("emissary", ent_x(PLAYER) + dx, ent_y(PLAYER) + dy, 2, MODE_IDLE, 0);
	em->alpha = 0;
	ent_fade(em, alpha16, t);
	wait(t);
	return em;
}

static void restore_tint(float t) {
	int r, g, b;
	map_modulate(&r, &g, &b);
	set_tint_wait(r, g, b, t);
}

// ------------------------------------------------------------ Aleixolo, el Glotón
static void kaelen_breaks(void) {
	Ent* kn = npc("kaelen");
	SAY(K("No... No, no, no. ¡YARA!"),
		T("Kaelen se lanza hacia donde estaba ella. Solo encuentra una mancha marrón en la tierra, todavía caliente."),
		T("Cae de rodillas. Araña el suelo con las manos hasta que le sangran los dedos."),
		K("La conocíamos desde los cinco años, {name}. Desde los CINCO."),
		K("Le tirábamos piedras al río. Nos curaba las rodillas. Nos regañaba por todo..."),
		K("Y tú estabas a su lado. Estabas a su lado y no hiciste NADA."),
		T("Kaelen grita. No es un grito de rabia: es el sonido de algo que se rompe por dentro y ya no se puede arreglar."));
	wait(0.6f);
	sfx("dark", -2.0f, 0.6f);
	SAY(T("Las raíces del suelo se mueven hacia él. Se le enroscan en los tobillos, en las muñecas, en el cuello."),
		T("Kaelen no las aparta. Las mira. Y las deja entrar."),
		T("La piel se le vuelve gris. Las venas, negras. Cuando levanta la cabeza, sus ojos son dos brasas violetas."));
	setf("kaelen_dark");
	setf("kaelen_corrupt");
	appearance_dirty();
	if (kn) {
		kn->up_frame = -1;
		kn->up_src = 0;
		shake(3.0f, 0.4f);
	}
	SAY(K("...Ya no duele."), K("Qué curioso. Ya no duele nada."),
		K("Vamos, {name}. Acabemos con esto. Y después... ya veremos qué queda en pie."),
		T("(Kaelen se ha corrompido.)"));
}

static void aleixolo_eats_yara(Ent* al) {
	Ent* yara = npc("yara");
	SAY(AL("Oh, oh, oh. ¿Y esta pelirroja tan dulce? Huele a flor de luna... ¡Postre!"), Y("{name}...!"));
	sfx("magic", -2.0f, 0.6f);
	if (yara) tint_tween(yara, 128, 77, 41, 0.7f);
	SAY(T("El rayo alcanza a Yara antes de que puedas moverte. Su mano, tendida hacia ti, se vuelve de chocolate."),
		T("Y luego todo lo demás."));
	if (yara) move_node(al, ent_x(yara) + 18, ent_y(yara) - 2, 0.5f, 1);
	sfx("crit", -2.0f, 0.5f);
	setf("yara_eaten");
	setf("yara_lost");
	G.comp[M_YARA].alive = 0;
	leave_party(M_YARA);
	remove_npc("yara");
	SAY(T("Ñam."), T("..."));
	set_tint_wait(90, 26, 64, 1.2f);
	sfx("dark", 0.0f, 0.5f);
	setf("player_dark");
	SAY(T("Algo se rompe dentro de ti."), T("No es tristeza. Es algo mucho más frío."),
		T("Una voz que no es la tuya te susurra que el mundo ya no merece ser salvado. Y tú... la escuchas."),
		T(sfmt("(Te has vuelto oscur%s. Ya no hay vuelta atrás.)", gfm("o", "a"))));
	if (has(M_KAELEN)) kaelen_breaks();
}

static void aleixolo(void) {
	setf("aleixolo_event");
	begin();
	int sx = marker_x("aleixolo"), sy = marker_y("aleixolo");
	pan_to(sx, sy, 0.9f);
	Ent* al = monster("aleixolo_05", sx, sy + 30, 0);
	Ent* elf = spawn_npc("prisoner", sx - 34, sy + 32, 2, MODE_IDLE, "elf_guard");
	SAY(T("En mitad del camino hay un hombre enorme, redondo como un tonel, con un gorro de cocinero manchado."),
		T("Lleva un cucharón gigante que gotea chocolate. A su lado, un elfo atado tiembla de miedo."));
	SAY(AL("¡Ay, qué hambre tengo! ¿Tú qué opinas, orejitas? ¿Elfo con leche o elfo negro?"));
	sfx("magic", -4.0f, 0.7f);
	tint_tween(elf, 128, 77, 41, 0.6f);
	SAY(T("Un rayo marrón sale del cucharón. El elfo se queda rígido, brillante... convertido en chocolate."));
	move_node(al, ent_x(elf) + 20, ent_y(elf) - 2, 0.6f, 1);
	sfx("crit", -4.0f, 0.6f);
	remove_npc("prisoner");
	for (int i = 1; i <= 24; ++i) { al->scale = 256 + 38 * i / 24; frame_wait(); }
	SAY(T("Ñam. Crunch. Ñam."), AL("¡Mmmm! Con avellanas. Cada bocado me hace más fuerte, ¿sabéis?"),
		AL("Soy Aleixolo, Gran Repostero de la Torre Negra. Nadie quiso contratarme... ¡pero la Torre sí! Y me dio este cucharón."),
		AL("Todo lo que toca se vuelve chocolate. Y todo el chocolate... ¡acaba aquí!"), T("Se da unas palmaditas en la barriga."));
	release_camera(0.3f);
	int weak = 0;
	if (item_count("curriculum") > 0) {
		int i = CHOOSE(&AL("¿Y vosotros? Tenéis cara de... ¡merienda!"), "Enseñarle su currículum.", "Luchar sin más.");
		if (i == 0) {
			weak = 1;
			remove_item("curriculum", 1);
			SAY(T("Sacas los papeles del pozo y se los plantas delante de la cara."),
				T("Aleixolo se queda blanco. Luego rojo. Luego empieza a temblarle el labio."),
				AL("¿D-de dónde habéis sacado eso? «Motivo del despido: comerse el género»... ¡No fue culpa mía! ¡Estaba muy rico!"),
				AL("Cuarenta cartas de rechazo... ¡CUARENTA! ¡Buaaaaaa!"),
				T("Aleixolo llora a moco tendido. Su cucharón tiembla y apenas brilla. Es el momento."));
		}
	}
	if (!weak && flag("romance_yara") && has(M_YARA)) aleixolo_eats_yara(al);
	else if (!weak) SAY(AL("¡A la mesa!"));
	end();
	battle_ex("aleixolo", ENEMIES("aleixolo"), "heart", "boss", 1, 0, weak);
}

static void after_aleixolo(void) {
	begin();
	setf("aleixolo_done");
	if (flag("player_dark")) {
		SAY(T("Aleixolo se derrite en un charco de chocolate humeante. No sientes nada."),
			T("Ni alegría, ni alivio. Solo hambre. Hambre de algo que no sabes nombrar."),
			flag("kaelen_corrupt") && has(M_KAELEN) ? K("¿Eso es todo? Esperaba que gritara más.") : T("Nadie se atreve a hablarte."));
	} else {
		SAY(T("Aleixolo cae de culo con un «¡plof!» y el cucharón se le parte en dos."),
			AL("¡Mi cucharón! Buaaa... Me voy a buscar trabajo honrado. En una panadería. O en dos."),
			T("Se aleja rodando entre las raíces, sollozando y mordisqueando una tableta."),
			T("Con las prisas, se le cae otra del bolsillo. Está intacta. Es un milagro."),
			T("(Obtienes: Tableta de Aleixolo. Cura toda la vida y 30 PM, pero te echas una siestecita de un turno.)"),
			K("¿Lo dejamos ir?"),
			has(M_YARA) ? Y("Creo que ya ha sufrido bastante. Cuarenta cartas, Kaelen.") : T("Nadie tiene valor para perseguirlo."));
		add_item("tableta_aleixolo", 1);
	}
	restore_tint(0.8f);
	end();
}

// ------------------------------------------------------------ Kaelen
static void kaelen_event(void) {
	setf("kaelen_event");
	begin();
	if (flag("kaelen_corrupt") && has(M_KAELEN)) {
		SAY(T("El suelo tiembla. De las raíces brotan siervos: diez, veinte... demasiados."),
			T("Kaelen sonríe. Es una sonrisa que no le habías visto nunca."),
			K("¿Contenerlos? No. Hoy no contengo a nadie."), K("Hoy los rompo a todos."));
		setf("fought_horde");
		battle("horde", ENEMIES("thrall", "brute", "thrall"), "heart", "boss", 1);
	}
	if (flag("kaelen_left")) {
		setf("kaelen_dark");
		spawn_npc("kaelen", ent_x(PLAYER), ent_y(PLAYER) - 60, 0, MODE_IDLE, "kaelen_dark");
		wait(0.3f);
		SAY(T("Una figura os espera en mitad del camino. Tiene la piel gris y los ojos violetas."),
			KD("Llegáis tarde. Como siempre llegaba yo, ¿verdad, {name}?"));
		if (flag("seed_stolen")) SAY(KD("La Semilla me lo explicó todo. La Madre Raíz no es el enemigo. Es poder. Y por una vez, el poder es mío."));
		else SAY(KD("La Madre Raíz me ha escuchado. Por primera vez alguien me escucha a mí."));
		if (has(M_YARA)) SAY(Y("Kaelen, por favor. Vuelve con nosotros. Esto no eres tú."));
		SAY(KD("Esto soy yo. Por fin. Y si queréis pasar, tendréis que tumbarme."));
		battle("kaelen_dark", ENEMIES("kaelen_dark"), "heart", "boss", 1);
	}
	SAY(T("El suelo tiembla. De las raíces brotan siervos: diez, veinte... demasiados."),
		K("Son demasiados. Si nos quedamos, nos rodean."),
		K("Idos. Yo los contengo en este paso. Es estrecho: puedo aguantar."));
	Line pr = has(M_YARA) ? Y("¡Kaelen, no!") : T("Kaelen te mira, esperando tu respuesta.");
	int i = choose_opts(OPTS("Ni hablar. Luchamos juntos.", "Confío en ti, Kaelen. Aguanta."), &pr);
	if (i == 0) {
		setf("fought_horde");
		change_kaelen_rivalry(-6);
		SAY(K("...Siempre tan cabezota. Vale. ¡Juntos!"));
		battle("horde", ENEMIES("thrall", "brute", "thrall"), "heart", "boss", 1);
	}
	setf("kaelen_held");
	if (has(M_BROM)) {
		setf("brom_helps_kaelen");
		SAY(B("¡Ja! ¿Solo? ¡Ni en sueños, muchacho! ¡Un enano no deja a nadie atrás!"),
			T("Brom se planta junto a Kaelen, hacha en mano."), K("...Gracias, barbas."), B("Ya me invitarás a una cerveza."));
		leave_party(M_BROM);
		remove_npc("brom");
	}
	SAY(K("{name}. Planta esa maldita semilla. Por Bartolo, por Tortosa... por nosotros."),
		T("Kaelen se vuelve hacia la marea de siervos, con la espada en alto."));
	leave_party(M_KAELEN);
	remove_npc("kaelen");
	setf("kaelen_resolved");
	end();
}

static void after_kaelen_fight(void) {
	begin();
	spawn_npc("kaelen", ent_x(PLAYER), ent_y(PLAYER) - 40, 0, MODE_IDLE, "kaelen_dark");
	SAY(T("Kaelen cae de rodillas. La luz violeta de sus ojos parpadea."), KD("¿Por qué...? ¿Por qué siempre ganas tú...?"));
	int i = CHOOSE(0, "Porque nunca luché contra ti, Kaelen. Vuelve.", "Vete. No quiero volver a verte.", "Acaba esto. (Rematarlo)");
	setf("kaelen_resolved");
	if (i == 0) {
		if (G.comp[M_KAELEN].approval >= 5 || flag("kaelen_oath") || rivalry() < 70) {
			setf("kaelen_redeemed");
			setf_v("kaelen_dark", 0);
			G.comp[M_KAELEN].rivalry = 0;
			SAY(T("Le tiendes la mano. Kaelen la mira mucho rato."),
				K("...Idiota. Siempre fuiste un{o} idiota."),
				T("La coge. La luz violeta se apaga en sus ojos. Solo queda un reflejo oscuro, al fondo."),
				flag("seed_stolen") ? K("La Semilla... toma. No debí cogerla nunca.") : K("Te debo una. Una muy grande."));
			if (flag("seed_stolen")) {
				setf_v("seed_stolen", 0);
				setf("seed_player");
			}
			remove_npc("kaelen");
			appearance_dirty();
			join(M_KAELEN);
			if (has(M_YARA)) {
				change_approval(M_YARA, 8);
				SAY(Y("Estás aquí. Estás aquí de verdad."), T("Yara abraza a Kaelen. Él no sabe qué hacer con los brazos."));
			}
		} else {
			setf("kaelen_lost");
			SAY(K("Es tarde, {name}. Demasiado tarde."), T("Kaelen se levanta tambaleándose y desaparece entre las raíces."));
			remove_npc("kaelen");
		}
	} else if (i == 1) {
		setf("kaelen_lost");
		SAY(K("...Como quieras. Siempre como tú quieras."), T("Se aleja cojeando hacia la oscuridad."));
		remove_npc("kaelen");
	} else {
		setf("kaelen_dead");
		G.comp[M_KAELEN].alive = 0;
		change_approval(M_YARA, -30);
		change_yara_corruption(25);
		SAY(T("Kaelen no se defiende. Cierra los ojos antes del golpe."), T("Cuando todo acaba, el Corazón se queda en silencio."));
		if (has(M_YARA)) SAY(Y("..."), Y("¿Qué has hecho, {name}? ¿Qué HAS HECHO?"), T("Yara no vuelve a mirarte en mucho rato."));
		remove_npc("kaelen");
	}
	if (flag("seed_stolen")) {
		setf_v("seed_stolen", 0);
		setf("seed_player");
		SAY(T("Recoges la Semilla del suelo. Late, furiosa, en tu mano."));
	}
	end();
}

static void after_horde(void) {
	begin();
	setf("kaelen_resolved");
	if (flag("kaelen_corrupt"))
		SAY(T("El último siervo cae. Kaelen sigue golpeando el cuerpo mucho después de que deje de moverse."),
			T("Cuando por fin para, se limpia la savia de la cara y no dice nada. Tú tampoco."));
	else SAY(T("El último siervo cae. Estáis vivos. Todos."), K("¡Ja! ¿Lo veis? Juntos somos imparables."));
	end();
}

// ------------------------------------------------------------ Final
static void yara_turns(void) {
	SAY(Y("Espera... La oigo. La Madre Raíz. Dice que puede darme todo el poder que necesito."),
		T("La piel de Yara se vuelve pálida. Sus manos gotean luz violeta."),
		Y("Podría curar a todo el mundo, {name}. A todos. Solo tengo que dejarla entrar."));
	int i = CHOOSE(0, "Yara, mírame. Vuelve conmigo.", "Haz lo que tengas que hacer.");
	int ap = G.comp[M_YARA].approval;
	if (i == 0 && (ap >= 15 || flag("romance_yara"))) {
		G.comp[M_YARA].corruption = 30;
		setf("yara_saved");
		SAY(T("Te mira. Tiembla. Y poco a poco, la luz violeta se retira de sus manos."),
			Y("...Estás aquí. Siempre estás aquí. Gracias."));
	} else {
		setf("yara_lost");
		leave_party(M_YARA);
		remove_npc("yara");
		SAY(Y("Adiós, {name}. Lo siento."), T("Yara se funde con las raíces y desaparece en la oscuridad del Corazón."));
	}
}

static void final_dark(void) {
	spawn_emissary(-90, -40, 12, 1.2f);
	SAY(T("Entre las raíces, en la sombra, hay una figura encapuchada. El Emisario."),
		T("Esta vez no se acerca. No ofrece ningún trato. No dice ni una palabra."),
		T("Solo se cruza de brazos y observa. Como quien mira una partida que ya sabe cómo termina."));
	if (flag("kaelen_corrupt") && has(M_KAELEN)) SAY(K("Que mire. Que mire bien."));
	SAY(T("La Madre Raíz abre todos sus ojos a la vez. Y por un instante, te reconoce."));
	battle("final", ENEMIES("mother_root"), "heart", "final", 1);
}

static void final_scene(void) {
	setf("final_started");
	begin();
	walk_party_to(marker_x("final_spot"), marker_y("final_spot") + 40, 50);
	ent_face(PLAYER, 3);
	SAY(T("La Madre Raíz ocupa toda la caverna: un corazón monstruoso atravesado por raíces, con ojos que se abren y se cierran."),
		T("Cada latido os golpea en el pecho."));
	if (flag("ov_done") && !flag("ov_e3")) {
		int both = flag("ov_e1") && flag("ov_e2");
		if (both) setf("ov_e3");
		sfx("dark", -4.0f, 0.6f);
		SAY(T("El arma del Soberano vibra como un animal que huele la sangre."),
			T("«Aquí late. Debajo de la Madre Raíz, detrás del Velo. Nhal'Zur.»"),
			both ? T("(Tercer eco del Soberano: 3/3)")
				: T(sfmt("«Te faltan ecos, heredero. No estás preparad%s para verle la cara.»", gfm("o", "a"))));
		if (both) setf("ov_ready");
	}
	if (flag("player_dark")) { final_dark(); return; }
	spawn_emissary(-50, -10, 16, 0.8f);
	SAY(EM("Os dije que volveríamos a hablar."),
		EM("Mirad a esa cosa. No podéis vencerla. Nadie puede... salvo la Torre Negra."),
		EM("Dadme la Semilla y la Madre Raíz se dormirá esta misma noche. Tortosa vivirá. Vuestras familias vivirán."),
		EM("Solo pedimos una cosa a cambio: que Tortosa, y su Catedral, nos pertenezcan. Un precio pequeño, ¿no?"));
	int i = CHOOSE(&EM("¿Y bien?"), "Jamás. Tortosa no está en venta.", "...Trato hecho. Toma la Semilla.");
	if (i == 1) {
		setf("pact_accepted");
		SAY(T("Le entregas la Semilla. Sus dedos están helados."),
			EM("Sabia decisión. La Torre Negra recompensa a los sabios."),
			T("Con un gesto, la Madre Raíz se encoge, gime, y se hunde en la tierra como un animal apaleado."),
			T("El silencio que queda es peor que cualquier rugido."));
		end();
		go_ending();
	}
	SAY(EM("Qué lástima. Veremos cuánto dura vuestra luz."));
	fade_npc("emissary", 0.6f);
	if (has(M_YARA) && corruption() >= 45) yara_turns();
	SAY(T("La Madre Raíz abre todos sus ojos a la vez."), T("Es ahora o nunca."));
	battle("final", ENEMIES("mother_root"), "heart", "final", 1);
}

// ------------------------------------------------------------ Ruta de Vael (todos los legendarios, sin Overlord)
static void vael(void) {
	setf("vael_route");
	music_stop(1.0f);
	SAY(T("La Madre Raíz se estremece por última vez... y en lugar de morir, se abre como una flor podrida."),
		T("Detrás no hay tierra. Hay un desgarrón en el aire, negro y rojo: el Velo."),
		T("Entonces tu equipo empieza a brillar. Las catorce reliquias legendarias cantan a la vez, cada una con una nota distinta."));
	set_tint_wait(320, 294, 205, 1.0f);
	SAY(T("Una figura de luz aparece a tu lado. Lleva una corona sencilla y te mira con una sonrisa cansada."),
		T("«Soy Vael. El Primer Soberano. Hace mil años encerré a Nhal'Zur y, para hacerlo, forjé mi alma en un arma de odio.»"),
		T("«Tú has hecho lo que yo no supe: reunir el legado de todos los pueblos sin tocar ese acero maldito.»"),
		T("«No necesitas mi arma, heredero. Necesitas mi nombre.»"),
		T("La luz de Vael entra en tu pecho. Por un momento, recuerdas mil años que no son tuyos."),
		T("(Te has convertido en Vael, el Dios Héroe.)"));
	spawn_npc("emissary", ent_x(PLAYER) - 40, ent_y(PLAYER) - 30, 2, MODE_IDLE, 0);
	SAY(EM("¿Vael? No... ¡NO! ¡Tú estabas encerrado en el acero!"),
		T("El Emisario se deshace en humo y huye hacia el desgarrón. Del Velo sale una cabeza coronada de cristal negro."),
		T("«NHAL'ZUR», dice el hambre. Y por primera vez en mil años, suena asustada."));
	remove_npc("emissary");
	if (has(M_YARA)) SAY(Y("{name}... estás brillando. ¿Eres tú?"), P("Somos los dos. Y vamos a terminar esto."));
	end();
	battle_ex("nhalzur", ENEMIES("nhalzur"), "heart", "soberano", 1, 1, 0);
}

static void after_vael(void) {
	begin();
	SAY(T("Nhal'Zur grita. Pero esta vez no hay odio que lo alimente: solo luz, y la luz no se puede comer."),
		T("El Velo se cierra despacio, como una herida que por fin cicatriza."),
		T("«Gracias», susurra Vael dentro de ti. «Ya puedo descansar. Pero alguien tiene que vigilar la puerta.»"));
	int i = CHOOSE(&T("La luz espera tu respuesta."), "Quedarme como guardián del Velo.", "Volver a casa... y vigilar desde allí.");
	setf(i == 0 ? "vael_guardian" : "vael_home");
	setf("ending_vael");
	setf("ov_end_chosen");
	SAY(T("La Semilla echa raíces sola. El Corazón del Bosque vuelve a latir, verde y limpio."));
	end();
	go_ending();
}

// ------------------------------------------------------------ Ruta del Soberano
static int is_overlord(int eq) { return eq >= 0 && starts_with(g_equip[eq].id, "overlord_"); }

// Contra Nhal'Zur hace falta el arma: si el jugador se la quitó, vuelve sola.
static void overlord_back_in_hand(void) {
	if (is_overlord(equipped(M_PLAYER, SL_WEAPON))) return;
	static const char* const KIND[] = {"sword", "sword", "axe", "daggers", "staff", "bow"};
	int wk = G.cls >= 0 ? g_classes[G.cls].weapon : 1;
	char eid[32];
	snprintf(eid, sizeof eid, "overlord_%s", KIND[wk < 6 ? wk : 1]);
	if (item_count(eid) <= 0)
		for (int k = 1; k < 6; ++k)
			if (item_count(sfmt("overlord_%s", KIND[k])) > 0) { snprintf(eid, sizeof eid, "overlord_%s", KIND[k]); break; }
	if (item_count(eid) <= 0) add_item(eid, 1);
	gs_equip(M_PLAYER, SL_WEAPON, db_equip(eid));
	appearance_dirty();
}

static void nhalzur(void) {
	music_stop(1.0f);
	SAY(T("La Madre Raíz se estremece por última vez... y en lugar de morir, se abre como una flor podrida."),
		T("Detrás de ella no hay tierra. Hay un desgarrón en el aire, negro y rojo, que cruje como hielo."),
		T("El Velo."));
	shake(6.0f, 1.0f);
	spawn_npc("emissary", ent_x(PLAYER) - 40, ent_y(PLAYER) - 30, 2, MODE_IDLE, 0);
	SAY(EM("Por fin. Mil años esperando a que alguien fuera lo bastante fuerte para matar a la Madre."),
		EM("Gracias, heredero de Vael. Me has abierto la puerta."),
		T("La capucha del Emisario se hunde hacia dentro, como si detrás no hubiera cara. Su cuerpo se deshace en humo y entra en el desgarrón."));
	remove_npc("emissary");
	set_tint_wait(128, 31, 56, 1.0f);
	SAY(T("Del Velo sale una cabeza coronada de cristal negro. Luego unos brazos. Luego ojos, demasiados ojos."),
		T("«NHAL'ZUR», dice la voz, y no es una voz: es hambre con forma de palabra."),
		T("El arma del Soberano grita en tu mano: «¡AHORA, HEREDERO! ¡TERMINA LO QUE EMPEZAMOS!»"));
	if (has(M_YARA)) SAY(Y("{name}... ¿Qué es eso? ¿Qué es esa arma?"), P("Luego te lo explico. Si hay un luego."));
	overlord_back_in_hand();
	end();
	battle("nhalzur", ENEMIES("nhalzur"), "heart", "soberano", 1);
}

static void after_nhalzur(void) {
	if (flag("vael_route")) { after_vael(); return; }
	begin();
	SAY(T("Nhal'Zur se retuerce. Sus ojos se apagan uno a uno, y su grito hace temblar el mundo entero."),
		T("El desgarrón del Velo empieza a cerrarse... pero no del todo. Algo lo mantiene abierto: el arma que llevas."),
		T("«Heredero», susurra el Soberano. «Ya lo has visto. Hay dos caminos.»"),
		T("«Puedes quedarte conmigo. Ocupar mi trono. Con este acero, nadie volverá a amenazar Vaelmoor... nunca. Ni nadie te dirá nunca que no.»"),
		T("«O puedes romperme contra el Velo. Mi alma lo sellará para siempre, y yo descansaré por fin. Pero el arma desaparecerá contigo... o sin ti.»"));
	int i = 0;
	if (!flag("player_dark"))
		i = CHOOSE(&T("El arma espera."), "Ocupar el trono del Soberano.", "Romper el arma y sellar el Velo para siempre.");
	else SAY(T("La voz fría ya ha elegido por ti."));
	setf("ov_end_chosen");
	if (i == 0) {
		setf("ending_sovereign");
		SAY(T("Levantas el arma. La corona de cristal negro se forma sola sobre tu cabeza."),
			T("Las raíces se inclinan. El Velo se cierra... a tu voluntad. Todo en Vaelmoor sabe, de golpe, quién manda."));
	} else {
		setf("ending_sealed");
		if (is_overlord(equipped(M_PLAYER, SL_WEAPON))) {
			gs_equip(M_PLAYER, SL_WEAPON, -1);
			appearance_dirty();
		}
		SAY(T("Clavas el arma en el desgarrón. El acero se agrieta, brilla en blanco... y estalla."),
			T("«Gracias, heredero», dice Vael, y por primera vez su voz suena en paz."),
			T("El Velo se cierra con un sonido de campana. En el Corazón del Bosque, la Semilla echa raíces sola."),
			T("Y todo, por fin, se queda en silencio."));
	}
	end();
	go_ending();
}

static void after_final(void) {
	begin();
	setf("final_done");
	if (flag("ov_ready")) { nhalzur(); return; }
	if (vael_worthy() && !flag("player_dark")) { vael(); return; }
	SAY(T("La Madre Raíz se estremece por última vez. Sus ojos se apagan uno a uno."),
		T("Las raíces se agrietan. Por las grietas se cuela, por primera vez, un rayo de luz verde."),
		T("La Semilla late con fuerza. Sabe que ha llegado su momento."));
	const char* carrier = "tú";
	if (flag("seed_yara") && has(M_YARA)) carrier = "Yara";
	else if (flag("seed_kaelen") && has(M_KAELEN)) carrier = "Kaelen";
	if (flag("player_dark")) {
		setf("ending_king");
		SAY(T("La Semilla late. Y tú ya no quieres plantarla."),
			T("La voz fría decide por ti. Aprietas la Semilla contra el pecho y dejas que su poder te llene."),
			T("Así, piensas, nadie volverá a arrebatarte nada."));
		end();
		go_ending();
	}
	Line pr = T(sfmt("La Semilla espera. (La lleva %s.)", carrier));
	int i = choose_opts(OPTS("Plantar la Semilla y purificar el Corazón.", "Absorber el poder de la Semilla.",
		"Fundirte con la Semilla, como Ysolde."), &pr);
	if (i == 0) {
		int pure = flag("ritual_ok") || (has(M_YARA) && corruption() < 30);
		int tainted = (flag("seed_yara") && corruption() >= 40) || (flag("seed_kaelen") && rivalry() >= 40);
		if (pure && !tainted) {
			setf("ending_purify");
			SAY(!flag("ritual_ok") ? T("La Semilla se hunde en la tierra negra.")
					: T("Dejas caer la Lágrima de la Savia sobre la Semilla y la hundes en la tierra negra."),
				T("Un brote verde atraviesa la podredumbre. Luego otro. Y otro más."),
				T("La luz se extiende como una ola por todo el Corazón del Bosque."));
		} else {
			SAY(T("Plantas la Semilla... pero la tierra no la acepta. La luz se apaga, vacilante."),
				T("Oyes la voz de Ysolde, lejana: «La tierra pide un corazón.»"));
			int j = CHOOSE(0, "Darle el mío.", "No. No así.");
			setf(j == 0 ? "ending_sacrifice" : "ending_withered");
		}
	} else if (i == 1) {
		setf("ending_king");
		SAY(T("Aprietas la Semilla contra tu pecho. El cristal se hunde en tu carne como si siempre hubiera sido tuyo."),
			T("El poder te inunda. Las raíces se inclinan ante ti. La Madre Raíz no ha muerto: ahora eres tú."));
	} else setf("ending_sacrifice");
	end();
	go_ending();
}

// ------------------------------------------------------------ entrada
static void h_ready(void) {
	if (flag("won_kaelen_dark") && !flag("kaelen_resolved")) { after_kaelen_fight(); return; }
	if (flag("won_horde") && !flag("kaelen_resolved")) { after_horde(); return; }
	if (flag("won_aleixolo") && !flag("aleixolo_done")) { after_aleixolo(); return; }
	if (flag("won_nhalzur") && !flag("ov_end_chosen")) { after_nhalzur(); return; }
	if (flag("won_final") && !flag("final_done")) { after_final(); return; }
	if (!flag("heart_seen")) {
		setf("heart_seen");
		begin();
		wait(0.5f);
		SAY(T("El Corazón del Bosque ya no es un bosque."),
			T("Raíces gruesas como torres se retuercen formando pasillos de carne leñosa. Todo late. Todo respira."),
			T("Y al fondo, muy arriba, algo enorme palpita con una luz violeta."));
		if (has(M_AELIS)) SAY(A("Aquí jugaba de niña. Aquí aprendí a disparar. Y mirad lo que queda..."));
		if (has(M_BROM)) SAY(B("He visto esto antes. En las grietas bajo Khazgurim. Esa luz... sale de la misma herida."));
		end();
	}
}

static void h_trigger(const char* id) {
	if (IS(id, "aleixolo_zone")) { if (!flag("aleixolo_event")) aleixolo(); }
	else if (IS(id, "kaelen_zone")) { if (!flag("kaelen_event")) kaelen_event(); }
	else if (IS(id, "final_zone")) { if (!flag("final_started")) final_scene(); }
}

const StoryDef story_heart = {.map = "heart", .music_override = h_music, .encounters_disabled = h_no_enc, .on_map_ready = h_ready,
	.on_trigger = h_trigger};
