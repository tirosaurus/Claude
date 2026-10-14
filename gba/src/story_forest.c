// Guion del Bosque Santo (port de forest.gd)
#include "story.h"
#include "particles.h"

static Ent* wolf;

static void well_sparkle(void) {
	ParticleDef d;
	memset(&d, 0, sizeof d);
	d.amount = 14; d.lifetime = 1.4f; d.radius = 10; d.gy = -16; d.vmin = 2; d.vmax = 8; d.spread = 180; d.dir_y = -1;
	d.size = 2; d.r = 255; d.g = 230; d.b = 128;
	particles_emit(&d, 700, 186, 1);
}

static void class_tutorial(void) {
	SAY(Y("Esperad. Si de verdad hay algo raro en el bosque, no quiero que vayamos a lo loco."),
		K("¿Otra de tus charlas de «seguridad ante todo»?"),
		Y("Sí. Y esta vez me vais a escuchar. {name}, ¿cómo sueles pelear tú? Y no me digas «a palos»."));
	int bi = CHOOSE(&Y("¿Qué se te da mejor?"), "Cuerpo a cuerpo: en primera línea, con acero.",
		"A distancia: con arco, hechizos o pactos oscuros.", "Apoyo: curando y protegiendo a los demás.");
	const BranchDef* br = &g_branches[bi];
	const char* opts[3];
	for (int k = 0; k < 3; ++k) {
		const ClassDef* c = &g_classes[br->classes[k]];
		opts[k] = sfmt("%s: %s", c->name, c->desc);
	}
	static const char* const INTRO[3] = {"Primera línea, entonces. ¿Pero de qué tipo? ¿Fuerza bruta, sigilo o fe?",
		"Desde lejos. ¿Magia pura, pactos con la sombra o un buen arco?",
		"Alguien que cuide del grupo. ¿Con luz, con el bosque o con tu propio cuerpo como escudo?"};
	Line pr = Y(INTRO[bi]);
	int j = choose_list(opts, 3, &pr);
	int cid = br->classes[j];
	choose_class(cid);
	const ClassDef* c = &g_classes[cid];
	Line gift = K("Toma, algo es algo.");
	switch (c->weapon) {
	case WK_SWORD: gift = K("Toma, la espada de práctica de mi padre. Está mellada, pero corta."); break;
	case WK_AXE: gift = K("Mi padre dejó esta hacha en la leñera. Pesa como un demonio. Te irá bien."); break;
	case WK_DAGGERS: gift = K("Un par de cuchillos de caza. No preguntes de dónde los he sacado."); break;
	case WK_STAFF: gift = Y("El bastón de roble de mi abuela. Dicen que canaliza bien la magia... o eso decía ella."); break;
	case WK_BOW: gift = K("Mi arco de cazar conejos. Si fallas, que sea contra otro."); break;
	}
	sfx("chest", -6.0f, 1.0f);
	SAY(gift, T(sfmt("(Clase: %s. %s)", c->name, c->desc)),
		Y("Escuchad, que esto es importante:"),
		Y("En un combate, cada uno actúa cuando se llena su barra de tiempo. Atacar no gasta nada; las habilidades, maná."),
		Y("El maná se recupera poco a poco en cada turno. No lo malgastéis al principio."),
		Y("Si curo, puedo elegir a quién. Y si alguien cae, una pluma de fénix lo levanta."),
		Y("Cuando crezcáis en fuerza aprenderéis técnicas nuevas: pensad bien en cuáles os especializáis."),
		T("(Menú START: Talentos para gastar puntos al subir de nivel, y Equipo para cambiar armas y armaduras de todo el grupo.)"),
		K("Vale, vale, profesora. ¿Podemos ir ya a buscar al ciervo?"));
}

