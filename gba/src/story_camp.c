// Guion del Claro de la Savia (port de elf_camp.gd)
#include "story.h"

static Ent* thorn;

static int ec_no_enc(void) { return 1; }

static int tears(void) { return flag("tear_stone") + flag("tear_crystal") + flag("tear_blood"); }

static void thorns(void) { thorn = scene_sprite("thorn_wall", 288, 30, 0, 0); }

static void to_ilvanis(void) {
	walk_party_to(marker_x("ilvanis"), marker_y("ilvanis") + 36, 60);
	ent_face(PLAYER, 3);
}

static void tears_ritual(void) {
	begin();
	to_ilvanis();
	SAY(N("ilvanis", "Piedra, cristal y sangre. Las Tres Lágrimas... Nunca pensé que volvería a verlas juntas."),
		T("Colocas las tres lágrimas en las raíces del Árbol Madre. La luz dorada se vuelve blanca."),
		T("Al norte, el Muro de Espinas cruje, se retuerce... y se abre como una flor marchita."));
	sfx("magic", -2.0f, 1.0f);
	set_tint_wait(333, 333, 282, 0.5f);
	int r, g, b;
	map_modulate(&r, &g, &b);
	set_tint_wait(r, g, b, 0.8f);
	if (thorn) { ent_remove(thorn); thorn = 0; }
	SAY(N("ilvanis", "Habéis visto el mundo que hay más allá de este bosque. Enanos, fantasmas, ejércitos."),
		N("ilvanis", "Todo lo que la Madre Raíz toca, se pudre. Y todo lo que habéis salvado por el camino, os mira ahora."),
		N("ilvanis", "El camino al Corazón está abierto. Recordad: la Semilla escucha. Luz... o sombra."),
		T("(Consejo: guarda la partida antes de partir. No habrá vuelta atrás fácil.)"));
	setf("ready_for_heart");
	end();
}

static void arrival(void) {
	setf("camp_arrived");
	begin();
	show_banner("Claro de la Savia");
	wait(0.5f);
	SAY(T("El bosque se abre de golpe en un claro dorado. Aquí la corrupción no ha llegado... todavía."),
		T("Tiendas de tela verde, farolillos colgando de las ramas y, en el centro, un árbol inmenso que brilla con luz propia."));
	if (G.race == RACE_ELF) SAY(N("elf_guard", "Por la Savia... ¿Una elfa... un elfo criado entre humanos? Bienvenid{o} a casa, herman{o}."));
	else if (G.race == RACE_DWARF)
		SAY(N("elf_guard", "Un enano. En el Claro. La Anciana debe de estar perdiendo el juicio."),
			T("Varios elfos te miran de reojo mientras pasas."));
	else SAY(N("elf_guard", "Humanos. Hacía cien años que no pisaba uno el Claro."));
	if (flag("elves_hostile")) SAY(N("elf_guard", "Y con el enano profanador, además. La Anciana os verá. Pero no esperéis sonrisas."));
	to_ilvanis();
	SAY(N("ilvanis", "Así que sois vosotros. Los niños que despertaron la Semilla."),
		N("ilvanis", "Soy Ilvanis, y he visto este bosque crecer durante seiscientos años. Nunca lo había visto enfermar así."),
		N("ilvanis", "Lo que os ataca no es una plaga. Es una raíz. La Madre Raíz: el primer brote de los Moronguls, que ha anidado en el Corazón del Bosque."),
		N("ilvanis", "Mientras viva, el bosque se pudrirá, y después vuestro pueblo, y después el mundo."),
		Y("¿Y la Semilla?"),
		N("ilvanis", "La Semilla es lo único que puede sanar el Corazón. Hay que plantarla donde late la Madre Raíz, una vez destruida."),
		N("ilvanis", "Pero la Semilla escucha a quien la lleva. Si su portador tiene el corazón turbio... la Semilla se enturbia con él."));
	if (flag("elves_hostile"))
		SAY(N("ilvanis", "El ritual requiere la Lágrima de la Savia. No se la daré a quien protege a un profanador."),
			N("ilvanis", "Sin ella, la Semilla pedirá algo más a cambio de sanar. Algo mucho más caro."),
			B("Por mi culpa... Lo siento, chicos."));
	else {
		setf("ritual_ok");
		SAY(N("ilvanis", "Tomad: la Lágrima de la Savia. Cuando plantéis la Semilla, dejadla caer sobre ella."),
			T("(Obtienes la Lágrima de la Savia.)"));
	}
	SAY(N("ilvanis", "Descansad esta noche en el Claro. Mañana, al alba, os abriré el camino al Corazón."),
		T("(Habla con los elfos, compra en el intercambio y descansa junto a la hoguera para pasar la noche.)"));
	end();
}

