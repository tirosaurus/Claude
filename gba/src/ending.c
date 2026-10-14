// Final: calcula el desenlace según las decisiones y lo narra en diapositivas (port de Ending.gd)
#include "world.h"
#include "ui.h"
#include "audio.h"
#include "scene.h"
#include "dialogue.h"
#include <stdio.h>

static int f(const char* k) { return flag(k); }

static int kaelen_heroic_death(void) { return f("kaelen_held") && !f("kaelen_oath") && !f("brom_helps_kaelen"); }

static int score(void) {
	int s = 0;
	if (f("bartolo_dead")) s--;
	if (f("nil_dead")) s--;
	if (f("kaelen_lost")) s--;
	if (f("kaelen_dead")) s -= 2;
	if (f("yara_lost")) s -= 2;
	if (f("brom_prisoner") && !f("brom_pardoned")) s--;
	if (kaelen_heroic_death()) s--;
	if (f("kaelen_oath") || f("kaelen_redeemed")) s++;
	if (f("romance_yara")) s++;
	return s;
}

typedef struct { const char* id; const char* cat; const char* title; } EndingInfo;

static EndingInfo ending(void) {
	if (f("ending_vael")) return (EndingInfo){"vael", "Final supremo", "Vael, el Dios Protector"};
	if (f("ending_sealed")) return (EndingInfo){"sealed", "Final secreto verdadero", "El Velo sellado"};
	if (f("ending_sovereign")) return (EndingInfo){"sovereign", "Final secreto", "El Nuevo Soberano"};
	if (f("pact_accepted")) return (EndingInfo){"pact", "Final malo", "La paz de la Torre"};
	if (f("ending_king")) return (EndingInfo){"king", "Final fatal", "El Rey de la Savia"};
	if (f("ending_withered")) return (EndingInfo){"withered", "Final fatal", "El bosque que se apagó"};
	if (f("ending_sacrifice")) return (EndingInfo){"sacrifice", "Final agridulce", "La tierra pidió un corazón"};
	int s = score();
	int kaelen_ok = !f("kaelen_dead") && !f("kaelen_lost") && !kaelen_heroic_death();
	if (s >= 1 && kaelen_ok && !f("yara_lost")) return (EndingInfo){"best", "Final muy bueno", "El alba de Tortosa"};
	if (s >= -2 && !f("kaelen_dead") && !f("yara_lost")) return (EndingInfo){"good", "Final bueno", "Cicatrices y brotes"};
	return (EndingInfo){"bitter", "Final malo", "Una victoria amarga"};
}

static const char* tortosa_fate(const char* id) {
	if (!strcmp(id, "bitter")) return "Tortosa sobrevivió, pero a medias. Demasiadas casas vacías, demasiadas tumbas nuevas junto a la Catedral.";
	if (!strcmp(id, "best"))
		return "Tortosa se reconstruyó en un verano. Esta vez, con una torre de vigía junto a la Catedral y la puerta de la cripta sellada con tres runas nuevas.";
	return "Tortosa se reconstruyó despacio. Las cicatrices del incendio se ven aún en los muros, como recordatorio.";
}

static const char* kaelen_fate(void) {
	if (f("kaelen_dead")) return "Kaelen descansa en el Corazón del Bosque. Nadie en Tortosa sabe cómo murió. Tú nunca se lo has contado a nadie.";
	if (f("kaelen_lost"))
		return "Dicen que un espadachín de ojos violetas vaga por las Tierras Quebradas, cazando Moronguls sin descanso. Nunca volvió a Tortosa.";
	if (kaelen_heroic_death())
		return "Kaelen nunca salió del Corazón. En el paso donde contuvo a los siervos crece hoy un árbol blanco. Los niños de Tortosa lo llaman «el Capitán».";
	if (f("kaelen_held"))
		return "Kaelen salió del Corazón al amanecer, cubierto de savia, cojeando... y sonriendo. «Juntos hasta el final», dijo. «Y el final no ha sido tan feo.»";
	if (f("kaelen_redeemed"))
		return "Kaelen carga todavía con una sombra en la mirada. Partió hacia el sur, a cazar los brotes de la Madre Raíz. Te escribe cada luna llena.";
	if (f("kaelen_oath")) return "Kaelen se convirtió en capitán de la guardia de Tortosa. Aún discute contigo por todo. Aún gana muy pocas veces.";
	return "Kaelen se marchó a Puerto Ceniza a unirse al Gremio de Aventureros, como siempre soñó. Vuelve cada invierno con historias exageradas.";
}

