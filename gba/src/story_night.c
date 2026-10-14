// Guion del ataque nocturno a Tortosa (port de village_night.gd)
#include "story.h"

static int vn_no_enc(void) { return 1; }

static void place_npcs(void) {
	if (flag("night_party") && flag("night_post")) {
		spawn_npc("remei", 390, 300, 1, MODE_IDLE, 0);
		if (!flag("nil_dead")) spawn_npc("nil", 410, 310, 1, MODE_IDLE, 0);
		spawn_npc("roc", 330, 290, 2, MODE_IDLE, 0);
		if (flag("bartolo_alive")) spawn_npc("bartolo", 368, 244, 0, MODE_IDLE, 0);
	}
}

static void first_attack(void) {
	begin();
	show_banner("Tortosa en llamas");
	wait(0.6f);
	SAY(T("Tortosa arde."),
		T("El tejado de los Ferrer es una antorcha. Hay gritos por todas partes, y entre el humo se mueven cosas."));
	// 1) Un vecino huye por la calle; un siervo lo alcanza
	pan_to(540, 410, 1.1f);
	Ent* v1 = spawn_npc("vill1", 700, 412, 1, MODE_IDLE, "villager_m");
	Ent* t1 = spawn_npc("th1", 740, 408, 1, MODE_IDLE, "thrall");
	walk_to_async(v1, 470, 412, 78, 0);
	wait(0.3f);
	SAY(T("«¡Socorro! ¡Que alguien me ayude!»"));
	walk_to(t1, 492, 410, 96, 0);
	sfx("growl", -4.0f, 1.3f);
	ent_face(t1, 1);
	wait(0.15f);
	knock_down(v1);
	shake(3.0f, 0.3f);
	wait(0.6f);
	SAY(T("El siervo se inclina sobre él. Tiene la cara gris, los ojos violetas... y lleva la ropa del molinero."));
	// 2) Larvas brotando de una grieta de savia en el suelo
	pan_to(420, 380, 1.2f);
	Ent* l1 = monster("larva_09", 420, 360, 0);
	Ent* l2 = monster("larva_09", 416, 362, 0);
	l1->alpha = 0;
	l2->alpha = 0;
	ent_fade(l1, 16, 0.5f);
	ent_fade(l2, 16, 0.5f);
	dust(420, 360);
	sfx("dark", -8.0f, 1.3f);
	Ent* v2 = spawn_npc("vill2", 470, 392, 1, MODE_IDLE, "villager_f");
	move_node_async(l1, 372, 392, 1.4f, 0);
	move_node(l2, 452, 396, 1.4f, 0);
	walk_to_async(v2, 620, 440, 80, 0);
	SAY(T("Del suelo, por una grieta que rezuma savia negra, brotan larvas del tamaño de un perro."),
		T("Una vecina huye con un niño en brazos."));
	// 3) Siervos golpeando los escombros de la Catedral
	pan_to(368, 250, 1.3f);
	Ent* t2 = spawn_npc("th2", 346, 236, 3, MODE_IDLE, "thrall");
	Ent* t3 = spawn_npc("th3", 392, 238, 3, MODE_IDLE, "thrall");
	for (int k = 0; k < 3; ++k) {
		ent_hop(t2);
		dust(360, 224);
		sfx("hit", -6.0f, 0.6f);
		shake(2.0f, 0.2f);
		ent_hop(t3);
		dust(380, 224);
		sfx("hit", -6.0f, 0.7f);
		wait(0.2f);
	}
	SAY(T("Frente a la Catedral Vieja, más siervos golpean los escombros con las manos desnudas."), T("Quieren entrar."));
	// 4) De vuelta al jugador
	release_camera(0.8f);
	remove_npc("vill1"); remove_npc("vill2"); remove_npc("th1"); remove_npc("th2"); remove_npc("th3");
	ent_remove(l2);
	ent_set_pos(l1, ent_x(PLAYER) + 46, ent_y(PLAYER) + 26);
	sfx("growl", -4.0f, 1.4f);
	SAY(P("¡¿Qué demonios son esas cosas?!"), T("Una larva se lanza hacia ti, chillando."));
	move_node(l1, ent_x(PLAYER) + 14, ent_y(PLAYER) + 6, 0.25f, 1);
	battle("night1", ENEMIES("larva"), "village_night", "night", 0);
}

