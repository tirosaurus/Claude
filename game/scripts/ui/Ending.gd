extends Control
## Calcula el final según las decisiones y lo narra en diapositivas.

var _bg: TextureRect
var _title: Label
var _text: Label
var _next := false
var _good := true
var _hint: Label


func f(flag_name: String) -> bool:
	return GameState.has_flag(flag_name)


func _ready() -> void:
	var dark := ColorRect.new()
	dark.color = Color(0.05, 0.04, 0.07)
	dark.set_anchors_preset(Control.PRESET_FULL_RECT)
	add_child(dark)
	_bg = TextureRect.new()
	_bg.size = Vector2(640, 360)
	_bg.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
	_bg.stretch_mode = TextureRect.STRETCH_SCALE
	add_child(_bg)
	_title = UIKit.label("", 30, Color(0.98, 0.86, 0.55), 8)
	_title.position = Vector2(0, 40)
	_title.size = Vector2(640, 80)
	_title.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	_title.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	add_child(_title)
	_text = UIKit.label("", 17, Color(0.95, 0.92, 0.85), 5)
	_text.position = Vector2(60, 130)
	_text.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	_text.size = Vector2(520, 180)
	_text.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	_text.vertical_alignment = VERTICAL_ALIGNMENT_CENTER
	add_child(_text)
	_hint = UIKit.label("E: continuar", 12, Color(0.6, 0.58, 0.65), 3)
	_hint.position = Vector2(560, 338)
	add_child(_hint)
	_run.call_deferred()


func _process(_d: float) -> void:
	if Input.is_action_just_pressed("interact"):
		_next = true


func _score() -> int:
	var s := 0
	for fl in ["bartolo_dead", "nil_dead", "kaelen_lost"]:
		if f(fl):
			s -= 1
	if f("kaelen_dead"):
		s -= 2
	if f("yara_lost"):
		s -= 2
	if f("brom_prisoner") and not f("brom_pardoned"):
		s -= 1
	if _kaelen_heroic_death():
		s -= 1
	if f("kaelen_oath") or f("kaelen_redeemed"):
		s += 1
	if f("romance_yara"):
		s += 1
	return s


func _kaelen_heroic_death() -> bool:
	return f("kaelen_held") and not f("kaelen_oath") and not f("brom_helps_kaelen")


func _ending() -> Array:
	# [id, categoría, título]
	if f("pact_accepted"):
		return ["pact", "Final malo", "La paz de la Torre"]
	if f("ending_king"):
		return ["king", "Final fatal", "El Rey de la Savia"]
	if f("ending_withered"):
		return ["withered", "Final fatal", "El bosque que se apagó"]
	if f("ending_sacrifice"):
		return ["sacrifice", "Final agridulce", "La tierra pidió un corazón"]
	var s := _score()
	var kaelen_ok := not f("kaelen_dead") and not f("kaelen_lost") and not _kaelen_heroic_death()
	if s >= 1 and kaelen_ok and not f("yara_lost"):
		return ["best", "Final muy bueno", "El alba de Tortosa"]
	if s >= -2 and not f("kaelen_dead") and not f("yara_lost"):
		return ["good", "Final bueno", "Cicatrices y brotes"]
	return ["bitter", "Final malo", "Una victoria amarga"]


func _slides(id: String) -> Array:
	var n := GameState.player_name()
	var out: Array = []
	match id:
		"pact":
			out.append("La Madre Raíz se durmió aquella noche, tal y como prometió el Emisario. Los Moronguls se retiraron al sur.")
			out.append("Una semana después, las banderas negras de la Torre ondeaban sobre la Catedral de Tortosa.")
			out.append("Los vecinos pagan ahora un tributo en grano y en hijos. Nadie habla de lo que ocurre en la cripta por las noches.")
			out.append("Te llaman «%s el Sensato». Lo dicen en voz baja, y nunca como un elogio." % n if not GameState.is_female() else "Te llaman «%s la Sensata». Lo dicen en voz baja, y nunca como un elogio." % n)
		"king":
			out.append("El bosque se inclinó ante su nuevo señor.")
			out.append("Las raíces negras cubrieron Tortosa en una sola noche. Nadie gritó: ya no quedaba nadie que pudiera gritar.")
			out.append("En el Corazón del Bosque late ahora un corazón nuevo. Tiene tu cara. Tiene tu voz.")
			out.append("Y cada noche, al sur, la Torre Negra observa... y sonríe.")
		"withered":
			out.append("La Semilla se marchitó en tus manos. Sin corazón que la sostuviera, la luz se apagó.")
			out.append("La Madre Raíz había muerto, pero la herida seguía abierta. El bosque se secó, árbol a árbol, durante años.")
			out.append("Tortosa resistió dos inviernos. Al tercero, sus últimos vecinos partieron hacia el sur, huyendo de la podredumbre.")
			out.append("La Catedral quedó vacía. Esperando a alguien más valiente... o más desesperado.")
		"sacrifice":
			out.append("Abrazaste la Semilla y dejaste que te atravesara. Como Ysolde, trescientos años antes.")
			out.append("De tu cuerpo brotó un árbol blanco que partió en dos la Madre Raíz. El Corazón del Bosque volvió a latir, verde y sano.")
			out.append("Los Moronguls se retiraron. El pacto se renovó. La tierra había recibido su corazón.")
	if id in ["good", "best", "bitter", "sacrifice"]:
		if id != "sacrifice":
			out.append("La Semilla echó raíces. En una sola mañana, el Corazón del Bosque se llenó de brotes verdes y la Madre Raíz se deshizo en ceniza.")
		out.append(_tortosa_fate(id))
		out.append(_kaelen_fate())
		var yf := _yara_fate()
		if yf != "":
			out.append(yf)
		var cf := _companion_fate()
		if cf != "":
			out.append(cf)
		out.append(_village_fate())
		out.append(_player_fate(id))
	out.append("Mientras tanto, en el sur, en lo alto de la Torre Negra, alguien anota un nombre en un libro muy antiguo: %s." % n)
	return out


