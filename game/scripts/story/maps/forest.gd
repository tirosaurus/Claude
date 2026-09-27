extends "res://scripts/story/StoryBase.gd"

var _wolf: Sprite2D


func on_map_ready(_m: String) -> void:
	if flag("act3_started"):
		return
	if not flag("yara_joined"):
		var y = world.spawn_npc("yara", world.marker("yara"), 0)
		y.face(0)
	if not flag("won_wolf"):
		_wolf = Sprite2D.new()
		_wolf.texture = load("res://assets/sprites/wolf.png")
		_wolf.hframes = 2
		_wolf.centered = false
		_wolf.offset = Vector2(-14, -20)
		_wolf.position = world.marker("wolf") + Vector2(80, 0)
		_wolf.flip_h = true
		_wolf.visible = false
		world.entities.add_child(_wolf)
	elif not flag("post_wolf_done"):
		await _post_wolf()


func on_trigger(id: String) -> void:
	if flag("act3_started"):
		return
	match id:
		"meet_yara":
			if not flag("yara_joined"):
				await _meet_yara()
		"wolf_zone":
			if flag("won_wolf"):
				return
			if not flag("yara_joined"):
				begin()
				world.player.position.x -= 10
				await say([K("Eh, ¿adónde vas? Yara estará en el claro de las flores, junto a la piedra santa.")])
				end()
				return
			await _wolf_encounter()


func _meet_yara() -> void:
	begin()
	var yara = npc("yara")
	await world.player.walk_to(Vector2(236, world.player.position.y), 60)
	await wait(0.2)
	yara.face(1)
	await yara.hop()
	await say([
		Y("¡Os estaba oyendo discutir desde la otra punta del bosque! Asustáis hasta a los ciervos."),
		K("Nosotros no discutimos. Debatimos. Con pasión."), Y("Ya.")])
	await world.player.walk_to(Vector2(yara.position.x - 22, yara.position.y + 2), 55)
	world.player.face(2)
	yara.face(1)
	await say([
		Y("Mirad: flor de luna. Solo crece donde la tierra está sana y el pacto sigue en pie."),
		K("La curandera oficial de Tortosa. ¿Sabes qué sería interesante? Encontrar la guarida del ciervo blanco."),
		Y("El ciervo blanco es un cuento para niños, Kaelen."),
		K("Eso dijiste de la cueva de los murciélagos, y mira."), Y("¡Casi nos morimos en la cueva de los murciélagos!")])
	var i := await choose(["Vamos a explorar. Será divertido.", "Yara tiene razón. Quedémonos cerca.",
		"Exploremos un poco, pero sin alejarnos del sendero."],
		Y("{name}, tú decides. ¿Le seguimos la corriente o nos quedamos aquí tranquilos?"))
	match i:
		0:
			GameState.change_kaelen_rivalry(-6)
			GameState.change_approval("kaelen", 4)
			GameState.change_approval("yara", -2)
			await say([K("¡Por fin alguien con sangre en las venas!"), Y("Siempre le das la razón a él..."), "Yara lo dice en broma. Casi."])
		1:
			GameState.change_kaelen_rivalry(6)
			GameState.change_approval("yara", 6)
			GameState.change_yara_kaelen_approval(-2)
			await say([K("Qué par de abuelos."), Y("Gracias, {name}. Alguien sensato en este grupo.")])
		2:
			GameState.change_approval("kaelen", 2)
			GameState.change_approval("yara", 3)
			GameState.change_kaelen_rivalry(-2)
			await say([Y("Diplomátic{o}, como siempre."), K("Me vale. Pero el que vea el ciervo primero invita a sidra.")])
	await say(["Yara guarda las flores en su zurrón. De repente, se queda muy quieta.",
		Y("Esperad... ¿lo oís?"), K("No oigo nada."),
		Y("Exacto. Ni un pájaro. Ni un grillo. El bosque nunca está tan callado."), "Yara se une al grupo."])
	setf("yara_joined")
	join("yara")
	if str(GameState.player_data.get("class", "")) == "":
		await _class_tutorial()
	end()