static void party_forms(void) {
	begin();
	Ent* kae = spawn_npc("kaelen", 520, 380, 1, MODE_IDLE, 0);
	SAY(K("¡{name}! ¡{name}, estás viv{o}!"));
	walk_to(kae, ent_x(PLAYER) + 24, ent_y(PLAYER) + 10, 90, 0);
	ent_face(kae, 1);
	ent_face(PLAYER, 2);
	SAY(K("He tumbado a uno con la pala de mi padre. ¡Con una pala! Y se ha vuelto a levantar."),
		K("Eran... eran como personas, {name}. Uno llevaba la capa del pastor Joan."),
		P("¿Has visto a Yara?"), K("Junto al pozo. Vamos."));
	join(M_KAELEN);
	Ent* yara = spawn_npc("yara", 380, 290, 1, MODE_IDLE, 0);
	spawn_npc("remei", 360, 294, 2, MODE_IDLE, 0);
	walk_party_to(356, 318, 80);
	ent_face(PLAYER, 3);
	ent_face_towards(yara, ent_x(PLAYER), ent_y(PLAYER));
	SAY(Y("¡Aquí! Ayudadme, Remei tiene un corte muy feo en la pierna."),
		T("Yara aprieta un vendaje con flor de luna. Sus manos brillan un instante con una luz verdosa."),
		N("remei", "Estoy bien, niña, estoy bien... Id. ¡Id!"),
		Y("Voy con vosotros. Nadie más se queda solo esta noche."));
	join(M_YARA);
	wait(0.3f);
	SAY(T("Entonces lo oyes todo a la vez."),
		M("¡¡Socorro!! ¡¡{name}!!"),
		T("El grito viene de tu casa. Algo ha reventado la puerta de la bodega."),
		N("roc", "¡Nil está atrapado bajo el carro! ¡No puedo levantarlo solo y esas cosas vienen hacia aquí!"),
		N("bartolo", "¡Aguantaré la puerta de la Catedral! ¡No dejéis que entren! ¡La Semilla...!"),
		T("Bartolo, con una antorcha y un bastón, se planta solo frente a los escombros de la Catedral."),
		Y("¡No podemos estar en tres sitios a la vez!"),
		K("¡Decide, {name}! ¡Rápido!"));
	int i = CHOOSE(0, "¡A por mi madre!", "¡Nil primero, es un niño!", "¡A la Catedral, con Bartolo!");
	setf("night_party");
	if (i == 0) {
		setf("chose_home");
		SAY(K("¡Vamos!"), Y("¡Roc, aguanta!"));
		ent_set_pos(PLAYER, 170, 380);
		battle("night_home", ENEMIES("thrall"), "village_night", "night", 0);
	} else if (i == 1) {
		setf("chose_plaza");
		if (rivalry() <= 0) {
			setf("kaelen_saves_mother");
			SAY(K("¡Yo voy a por tu madre! ¡Tú ayuda a Nil!"), T("Kaelen echa a correr hacia tu casa sin mirar atrás."));
			change_kaelen_rivalry(-5);
		} else {
			setf("mother_wounded");
			SAY(K("¿Y tu madre? ¿La dejas sola?"), P("¡Nil es un niño, Kaelen!"), K("...Como quieras. Siempre como tú quieras."));
			change_kaelen_rivalry(6);
		}
		ent_set_pos(PLAYER, 300, 280);
		battle("night_plaza", ENEMIES("thrall", "larva"), "village_night", "night", 0);
	} else {
		setf("chose_cath");
		setf("bartolo_alive");
		SAY(Y("¿Y Nil? ¿Y tu madre?"), P("¡Si entran en la Catedral, estamos todos muertos!"),
			K("...Tiene sentido. Duele, pero tiene sentido."));
		change_approval(M_YARA, -4);
		change_kaelen_rivalry(-2);
		ent_set_pos(PLAYER, 368, 230);
		battle("night_cath", ENEMIES("thrall", "thrall"), "village_night", "night", 0);
	}
}

