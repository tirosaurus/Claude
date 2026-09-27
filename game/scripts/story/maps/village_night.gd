extends "res://scripts/story/StoryBase.gd"

var _brute: Sprite2D


func encounters_disabled() -> bool:
	return true


func before_exit(to: String) -> bool:
	return true


func on_map_ready(_m: String) -> void:
	_place_npcs()
	if flag("brute_defeated"):
		world.set_prop_sprite("cathedral", "cathedral_open")
		world.remove_prop("rubble")
	if not flag("won_night1"):
		await _first_attack()
	elif not flag("night_party"):
		await _party_forms()
	elif not flag("night_post") and (flag("won_night_home") or flag("won_night_plaza") or flag("won_night_cath")):
		await _after_choice()
	elif flag("won_brute") and not flag("brute_defeated"):
		await _after_brute()


func _place_npcs() -> void:
	if flag("night_party"):
		if flag("night_post"):
			world.spawn_npc("remei", Vector2(390, 300), 1)
			if not flag("nil_dead"):
				world.spawn_npc("nil", Vector2(410, 310), 1)
			world.spawn_npc("roc", Vector2(330, 290), 2)
			if flag("bartolo_alive"):
				world.spawn_npc("bartolo", Vector2(368, 244), 0)


func _first_attack() -> void:
	begin()
	world.show_banner("Tortosa en llamas")
	await wait(0.6)
	await say(["Tortosa arde.",
		"El tejado de los Ferrer es una antorcha. Hay gritos por todas partes, y entre el humo se mueven cosas."])
	var vtex := Appearance.tex("res://assets/chars/villager_m.png")
	var ftex := Appearance.tex("res://assets/chars/villager_f.png")
	var ttex := Appearance.tex("res://assets/chars/thrall.png")
	# 1) Un vecino huye por la calle; un siervo lo alcanza
	await pan_to(Vector2(520, 330), 1.1)
	var v1 = world.spawn_npc("vill1", Vector2(690, 336), 1, 0, vtex)
	var t1 = world.spawn_npc("th1", Vector2(740, 330), 1, 0, ttex)
	v1.walk_to(Vector2(470, 336), 78)
	await wait(0.3)
	await say(["«¡Socorro! ¡Que alguien me ayude!»"])
	await t1.walk_to(Vector2(492, 334), 96)
	Audio.sfx("growl", -4.0, 1.3)
	t1.face(1)
	await wait(0.15)
	knock_down(v1)
	shake(3.0, 0.3)
	await wait(0.6)
	await say(["El siervo se inclina sobre él. Tiene la cara gris, los ojos violetas... y lleva la ropa del molinero."])
	# 2) Larvas saliendo de la casa en llamas
	await pan_to(Vector2(190, 390), 1.2)
	var l1 := monster("res://assets/enemies/larva.png", 2, Vector2(150, 372), 0.9)
	var l2 := monster("res://assets/enemies/larva.png", 2, Vector2(180, 366), 0.9)
	var v2 = world.spawn_npc("vill2", Vector2(250, 400), 1, 0, ftex)
	move_node(l1, Vector2(215, 400), 1.4)
	await move_node(l2, Vector2(238, 394), 1.4)
	v2.walk_to(Vector2(330, 440), 80)
	await say(["Del fuego salen larvas del tamaño de un perro, chillando. Una vecina huye con un niño en brazos."])
	# 3) Siervos golpeando los escombros de la Catedral
	await pan_to(Vector2(368, 250), 1.3)
	var t2 = world.spawn_npc("th2", Vector2(346, 236), 3, 0, ttex)
	var t3 = world.spawn_npc("th3", Vector2(392, 238), 3, 0, ttex)
	for k in 3:
		await t2.hop()
		dust(Vector2(360, 224))
		Audio.sfx("hit", -6.0, 0.6)
		shake(2.0, 0.2)
		await t3.hop()
		dust(Vector2(380, 224))
		Audio.sfx("hit", -6.0, 0.7)
		await wait(0.2)
	await say(["Frente a la Catedral Vieja, más siervos golpean los escombros con las manos desnudas.", "Quieren entrar."])
	# 4) De vuelta al jugador
	await release_camera(0.8)
	for id in ["vill1", "vill2", "th1", "th2", "th3"]:
		world.remove_npc(id)
	l2.queue_free()
	l1.position = world.player.position + Vector2(46, 26)
	Audio.sfx("growl", -4.0, 1.4)
	await say([P("¡¿Qué demonios son esas cosas?!"), "Una larva se lanza hacia ti, chillando."])
	await move_node(l1, world.player.position + Vector2(14, 6), 0.25)
	await battle("night1", ["larva"], "village_night", "night")