static void meet_yara(void) {
	begin();
	Ent* yara = npc("yara");
	walk_to(PLAYER, 236, ent_y(PLAYER), 60, 0);
	wait(0.2f);
	ent_face(yara, 1);
	ent_hop(yara);
	SAY(Y("¡Os estaba oyendo discutir desde la otra punta del bosque! Asustáis hasta a los ciervos."),
		K("Nosotros no discutimos. Debatimos. Con pasión."), Y("Ya."));
	walk_to(PLAYER, ent_x(yara) - 22, ent_y(yara) + 2, 55, 0);
	ent_face(PLAYER, 2);
	ent_face(yara, 1);
	SAY(Y("Mirad: flor de luna. Solo crece donde la tierra está sana y el pacto sigue en pie."),
		K("La curandera oficial de Tortosa. ¿Sabes qué sería interesante? Encontrar la guarida del ciervo blanco."),
		Y("El ciervo blanco es un cuento para niños, Kaelen."),
		K("Eso dijiste de la cueva de los murciélagos, y mira."), Y("¡Casi nos morimos en la cueva de los murciélagos!"));
	int i = CHOOSE(&Y("{name}, tú decides. ¿Le seguimos la corriente o nos quedamos aquí tranquilos?"),
		"Vamos a explorar. Será divertido.", "Yara tiene razón. Quedémonos cerca.", "Exploremos un poco, pero sin alejarnos del sendero.");
	if (i == 0) {
		change_kaelen_rivalry(-6);
		change_approval(M_KAELEN, 4);
		change_approval(M_YARA, -2);
		SAY(K("¡Por fin alguien con sangre en las venas!"), Y("Siempre le das la razón a él..."), T("Yara lo dice en broma. Casi."));
	} else if (i == 1) {
		change_kaelen_rivalry(6);
		change_approval(M_YARA, 6);
		change_yara_kaelen_approval(-2);
		SAY(K("Qué par de abuelos."), Y("Gracias, {name}. Alguien sensato en este grupo."));
	} else {
		change_approval(M_KAELEN, 2);
		change_approval(M_YARA, 3);
		change_kaelen_rivalry(-2);
		SAY(Y("Diplomátic{o}, como siempre."), K("Me vale. Pero el que vea el ciervo primero invita a sidra."));
	}
	SAY(T("Yara guarda las flores en su zurrón. De repente, se queda muy quieta."),
		Y("Esperad... ¿lo oís?"), K("No oigo nada."),
		Y("Exacto. Ni un pájaro. Ni un grillo. El bosque nunca está tan callado."), T("Yara se une al grupo."));
	setf("yara_joined");
	join(M_YARA);
	if (G.cls < 0) class_tutorial();
	end();
}

static void wolf_encounter(void) {
	begin();
	ent_face(PLAYER, 2);
	music_stop(1.2f);
	wait(0.4f);
	sfx("growl", -2.0f, 1.0f);
	wait(0.4f);
	SAY(Y("¿Qué ha sido eso?"));
	wolf->visible = 1;
	move_node_async(wolf, ent_x(PLAYER) + 62, ent_y(PLAYER), 1.0f, 1);
	for (int k = 0; k < 8; ++k) { wolf->ms_frame = k % 2; wait(0.12f); }
	wait(0.05f);
	ent_set_pos(wolf, ent_x(PLAYER) + 62, ent_y(PLAYER));
	sfx("growl", 0.0f, 0.9f);
	SAY(K("¿Un lobo? ¿Tan cerca del pueblo?"),
		Y("Mirad su lomo... esas venas negras. Y los ojos. Eso no es normal."),
		K("Sea lo que sea, viene a por nosotros. ¡{name}, a mi lado!"),
		Y("¡Yo os curo! ¡Elegid bien a quién!"),
		T("(Combate: cuando se llene la barra de tiempo de alguien, elige su acción. Con Curar puedes elegir a quién sanar.)"));
	battle("wolf", ENEMIES("wolf_alpha"), "forest", "boss", 1);
}

static void post_wolf(void) {
	begin();
	int px = ent_x(PLAYER) + 52, py = ent_y(PLAYER) - 4;
	scene_sprite("sap_puddle", px, py + 4, 0, 0);
	Ent* yara = npc("yara");
	ent_face(PLAYER, 2);
	wait(0.8f);
	SAY(K("Uf... ¿estáis todos bien?"),
		T("El lobo ha huido entre la maleza, herido. En el suelo solo queda un charco espeso y negro."));
	if (yara) {
		yara->busy = 1;
		walk_to(yara, px - 6, py + 6, 40, 0);
		ent_face(yara, 3);
	}
	SAY(Y("Esto no es sangre. Es... como savia. Savia podrida."),
		Y("Huele igual que la raíz muerta que vi ayer junto a la piedra santa."),
		K("¿Me estás diciendo que el bosque está... enfermando?"));
	if (flag("dream_told"))
		SAY(T("Las raíces negras de tu sueño. Latiendo. Susurrando tu nombre."), T("Sientes un escalofrío, pero no dices nada."));
	int i = CHOOSE(&K("{name}... ¿tú qué crees que está pasando?"), "No lo sé, pero lo averiguaremos juntos.",
		"Deberíamos volver ya. Esto nos viene grande.", "Si algo amenaza Tortosa, lo pararé. Cueste lo que cueste.");
	if (i == 0) {
		change_kaelen_rivalry(-5);
		change_approval(M_YARA, 4);
		SAY(K("Juntos. Como siempre."), Y("Como siempre."));
	} else if (i == 1) {
		change_approval(M_YARA, 3);
		change_kaelen_rivalry(4);
		SAY(Y("Es lo más sensato."), K("...Sí. Supongo que sí."), T("Kaelen mira hacia el sur del bosque un rato más de la cuenta."));
	} else {
		change_approval(M_KAELEN, 4);
		change_yara_corruption(3);
		SAY(K("Eso es hablar."), Y("No digas eso tan a la ligera. «Cueste lo que cueste» es mucho decir."));
	}
	SAY(Y("Volvamos a Tortosa. Hay que enseñarle esto a Bartolo."), K("Y a todos. Esta noche, en la plaza."));
	if (yara) {
		yara->busy = 0;
		make_follower(M_YARA);
	}
	setf("post_wolf_done");
	setf("act2_started");
	tod_set("dusk");
	int j = CHOOSE(0, "Volver a Tortosa ahora.", "Echar un vistazo por el bosque primero.");
	if (j == 1) {
		SAY(K("Vale, pero no tardes. Se está haciendo de noche."), T("(Cuando quieras volver, sal por el camino del oeste.)"));
		end();
		return;
	}
	end();
	go_to_map("village", "from_forest", 0);
}