static const char* yara_fate(void) {
	if (f("yara_eaten"))
		return "De Yara solo quedó una flor de luna, caída en el camino del Corazón. Tu madre la guarda en un vaso junto a la ventana. Nunca habláis de ello.";
	if (f("yara_lost"))
		return "Yara no regresó. Los leñadores hablan de una bruja en lo más oscuro del bosque, que cura a los animales heridos... y castiga a los que talan.";
	int witch = corruption() >= 40;
	int kaelen_alive = !f("kaelen_dead") && !f("kaelen_lost") && !kaelen_heroic_death();
	if (f("trio_ok") && kaelen_alive)
		return "Yara, Kaelen y tú compartís una casa junto al molino. En Tortosa murmuran. A vosotros os da exactamente igual.";
	if (f("romance_yara"))
		return sfmt("Yara y tú os casasteis la primavera siguiente, bajo el Árbol Madre de los elfos. Ahora huele a flor de luna toda la casa.%s",
			witch ? " Sus manos, a veces, brillan violeta en sueños." : "");
	if (G.comp[M_YARA].romance_target == ROM_KAELEN && kaelen_alive)
		return "Yara y Kaelen se casaron dos años después. Tú fuiste quien los empujó a dejar de hacer el tonto. Te lo agradecen cada vez que te ven.";
	if (witch) return "Yara sigue curando en Tortosa, aunque ahora sus remedios huelen a tierra oscura. Nadie se atreve a preguntarle por qué.";
	return "Yara se convirtió en la mejor curandera del norte. Los elfos la invitan cada otoño a enseñar sus ungüentos en el Claro.";
}

static const char* companion_fate(void) {
	if (f("aelis_joined"))
		return "Aelis volvió a la Corte de la Savia como heroína. Cada año cruza el bosque para visitar Tortosa y burlarse de vuestra puntería.";
	if (f("brom_joined") && !f("kaelen_held"))
		return "Brom volvió a Khazgurim y convenció a su pueblo de sellar las grietas bajo las minas. Te manda un barril de cerveza cada año.";
	if (f("brom_helps_kaelen"))
		return "Brom salió del Corazón junto a Kaelen, cargándolo a hombros. Desde entonces se llaman «hermano de hacha» y nadie entiende por qué.";
	if (f("brom_pardoned"))
		return "Brom regresó a su montaña gracias a tu petición de clemencia. Dicen que en Khazgurim hay una taberna llamada «Los Guardianes».";
	if (f("brom_prisoner"))
		return "Brom pasó un año en las celdas de la Corte de la Savia antes de ser liberado. Nunca volvió a hablar con humanos.";
	return 0;
}

static const char* village_fate(void) {
	return sfmt("%s %s %s",
		f("nil_dead") ? "En la plaza hay una pequeña estatua de un niño con una espada de madera: Nil."
			: "Nil empezó como aprendiz en la herrería de Roc. Dice a todo el mundo que será un Guardián.",
		f("bartolo_dead") ? "La casa de Bartolo guarda ahora sus cuentos, escritos por Remei para que nadie los olvide."
			: "Bartolo sigue contando historias en la plaza. Ahora todo el pueblo le escucha.",
		f("mother_wounded") ? "Tu madre nunca recuperó del todo el brazo, pero sigue haciendo el mejor pan del norte."
			: "Tu madre sigue haciendo el mejor pan del norte, y sigue esperándote con la cena caliente.");
}

static void lower(char* s) {
	for (; *s; ++s) if (*s >= 'A' && *s <= 'Z') *s += 32;
}