func _party_forms() -> void:
	begin()
	var kae = world.spawn_npc("kaelen", Vector2(520, 380), 1)
	await say([K("¡{name}! ¡{name}, estás viv{o}!")])
	await kae.walk_to(world.player.position + Vector2(24, 10), 90)
	kae.face(1)
	world.player.face(2)
	await say([
		K("He tumbado a uno con la pala de mi padre. ¡Con una pala! Y se ha vuelto a levantar."),
		K("Eran... eran como personas, {name}. Uno llevaba la capa del pastor Joan."),
		P("¿Has visto a Yara?"), K("Junto al pozo. Vamos.")])
	join("kaelen")
	var yara = world.spawn_npc("yara", Vector2(380, 290), 1)
	var remei = world.spawn_npc("remei", Vector2(360, 294), 2)
	await walk_party_to(Vector2(356, 318), 80)
	world.player.face(3)
	yara.face_towards(world.player.position)
	await say([
		Y("¡Aquí! Ayudadme, Remei tiene un corte muy feo en la pierna."),
		"Yara aprieta un vendaje con flor de luna. Sus manos brillan un instante con una luz verdosa.",
		N("remei", "Estoy bien, niña, estoy bien... Id. ¡Id!"),
		Y("Voy con vosotros. Nadie más se queda solo esta noche.")])
	join("yara")
	await wait(0.3)
	await say([
		"Entonces lo oyes todo a la vez.",
		M("¡¡Socorro!! ¡¡{name}!!"),
		"El grito viene de tu casa. Algo ha reventado la puerta de la bodega.",
		N("roc", "¡Nil está atrapado bajo el carro! ¡No puedo levantarlo solo y esas cosas vienen hacia aquí!"),
		N("bartolo", "¡Aguantaré la puerta de la Catedral! ¡No dejéis que entren! ¡La Semilla...!"),
		"Bartolo, con una antorcha y un bastón, se planta solo frente a los escombros de la Catedral.",
		Y("¡No podemos estar en tres sitios a la vez!"),
		K("¡Decide, {name}! ¡Rápido!")])
	var i := await choose(["¡A por mi madre!", "¡Nil primero, es un niño!", "¡A la Catedral, con Bartolo!"])
	setf("night_party")
	match i:
		0:
			setf("chose_home")
			await say([K("¡Vamos!"), Y("¡Roc, aguanta!")])
			world.player.position = Vector2(170, 380)
			await battle("night_home", ["thrall"], "village_night", "night")
		1:
			setf("chose_plaza")
			if GameState.rivalry() <= 0:
				setf("kaelen_saves_mother")
				await say([K("¡Yo voy a por tu madre! ¡Tú ayuda a Nil!"), "Kaelen echa a correr hacia tu casa sin mirar atrás."])
				GameState.change_kaelen_rivalry(-5)
			else:
				setf("mother_wounded")
				await say([K("¿Y tu madre? ¿La dejas sola?"), P("¡Nil es un niño, Kaelen!"), K("...Como quieras. Siempre como tú quieras.")])
				GameState.change_kaelen_rivalry(6)
			world.player.position = Vector2(300, 280)
			await battle("night_plaza", ["thrall", "larva"], "village_night", "night")
		2:
			setf("chose_cath")
			setf("bartolo_alive")
			await say([Y("¿Y Nil? ¿Y tu madre?"), P("¡Si entran en la Catedral, estamos todos muertos!"),
				K("...Tiene sentido. Duele, pero tiene sentido.")])
			GameState.change_approval("yara", -4)
			GameState.change_kaelen_rivalry(-2)
			world.player.position = Vector2(368, 230)
			await battle("night_cath", ["thrall", "thrall"], "village_night", "night")