static void brute_arrives(void) {
	music_stop(0.6f);
	wait(0.3f);
	pan_to(440, 230, 1.0f);
	sfx("growl", 0.0f, 0.6f);
	shake(3.0f, 0.5f);
	SAY(T("La tierra tiembla."), T("Algo enorme sale de entre el humo y se dirige a la Catedral."));
	Ent* br = monster("brute_075", 560, 246, 0);
	for (int k = 0; k < 4; ++k) {
		move_node(br, ent_x(br) - 34, ent_y(br) - 4, 0.35f, 1);
		shake(2.5f, 0.2f);
		sfx("hit", -8.0f, 0.5f);
	}
	for (int k = 0; k < 2; ++k) {
		move_node(br, ent_x(br) - 6, ent_y(br) - 4, 0.12f, 1);
		dust(372, 222);
		shake(5.0f, 0.3f);
		sfx("crit", -4.0f, 0.6f);
		move_node(br, ent_x(br) + 6, ent_y(br) + 4, 0.2f, 1);
	}
	SAY(T("El bruto golpea los escombros de la Catedral con los puños. Una vez. Dos. Las piedras empiezan a ceder."),
		K("¡Quiere entrar! ¡Busca la Semilla!"), Y("¡Hay que pararlo!"));
	release_camera(0.3f);
	walk_party_to(380, 262, 90);
	battle("brute", ENEMIES("brute", "larva"), "village_night", "boss", 1);
}

static void after_choice(void) {
	begin();
	setf("night_post");
	wait(0.5f);
	if (flag("chose_home")) {
		setf("bartolo_dead");
		setf("roc_wounded");
		SAY(T("Encuentras a tu madre acurrucada tras los barriles de la bodega, con una sartén en la mano. Viva."),
			M("¡Estás bien! ¡Estáis bien!"),
			T("Cuando volvéis a la plaza, Roc ha sacado a Nil de debajo del carro... con un brazo destrozado por las garras."),
			T("Y frente a la Catedral, el cuerpo de Bartolo yace junto a su antorcha apagada."));
		SAY(Y("Bartolo..."), K("Aguantó solo. Hasta el final."));
		change_approval(M_KAELEN, 2);
	} else if (flag("chose_plaza")) {
		setf("bartolo_dead");
		setf("nil_alive");
		SAY(T("Levantáis el carro entre todos. Nil sale arrastrándose, llorando, pero entero."),
			N("nil", "¡Sabía que vendríais! ¡Lo sabía!"));
		if (flag("kaelen_saves_mother")) {
			SAY(T("Kaelen vuelve cojeando, con tu madre del brazo. Tiene un zarpazo en la espalda, pero sonríe."),
				K("Tu madre pega más fuerte que yo con la sartén. Que conste."));
			change_approval(M_KAELEN, 5);
		} else
			SAY(T("Cuando llegas a casa, tu madre está en el suelo, herida en el hombro, pero viva. Remei ya la está vendando."),
				M("Estoy bien... Hiciste bien. Nil es solo un niño."));
		SAY(T("Pero frente a la Catedral, Bartolo yace inmóvil junto a su antorcha apagada."), Y("No... Bartolo..."));
	} else if (flag("chose_cath")) {
		setf("nil_dead");
		SAY(T("Bartolo sigue en pie, jadeando, apoyado en su bastón."),
			N("bartolo", "Llegasteis... llegasteis a tiempo, muchachos."),
			T("Pero cuando la plaza queda en silencio, Roc se acerca despacio. Lleva a alguien pequeño en brazos."),
			N("roc", "No pude levantar el carro solo. No pude... Nil..."),
			T("Yara se tapa la boca. Kaelen aparta la mirada."));
		SAY(Y("Era solo un niño, {name}. Solo un niño."),
			T("Tu madre está a salvo: Remei la sacó de la bodega a tiempo. Pero ahora mismo eso no consuela a nadie."));
		change_approval(M_YARA, -6);
		change_yara_corruption(6);
	}
	wait(0.4f);
	brute_arrives();
}

