// Guiones de la Cueva de los Susurros, los Montes y las Minas de Khazgurim (port de cave.gd, montes.gd y minas.gd)
#include "story.h"

// ------------------------------------------------------------------ Cueva de los Susurros (mazmorra opcional)
extern void abyss_enter(void);

static void after_guardian(void) {
	begin();
	setf("cave_guardian_done");
	SAY(T("El guardián se desmorona en un montón de cristal apagado."),
		T("Los susurros callan. Por primera vez en siglos, la cueva está en silencio."),
		K("Bueno. Ahora sí que me merezco lo que haya en esos cofres."));
	end();
}

static void cave_ready(void) {
	if (flag("won_cave_guardian") && !flag("cave_guardian_done")) { after_guardian(); return; }
	if (!flag("cave_seen")) {
		setf("cave_seen");
		begin();
		wait(0.5f);
		SAY(T("Una cueva húmeda, iluminada por cristales que laten como luciérnagas."),
			T("Desde el fondo llegan susurros. Palabras en una lengua que nadie habla desde hace siglos."),
			Y("Aquí dentro hay algo muy antiguo, {name}. Y no creo que quiera visitas."),
			K("Lo antiguo suele ser valioso. Y lo valioso, peligroso. Me encanta."),
			T("(Mazmorra opcional: enemigos duros y tesoros legendarios. Se recomienda nivel 8.)"));
		end();
	}
}

static void cave_trigger(const char* id) {
	if (IS(id, "guardian_zone") && !flag("won_cave_guardian")) {
		begin();
		music_stop(0.8f);
		SAY(T("La estatua del fondo gira la cabeza. Piedra contra piedra."),
			T("«Ermengol selló el Velo. Thrain abrió la montaña. Selen lloró a su emperador.»"),
			T("«Sus armas duermen aquí. Solo quien me venza podrá despertarlas.»"));
		battle("cave_guardian", ENEMIES("custodian", "spectre", "spectre"), "crypt", "boss", 1);
	}
}

static void cave_interact(const char* id) {
	if ((IS(id, "chest_cave4") || IS(id, "chest_cave5")) && !flag("won_cave_guardian")) {
		begin();
		SAY(T("Un sello de cristal rodea el cofre. Algo lo protege... la estatua del fondo te observa."));
		end();
		return;
	}
	base_on_interact(id);
}

static const char* cave_req(const char* to, const char* def) {
	if (starts_with(to, "abismo")) return flag("won_cave_guardian") && G.level >= MAX_LEVEL ? "" : "abyss_open";
	return def;
}

static const char* cave_blocked(const char* to, const char* def) {
	if (starts_with(to, "abismo")) {
		if (!flag("won_cave_guardian")) return "Unas escaleras bajan hacia una oscuridad que susurra. El guardián del fondo te impide acercarte.";
		return sfmt("Los susurros te rechazan: «Vuelve cuando hayas alcanzado tu máximo poder». (Necesitas nivel %d. Tienes nivel %ld.)",
			MAX_LEVEL, (long)G.level);
	}
	return def;
}

static void abyss_go(const char* a) { (void)a; abyss_enter(); }

static int cave_before_exit(const char* to) {
	if (starts_with(to, "abismo")) {
		story_call(abyss_go, "");
		return 0;
	}
	return 1;
}

static int cave_lines(const char* id, LineBuf* o) {
	if (IS(id, "statue_broken")) LB(o, T("Una estatua sin rostro. En su base, tres nombres: Ermengol, Thrain, Selen."));
	return o->n;
}

const StoryDef story_cave = {.map = "cave", .on_map_ready = cave_ready, .on_trigger = cave_trigger, .on_interact = cave_interact,
	.exit_requirement = cave_req, .exit_blocked_message = cave_blocked, .before_exit = cave_before_exit, .interact_lines = cave_lines};

// ------------------------------------------------------------------ Montes de Khazgurim
static const struct { const char* id; const char* name; u8 r, g, b, face; } DW[4] = {
	{"hilda", "Hilda, la minera", 255, 217, 191, 0},
	{"torbin", "Viejo Torbin", 204, 204, 217, 1},
	{"bori", "Bori", 255, 255, 230, 0},
	{"kazrik", "Kazrik, guardia de la mina", 191, 204, 255, 0},
};