func _after_choice() -> void:
	begin()
	setf("night_post")
	await wait(0.5)
	if flag("chose_home"):
		setf("bartolo_dead")
		setf("roc_wounded")
		await say([
			"Encuentras a tu madre acurrucada tras los barriles de la bodega, con una sartén en la mano. Viva.",
			M("¡Estás bien! ¡Estáis bien!"),
			"Cuando volvéis a la plaza, Roc ha sacado a Nil de debajo del carro... con un brazo destrozado por las garras.",
			"Y frente a la Catedral, el cuerpo de Bartolo yace junto a su antorcha apagada."])
		await say([Y("Bartolo..."), K("Aguantó solo. Hasta el final.")])
		GameState.change_approval("kaelen", 2)
	elif flag("chose_plaza"):
		setf("bartolo_dead")
		setf("nil_alive")
		await say(["Levantáis el carro entre todos. Nil sale arrastrándose, llorando, pero entero.",
			N("nil", "¡Sabía que vendríais! ¡Lo sabía!")])
		if flag("kaelen_saves_mother"):
			await say(["Kaelen vuelve cojeando, con tu madre del brazo. Tiene un zarpazo en la espalda, pero sonríe.",
				K("Tu madre pega más fuerte que yo con la sartén. Que conste.")])
			GameState.change_approval("kaelen", 5)
		else:
			await say(["Cuando llegas a casa, tu madre está en el suelo, herida en el hombro, pero viva. Remei ya la está vendando.",
				M("Estoy bien... Hiciste bien. Nil es solo un niño.")])
		await say(["Pero frente a la Catedral, Bartolo yace inmóvil junto a su antorcha apagada.", Y("No... Bartolo...")])
	elif flag("chose_cath"):
		setf("nil_dead")
		await say(["Bartolo sigue en pie, jadeando, apoyado en su bastón.",
			N("bartolo", "Llegasteis... llegasteis a tiempo, muchachos."),
			"Pero cuando la plaza queda en silencio, Roc se acerca despacio. Lleva a alguien pequeño en brazos.",
			N("roc", "No pude levantar el carro solo. No pude... Nil..."),
			"Yara se tapa la boca. Kaelen aparta la mirada."])
		await say([Y("Era solo un niño, {name}. Solo un niño."),
			"Tu madre está a salvo: Remei la sacó de la bodega a tiempo. Pero ahora mismo eso no consuela a nadie."])
		GameState.change_approval("yara", -6)
		GameState.change_yara_corruption(6)
	await wait(0.4)
	await _brute_arrives()


func _brute_arrives() -> void:
	Audio.stop_music(0.6)
	await wait(0.3)
	await pan_to(Vector2(420, 230), 1.0)
	Audio.sfx("growl", 0.0, 0.6)
	shake(3.0, 0.5)
	await say(["La tierra tiembla.", "Algo enorme sale de entre el humo y se dirige a la Catedral."])
	_brute = monster("res://assets/enemies/brute.png", 2, Vector2(560, 262), 0.75)
	for k in 4:
		await move_node(_brute, _brute.position + Vector2(-34, -8), 0.35)
		shake(2.5, 0.2)
		Audio.sfx("hit", -8.0, 0.5)
	for k in 2:
		await move_node(_brute, _brute.position + Vector2(-6, -4), 0.12)
		dust(Vector2(372, 222), Color(0.6, 0.55, 0.5))
		shake(5.0, 0.3)
		Audio.sfx("crit", -4.0, 0.6)
		await move_node(_brute, _brute.position + Vector2(6, 4), 0.2)
	await say(["El bruto golpea los escombros de la Catedral con los puños. Una vez. Dos. Las piedras empiezan a ceder.",
		K("¡Quiere entrar! ¡Busca la Semilla!"), Y("¡Hay que pararlo!")])
	await release_camera(0.3)
	await walk_party_to(Vector2(380, 262), 90)
	await battle("brute", ["brute", "larva"], "village_night", "boss", true)