static void f_ready(void) {
	wolf = 0;
	if (flag("act3_started")) {
		if (flag("marta_saved") && !flag("curriculum_found")) {
			well_sparkle();
			if (!flag("well_hint")) {
				setf("well_hint");
				begin();
				wait(0.4f);
				SAY(Y("Mirad, el claro del este. Ahí está el pozo viejo del que habló Marta."),
					K("El del gordo del cucharón. Si tiró algo que le daba miedo, más vale tenerlo nosotros."),
					T("(El pozo abandonado está al este del claro, al norte del camino. Examínalo con A.)"));
				end();
			}
		}
		return;
	}
	if (!flag("yara_joined")) {
		Ent* y = spawn_npc("yara", marker_x("yara"), marker_y("yara"), 0, MODE_IDLE, 0);
		ent_face(y, 0);
	}
	if (!flag("won_wolf")) {
		wolf = scene_sprite("wolf_map", marker_x("wolf") + 80, marker_y("wolf"), 1, 0);
		if (wolf) wolf->visible = 0;
	} else if (!flag("post_wolf_done")) post_wolf();
}

static void f_trigger(const char* id) {
	if (flag("act3_started")) return;
	if (IS(id, "meet_yara")) {
		if (!flag("yara_joined")) meet_yara();
	} else if (IS(id, "wolf_zone")) {
		if (flag("won_wolf")) return;
		if (!flag("yara_joined")) {
			begin();
			ent_set_pos(PLAYER, ent_x(PLAYER) - 10, ent_y(PLAYER));
			SAY(K("Eh, ¿adónde vas? Yara estará en el claro de las flores, junto a la piedra santa."));
			end();
			return;
		}
		if (wolf) wolf_encounter();
	}
}

static void old_well(void) {
	begin();
	if (flag("curriculum_found")) {
		SAY(T("El pozo abandonado. Ya no queda nada dentro, salvo un olor dulzón a cacao."));
		end();
		return;
	}
	if (!flag("marta_saved") || !flag("act3_started")) {
		SAY(T("Un pozo viejo, seco y tapado con tablas podridas."), T("Por alguna razón, huele a chocolate."));
		end();
		return;
	}
	SAY(T("El pozo del que habló Marta. Apartas las tablas: hay una escalerilla de hierro oxidado."),
		T("Bajas. Al fondo, entre cáscaras de nuez y envoltorios de chocolate, hay unos papeles doblados."));
	setf("curriculum_found");
	add_item("curriculum", 1);
	sfx("chest", -4.0f, 1.0f);
	SAY(T("Obtienes: Currículum de Aleixolo."),
		T("«ALEIXOLO. Aspirante a Maestro Repostero. Experiencia: tres meses de ayudante. Motivo del despido: comerse el género.»"),
		T("«Aficiones: el chocolate. Otras aficiones: el chocolate con leche.»"),
		T("Grapada al currículum hay una carta: «Estimado Aleixolo: lamentamos comunicarle que no ha sido seleccionado.» Hay otras cuarenta iguales."),
		K("Esto es lo más triste que he leído en mi vida."),
		T("(Úsalo en combate contra Aleixolo desde Objetos. O enséñaselo antes de luchar.)"));
	end();
}

static void well_prompt(const char* a) {
	(void)a;
	begin();
	int i = CHOOSE(&Y("Espera, {name}. ¿Y el pozo de Marta? Arriesgaste la vida por esa pista."),
		"Tienes razón. Vamos al pozo primero.", "No. Sigamos: ese gordo no me preocupa.");
	if (i == 0) {
		walk_party_to(700, 222, 80);
		ent_face(PLAYER, 3);
		end();
		old_well();
		return;
	}
	setf("well_skipped");
	change_approval(M_YARA, -3);
	SAY(Y("...Como quieras. Espero que no nos arrepintamos."));
	end();
	go_to_map("forest_deep", "from_forest", 0);
}

