// Guion de Tortosa de día y al anochecer (port de village.gd)
#include "story.h"

static int dusk(void) { return tod_is("dusk"); }

static const char* v_req(const char* to, const char* def) { return IS(to, "forest") && dusk() ? "never" : def; }
static const char* v_blocked(const char* to, const char* def) {
	return IS(to, "forest") && dusk() ? "Ya anochece. Nadie en su sano juicio entraría ahora en el bosque." : def;
}

static Ent* at_marker(const char* id, const char* mk, int face, int mode) {
	return spawn_npc(id, marker_x(mk), marker_y(mk), face, mode, 0);
}

static void dusk_ready(void) {
	at_marker("remei", "remei", 1, MODE_IDLE);
	at_marker("roc", "roc", 1, MODE_IDLE);
	Ent* nil = spawn_npc("nil", 300, 330, 0, MODE_WANDER, 0);
	nil->wradius = 40;
	if (flag("council_done")) {
		spawn_npc("bartolo", 350, 290, 0, MODE_IDLE, 0);
		spawn_npc("mother", 180, 380, 3, MODE_IDLE, 0);
		return;
	}
	spawn_npc("bartolo", 368, 292, 0, MODE_IDLE, 0);
	Ent* mom = spawn_npc("mother", 330, 300, 2, MODE_IDLE, 0);
	begin();
	show_banner("Tortosa, al anochecer");
	wait(0.6f);
	walk_party_to(380, 330, 70);
	ent_face(PLAYER, 3);
	SAY(T("En la plaza se ha reunido medio pueblo. Kaelen enseña a todos un frasco con la savia negra del lobo."),
		N("bartolo", "Savia podrida... Así empezó, según contaba mi abuelo. Primero los animales. Luego el bosque. Luego..."),
		N("roc", "Luego nada, Bartolo. Es un lobo enfermo. Mañana lo quemamos y en paz."),
		N("bartolo", "¿Y las runas de la piedra santa? Las he visto apagarse desde mi ventana, noche tras noche."),
		Y("Es verdad. Hoy apenas brillaban."),
		N("bartolo", "Escuchadme bien, muchachos. La Catedral no está en ruinas por casualidad. Guarda algo: la Semilla del Pacto."),
		N("bartolo", "Los primeros de Tortosa la custodiaron bajo el altar. Mientras esté a salvo, el bosque aguanta. Si algo la busca..."),
		K("¿Algo como qué?"),
		N("bartolo", "Como lo que vive en las Tierras del Sur. Los viejos los llamaban Moronguls. Los que pudren todo lo que tocan."));
	int i = CHOOSE(&N("bartolo", "¿Tú qué dices, {hijo} de Elena?"), "Te creo, Bartolo. Deberíamos vigilar la Catedral.",
		"Son cuentos, abuelo. No asustes a la gente.", "Sea lo que sea, estaremos preparados.");
	if (i == 0) {
		setf("believed_bartolo");
		change_approval(M_YARA, 3);
		SAY(N("bartolo", "Por fin alguien con sesera."), K("Genial. Ahora tendremos guardia en unas ruinas."));
	} else if (i == 1) {
		change_approval(M_YARA, -2);
		change_kaelen_rivalry(-2);
		SAY(N("bartolo", "Ojalá tengas razón, criatura. Ojalá."), K("¡Eso digo yo!"));
	} else {
		change_approval(M_KAELEN, 2);
		SAY(N("roc", "Así me gusta. Mi herrería está abierta: si queréis pociones o algo mejor, pasad antes de dormir."));
	}
	ent_face_towards(mom, ent_x(PLAYER), ent_y(PLAYER));
	SAY(M("Ya basta de historias por hoy. Todos a casa. {name}, tú también, que mañana hay que ir al molino."),
		Y("Buenas noches, {name}. Nos vemos mañana... si Bartolo no tiene razón."),
		K("Si mañana hay Moronguls, me pido el más feo."),
		T("Kaelen y Yara se marchan a sus casas."));
	leave_party(M_KAELEN);
	leave_party(M_YARA);
	remove_npc("kaelen");
	remove_npc("yara");
	setf("council_done");
	SAY(T("(Puedes comprar en la herrería de Maese Roc antes de irte a dormir. Tu cama te espera en casa.)"));
	end();
}