func _after_brute() -> void:
	begin()
	setf("brute_defeated")
	world.set_prop_sprite("cathedral", "cathedral_open")
	world.remove_prop("rubble")
	await wait(0.4)
	await say(["El bruto se desploma sobre los escombros... y con él, la entrada de la Catedral se viene abajo.",
		"Donde antes había piedra, ahora hay un agujero negro."])
	var ttex := Appearance.tex("res://assets/chars/thrall.png")
	var starts := [Vector2(250, 330), Vector2(500, 320), Vector2(300, 400), Vector2(460, 390)]
	var ths: Array = []
	for k in starts.size():
		var t = world.spawn_npc("thin%d" % k, starts[k], 3, 0, ttex)
		ths.append(t)
		t.walk_to(Vector2(368 + (k - 1.5) * 6, 214), 55 + k * 6)
	await pan_to(Vector2(368, 250), 0.8)
	await wait(2.2)
	for t in ths:
		var tw := create_tween()
		tw.tween_property(t, "modulate:a", 0.0, 0.4)
	await wait(0.5)
	for k in starts.size():
		world.remove_npc("thin%d" % k)
	await release_camera(0.4)
	await say(["Los siervos supervivientes se arrastran hacia dentro, uno tras otro, como hormigas hacia la miel.",
		K("Se están metiendo en la Catedral."), Y("Van a por la Semilla. Si la encuentran...")])
	if flag("bartolo_alive"):
		await say([N("bartolo", "Escuchadme. Bajo el altar hay tres runas: Fe, Memoria y Sacrificio."),
			N("bartolo", "Si se encienden las tres, se abre el camino a la cripta. La Semilla os estará esperando."),
			N("bartolo", "Yo soy demasiado viejo. Pero vosotros... vosotros sois los Guardianes de Tortosa, ¿no?")])
	else:
		await say([N("remei", "Bartolo me dio esto hace años. Dijo que si algún día pasaba... que se lo diera a los chicos."),
			"Remei te entrega un cuaderno gastado: el diario de Bartolo.",
			"«Bajo el altar, tres runas: Fe, Memoria, Sacrificio. Enciéndelas y la cripta se abrirá. Protege la Semilla.»"])
	await say([K("Pues vamos. Antes de que sea tarde."),
		"(Consejo: abre el menú con Esc para guardar la partida. En la Catedral habrá enemigos.)"])
	end()


func npc_lines(id: String) -> Array:
	match id:
		"remei":
			return [N("remei", "Id con cuidado, niños. Yo me quedo con los heridos.")]
		"nil":
			return [N("nil", "¿Vais a entrar ahí? ¡Sois los más valientes de todo Vaelmoor!")]
		"roc":
			return [N("roc", "Si salís de esa, os forjaré lo que queráis. Gratis. ...Casi gratis.")]
		"bartolo":
			return [N("bartolo", "Fe, Memoria, Sacrificio. No lo olvidéis.")]
	return []


func interact_lines(id: String) -> Array:
	match id:
		"cathedral":
			if flag("brute_defeated"):
				return ["La entrada de la Catedral está abierta. De dentro sale un aire helado."]
			return ["Los escombros tiemblan. Algo al otro lado quiere salir... o algo aquí fuera quiere entrar."]
		"well":
			return ["El pozo. Alguien ha dejado un cubo lleno junto al brocal, para las llamas."]
	return []