static int dw_index(const char* id) {
	for (int i = 0; i < 4; ++i) if (IS(id, DW[i].id)) return i;
	return -1;
}

static void montes_ready(void) {
	spawn_npc("durgan", marker_x("durgan"), marker_y("durgan"), 0, MODE_IDLE, "brom");
	for (int i = 0; i < 4; ++i) {
		Ent* n = spawn_npc(DW[i].id, marker_x(DW[i].id), marker_y(DW[i].id), DW[i].face, MODE_IDLE, "brom");
		ent_set_tint(n, DW[i].r, DW[i].g, DW[i].b);
	}
	if (!flag("montes_seen")) {
		setf("montes_seen");
		begin();
		show_banner("Montes de Khazgurim");
		wait(0.5f);
		SAY(T("El bosque se acaba de golpe. Delante, montañas grises con las cumbres nevadas y un viento que corta."),
			T("En la ladera, la boca de una mina con vigas de roble y farolillos apagados."));
		if (has(M_BROM))
			SAY(B("Khazgurim... Hace diez inviernos que no pisaba mi tierra."),
				B("Algo va mal. Las forjas deberían humear. Se oiría el martillo desde aquí."));
		else SAY(Y("Esto está demasiado silencioso para ser una mina enana, ¿no?"));
		end();
	}
}

static void dwarf_lines(int i, LineBuf* o) {
	const char* who = DW[i].name;
	int done = flag("tear_stone"), saved = flag("dwarves_saved");
	switch (i) {
	case 0:
		if (!done) LB(o, N(who, "Mi pico se quedó dentro, junto a mi hermano. El pico me da igual. Mi hermano no."),
			N(who, "Si bajas, ve por la galería del este. Los golpes que se oyen son nuestros chicos."));
		else if (saved) LB(o, N(who, "¡Mi hermano está vivo! Toma, no es mucho, pero es lo mejor que tengo: un abrazo de enana. Cuidado con las costillas."));
		else LB(o, N(who, "...Sellaste la galería. Lo sé, lo sé. La savia. Era lo correcto."), N(who, "Déjame sola un rato, por favor."));
		break;
	case 1:
		if (!done) LB(o, N(who, "Ochenta años picando piedra y nunca vi una veta sangrar. Eso que hay ahí abajo no es de este mundo, muchacho."),
			N(who, "Thrain, el fundador, decía que bajo Khazgurim duerme una lágrima de la tierra. Siempre creí que era un cuento."));
		else LB(o, N(who, "Así que el cuento era verdad. La Lágrima de Piedra... Mi abuelo estaría orgulloso de haberla visto."),
			N(who, "Ve con cuidado, y no gastes la suerte enana toda de golpe."));
		break;
	case 2:
		if (!done) LB(o, N(who, "¿Tú vas a matar al gusano gigante? ¿De verdad? ¿Me traes un diente?"),
			N(who, "Mi papá dice que los humanos son blandos como el pan. ¡Tú no pareces de pan!"));
		else if (saved) LB(o, N(who, "¡Mi papá ha vuelto! ¡Dice que eres la persona menos de pan que ha visto nunca!"));
		else LB(o, N(who, "Mi papá todavía no ha vuelto de la mina... ¿Tú sabes cuándo vuelve?"));
		break;
	case 3:
		if (!done) {
			if (has(M_BROM)) LB(o, N(who, "¿Brom Barbarroja? ¡Por las barbas de Thrain! Pasa, pasa. Y tus amigos también, supongo."),
				B("Kazrik, sigues teniendo cara de yunque."), N(who, "Y tú de cerveza rancia. Bienvenido a casa."));
			else LB(o, N(who, "Alto. Nadie entra en la mina sin... bah. Nadie quiere entrar en la mina. Adelante, valientes."));
		} else LB(o, N(who, "Desde que cayó el gusano, la montaña ha dejado de temblar. Volvemos a oír el martillo. Gracias, forastero."));
		break;
	}
}