static int f_before_exit(const char* to) {
	if (IS(to, "forest_deep") && flag("marta_saved") && !flag("curriculum_found") && !flag("well_skipped")) {
		ent_set_pos(PLAYER, ent_x(PLAYER) - 12, ent_y(PLAYER));
		story_call(well_prompt, "");
		return 0;
	}
	return 1;
}

static void overlord(void) {
	begin();
	setf("ov_done");
	music_stop(1.0f);
	SAY(T("Apoyas la mano en la piedra santa."), T("Las runas no parpadean. Se apagan del todo."),
		T("Y entonces, todas a la vez, arden en rojo."));
	set_tint_wait(141, 51, 64, 1.2f);
	sfx("dark", -2.0f, 0.6f);
	ParticleDef d;
	memset(&d, 0, sizeof d);
	d.amount = 30; d.lifetime = 1.6f; d.radius = 30; d.gy = -30; d.vmin = 4; d.vmax = 12; d.spread = 180; d.dir_y = -1;
	d.size = 3; d.r = 255; d.g = 51; d.b = 64;
	int glow = particles_emit(&d, ent_x(PLAYER), ent_y(PLAYER) - 10, 1);
	SAY(T("El bosque desaparece. Estás en un claro que no existe, bajo un cielo sin estrellas."),
		T("Delante de ti flota un arma negra y roja, girando despacio, como si te hubiera estado esperando."),
		T("«Por fin. Alguien que sabe mirar donde nadie mira.»"),
		T("«Mil años he dormido bajo esta piedra. Mil años esperando a quien recordara el orden de las cosas pequeñas.»"),
		T("«Tómame. Y que tiemblen los que se creen dioses.»"));
	static const char* const KIND[] = {"", "sword", "axe", "daggers", "staff", "bow"};
	int wk = g_classes[G.cls].weapon;
	const char* eid = sfmt("overlord_%s", KIND[wk < 6 ? wk : 1]);
	char eidb[32];
	strncpy(eidb, eid, 31);
	eidb[31] = 0;
	add_item(eidb, 1);
	int eq = db_equip(eidb);
	gs_equip(M_PLAYER, SL_WEAPON, eq);
	sfx("levelup", 0.0f, 0.7f);
	SAY(T(sfmt("Obtienes: %s.", g_equip[eq].name)), T("(Se ha equipado. Tu ataque es ahora... absurdo.)"),
		T("«Una cosa más, heredero. No te he despertado para matar lobos.»"),
		T("«Busca mis tres ecos: donde la luz selló la puerta, donde el árbol recuerda, y donde el corazón late.»"),
		T("«Cuando los tengas, sabrás contra quién luchas de verdad.»"),
		T("(Misión secreta: «La Ruta del Soberano».)"));
	particles_stop(glow);
	int r, g, b;
	map_modulate(&r, &g, &b);
	set_tint_wait(r, g, b, 1.0f);
	if (has(M_KAELEN))
		SAY(K("¿{name}? ¿Qué acaba de pasar? Te has quedado mirando la piedra un minuto entero."),
			Y("Y... ¿de dónde has sacado eso?"), P("...Mejor no preguntéis."));
	music_play("forest", 1.0f);
	end();
}

static void f_interact(const char* id) {
	if (IS(id, "old_well")) { old_well(); return; }
	if (IS(id, "shrine") && flag("ov3") && !flag("ov_done") && !flag("won_wolf") && G.cls >= 0) { overlord(); return; }
	base_on_interact(id);
}

static int f_lines(const char* id, LineBuf* o) {
	if (IS(id, "shrine")) {
		if (flag("act3_started"))
			LB(o, T("La piedra santa. Sus runas están casi apagadas. Solo un leve pulso azul, como un corazón cansado."));
		else
			LB(o, T("La piedra santa. Dicen que aquí se selló el pacto que protege el bosque."),
				T("Las runas brillan con una luz azulada... débil, parpadeante."));
		if (has(M_YARA) && !flag("act3_started"))
			LB(o, Y("De pequeña brillaban tanto que de noche se veían desde el pueblo. Algo va mal, {name}."));
	}
	return o->n;
}

const StoryDef story_forest = {.map = "forest", .on_map_ready = f_ready, .on_trigger = f_trigger, .before_exit = f_before_exit,
	.on_interact = f_interact, .interact_lines = f_lines};
