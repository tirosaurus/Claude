// Guiones del dormitorio y la casa (port de bedroom.gd y housemain.gd)
#include "story.h"

// ------------------------------------------------------------------ dormitorio
static const char* bed_music(void) { return tod_is("night") ? "night" : ""; }

static void bed_ready(void) {
	if (!flag("woke_up")) {
		setf("woke_up");
		begin();
		ent_face(PLAYER, 0);
		wait(0.9f);
		SAY(T("La luz de la mañana se cuela entre las contraventanas y te da de lleno en la cara."),
			T("Desde abajo llega olor a pan recién hecho y el tintineo de los cacharros de tu madre."),
			T("Otro día más en Tortosa... o eso crees."));
		end();
		return;
	}
	if (flag("night_attack") && !flag("night_woke")) {
		setf("night_woke");
		begin();
		wait(0.8f);
		sfx("growl", -2.0f, 0.8f);
		wait(0.5f);
		SAY(T("Un grito te arranca del sueño."),
			T("Luego otro. Y otro más. El cielo, al otro lado de la ventana, es de color naranja."),
			T("Huele a humo."));
		ent_face(PLAYER, 2);
		SAY(P("¿Qué...? ¡Madre!"));
		end();
	}
}

static int bed_lines(const char* id, LineBuf* o) {
	int night = tod_is("night");
	if (IS(id, "bed")) {
		if (night) LB(o, T("No es momento de dormir. ¡Algo está pasando ahí fuera!"));
		else CYCLE(o, "bed", OPT(T("Tu cama. Todavía está calentita..."), T("...pero ya has dormido más que suficiente. Hoy hace un día precioso.")),
			OPT(T("Ya has dormido bastante. ¡Arriba, que el día no espera!")));
	} else if (IS(id, "nightstand")) {
		LB(o, T("Una vela casi consumida y un libro abierto: «Leyendas del Imperio de Cristal»."),
			T("Anoche te quedaste leyendo hasta tarde. Otra vez."));
	} else if (IS(id, "wardrobe")) {
		LB(o, T("Tu ropa de siempre y la capa que heredaste de tu padre."), T("Nada digno de un héroe... todavía."));
	} else if (IS(id, "chest")) {
		CYCLE(o, "chest", OPT(T("Tu cofre de tesoros de la infancia."),
				T("Una piedra con forma de corazón, una pluma de cuervo y un dibujo hecho por Yara:"),
				T("vosotros tres delante de la Catedral Vieja, y debajo, en letras torcidas: «LOS GUARDIANES DE TORTOSA»."),
				T("Sonríes sin querer.")),
			OPT(T("«Los Guardianes de Tortosa». Kaelen insistió en que él era el capitán.")));
	} else if (IS(id, "desk")) {
		LB(o, T("Un mapa de Vaelmoor copiado a mano. Tortosa es un puntito en los montes del norte."),
			T("El resto del mundo parece enorme... y lleno de nombres que suenan a aventura."));
	} else if (IS(id, "window")) {
		if (night) LB(o, T("Por la ventana ves llamas sobre los tejados de Tortosa. Sombras deformes corren entre las casas."));
		else LB(o, T("Desde la ventana se ve la Catedral Vieja, enorme y rota sobre los tejados."),
			T("Más allá, el verde oscuro del Bosque Santo se pierde hacia el este."));
	} else if (IS(id, "sword")) {
		LB(o, T("La espada de madera que Kaelen te talló cuando teníais ocho años."),
			T("Tiene más muescas que filo. Aun así, nunca has sido capaz de tirarla."));
	}
	return o->n;
}

static void bed_interact(const char* id) {
	if (IS(id, "bed") && flag("council_done") && !flag("night_attack")) {
		begin();
		int i = CHOOSET("¿Te acuestas? Mañana será un día largo.", "Dormir", "Todavía no");
		if (i == 0) {
			setf("night_attack");
			tod_set("night");
			heal_all();
			SAY(T("Te tumbas. Tardas en dormirte: no dejas de pensar en la savia negra, en los ojos del lobo..."));
			end();
			music_stop(1.2f);
			fade_out(1.2f);
			wait(1.0f);
			goto_world("bedroom", "start", 0);
		}
		end();
		return;
	}
	base_on_interact(id);
}