static void v_ready(void) {
	if (dusk()) { dusk_ready(); return; }
	Ent* bart = at_marker("bartolo", "bartolo", 3, MODE_IDLE);
	at_marker("remei", "remei", 1, MODE_IDLE);
	at_marker("roc", "roc", 1, MODE_IDLE);
	Ent* nil = at_marker("nil", "nil", 0, MODE_WANDER);
	nil->wradius = 60;
	ent_face(bart, 3);
	if (flag("kaelen_joined")) return;
	Ent* kae = at_marker("kaelen", "kaelen_start", 1, MODE_IDLE);
	if (!spawn_is("home_door")) return;
	begin();
	wait(0.5f);
	ent_face_towards(kae, ent_x(PLAYER), ent_y(PLAYER));
	ent_hop(kae);
	SAY(K("¡{name}! ¡Eh, {name}! ¡Aquí!"));
	walk_to(kae, ent_x(PLAYER) + 26, ent_y(PLAYER) + 18, 80, 0);
	ent_face(kae, 1);
	ent_face(PLAYER, 2);
	SAY(K("¡Por fin! Llevo media mañana tirando piedras al río para no morirme de aburrimiento."),
		K("En este pueblo nunca pasa nada. Nada. El viejo Bartolo dice que la Catedral está maldita, pero lleva cuarenta años diciendo lo mismo."),
		K("Yara se ha ido al Bosque Santo a por sus flores. Vamos a buscarla."));
	int i = CHOOSE(&K("¿Vienes o te vas a quedar ahí plantad{o}?"), "¡Claro! Como en los viejos tiempos.",
		"¿Otra vez? Siempre acabas metiéndonos en líos.", "Vale, pero esta vez no te pierdas.");
	if (i == 0) {
		change_kaelen_rivalry(-8);
		change_approval(M_KAELEN, 5);
		SAY(K("¡Esa es la actitud! Sabía que no me fallarías."));
	} else if (i == 1) {
		change_kaelen_rivalry(6);
		SAY(K("Y siempre salimos de ellos, ¿no? Bueno... casi siempre."),
			K("No seas aguafiestas. Si pasa algo, yo te cubro. Como siempre."),
			T("Lo dice sonriendo, pero hay algo de pique en su voz."));
	} else {
		change_approval(M_KAELEN, 3);
		change_kaelen_rivalry(-3);
		SAY(K("¡Fue UNA vez! Y tenía diez años. Y la niebla era muy espesa."), K("...Y no se lo cuentes a Yara. Otra vez."));
	}
	SAY(K("El sendero del bosque sale por el este, pasado el cartel. ¡Vamos!"), T("Kaelen se une al grupo."));
	setf("kaelen_joined");
	join(M_KAELEN);
	end();
}