static void kaelen_night(void) {
	int r = rivalry();
	SAY(T("Kaelen está solo junto a la hoguera, afilando una espada élfica que no es suya."));
	if (r >= 15) {
		SAY(!flag("seed_kaelen") ? K("¿Sabes qué me dijo la Semilla hoy? No, da igual.")
				: K("La Semilla me habla, ¿sabes? Dice que soy fuerte. Que no necesito ir detrás de nadie."),
			K("Toda la vida igual. {name} decide, {name} elige, {name} es el héroe. Y Kaelen... Kaelen lleva las bolsas."),
			K("Ni siquiera Yara me mira ya."));
		int i = CHOOSE(&K("¿Nada que decir?"), "Tienes razón. Lo siento. Sin ti no habría llegado aquí.",
			"Deja de compadecerte. Esto no va de ti.", "Si tanto te molesta, puedes irte.");
		if (i == 0) {
			change_kaelen_rivalry(-18);
			change_approval(M_KAELEN, 8);
			SAY(K("..."), K("...Idiota. No hacía falta que lo dijeras. Pero... gracias."), T("Por primera vez en días, Kaelen sonríe de verdad."));
		} else if (i == 1) {
			change_kaelen_rivalry(14);
			SAY(K("No. Claro. Nunca va de mí."));
		} else {
			change_kaelen_rivalry(25);
			SAY(K("...Puede que lo haga."));
		}
		if (rivalry() >= 40) {
			setf("kaelen_left");
			G.comp[M_KAELEN].left = 1;
			leave_party(M_KAELEN);
			remove_npc("kaelen");
			SAY(T("Esa noche, cuando despiertas, el sitio de Kaelen junto al fuego está vacío."),
				T("Sus huellas se pierden hacia el norte. Hacia el Corazón."));
			if (flag("seed_kaelen")) {
				setf("seed_stolen");
				SAY(T("Y la Semilla se ha ido con él."), Y("No... Kaelen, ¿qué has hecho?"));
			}
		}
	} else if (r <= -20) {
		SAY(K("¿Te acuerdas del juramento de los Guardianes? Lo inventamos a los nueve años, en la Catedral."),
			K("«Juntos hasta el final, aunque el final sea feo.»"));
		int i2 = CHOOSE(0, "Juntos hasta el final, hermano.", "Éramos unos críos.", "Espero que el final no sea tan feo.");
		if (i2 == 0) {
			setf("kaelen_oath");
			change_kaelen_rivalry(-10);
			SAY(K("Hasta el final."), T("Chocáis los antebrazos, como cuando erais niños. Algo pesado se os quita de encima a los dos."));
		} else if (i2 == 1) {
			change_kaelen_rivalry(4);
			SAY(K("Ya. Supongo."));
		} else {
			setf("kaelen_oath");
			SAY(K("Ja. Si lo es, al menos lo será con estilo."));
		}
	} else {
		SAY(K("Mañana vamos a pelear contra una raíz gigante. Con espadas. Si Bartolo nos viera..."),
			K("Oye. Pase lo que pase... me alegro de que fueras tú. Y no otr{o}."));
		change_kaelen_rivalry(-4);
	}
}