## Yara explica cómo se lucha y eliges senda, clase y arma antes del lobo.
func _class_tutorial() -> void:
	await say([Y("Esperad. Si de verdad hay algo raro en el bosque, no quiero que vayamos a lo loco."),
		K("¿Otra de tus charlas de «seguridad ante todo»?"),
		Y("Sí. Y esta vez me vais a escuchar. {name}, ¿cómo sueles pelear tú? Y no me digas «a palos».")])
	var sendas := ["melee", "ranged", "support"]
	var i := await choose(["Cuerpo a cuerpo: en primera línea, con acero.",
		"A distancia: con arco, hechizos o pactos oscuros.",
		"Apoyo: curando y protegiendo a los demás."], Y("¿Qué se te da mejor?"))
	var branch: String = sendas[i]
	var classes: Array = DB.BRANCHES[branch]["classes"]
	var opts: Array = []
	for cid in classes:
		opts.append("%s: %s" % [DB.CLASSES[cid]["name"], DB.CLASSES[cid]["desc"]])
	var intro := {
		"melee": Y("Primera línea, entonces. ¿Pero de qué tipo? ¿Fuerza bruta, sigilo o fe?"),
		"ranged": Y("Desde lejos. ¿Magia pura, pactos con la sombra o un buen arco?"),
		"support": Y("Alguien que cuide del grupo. ¿Con luz, con el bosque o con tu propio cuerpo como escudo?"),
	}
	var j := await choose(opts, intro[branch])
	var cid: String = classes[j]
	GameState.choose_class(cid)
	var w: String = str(DB.CLASSES[cid]["weapon"])
	var gift := {
		"sword": K("Toma, la espada de práctica de mi padre. Está mellada, pero corta."),
		"axe": K("Mi padre dejó esta hacha en la leñera. Pesa como un demonio. Te irá bien."),
		"daggers": K("Un par de cuchillos de caza. No preguntes de dónde los he sacado."),
		"staff": Y("El bastón de roble de mi abuela. Dicen que canaliza bien la magia... o eso decía ella."),
		"bow": K("Mi arco de cazar conejos. Si fallas, que sea contra otro."),
	}
	Audio.sfx("chest", -6.0)
	await say([gift.get(w, K("Toma, algo es algo.")),
		"(Clase: %s. %s)" % [DB.CLASSES[cid]["name"], DB.CLASSES[cid]["desc"]],
		Y("Escuchad, que esto es importante:"),
		Y("En un combate, cada uno actúa cuando se llena su barra de tiempo. Atacar no gasta nada; las habilidades, maná."),
		Y("El maná se recupera poco a poco en cada turno. No lo malgastéis al principio."),
		Y("Si curo, puedo elegir a quién. Y si alguien cae, una pluma de fénix lo levanta."),
		Y("Cuando crezcáis en fuerza aprenderéis técnicas nuevas: pensad bien en cuáles os especializáis."),
		"(Menú Esc: Talentos para gastar puntos al subir de nivel, y Equipo para cambiar armas y armaduras de todo el grupo.)",
		K("Vale, vale, profesora. ¿Podemos ir ya a buscar al ciervo?")])


func on_interact(id: String) -> void:
	if id == "shrine" and flag("ov3") and not flag("ov_done") and not flag("won_wolf") \
			and str(GameState.player_data.get("class", "")) != "":
		await _overlord()
		return
	await super.on_interact(id)


## Camino secreto: el arma del Soberano.
func _overlord() -> void:
	begin()
	setf("ov_done")
	Audio.stop_music(1.0)
	await say(["Apoyas la mano en la piedra santa.", "Las runas no parpadean. Se apagan del todo.",
		"Y entonces, todas a la vez, arden en rojo."])
	await world.set_tint(Color(0.55, 0.2, 0.25), 1.2)
	Audio.sfx("dark", -2.0, 0.6)
	world.camera.position = world.player.position
	var glow := CPUParticles2D.new()
	glow.position = world.player.position + Vector2(0, -10)
	glow.amount = 60
	glow.lifetime = 1.6
	glow.emission_shape = CPUParticles2D.EMISSION_SHAPE_SPHERE
	glow.emission_sphere_radius = 30
	glow.gravity = Vector2(0, -30)
	glow.color = Color(1, 0.2, 0.25)
	glow.scale_amount_min = 1.5
	glow.scale_amount_max = 3
	world.add_child(glow)
	await say(["El bosque desaparece. Estás en un claro que no existe, bajo un cielo sin estrellas.",
		"Delante de ti flota un arma negra y roja, girando despacio, como si te hubiera estado esperando.",
		"«Por fin. Alguien que sabe mirar donde nadie mira.»",
		"«Mil años he dormido bajo esta piedra. Mil años esperando a quien recordara el orden de las cosas pequeñas.»",
		"«Tómame. Y que tiemblen los que se creen dioses.»"])
	var kind: String = str(DB.CLASSES[GameState.player_data["class"]]["weapon"])
	var eid := "overlord_" + kind
	GameState.add_item(eid)
	GameState.equip("player", "weapon", eid)
	Audio.sfx("levelup", 0.0, 0.7)
	await say(["Obtienes: %s." % DB.EQUIP[eid]["name"], "(Se ha equipado. Tu ataque es ahora... absurdo.)",
		"«Una cosa más, heredero. No te he despertado para matar lobos.»",
		"«Busca mis tres ecos: donde la luz selló la puerta, donde el árbol recuerda, y donde el corazón late.»",
		"«Cuando los tengas, sabrás contra quién luchas de verdad.»",
		"(Misión secreta: «La Ruta del Soberano».)"])
	glow.queue_free()
	await world.set_tint(world._map_modulate(), 1.0)
	if has("kaelen"):
		await say([K("¿{name}? ¿Qué acaba de pasar? Te has quedado mirando la piedra un minuto entero."),
			Y("Y... ¿de dónde has sacado eso?"), P("...Mejor no preguntéis.")])
	Audio.play_music("forest", 1.0)
	end()