static const char* player_fate(const char* id) {
	const char* n = player_name();
	if (!strcmp(id, "sacrifice"))
		return sfmt("%s no volvió a Tortosa. Pero cada primavera, junto a la piedra santa, florece un árbol blanco que no estaba ahí antes.", n);
	char cls[40], race[24];
	snprintf(cls, sizeof cls, "%s", !strcmp(class_name_str(), "Sin clase") ? branch_name() : class_name_str());
	snprintf(race, sizeof race, "%s", race_name());
	lower(cls);
	lower(race);
	return sfmt("%s, %s, %s de Tortosa, siguió protegiendo el norte. Pero esa... es otra historia.", n, race, cls);
}

#define MAX_SLIDES 14
extern u8 g_scratch[];
#define slides ((char (*)[420])g_scratch)   // el mundo no está activo durante el final
static int nslides;
static void add(const char* s) {
	if (!s || nslides >= MAX_SLIDES) return;
	snprintf(slides[nslides++], 420, "%s", s);
}

static void build_slides(const char* id) {
	const char* n = player_name();
	nslides = 0;
	int choco = f("aleixolo_done") && !f("player_dark");
	if (!strcmp(id, "vael")) {
		if (f("vael_guardian")) {
			add(sfmt("%s se quedó en el Corazón del Bosque, convertid%s en luz. Desde entonces, el Velo no ha vuelto a abrirse ni un milímetro.", n, gfm("o", "a")));
			add("Los elfos lo llaman «el Guardián». Los niños de Tortosa, simplemente, «Vael». Cada primavera le dejan pan recién hecho junto a la piedra santa.");
		} else {
			add(sfmt("%s volvió a Tortosa caminando, con la ropa rota y los ojos llenos de luz. Su madre le esperaba en la puerta con la cena caliente.", n));
			add("Nadie en el pueblo sabe que la persona que ayuda a Nil en la herrería es un dios. Y así está bien.");
		}
		add("Sin Nhal'Zur, la Torre Negra se derrumbó. Los Moronguls volvieron al sur, recordando de pronto quiénes habían sido.");
		if (choco) add("Aleixolo abrió una panadería en Tortosa. Es el único negocio del mundo protegido por un dios. El dios va todas las mañanas a por bollos.");
		add("Has conseguido el final más difícil de Vaelmoor.");
		add("Y un último secreto, para tu próxima vida: «Aquel acero dormía en lo más pequeño. Cama, armario, cama y cofre. Pregunta a tu madre por el pan. Mira tres veces el pozo de la plaza. Y antes del primer lobo, toca la piedra santa.»");
	} else if (!strcmp(id, "sealed")) {
		add("El Velo se cerró con el alma de Vael, el Primer Soberano. Nhal'Zur volvió al hambre sin fondo de la que había salido.");
		add("Sin su señor, la Torre Negra se derrumbó en una sola noche. Los Moronguls regresaron al sur, confusos, como quien despierta de un sueño.");
		add("La Semilla floreció en el Corazón del Bosque. En Tortosa, las campanas de la Catedral volvieron a sonar después de mil años.");
		add(sfmt("Nadie supo nunca qué pasó de verdad en el Corazón. Solo %s, y un nombre que ya nadie olvidaría: Vael.", n));
		if (choco) add("Ah, y en Tortosa abrió una panadería nueva. Su dueño, un tal Aleixolo, jura que esta vez no se comerá el género. Lleva tres semanas sin vender una sola barra.");
	} else if (!strcmp(id, "sovereign")) {
		add(sfmt("%s volvió a Tortosa con una corona de cristal negro. Nadie se atrevió a preguntar.", n));
		add("El bosque sanó. La Torre Negra cayó. Los Moronguls juraron lealtad al nuevo Soberano de Vaelmoor.");
		add("Hubo paz. Una paz perfecta, absoluta, que nadie se atrevía a discutir.");
		add("Y a veces, por las noches, el Soberano oye un susurro en el acero: «Tengo hambre». Cada año, un poco más alto.");
		if (f("aleixolo_done")) add("El nuevo Repostero Real de la corte es un tal Aleixolo. Ha engordado doce kilos y no ha horneado ni una sola tarta. Nadie se atreve a despedirle.");
	} else if (!strcmp(id, "pact")) {
		add("La Madre Raíz se durmió aquella noche, tal y como prometió el Emisario. Los Moronguls se retiraron al sur.");
		add("Una semana después, las banderas negras de la Torre ondeaban sobre la Catedral de Tortosa.");
		add("Los vecinos pagan ahora un tributo en grano y en hijos. Nadie habla de lo que ocurre en la cripta por las noches.");
		add(sfmt("Te llaman «%s %s». Lo dicen en voz baja, y nunca como un elogio.", n, is_female() ? "la Sensata" : "el Sensato"));
	} else if (!strcmp(id, "king")) {
		add("El bosque se inclinó ante su nuevo señor.");
		add("Las raíces negras cubrieron Tortosa en una sola noche. Nadie gritó: ya no quedaba nadie que pudiera gritar.");
		add("En el Corazón del Bosque late ahora un corazón nuevo. Tiene tu cara. Tiene tu voz.");
		if (f("kaelen_corrupt")) add("A tu derecha camina siempre un caballero de piel gris y ojos violetas. Nunca habla de Yara. Nunca habla de nada.");
		add("Y cada noche, al sur, la Torre Negra observa... y sonríe.");
	} else if (!strcmp(id, "withered")) {
		add("La Semilla se marchitó en tus manos. Sin corazón que la sostuviera, la luz se apagó.");
		add("La Madre Raíz había muerto, pero la herida seguía abierta. El bosque se secó, árbol a árbol, durante años.");
		add("Tortosa resistió dos inviernos. Al tercero, sus últimos vecinos partieron hacia el sur, huyendo de la podredumbre.");
		add("La Catedral quedó vacía. Esperando a alguien más valiente... o más desesperado.");
	} else if (!strcmp(id, "sacrifice")) {
		add("Abrazaste la Semilla y dejaste que te atravesara. Como Ysolde, trescientos años antes.");
		add("De tu cuerpo brotó un árbol blanco que partió en dos la Madre Raíz. El Corazón del Bosque volvió a latir, verde y sano.");
		add("Los Moronguls se retiraron. El pacto se renovó. La tierra había recibido su corazón.");
	}
	if (!strcmp(id, "good") || !strcmp(id, "best") || !strcmp(id, "bitter") || !strcmp(id, "sacrifice")) {
		if (strcmp(id, "sacrifice"))
			add("La Semilla echó raíces. En una sola mañana, el Corazón del Bosque se llenó de brotes verdes y la Madre Raíz se deshizo en ceniza.");
		add(tortosa_fate(id));
		add(kaelen_fate());
		add(yara_fate());
		add(companion_fate());
		add(village_fate());
		if (choco) add("Dicen que en Tortosa hay una panadería nueva, regentada por un señor muy redondo con gorro de cocinero. El pan es buenísimo... cuando llega al mostrador, que es casi nunca.");
		add(player_fate(id));
	}
	add(sfmt("Mientras tanto, en el sur, en lo alto de la Torre Negra, alguien anota un nombre en un libro muy antiguo: %s.", n));
}