static void yara_night(void) {
	SAY(T("Encuentras a Yara junto al lago de la luna, con los pies en el agua."));
	if (corruption() >= 15) {
		SAY(Y("¿Puedo contarte un secreto? Oigo la Semilla. Incluso cuando no la tengo cerca."),
			Y("Me dice que puedo curarlo todo. A Bartolo, a Nil, a todos. Que solo tengo que... aceptar lo que me ofrece."),
			T("Sus manos brillan con una luz violeta. Por un instante, sus ojos también."));
		int i = CHOOSE(&Y("¿Tú qué harías?"), "Esa voz miente, Yara. Tú eres más fuerte que ella.",
			"¿Y si tiene razón? Podrías salvar a mucha gente.", "Si pasa algo, estaré a tu lado.");
		if (i == 0) {
			change_yara_corruption(-22);
			change_approval(M_YARA, 5);
			SAY(Y("...Tienes razón. Gracias por recordármelo."), T("La luz violeta se apaga. Yara respira hondo, aliviada."));
		} else if (i == 1) {
			change_yara_corruption(22);
			SAY(Y("Sí... Sí. Podría. ¿Verdad que podría?"), T("Sonríe. No es su sonrisa de siempre."));
		} else {
			change_yara_corruption(-8);
			change_approval(M_YARA, 6);
			SAY(Y("Lo sé. Siempre lo has estado."));
		}
	}
	int ap = G.comp[M_YARA].approval;
	if (ap >= 12 && corruption() < 50) {
		SAY(Y("¿Sabes? De pequeña pensaba que me casaría con uno de los dos. Con Kaelen o contigo. Nunca supe con quién."),
			T("La luna se refleja en el agua. Yara te mira, esperando."));
		const char* opts[4] = {"Me alegra que al final sea conmigo. (Besarla)", "Eres mi mejor amiga, Yara. Siempre.",
			"Kaelen te quiere, ¿lo sabías?"};
		int n = 3;
		if (has(M_KAELEN) && rivalry() <= -20 && G.comp[M_YARA].kaelen_approval >= 20) opts[n++] = "¿Y si no tuvieras que elegir?";
		int j = choose_opts(opts, n, 0);
		if (j == 0) {
			setf("romance_yara");
			change_approval(M_YARA, 10);
			change_yara_corruption(-10);
			change_kaelen_rivalry(8);
			SAY(T("Yara se ríe bajito y te besa. Sabe a flor de luna."), Y("Tardabas mucho, {name}."));
		} else if (j == 1) {
			change_approval(M_YARA, 3);
			SAY(Y("Siempre. Los Guardianes de Tortosa, ¿no?"), T("Apoya la cabeza en tu hombro un rato."));
		} else if (j == 2) {
			change_yara_kaelen_approval(30);
			change_kaelen_rivalry(-6);
			SAY(Y("...Lo sé. Ese idiota nunca se atreve a decirlo."), Y("Gracias, {name}. De verdad."));
		} else {
			setf("romance_yara");
			setf("trio_ok");
			change_approval(M_YARA, 8);
			change_yara_kaelen_approval(20);
			SAY(Y("¿Qué quieres decir...?"), T("Kaelen, que lo ha oído todo desde detrás de un árbol, carraspea."),
				K("Eh... Hola. Pasaba por aquí. Casualmente."), Y("...¿Los tres?"), K("Siempre hemos sido tres, ¿no?"),
				T("Yara se ríe hasta que le salen lágrimas. Luego os abraza a los dos a la vez."));
		}
	} else if (ap < 12)
		SAY(Y("Mañana... ten cuidado, {name}. Aunque estemos un poco raros tú y yo, no quiero perderte."));
}