func _tortosa_fate(id: String) -> String:
	if id == "bitter":
		return "Tortosa sobrevivió, pero a medias. Demasiadas casas vacías, demasiadas tumbas nuevas junto a la Catedral."
	if id == "best":
		return "Tortosa se reconstruyó en un verano. Esta vez, con una torre de vigía junto a la Catedral y la puerta de la cripta sellada con tres runas nuevas."
	return "Tortosa se reconstruyó despacio. Las cicatrices del incendio se ven aún en los muros, como recordatorio."


func _kaelen_fate() -> String:
	if f("kaelen_dead"):
		return "Kaelen descansa en el Corazón del Bosque. Nadie en Tortosa sabe cómo murió. Tú nunca se lo has contado a nadie."
	if f("kaelen_lost"):
		return "Dicen que un espadachín de ojos violetas vaga por las Tierras Quebradas, cazando Moronguls sin descanso. Nunca volvió a Tortosa."
	if _kaelen_heroic_death():
		return "Kaelen nunca salió del Corazón. En el paso donde contuvo a los siervos crece hoy un árbol blanco. Los niños de Tortosa lo llaman «el Capitán»."
	if f("kaelen_held"):
		return "Kaelen salió del Corazón al amanecer, cubierto de savia, cojeando... y sonriendo. «Juntos hasta el final», dijo. «Y el final no ha sido tan feo.»"
	if f("kaelen_redeemed"):
		return "Kaelen carga todavía con una sombra en la mirada. Partió hacia el sur, a cazar los brotes de la Madre Raíz. Te escribe cada luna llena."
	if f("kaelen_oath"):
		return "Kaelen se convirtió en capitán de la guardia de Tortosa. Aún discute contigo por todo. Aún gana muy pocas veces."
	return "Kaelen se marchó a Puerto Ceniza a unirse al Gremio de Aventureros, como siempre soñó. Vuelve cada invierno con historias exageradas."


func _yara_fate() -> String:
	var y: Dictionary = GameState.companions["yara"]
	if f("yara_lost"):
		return "Yara no regresó. Los leñadores hablan de una bruja en lo más oscuro del bosque, que cura a los animales heridos... y castiga a los que talan."
	var witch := GameState.corruption() >= 40
	if f("trio_ok") and not f("kaelen_dead") and not f("kaelen_lost") and not _kaelen_heroic_death():
		return "Yara, Kaelen y tú compartís una casa junto al molino. En Tortosa murmuran. A vosotros os da exactamente igual."
	if f("romance_yara"):
		var extra := " Sus manos, a veces, brillan violeta en sueños." if witch else ""
		return "Yara y tú os casasteis la primavera siguiente, bajo el Árbol Madre de los elfos. Ahora huele a flor de luna toda la casa." + extra
	if str(y.get("romance_target", "none")) == "kaelen" and not f("kaelen_dead") and not f("kaelen_lost") and not _kaelen_heroic_death():
		return "Yara y Kaelen se casaron dos años después. Tú fuiste quien los empujó a dejar de hacer el tonto. Te lo agradecen cada vez que te ven."
	if witch:
		return "Yara sigue curando en Tortosa, aunque ahora sus remedios huelen a tierra oscura. Nadie se atreve a preguntarle por qué."
	return "Yara se convirtió en la mejor curandera del norte. Los elfos la invitan cada otoño a enseñar sus ungüentos en el Claro."


func _companion_fate() -> String:
	if f("aelis_joined"):
		return "Aelis volvió a la Corte de la Savia como heroína. Cada año cruza el bosque para visitar Tortosa y burlarse de vuestra puntería."
	if f("brom_joined") and not f("kaelen_held"):
		return "Brom volvió a Khazgurim y convenció a su pueblo de sellar las grietas bajo las minas. Te manda un barril de cerveza cada año."
	if f("brom_helps_kaelen"):
		return "Brom salió del Corazón junto a Kaelen, cargándolo a hombros. Desde entonces se llaman «hermano de hacha» y nadie entiende por qué."
	if f("brom_pardoned"):
		return "Brom regresó a su montaña gracias a tu petición de clemencia. Dicen que en Khazgurim hay una taberna llamada «Los Guardianes»."
	if f("brom_prisoner"):
		return "Brom pasó un año en las celdas de la Corte de la Savia antes de ser liberado. Nunca volvió a hablar con humanos."
	return ""