const StoryDef story_bedroom = {.map = "bedroom", .music_override = bed_music, .on_map_ready = bed_ready,
	.interact_lines = bed_lines, .on_interact = bed_interact};

// ------------------------------------------------------------------ casa (planta baja)
static const char* house_music(void) {
	if (flag("night_attack") && !flag("night_over")) return "night";
	if (flag("night_over")) return "sad";
	return "";
}

static void house_ready(void) {
	if (flag("night_over")) {
		Ent* mom = spawn_npc("mother", 150, 96, 0, MODE_IDLE, 0);
		if (flag("mother_wounded")) ent_face(mom, 0);
		return;
	}
	if (flag("night_attack")) {
		if (flag("mother_hidden")) return;
		Ent* mom2 = spawn_npc("mother", marker_x("mother"), marker_y("mother"), 2, MODE_IDLE, 0);
		if (!spawn_is("from_upstairs")) return;
		begin();
		wait(0.3f);
		walk_to(PLAYER, ent_x(PLAYER), ent_y(PLAYER) + 14, 60, 0);
		ent_face_towards(mom2, ent_x(PLAYER), ent_y(PLAYER));
		ent_hop(mom2);
		SAY(M("¡{name}! ¡Gracias a los dioses, estás bien!"),
			M("Algo ha salido del bosque. Cosas... cosas negras. Han prendido fuego a la casa de los Ferrer."),
			M("He oído a Kaelen gritar tu nombre hacia la plaza. Y a Yara, junto al pozo."));
		int i = CHOOSE(&M("¿Qué hacemos?"), "Escóndete en la bodega. Voy a buscarlos.", "Ven conmigo, no pienso dejarte sola.",
			"Madre... ¿tú sabías que esto podía pasar?");
		if (i == 0)
			SAY(M("Ten cuidado, cariño. Y... coge esto."),
				T("Te da la vieja espada de tu padre, envuelta en un trapo. Pesa más de lo que recordabas."),
				M("Él habría querido que la tuvieras."));
		else if (i == 1)
			SAY(M("Solo te estorbaría. Soy vieja, no tonta. Me esconderé en la bodega."),
				M("Pero coge esto: era de tu padre."), T("La vieja espada familiar. Pesa más de lo que recordabas."));
		else {
			setf("asked_mother");
			SAY(M("..."), M("Tu abuela decía que la Catedral guardaba algo. Que si el bosque callaba, vendrían."),
				M("Pensé que eran cuentos de vieja. Toma: la espada de tu padre. Y vete, ¡vete!"));
		}
		setf("mother_hidden");
		add_item("pocion", 2);
		SAY(T("Obtienes 2 pociones del armario de la cocina."));
		walk_to(mom2, 62, 96, 60, 0);
		walk_to(mom2, 62, 136, 60, 0);
		SAY(T("Tu madre levanta la trampilla de la bodega y baja, sin dejar de mirarte."));
		fade_npc("mother", 0.5f);
		end();
		return;
	}
	Ent* mom3 = spawn_npc("mother", marker_x("mother"), marker_y("mother"), 1, MODE_IDLE, 0);
	if (tod_is("dusk") || flag("mother_talked")) return;
	if (!spawn_is("from_upstairs")) return;
	begin();
	wait(0.3f);
	walk_to(PLAYER, ent_x(PLAYER), ent_y(PLAYER) + 14, 50, 0);
	ent_face_towards(mom3, ent_x(PLAYER), ent_y(PLAYER));
	ent_hop(mom3);
	SAY(M("¡Mira quién se ha despertado! Ya pensaba que tendría que subir con el cubo de agua fría."));
	walk_to(mom3, ent_x(PLAYER) - 44, ent_y(PLAYER) + 10, 50, 0);
	ent_face_towards(mom3, ent_x(PLAYER), ent_y(PLAYER));
	ent_face(PLAYER, 1);
	SAY(M("Hoy hace muy buen día, de esos que ya casi no quedan."));
	int i2 = CHOOSE(&M("¿Has descansado bien, cariño?"), "Como un tronco, madre.", "He tenido un sueño muy raro...",
		"¿Eso es pan recién hecho?");
	if (i2 == 2 && flag("ov1") && !flag("ov2")) {
		setf("ov2");
		sfx("magic", -24.0f, 0.5f);
	}
	if (i2 == 0) SAY(M("Eso es que tienes la conciencia tranquila. Disfrútalo mientras dure."));
	else if (i2 == 1) {
		setf("dream_told");
		SAY(M("¿Raro? ¿Raro cómo?"),
			T("Recuerdas raíces negras creciendo bajo el bosque, latiendo como venas... y una voz que susurraba tu nombre."),
			P("Nada... tonterías. Raíces, oscuridad... ya se me está olvidando."),
			M("Bah, será la cena de anoche. Te dije que no repitieras queso."),
			T("Por un instante, a tu madre se le borra la sonrisa. Luego vuelve a remover el guiso como si nada."));
	} else SAY(M("De hogaza, con la harina del molino de los Ferrer. Coge un trozo, anda, que estás en los huesos."));
	SAY(M("Por cierto, Kaelen ha pasado esta mañana preguntando por ti. Tres veces."),
		M("Anda, sal a que te dé el aire. Y volved antes de que anochezca, ¿eh? Últimamente el bosque... bueno, tú vuelve pronto."));
	setf("mother_talked");
	end();
}