static void night(void) {
	begin();
	setf("camp_night");
	tod_set("night");
	fade_out(0.8f);
	set_tint(115, 123, 192, 0.01f);
	wait(0.3f);
	fade_in(0.8f);
	SAY(T("Cae la noche sobre el Claro. Las luciérnagas se encienden una a una."));
	if (has(M_KAELEN)) kaelen_night();
	if (has(M_YARA)) yara_night();
	if (has(M_AELIS)) {
		SAY(A("No duermes, {raza}. Yo tampoco. El Corazón llama a todos los que llevamos savia en la sangre."),
			A("Mañana, dispara primero y pregunta después."));
		change_approval(M_AELIS, 3);
	} else if (has(M_BROM))
		SAY(B("¿Sabes qué echo de menos? La piedra. Aquí todo es blando y verde y respira."),
			B("Mañana te cubro la espalda. Un enano paga sus deudas."));
	SAY(T("Al final, el sueño os vence a todos."));
	fade_out(1.0f);
	tod_set("day");
	heal_all();
	int r, g, b;
	map_modulate(&r, &g, &b);
	set_tint(r, g, b, 0.01f);
	wait(0.4f);
	fade_in(1.0f);
	SAY(T("Amanece. Ilvanis os espera junto al gran árbol."));
	to_ilvanis();
	SAY(N("ilvanis", "Tengo malas noticias. Esta noche la Madre Raíz ha sentido la Semilla... y ha cerrado el Corazón."),
		N("ilvanis", "Un Muro de Espinas se alza al norte. Ni el fuego ni el acero lo atraviesan. Lo he intentado."),
		Y("¿Entonces? ¿Esperamos a que el bosque se muera?"),
		N("ilvanis", "No. Hay una forma. Cuando Ysolde plantó la Semilla, tres pueblos lloraron con ella: las Tres Lágrimas de la Savia."),
		N("ilvanis", "La Lágrima de Piedra, en las Minas de Khazgurim, al este, bajo los montes de los enanos."),
		N("ilvanis", "La Lágrima de Cristal, en las Ruinas del Imperio de Selen, más allá del pantano del sur."),
		N("ilvanis", "Y la Lágrima de Sangre... la llevaba el último campeón del Paso del Sur. Hoy ese paso es un campamento de guerra morongul."),
		N("ilvanis", "Traedme las tres y el Muro caerá."),
		has(M_KAELEN) ? K("Tres mazmorras, tres tesoros. Por fin algo que suena a aventura de verdad.")
			: Y("Tres viajes. Sin Kaelen. Bueno... habrá que apañárselas."),
		T("(Acto IV: Las Tres Lágrimas. Salidas nuevas: el este lleva a los Montes; el sur, al Pantano de Selen.)"));
	setf("act4_started");
	thorns();
	end();
}

static void talk_ilvanis(void) {
	begin();
	const char* opts[4] = {"¿Qué es exactamente la Madre Raíz?", "¿Quién era la discípula Ysolde?"};
	int n = 2, pardon = -1;
	if (flag("brom_prisoner") && !flag("brom_pardoned") && !flag("asked_pardon")) { pardon = n; opts[n++] = "Te pido clemencia para Brom."; }
	opts[n++] = "Nada más.";
	Line pr = N("ilvanis", "¿Qué deseas saber?");
	int i = choose_opts(opts, n, &pr);
	if (i == 0)
		SAY(N("ilvanis", "Cuando el Archimago rasgó el Velo, algo se coló por la herida: una podredumbre viva, hambrienta."),
			N("ilvanis", "Crece como una raíz. Cada Morongul es un brote suyo. La Madre Raíz es la primera, la más antigua."),
			N("ilvanis", "Y alguien la ha despertado. Sospecho de la Torre Negra... siempre están donde huele a poder."));
	else if (i == 1)
		SAY(N("ilvanis", "Ysolde fue mi maestra. Humana. Plantó la Semilla para coser el Velo y murió al hacerlo."),
			N("ilvanis", "La tierra le pidió un corazón. Ella le dio el suyo."), Y("...«La tierra pide un corazón»."));
	else if (i == pardon) {
		setf("asked_pardon");
		int can = G.race == RACE_ELF || G.comp[M_YARA].approval >= 12 || (flag("aelis_joined") && G.comp[M_AELIS].approval >= 5);
		if (can) {
			setf("brom_pardoned");
			SAY(N("ilvanis", "...Hablas con sinceridad. Y el enano talaba un árbol que ya estaba muerto."),
				N("ilvanis", "Sea. Será libre de volver a su montaña. Pero no con vosotros: su pueblo debe saber lo que ocurre bajo sus minas."),
				T("Más tarde, Brom se despide con un abrazo que casi te rompe las costillas."),
				B("¡Os debo la vida dos veces! Si algún día pisáis Khazgurim, preguntad por Brom. ¡Cerveza gratis de por vida!"));
			remove_npc("brom");
		} else
			SAY(N("ilvanis", "La ley de la Savia no se dobla por unas palabras bonitas, {raza}."),
				N("ilvanis", "Quizá, cuando el Corazón sane, lo reconsidere."));
	} else SAY(N("ilvanis", "Descansad. Mañana será el día más largo de vuestras vidas."));
	end();
}