static void durgan(void) {
	begin();
	if (flag("tear_stone")) {
		if (flag("dwarves_saved")) SAY(N("durgan", "¡Mi hijo está en casa gracias a ti! Khazgurim no olvida. Si alguna vez necesitas un hacha, aquí tienes cien."));
		else SAY(N("durgan", "Hiciste lo que tenías que hacer. La montaña sigue en pie. No te lo reprocho... pero no me pidas que sonría."));
		end();
		return;
	}
	if (flag("durgan_met")) {
		SAY(N("durgan", "La mina está al norte. Que Thrain guíe tu hacha. O tu lo que sea que lleves."));
		end();
		return;
	}
	setf("durgan_met");
	SAY(N("durgan", "¡Alto ahí! ... Bah, no sois moronguls. Los moronguls no huelen a flores."),
		N("durgan", "Soy Durgan, capataz de Khazgurim. O de lo que queda."),
		N("durgan", "Hace una luna, cavando en la veta honda, rompimos una pared. Detrás había savia negra... y un gusano del tamaño de una torre."),
		N("durgan", "Se tragó la galería entera. Media cuadrilla quedó atrapada dentro. Mi hijo entre ellos."));
	if (flag("brom_pardoned") && !has(M_BROM))
		SAY(N("durgan", "Y un tal Brom, que volvió hace poco del bosque hablando de elfos. Entró a buscarlos solo, el muy cabezota."));
	else if (has(M_BROM))
		SAY(N("durgan", "¿Brom? ¿Brom Barbarroja? ¡Te dábamos por muerto, bribón!"), B("Casi, Durgan. Casi. Estos chicos me salvaron el pellejo."));
	SAY(P("Buscamos la Lágrima de Piedra."),
		N("durgan", "La Lágrima... está en el santuario de Thrain, al fondo de la mina. Justo donde duerme el gusano."),
		N("durgan", "Matad a esa bestia y es vuestra. Y si podéis... sacad a mis chicos."));
	end();
}

static void montes_on_npc(const char* id) {
	if (IS(id, "durgan")) { durgan(); return; }
	int i = dw_index(id);
	if (i >= 0) {
		Ent* n = npc(id);
		if (n) ent_face_towards(n, ent_x(PLAYER), ent_y(PLAYER));
		LineBuf lb;
		lb.n = 0;
		dwarf_lines(i, &lb);
		begin();
		say_lines(lb.l, lb.n);
		end();
		return;
	}
	base_on_npc(id);
}

static int montes_lines(const char* id, LineBuf* o) {
	if (IS(id, "sign_montes")) LB(o, T("«Khazgurim. Minas de Thrain. Visitantes: llamad fuerte. Moronguls: dad media vuelta.»"));
	else if (IS(id, "dwarf_forge")) LB(o, T("Una forja enana. Aunque la mina esté en peligro, aquí nadie deja que se apague el fuego."));
	return o->n;
}

const StoryDef story_montes = {.map = "montes", .on_map_ready = montes_ready, .on_npc = montes_on_npc, .interact_lines = montes_lines};

// ------------------------------------------------------------------ Minas de Khazgurim
static const char* minas_music(void) { return flag("tear_stone") ? "mountain" : ""; }