static int v_lines(const char* id, LineBuf* o) {
	if (IS(id, "cathedral")) {
		if (dusk()) LB(o, T("La Catedral Vieja se recorta contra el cielo rojo. Por un instante te parece ver una luz azulada entre los escombros."));
		else CYCLE(o, "cathedral", OPT(T("La Catedral Vieja."), T("Sus torres rotas se alzan sobre Tortosa como los dedos de un gigante dormido."),
				T("La gran puerta está sellada por los escombros desde antes de que naciera nadie del pueblo.")),
			OPT(T("Entre las piedras caídas se cuela un aire frío que huele a humedad... y a algo más antiguo.")));
	} else if (IS(id, "well")) LB(o, T("El pozo de la plaza. El agua está helada y tiene un regusto a hierro que antes no tenía."));
	else if (IS(id, "stall")) LB(o, T("El puesto de la plaza está cerrado. Hoy medio pueblo está en el monte."));
	else if (IS(id, "sign_forest")) LB(o, T("«→ Bosque Santo. Que la luz del pacto os guarde.»"), T("Alguien ha grabado debajo: «Nil estuvo aquí»."));
	else if (IS(id, "anvil")) LB(o, T("El yunque de Maese Roc. Todavía está tibio."));
	else if (IS(id, "smithy")) LB(o, T("La herrería de Maese Roc. Herraduras, azadas y, colgada en la pared, una espada de verdad que nunca vende."));
	else if (IS(id, "house_kaelen")) LB(o, T("La casa de Kaelen. Su padre está en el monte con las cabras."));
	else if (IS(id, "house_brown")) LB(o, T("La casa de los Ferrer, los molineros. Huele a harina y a manzanas asadas."));
	else if (IS(id, "house_south")) LB(o, T("La casa de la Tía Remei. En la ventana, un gato naranja te juzga en silencio."));
	else if (IS(id, "house_player")) LB(o, T("Tu casa."));
	return o->n;
}

static void v_on_npc(const char* id) {
	if (IS(id, "roc") && flag("act2_started")) {
		begin();
		Ent* n = npc("roc");
		if (n) ent_face_towards(n, ent_x(PLAYER), ent_y(PLAYER));
		int i = CHOOSE(&N("roc", "¿Qué te pongo? Pociones, pan, y si tienes coronas, acero de verdad."), "Comprar", "Hablar", "Nada");
		end();
		if (i == 0) open_shop("roc");
		else if (i == 1) {
			begin();
			SAY(N("roc", "Los mercaderes del sur hablaban de pueblos vacíos. Bah. ...Aunque les compré un hacha, por si acaso."));
			end();
		}
		return;
	}
	base_on_npc(id);
}

static int v_npc_lines(const char* id, LineBuf* o) {
	if (IS(id, "bartolo")) {
		if (dusk()) LB(o, N("bartolo", "Esta noche no pienso dormir. Me quedaré mirando la Catedral."));
		else CYCLE(o, "bartolo",
			OPT(N("bartolo", "¿La Catedral? Nadie sabe quién la levantó. Mi abuelo decía que ya estaba en ruinas cuando llegaron los primeros de Tortosa.")),
			OPT(N("bartolo", "Algunas noches se oyen campanas ahí dentro. Y esa catedral no tiene campanas desde hace siglos.")));
	} else if (IS(id, "remei"))
		CYCLE(o, "remei", OPT(N("remei", "El agua del pozo está más fría estos días. Y sabe a hierro. Qué cosas.")),
			OPT(N("remei", "Mi gato no quiere salir de casa. Ese bicho no le teme a nada... salvo a lo que yo no veo.")));
	else if (IS(id, "roc"))
		CYCLE(o, "roc", OPT(N("roc", "Han pasado mercaderes del sur contando cosas feas. Pueblos vacíos, bosques que se pudren en una noche.")),
			OPT(N("roc", "Si algún día necesitas una espada de verdad, ven a verme. Pero trae algo más que sonrisas.")));
	else if (IS(id, "nil"))
		CYCLE(o, "nil", OPT(N("nil", "¿Vais al bosque? ¡Mi madre dice que no se puede pasar de la piedra santa!")),
			OPT(N("nil", "¡Cuando sea mayor seré más fuerte que Kaelen! ¡Y más listo que Yara! ¡Y más...! ¡Más todo!")));
	else if (IS(id, "mother")) LB(o, M("Vamos, a casa. Que ya es de noche."));
	return o->n;
}

const StoryDef story_village = {.map = "village", .on_map_ready = v_ready, .exit_requirement = v_req,
	.exit_blocked_message = v_blocked, .interact_lines = v_lines, .on_npc = v_on_npc, .npc_lines = v_npc_lines};