static void ec_ready(void) {
	thorn = 0;
	spawn_npc("ilvanis", marker_x("ilvanis"), marker_y("ilvanis"), 0, MODE_IDLE, 0);
	spawn_npc("elf_guard", marker_x("shop"), marker_y("shop"), 3, MODE_IDLE, 0);
	if (flag("brom_prisoner") && !flag("brom_pardoned")) spawn_npc("brom", 500, 320, 0, MODE_IDLE, 0);
	if (flag("camp_night") && !flag("act4_started")) tod_set("night");
	if (flag("act4_started") && !flag("ready_for_heart")) thorns();
	if (!flag("camp_arrived")) arrival();
	else if (flag("act4_started") && !flag("ready_for_heart") && tears() == 3) tears_ritual();
}

static void ec_interact(const char* id) {
	if (IS(id, "campfire")) {
		if (flag("camp_arrived") && !flag("camp_night")) {
			begin();
			int i = CHOOSET("¿Descansáis hasta mañana?", "Pasar la noche", "Todavía no");
			end();
			if (i == 0) night();
			return;
		}
		rest_and_save("La hoguera élfica crepita con llamas verdes.");
		return;
	}
	if (IS(id, "elf_shop")) { open_shop("elves"); return; }
	base_on_interact(id);
}

static void ec_on_npc(const char* id) {
	if (IS(id, "elf_guard")) {
		begin();
		int i = CHOOSE(&N("elf_guard", "Intercambio élfico. Aceptamos coronas humanas, qué remedio."), "Comprar", "Nada");
		end();
		if (i == 0) open_shop("elves");
		return;
	}
	if (IS(id, "ilvanis")) { talk_ilvanis(); return; }
	if (IS(id, "brom") && flag("brom_prisoner") && !flag("brom_pardoned")) {
		begin();
		SAY(B("Vaya, vaya. Los que me vendieron. ¿Venís a ver al enano enjaulado?"),
			B("...Bah. No os guardo rencor. Mucho. Hablad con la vieja, a ver si os escucha a vosotros."));
		end();
		return;
	}
	base_on_npc(id);
}

static int ec_npc(const char* id, LineBuf* o) {
	if (IS(id, "brom")) LB(o, B("¡Ja!"));
	return o->n;
}

static int ec_lines(const char* id, LineBuf* o) {
	if (IS(id, "sacred_tree")) {
		if (flag("act4_started") && !flag("ready_for_heart"))
			LB(o, T(sfmt("Las raíces del Árbol Madre tienen tres huecos con forma de lágrima. (Lágrimas: %d/3)", tears())));
		else LB(o, T("El Árbol Madre de la Corte de la Savia. Su luz dorada pulsa despacio, como si respirara contigo."));
	} else if (IS(id, "tent1") || IS(id, "tent2") || IS(id, "tent3")) LB(o, T("Una tienda élfica. Huele a resina y a té de menta."));
	return o->n;
}

const StoryDef story_elf_camp = {.map = "elf_camp", .encounters_disabled = ec_no_enc, .on_map_ready = ec_ready,
	.on_interact = ec_interact, .on_npc = ec_on_npc, .npc_lines = ec_npc, .interact_lines = ec_lines};