// ------------------------------------------------------------------ dibujo
static void draw_centered_wrapped(int y_center, int maxw, const char* text, int color, int outline) {
	char buf[700];
	wrap_text(text, maxw, buf, sizeof buf);
	const char* lines[12];
	int n = 0;
	char* p = buf;
	lines[n++] = p;
	while (*p && n < 12) {
		if (*p == '\n') { *p = 0; lines[n++] = p + 1; }
		++p;
	}
	int lh = 12;
	int y = y_center - n * lh / 2;
	for (int i = 0; i < n; ++i) ui_text_center(0, y + i * lh, 240, lines[i], color, outline);
}

static int wait_or_skip(float hold) {
	int n = (int)(hold * 60);
	for (int i = 0; i < n; ++i) {
		frame_wait();
		if (key_hit(KEY_A)) return 1;
	}
	return 0;
}

static void slide(const char* title, const char* text, float hold) {
	ui_clear();
	if (title && title[0]) ui_text_center(0, 26, 240, title, UC_DARK_GOLD, UC_DARK_OUT);
	draw_centered_wrapped(title && title[0] ? 90 : 78, 216, text, UC_DARK_WHITE, UC_DARK_OUT);
	ui_text_right(0, 148, 236, "A: continuar", UC_DARK_DIM, -1);
	fade_in(0.6f);
	input_flush();
	wait_or_skip(hold);
	fade_out(0.5f);
}