static void after_worm(void) {
	begin();
	SAY(T("El gusano se desploma con un gemido que hace temblar la montaña entera."),
		T("Por la galería del este se oyen golpes. Voces. «¡Aquí! ¡Estamos aquí!»"),
		T("Los mineros atrapados. Pero el techo cruje: la savia del gusano sigue manando por la grieta y se extiende por las vetas."));
	int i = CHOOSE(&T("La savia avanza. No hay tiempo para las dos cosas."), "Sacar a los mineros primero (el túnel puede ceder)",
		"Sellar la grieta ya (los mineros quedarán dentro)");
	if (i == 0) {
		setf("dwarves_saved");
		SAY(T("Apartáis roca a roca mientras la savia os quema los tobillos."),
			T("Uno a uno salen los mineros: sucios, hambrientos, vivos."));
		if (flag("brom_pardoned") && !has(M_BROM)) {
			SAY(B("¡Sabía que vendríais! ¡Os lo dije, cerveza gratis de por vida!"),
				B("Y esta vez voy con vosotros. Khazgurim puede apañárselas sin mí un poco más."));
			join(M_BROM);
		}
		SAY(T("Al final, la grieta se cierra sola, taponada por el cuerpo del gusano. Habéis tenido suerte."));
		change_approval(M_YARA, 3);
		change_yara_corruption(-3);
	} else {
		setf("dwarves_sealed");
		SAY(T("Derrumbáis la galería del este sobre la grieta. El polvo lo cubre todo."),
			T("Los golpes al otro lado se hacen más débiles... y callan."));
		if (has(M_YARA)) SAY(Y("...Había gente ahí, {name}."));
		if (has(M_KAELEN)) SAY(K("La savia habría llegado a la montaña entera. Era lo correcto. Creo."));
		change_approval(M_YARA, -3);
		change_approval(M_KAELEN, 2);
		change_yara_corruption(3);
	}
	SAY(T("Tomas la gema del altar. Pesa como una montaña y tiembla como un corazón."), T("(Obtienes la Lágrima de Piedra. 1 de 3.)"));
	setf("tear_stone");
	sfx("magic", -4.0f, 1.0f);
	place_waystone();
	SAY(T("Junto a ti, una piedra rúnica se enciende con luz azul. (Piedra de retorno: te lleva al Claro de la Savia cuando quieras.)"));
	end();
}

static void minas_ready(void) {
	if (flag("won_worm") && !flag("tear_stone")) { after_worm(); return; }
	if (!flag("minas_seen")) {
		setf("minas_seen");
		begin();
		wait(0.4f);
		SAY(T("Galerías de piedra tallada, vías de vagoneta y forjas que aún guardan brasas."),
			T("El suelo tiembla de vez en cuando. Algo enorme se mueve bajo vuestros pies."),
			T("(Mazmorra: Minas de Khazgurim. Busca el santuario de Thrain, al norte.)"));
		end();
	}
}

static void minas_trigger(const char* id) {
	if (IS(id, "worm_zone") && !flag("won_worm")) {
		begin();
		music_stop(0.8f);
		shake(6.0f, 1.0f);
		SAY(T("El santuario de Thrain. Un altar de piedra con una gema parda, lisa como una lágrima."),
			T("La roca detrás del altar revienta. Una boca redonda llena de dientes, goteando savia negra."),
			has(M_KAELEN) ? K("Vale. Eso es un gusano. Un gusano muy, muy grande.") : Y("Eso... es un gusano. Un gusano enorme. {name}, ¡cuidado!"));
		if (has(M_BROM)) SAY(B("¡Por Thrain y por Khazgurim! ¡Hoy como gusano a la brasa!"));
		battle("worm", ENEMIES("gusano"), "crypt", "boss", 1);
	}
}

static void minas_interact(const char* id) {
	if (IS(id, "chest_mina3") && !flag("won_worm")) {
		begin();
		SAY(T("El cofre está enterrado bajo escombros... y algo enorme respira detrás de la pared."));
		end();
		return;
	}
	if (IS(id, "trapped_dwarves")) {
		begin();
		if (flag("dwarves_saved")) SAY(T("La galería está despejada. Los mineros se han ido a casa."));
		else if (flag("dwarves_sealed")) SAY(T("Roca y silencio."));
		else SAY(T("Una galería derrumbada. Se oyen golpes débiles al otro lado: «¡Socorro!»"),
			T("Imposible abrirla con el gusano haciendo temblar la montaña."));
		end();
		return;
	}
	if (IS(id, "forge")) { rest_and_save("Las brasas de la forja aún calientan."); return; }
	base_on_interact(id);
}

const StoryDef story_minas = {.map = "minas", .music_override = minas_music, .on_map_ready = minas_ready, .on_trigger = minas_trigger,
	.on_interact = minas_interact};