static int house_lines(const char* id, LineBuf* o) {
	if (IS(id, "fireplace")) {
		if (flag("night_attack") && !flag("night_over")) LB(o, T("El fuego de la chimenea sigue encendido, como si nada pasara ahí fuera."));
		else LB(o, T("El guiso de tu madre burbujea al fuego. Huele a romero y a tomillo del monte."));
	} else if (IS(id, "kitchen_table")) {
		if (!flag("ate_bread")) {
			setf("ate_bread");
			heal_all();
			add_item("pan", 1);
			LB(o, T("Pan de hogaza, queso de cabra y un tazón de leche."),
				T("Comes algo y te guardas un trozo de pan en el zurrón. (Pan de hogaza ×1)"));
		} else LB(o, T("Ya has comido. Si comes más, tu madre te hará subir leña como pago."));
	} else if (IS(id, "cupboard")) {
		LB(o, T("Tarros de miel, hierbas secas y un libro de recetas con la letra apretada de tu abuela."),
			T("En la última página, alguien escribió: «Si el bosque calla, cerrad las puertas»."));
	}
	return o->n;
}

static int house_npc(const char* id, LineBuf* o) {
	if (!IS(id, "mother")) return 0;
	if (flag("night_over")) {
		if (flag("mother_wounded"))
			CYCLE(o, "mom_dawn_w", OPT(M("No es nada, un rasguño... Remei dice que en una semana estaré como nueva."),
					M("Ve. Haz lo que tengas que hacer. Pero vuelve, ¿me oyes? Vuelve.")),
				OPT(M("Tu padre estaría orgulloso. Y aterrado. Como yo.")));
		else
			CYCLE(o, "mom_dawn", OPT(M("Estoy bien, cariño. Gracias a ti."),
					M("Si tenéis que ir al bosque de los elfos... llévate provisiones. Y vuelve.")),
				OPT(M("Tu padre estaría orgulloso de ti. Y aterrado. Como yo.")));
	} else if (tod_is("dusk"))
		CYCLE(o, "mom_dusk", OPT(M("¿Una reunión en la plaza? Ay... Bartolo y sus cuentos. Pero ve, ve."),
				M("Y luego a dormir, que tienes una cara que da miedo.")),
			OPT(M("Tu cama está hecha. Descansa esta noche, ¿vale?")));
	else
		CYCLE(o, "mother", OPT(M("¿Todavía aquí? Kaelen va a echar la puerta abajo.")),
			OPT(M("Coge algo de pan de la mesa si tienes hambre.")),
			OPT(M("Y dile a Yara que me guarde unas flores de luna, que me quedan pocas.")));
	return o->n;
}

const StoryDef story_housemain = {.map = "housemain", .music_override = house_music, .on_map_ready = house_ready,
	.interact_lines = house_lines, .npc_lines = house_npc};