static void summary(EndingInfo e) {
	ui_clear();
	ui_text_center(0, 6, 240, sfmt("%s: %s", e.cat, e.title), UC_DARK_GOLD, UC_DARK_OUT);
	ui_panel(NP_PANEL, PAL_UI_LIGHT, 2, 3, 26, 13);
	const char* k = "—";
	if (G.comp[M_KAELEN].alive && !f("kaelen_lost")) k = kaelen_bond_label();
	char kb[40];
	snprintf(kb, sizeof kb, "%s", k);
	const char* txt = sfmt("%s · %s · %s · Nivel %ld\nTiempo de juego: %s\n\nKaelen: %s\nYara: %s\n\nHay otros finales esperándote. ¿Qué habría pasado si hubieras elegido distinto?",
		player_name(), race_name(), class_name_str(), (long)G.level, format_time(G.play_frames), kb,
		f("yara_lost") ? "Perdida en el bosque" : yara_bond_label());
	ui_text_wrapped(28, 32, 184, 11, txt, UC_LIGHT_TEXT, -1, 8);
	ui_button(9, 16, 12, 2, "Volver al título", 1, 0, 0);
	ui_text_center(0, 150, 240, "Vaelmoor · Hecho con Claude · Pixelify Sans (OFL)", UC_DARK_DIM, -1);
	fade_in(0.6f);
	input_flush();
	for (;;) {
		frame_wait();
		if (key_hit(KEY_A) || key_hit(KEY_START)) {
			sfx("confirm", -8.0f, 1.0f);
			fade_out(0.6f);
			scene_request(SC_TITLE);
			block_forever();
		}
	}
}

static void hook(void) {
	oam_begin();
	ui_frame_sprites();
	oam_end();
}

void ending_scene(void* arg) {
	(void)arg;
	g_vid.dispcnt = DCNT_BLANK;
	fade_set(16, 0);
	objt_reset();
	objpal_reset();
	ui_init();
	ui_load_palettes(1);
	ui_sprites_reset();
	g_ui_free_pal = PAL_UI_DARK;
	EndingInfo e = ending();
#ifdef AUTOTEST
	{ extern void at_ending(const char* id); at_ending(e.id); }
#endif
	build_slides(e.id);
	int good = !strcmp(e.id, "best") || !strcmp(e.id, "good") || !strcmp(e.id, "sacrifice") || !strcmp(e.id, "sealed") || !strcmp(e.id, "vael");
	meta_set_s("ending_id", e.id);
	bg_image_load(good ? &bg_ending_dawn : &bg_title, 1, 28, 1);
	g_vid.bgcnt[1] = (1 << 2) | (28 << 8) | (1 << 14) | 3;
	g_vid.hofs[1] = 40;
	g_vid.vofs[1] = 10;
	if (good) world_tint(141, 128, 141);
	else world_tint(90, 72, 102);
	g_frame_hook = hook;
	g_vid.dispcnt = DCNT_MODE0 | DCNT_OBJ1D | DCNT_BG0 | DCNT_BG1 | DCNT_OBJ;
	g_trans_busy = 0;
	music_play(good ? "ending_good" : "ending_bad", 1.5f);
	slide(e.cat, e.title, 4.0f);
	for (int i = 0; i < nslides; ++i) slide("", slides[i], 7.0f);
	summary(e);
}