func _wolf_encounter() -> void:
	begin()
	world.player.face(2)
	Audio.stop_music(1.2)
	await wait(0.4)
	Audio.sfx("growl", -2.0)
	await wait(0.4)
	await say([Y("¿Qué ha sido eso?")])
	_wolf.visible = true
	var target: Vector2 = world.player.position + Vector2(62, 0)
	var tw := create_tween()
	tw.tween_property(_wolf, "position", target, 1.0)
	for k in 8:
		_wolf.frame = k % 2
		await wait(0.12)
	await tw.finished
	Audio.sfx("growl", 0.0, 0.9)
	await say([
		K("¿Un lobo? ¿Tan cerca del pueblo?"),
		Y("Mirad su lomo... esas venas negras. Y los ojos. Eso no es normal."),
		K("Sea lo que sea, viene a por nosotros. ¡{name}, a mi lado!"),
		Y("¡Yo os curo! ¡Elegid bien a quién!"),
		"(Combate: cuando se llene la barra de tiempo de alguien, elige su acción. Con Curar puedes elegir a quién sanar.)"])
	await battle("wolf", ["wolf_alpha"], "forest", "boss", true)


func _post_wolf() -> void:
	begin()
	var puddle := Sprite2D.new()
	puddle.texture = load("res://assets/sprites/sap_puddle.png")
	puddle.position = world.player.position + Vector2(52, -4)
	puddle.z_index = -5
	world.add_child(puddle)
	var yara = npc("yara")
	world.player.face(2)
	await wait(0.8)
	await say([K("Uf... ¿estáis todos bien?"),
		"El lobo ha huido entre la maleza, herido. En el suelo solo queda un charco espeso y negro."])
	if yara:
		yara.busy = true
		await yara.walk_to(puddle.position + Vector2(-6, 6), 40)
		yara.face(3)
	await say([
		Y("Esto no es sangre. Es... como savia. Savia podrida."),
		Y("Huele igual que la raíz muerta que vi ayer junto a la piedra santa."),
		K("¿Me estás diciendo que el bosque está... enfermando?")])
	if flag("dream_told"):
		await say(["Las raíces negras de tu sueño. Latiendo. Susurrando tu nombre.", "Sientes un escalofrío, pero no dices nada."])
	var i := await choose(["No lo sé, pero lo averiguaremos juntos.", "Deberíamos volver ya. Esto nos viene grande.",
		"Si algo amenaza Tortosa, lo pararé. Cueste lo que cueste."], K("{name}... ¿tú qué crees que está pasando?"))
	match i:
		0:
			GameState.change_kaelen_rivalry(-5)
			GameState.change_approval("yara", 4)
			await say([K("Juntos. Como siempre."), Y("Como siempre.")])
		1:
			GameState.change_approval("yara", 3)
			GameState.change_kaelen_rivalry(4)
			await say([Y("Es lo más sensato."), K("...Sí. Supongo que sí."), "Kaelen mira hacia el sur del bosque un rato más de la cuenta."])
		2:
			GameState.change_approval("kaelen", 4)
			GameState.change_yara_corruption(3)
			await say([K("Eso es hablar."), Y("No digas eso tan a la ligera. «Cueste lo que cueste» es mucho decir.")])
	await say([Y("Volvamos a Tortosa. Hay que enseñarle esto a Bartolo."), K("Y a todos. Esta noche, en la plaza.")])
	if yara:
		yara.busy = false
		world.make_follower("yara")
	setf("post_wolf_done")
	setf("act2_started")
	GameState.set_meta("tod", "dusk")
	var j := await choose(["Volver a Tortosa ahora.", "Echar un vistazo por el bosque primero."])
	if j == 1:
		await say([K("Vale, pero no tardes. Se está haciendo de noche."),
			"(Cuando quieras volver, sal por el camino del oeste.)"])
		end()
		return
	end()
	Transition.go_to_map("village", "from_forest")


func interact_lines(id: String) -> Array:
	if id == "shrine":
		var lines := ["La piedra santa. Dicen que aquí se selló el pacto que protege el bosque.",
			"Las runas brillan con una luz azulada... débil, parpadeante."]
		if flag("act3_started"):
			lines = ["La piedra santa. Sus runas están casi apagadas. Solo un leve pulso azul, como un corazón cansado."]
		if has("yara") and not flag("act3_started"):
			lines.append(Y("De pequeña brillaban tanto que de noche se veían desde el pueblo. Algo va mal, {name}."))
		return lines
	return []