static void after_brute(void) {
	begin();
	setf("brute_defeated");
	set_prop_sprite("cathedral", "cathedral_open");
	remove_prop("rubble");
	wait(0.4f);
	SAY(T("El bruto se desploma sobre los escombros... y con él, la entrada de la Catedral se viene abajo."),
		T("Donde antes había piedra, ahora hay un agujero negro."));
	static const s16 ST[4][2] = {{250, 330}, {500, 320}, {300, 400}, {460, 390}};
	Ent* ths[4];
	for (int k = 0; k < 4; ++k) {
		ths[k] = spawn_npc(sfmt("thin%d", k), ST[k][0], ST[k][1], 3, MODE_IDLE, "thrall");
		walk_to_async(ths[k], 368 + (int)((k - 1.5f) * 6), 214, 55 + k * 6, 0);
	}
	pan_to(368, 250, 0.8f);
	wait(2.2f);
	for (int k = 0; k < 4; ++k) ent_fade(ths[k], 0, 0.4f);
	wait(0.5f);
	for (int k = 0; k < 4; ++k) remove_npc(sfmt("thin%d", k));
	release_camera(0.4f);
	SAY(T("Los siervos supervivientes se arrastran hacia dentro, uno tras otro, como hormigas hacia la miel."),
		K("Se están metiendo en la Catedral."), Y("Van a por la Semilla. Si la encuentran..."));
	if (flag("bartolo_alive"))
		SAY(N("bartolo", "Escuchadme. Bajo el altar hay tres runas: Fe, Memoria y Sacrificio."),
			N("bartolo", "Si se encienden las tres, se abre el camino a la cripta. La Semilla os estará esperando."),
			N("bartolo", "Yo soy demasiado viejo. Pero vosotros... vosotros sois los Guardianes de Tortosa, ¿no?"));
	else
		SAY(N("remei", "Bartolo me dio esto hace años. Dijo que si algún día pasaba... que se lo diera a los chicos."),
			T("Remei te entrega un cuaderno gastado: el diario de Bartolo."),
			T("«Bajo el altar, tres runas: Fe, Memoria, Sacrificio. Enciéndelas y la cripta se abrirá. Protege la Semilla.»"));
	SAY(K("Pues vamos. Antes de que sea tarde."),
		T("(Consejo: abre el menú con START para guardar la partida. En la Catedral habrá enemigos.)"));
	end();
}

static void vn_ready(void) {
	place_npcs();
	if (flag("brute_defeated")) {
		set_prop_sprite("cathedral", "cathedral_open");
		remove_prop("rubble");
	}
	if (!flag("won_night1")) first_attack();
	else if (!flag("night_party")) party_forms();
	else if (!flag("night_post") && (flag("won_night_home") || flag("won_night_plaza") || flag("won_night_cath"))) after_choice();
	else if (flag("won_brute") && !flag("brute_defeated")) after_brute();
}

static int vn_npc(const char* id, LineBuf* o) {
	if (IS(id, "remei")) LB(o, N("remei", "Id con cuidado, niños. Yo me quedo con los heridos."));
	else if (IS(id, "nil")) LB(o, N("nil", "¿Vais a entrar ahí? ¡Sois los más valientes de todo Vaelmoor!"));
	else if (IS(id, "roc")) LB(o, N("roc", "Si salís de esa, os forjaré lo que queráis. Gratis. ...Casi gratis."));
	else if (IS(id, "bartolo")) LB(o, N("bartolo", "Fe, Memoria, Sacrificio. No lo olvidéis."));
	return o->n;
}

static int vn_lines(const char* id, LineBuf* o) {
	if (IS(id, "cathedral")) {
		if (flag("brute_defeated")) LB(o, T("La entrada de la Catedral está abierta. De dentro sale un aire helado."));
		else LB(o, T("Los escombros tiemblan. Algo al otro lado quiere salir... o algo aquí fuera quiere entrar."));
	} else if (IS(id, "well")) LB(o, T("El pozo. Alguien ha dejado un cubo lleno junto al brocal, para las llamas."));
	return o->n;
}

const StoryDef story_village_night = {.map = "village_night", .encounters_disabled = vn_no_enc, .on_map_ready = vn_ready,
	.npc_lines = vn_npc, .interact_lines = vn_lines};