func _village_fate() -> String:
	var parts: Array = []
	if f("nil_dead"):
		parts.append("En la plaza hay una pequeña estatua de un niño con una espada de madera: Nil.")
	else:
		parts.append("Nil empezó como aprendiz en la herrería de Roc. Dice a todo el mundo que será un Guardián.")
	if f("bartolo_dead"):
		parts.append("La casa de Bartolo guarda ahora sus cuentos, escritos por Remei para que nadie los olvide.")
	else:
		parts.append("Bartolo sigue contando historias en la plaza. Ahora todo el pueblo le escucha.")
	if f("mother_wounded"):
		parts.append("Tu madre nunca recuperó del todo el brazo, pero sigue haciendo el mejor pan del norte.")
	else:
		parts.append("Tu madre sigue haciendo el mejor pan del norte, y sigue esperándote con la cena caliente.")
	return " ".join(parts)


func _player_fate(id: String) -> String:
	var n := GameState.player_name()
	var cls := GameState.class_name_str()
	if cls == "Sin clase":
		cls = GameState.branch_name()
	if id == "sacrifice":
		return "%s no volvió a Tortosa. Pero cada primavera, junto a la piedra santa, florece un árbol blanco que no estaba ahí antes." % n
	return "%s, %s, %s de Tortosa, siguió protegiendo el norte. Pero esa... es otra historia." % [n, GameState.race_name().to_lower(), cls.to_lower()]


func _run() -> void:
	var e := _ending()
	_good = e[0] in ["best", "good", "sacrifice"]
	GameState.set_meta("ending_id", e[0])
	Audio.play_music("ending_good" if _good else "ending_bad", 1.5)
	_bg.texture = load("res://assets/bg/ending_dawn.png") if _good else load("res://assets/bg/title.png")
	_bg.modulate = Color(0.55, 0.5, 0.55) if _good else Color(0.35, 0.28, 0.4)
	_text.add_theme_font_size_override("font_size", 28)
	await _slide("%s" % e[1], e[2], 4.0)
	_text.add_theme_font_size_override("font_size", 17)
	for s in _slides(e[0]):
		await _slide("", s, 7.0)
	await _summary(e)


func _slide(title: String, text: String, hold: float) -> void:
	_title.text = title
	_text.text = text
	_title.modulate.a = 0.0
	_text.modulate.a = 0.0
	var tw := create_tween().set_parallel(true)
	tw.tween_property(_title, "modulate:a", 1.0, 0.8)
	tw.tween_property(_text, "modulate:a", 1.0, 0.8)
	await tw.finished
	_next = false
	var t := 0.0
	while t < hold and not _next:
		await get_tree().process_frame
		t += get_process_delta_time()
	var tw2 := create_tween().set_parallel(true)
	tw2.tween_property(_title, "modulate:a", 0.0, 0.6)
	tw2.tween_property(_text, "modulate:a", 0.0, 0.6)
	await tw2.finished


func _summary(e: Array) -> void:
	_hint.visible = false
	_title.text = "%s: %s" % [e[1], e[2]]
	_title.modulate.a = 1.0
	_title.add_theme_font_size_override("font_size", 22)
	_title.position.y = 24
	var panel := UIKit.panel(Rect2(120, 80, 400, 200))
	add_child(panel)
	var k := "—"
	if GameState.companions["kaelen"]["alive"] and not f("kaelen_lost"):
		k = GameState.kaelen_bond_label()
	var txt := "%s · %s · %s · Nivel %d\nTiempo de juego: %s\n\nKaelen: %s\nYara: %s\n\nHay otros finales esperándote. ¿Qué habría pasado si hubieras elegido distinto?" % [
		GameState.player_name(), GameState.race_name(), GameState.class_name_str(), GameState.level,
		GameState.format_time(GameState.play_time), k, "Perdida en el bosque" if f("yara_lost") else GameState.yara_bond_label()]
	var l := UIKit.label("", 14)
	l.position = Vector2(22, 16)
	l.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	l.size = Vector2(356, 170)
	l.text = txt
	panel.add_child(l)
	var b := UIKit.button("Volver al título", 16)
	b.position = Vector2(245, 294)
	b.size = Vector2(150, 30)
	b.pressed.connect(func(): Transition.go_to_scene("res://scenes/Title.tscn"))
	add_child(b)
	b.grab_focus()
	var credit := UIKit.label("Vaelmoor · Arte, música, código e historia generados con Claude · Fuente Pixelify Sans (OFL)", 11, Color(0.75, 0.72, 0.8), 3)
	credit.position = Vector2(0, 338)
	credit.size = Vector2(640, 16)
	credit.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	add_child(credit)
