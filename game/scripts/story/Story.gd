extends Node
## Guion del prólogo: escenas, diálogos, decisiones y reacciones del mundo.

const NPCScript := preload("res://scripts/world/NPC.gd")

var world: Node
var _talk_count := {}
var _wolf: Sprite2D


# ------------------------------------------------------------ Atajos
func P(t: String) -> Dictionary:
	return {"who": "player", "text": t}


func K(t: String) -> Dictionary:
	return {"who": "kaelen", "text": t}


func Y(t: String) -> Dictionary:
	return {"who": "yara", "text": t}


func M(t: String) -> Dictionary:
	return {"who": "mother", "text": t}


func say(lines: Array) -> void:
	await Dialogue.say(lines)


func begin() -> void:
	world.cutscene = true


func end() -> void:
	world.cutscene = false


func wait(sec: float) -> void:
	await get_tree().create_timer(sec).timeout


func _cycle(key: String, options: Array) -> Array:
	var n: int = int(GameState.get_meta("talk_" + key, 0))
	GameState.set_meta("talk_" + key, n + 1)
	var size := options.size()
	var idx := n
	if n >= size:
		idx = 1 + (n - 1) % (size - 1) if size > 1 else 0
	var pick = options[idx]
	return pick if pick is Array else [pick]


# ------------------------------------------------------------ Entrada en mapas
func on_map_ready(map_id: String) -> void:
	match map_id:
		"bedroom":
			await _bedroom_ready()
		"housemain":
			await _house_ready()
		"village":
			await _village_ready()
		"forest":
			await _forest_ready()


func _bedroom_ready() -> void:
	if GameState.has_flag("woke_up"):
		return
	GameState.set_flag("woke_up")
	begin()
	world.player.face(0)
	await wait(0.9)
	await say([
		"La luz de la mañana se cuela entre las contraventanas y te da de lleno en la cara.",
		"Desde abajo llega olor a pan recién hecho y el tintineo de los cacharros de tu madre.",
		"Otro día más en Tortosa... o eso crees."])
	end()


func _house_ready() -> void:
	var mom = world.spawn_npc("mother", world.marker("mother"), 1)
	if GameState.has_flag("mother_talked"):
		mom.set_mode(NPCScript.Mode.IDLE)
		return
	if GameState.current_spawn != "from_upstairs":
		return
	begin()
	await wait(0.3)
	await world.player.walk_to(world.player.position + Vector2(0, 14), 50)
	mom.face_towards(world.player.position)
	await mom.hop()
	await say([M("¡Mira quién se ha despertado! Ya pensaba que tendría que subir con el cubo de agua fría.")])
	await mom.walk_to(world.player.position + Vector2(-44, 10), 50)
	mom.face_towards(world.player.position)
	world.player.face(1)
	await say([M("Hoy hace muy buen día, de esos que ya casi no quedan.")])
	var i := await Dialogue.choose(["Como un tronco, madre.", "He tenido un sueño muy raro...", "¿Eso es pan recién hecho?"],
		M("¿Has descansado bien, cariño?"))
	match i:
		0:
			await say([M("Eso es que tienes la conciencia tranquila. Disfrútalo mientras dure, que la edad no perdona.")])
		1:
			GameState.set_flag("dream_told")
			await say([M("¿Raro? ¿Raro cómo?"),
				"Recuerdas raíces negras creciendo bajo el bosque, latiendo como venas... y una voz que susurraba tu nombre.",
				P("Nada... tonterías. Raíces, oscuridad... ya se me está olvidando."),
				M("Bah, será la cena de anoche. Te dije que no repitieras queso."),
				"Por un instante, a tu madre se le borra la sonrisa. Luego vuelve a remover el guiso como si nada."])
		2:
			await say([M("De hogaza, con la harina del molino de los Ferrer. Coge un trozo, anda, que estás en los huesos.")])
	await say([
		M("Por cierto, Kaelen ha pasado esta mañana preguntando por ti. Tres veces."),
		M("Ese chico no sabe estarse quieto. Igualito que su padre."),
		M("Anda, sal a que te dé el aire. Y volved antes de que anochezca, ¿eh? Últimamente el bosque... bueno, tú vuelve pronto.")])
	GameState.set_flag("mother_talked")
	end()


func _village_ready() -> void:
	var bart = world.spawn_npc("bartolo", world.marker("bartolo"), 3)
	world.spawn_npc("remei", world.marker("remei"), 1)
	world.spawn_npc("roc", world.marker("roc"), 1)
	var nil = world.spawn_npc("nil", world.marker("nil"), 0, NPCScript.Mode.WANDER)
	nil.wander_radius = 60.0
	bart.face(3)
	if GameState.has_flag("kaelen_joined"):
		return
	var kae = world.spawn_npc("kaelen", world.marker("kaelen_start"), 1)
	if GameState.current_spawn != "home_door":
		return
	begin()
	await wait(0.5)
	kae.face_towards(world.player.position)
	await kae.hop()
	await say([K("¡{name}! ¡Eh, {name}! ¡Aquí!")])
	await kae.walk_to(Vector2(world.player.position.x + 26, world.player.position.y + 18), 80)
	kae.face(1)
	world.player.face(2)
	await say([
		K("¡Por fin! Llevo media mañana tirando piedras al río para no morirme de aburrimiento."),
		K("En este pueblo nunca pasa nada. Nada. El viejo Bartolo dice que la Catedral está maldita, pero lleva cuarenta años diciendo lo mismo."),
		K("Yara se ha ido al Bosque Santo a por sus flores. Vamos a buscarla. Seguro que encontramos algo interesante por el camino.")])
	var i := await Dialogue.choose([
		"¡Claro! Como en los viejos tiempos.",
		"¿Otra vez? Siempre acabas metiéndonos en líos.",
		"Vale, pero esta vez no te pierdas."], K("¿Vienes o te vas a quedar ahí plantado?"))
	match i:
		0:
			GameState.change_kaelen_rivalry(-8)
			GameState.change_approval("kaelen", 5)
			await say([K("¡Esa es la actitud! Sabía que no me fallarías.")])
		1:
			GameState.change_kaelen_rivalry(6)
			await say([K("Y siempre salimos de ellos, ¿no? Bueno... casi siempre."),
				K("No seas aguafiestas. Si pasa algo, yo te cubro. Como siempre."),
				"Lo dice sonriendo, pero hay algo de pique en su voz."])
		2:
			GameState.change_approval("kaelen", 3)
			GameState.change_kaelen_rivalry(-3)
			await say([K("¡Fue UNA vez! Y tenía diez años. Y la niebla era muy espesa."),
				K("...Y no se lo cuentes a Yara. Otra vez.")])
	await say([K("El sendero del bosque sale por el este, pasado el cartel. ¡Vamos!"),
		"Kaelen se une al grupo."])
	GameState.set_flag("kaelen_met")
	GameState.set_flag("kaelen_joined")
	GameState.join_party("kaelen")
	kae.follow_target = world.player
	kae.follow_gap = 16
	kae.set_mode(NPCScript.Mode.FOLLOW)
	end()


func _forest_ready() -> void:
	if not GameState.has_flag("yara_met"):
		var y = world.spawn_npc("yara", world.marker("yara"), 0)
		y.face(0)
	if not GameState.has_flag("wolf_defeated"):
		_wolf = Sprite2D.new()
		_wolf.texture = load("res://assets/sprites/wolf.png")
		_wolf.hframes = 2
		_wolf.centered = false
		_wolf.offset = Vector2(-14, -20)
		_wolf.position = world.marker("wolf") + Vector2(80, 0)
		_wolf.flip_h = true
		_wolf.visible = false
		world.entities.add_child(_wolf)
	elif not GameState.has_flag("post_wolf_done"):
		await _post_wolf()


# ------------------------------------------------------------ Zonas
func on_trigger(id: String) -> void:
	match id:
		"meet_yara":
			if not GameState.has_flag("yara_met"):
				await _meet_yara()
		"wolf_zone":
			if GameState.has_flag("wolf_defeated"):
				return
			if not GameState.has_flag("yara_met"):
				begin()
				world.player.position.x -= 10
				await say([K("Eh, eh, ¿adónde vas? Yara estará en el claro de las flores, junto a la piedra santa.")])
				end()
				return
			await _wolf_encounter()


func is_interactable(id: String) -> bool:
	return not id in ["meet_yara", "wolf_zone"]


func follower_talkable(_id: String) -> bool:
	return true


func _meet_yara() -> void:
	begin()
	var yara = world.npcs.get("yara")
	await world.player.walk_to(Vector2(236, world.player.position.y), 60)
	await wait(0.2)
	yara.face(1)
	await yara.hop()
	await say([
		Y("¡Os estaba oyendo discutir desde la otra punta del bosque! Asustáis hasta a los ciervos."),
		K("Nosotros no discutimos. Debatimos. Con pasión."),
		Y("Ya.")])
	await world.player.walk_to(Vector2(yara.position.x - 22, yara.position.y + 2), 55)
	world.player.face(2)
	yara.face(1)
	await say([
		Y("Mirad: flor de luna. Solo crece donde la tierra está sana y el pacto sigue en pie."),
		Y("Con esto preparo el ungüento de las quemaduras de Maese Roc y el jarabe de la tos de medio pueblo."),
		K("La curandera oficial de Tortosa. ¿Sabes qué sería interesante de verdad? Encontrar la guarida del ciervo blanco."),
		Y("El ciervo blanco es un cuento para niños, Kaelen."),
		K("Eso dijiste de la cueva de los murciélagos, y mira."),
		Y("¡Casi nos morimos en la cueva de los murciélagos!")])
	var i := await Dialogue.choose([
		"Vamos a explorar. Será divertido.",
		"Yara tiene razón. Quedémonos cerca.",
		"Exploremos un poco, pero sin alejarnos del sendero."],
		Y("{name}, tú decides. ¿Le seguimos la corriente o nos quedamos aquí tranquilos?"))
	match i:
		0:
			GameState.change_kaelen_rivalry(-6)
			GameState.change_approval("kaelen", 4)
			GameState.change_approval("yara", -2)
			await say([K("¡Por fin alguien con sangre en las venas!"),
				Y("Siempre le das la razón a él..."),
				"Yara lo dice en broma. Casi."])
		1:
			GameState.change_kaelen_rivalry(6)
			GameState.change_approval("yara", 6)
			GameState.change_yara_kaelen_approval(-2)
			await say([K("Qué par de abuelos. Lo que yo te diga: acabaréis poniendo una tienda de hierbas juntos."),
				Y("Gracias, {name}. Alguien sensato en este grupo.")])
		2:
			GameState.change_approval("kaelen", 2)
			GameState.change_approval("yara", 3)
			GameState.change_kaelen_rivalry(-2)
			await say([Y("Diplomático, como siempre."), K("Me vale. Pero el que vea el ciervo primero invita a sidra.")])
	await say([
		"Yara guarda las flores en su zurrón. De repente, se queda muy quieta.",
		Y("Esperad... ¿lo oís?"),
		K("No oigo nada."),
		Y("Exacto. Ni un pájaro. Ni un grillo. El bosque nunca está tan callado."),
		"Yara se une al grupo."])
	GameState.set_flag("yara_met")
	GameState.join_party("yara")
	yara.follow_target = world.player
	yara.follow_gap = 32
	yara.set_mode(NPCScript.Mode.FOLLOW)
	end()


func _wolf_encounter() -> void:
	begin()
	var kae = world.npcs.get("kaelen")
	var yara = world.npcs.get("yara")
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
	if kae:
		kae.face(2)
	if yara:
		yara.face(2)
	await say([
		K("¿Un lobo? ¿Tan cerca del pueblo?"),
		Y("Mirad su lomo... esas venas negras. Y los ojos. Eso no es normal. Está enfermo... o algo peor."),
		K("Sea lo que sea, viene a por nosotros. ¡{name}, detrás de mí no, a mi lado!"),
		Y("¡Yo os curo! ¡No dejéis que os muerda!")])
	GameState.pending_encounter = {"enemy": "wolf"}
	GameState.battle_return = {"map": "forest", "spawn": "party_vs_wolf"}
	GameState.current_map = "forest"
	GameState.current_spawn = "party_vs_wolf"
	await Transition.battle_flash()


func _post_wolf() -> void:
	begin()
	var puddle := Sprite2D.new()
	puddle.texture = load("res://assets/sprites/sap_puddle.png")
	puddle.position = world.player.position + Vector2(52, -4)
	puddle.z_index = -5
	world.add_child(puddle)
	var kae = world.npcs.get("kaelen")
	var yara = world.npcs.get("yara")
	world.player.face(2)
	await wait(0.8)
	await say([K("Uf... ¿estáis todos bien?"),
		"El lobo ha desaparecido entre la maleza, herido. En el suelo solo queda un charco espeso y negro."])
	if yara:
		await yara.walk_to(puddle.position + Vector2(-6, 6), 40)
		yara.face(3)
	await say([
		Y("Esto no es sangre. Es... como savia. Savia podrida."),
		Y("Huele igual que la raíz muerta que vi ayer junto a la piedra santa. Pensé que era un tronco enfermo."),
		K("¿Me estás diciendo que el bosque está... enfermando?")])
	if GameState.has_flag("dream_told"):
		await say(["Las raíces negras de tu sueño. Latiendo. Susurrando tu nombre.",
			"Sientes un escalofrío, pero no dices nada. Todavía no."])
	await say([
		Y("No lo sé. Pero las runas de la piedra santa estaban más apagadas que nunca. Y ahora esto."),
		K("Los mercaderes del sur hablaban de bosques que se pudrían en una sola noche. Pensaba que eran cuentos para vender más caro."),
		Y("Tenemos que volver y contárselo a alguien. A Bartolo, a tu madre, a quien sea."),
		K("{name}... ¿tú qué crees que está pasando?")])
	var i := await Dialogue.choose([
		"No lo sé, pero lo averiguaremos juntos.",
		"Deberíamos volver ya. Esto nos viene grande.",
		"Si algo amenaza Tortosa, lo pararé. Cueste lo que cueste."])
	match i:
		0:
			GameState.change_kaelen_rivalry(-5)
			GameState.change_approval("yara", 4)
			await say([K("Juntos. Como siempre."), Y("Como siempre.")])
		1:
			GameState.change_approval("yara", 3)
			GameState.change_kaelen_rivalry(4)
			await say([Y("Es lo más sensato."), K("...Sí. Supongo que sí."),
				"Kaelen mira hacia el sur del bosque un rato más de la cuenta."])
		2:
			GameState.change_approval("kaelen", 4)
			GameState.change_yara_corruption(2)
			await say([K("Eso es hablar."), Y("No digas eso tan a la ligera, {name}. «Cueste lo que cueste» es mucho decir."),
				"Por un momento, la mirada de Yara se pierde en el charco negro. Luego parpadea y aparta la vista."])
	await say(["Detrás de vosotros, muy lejos, en lo más profundo del Bosque Santo, algo cruje. Algo grande.",
		"Y el bosque sigue en silencio."])
	GameState.set_flag("post_wolf_done")
	Audio.stop_music(1.5)
	await wait(0.6)
	await Transition.go_to_scene("res://scenes/DemoEnd.tscn", 1.2)


# ------------------------------------------------------------ Interacción
func on_interact(id: String) -> void:
	var lines: Array = []
	match id:
		"bed":
			lines = _cycle("bed", [
				["Tu cama. Todavía está calentita...", "...pero ya has dormido más que suficiente. Hoy hace un día precioso."],
				"Ya has dormido bastante. ¡Arriba, que el día no espera!"])
		"nightstand":
			lines = ["Una vela casi consumida y un libro abierto: «Leyendas del Imperio de Cristal»."
				, "Anoche te quedaste leyendo hasta tarde. Otra vez."]
		"wardrobe":
			lines = ["Tu ropa de siempre: dos túnicas, unos pantalones remendados y la capa que heredaste de tu padre.",
				"Nada digno de un héroe... todavía."]
		"chest":
			lines = _cycle("chest", [
				["Tu cofre de tesoros de la infancia.",
					"Una piedra con forma de corazón, una pluma de cuervo y un dibujo hecho por Yara:",
					"vosotros tres delante de la Catedral Vieja, y debajo, en letras torcidas: «LOS GUARDIANES DE TORTOSA».",
					"Sonríes sin querer."],
				"«Los Guardianes de Tortosa». Kaelen insistió en que él era el capitán."])
		"desk":
			lines = ["Un mapa de Vaelmoor copiado a mano, lleno de anotaciones tuyas.",
				"Tortosa es un puntito en los montes del norte. El resto del mundo parece enorme... y lleno de nombres que suenan a aventura."]
		"window":
			lines = ["Desde la ventana se ve la Catedral Vieja, enorme y rota sobre los tejados del pueblo.",
				"Más allá, el verde oscuro del Bosque Santo se pierde hacia el este."]
		"sword":
			lines = ["La espada de madera que Kaelen te talló cuando teníais ocho años.",
				"Tiene más muescas que filo. Aun así, nunca has sido capaz de tirarla."]
		"fireplace":
			lines = ["El guiso de tu madre burbujea al fuego. Huele a romero y a tomillo del monte."]
		"kitchen_table":
			if not GameState.has_flag("ate_bread"):
				GameState.set_flag("ate_bread")
				GameState.heal_party()
				lines = ["Pan de hogaza, queso de cabra y un tazón de leche.",
					"Coges un buen trozo de pan. Te sientes con fuerzas para cualquier cosa."]
			else:
				lines = ["Ya has desayunado. Si comes más, tu madre te hará subir leña como pago."]
		"cupboard":
			lines = ["Tarros de miel, hierbas secas y un libro de recetas con la letra apretada de tu abuela.",
				"En la última página, alguien escribió: «Si el bosque calla, cerrad las puertas». No sabes qué significa."]
		"cathedral":
			lines = _cycle("cathedral", [
				["La Catedral Vieja.",
					"Sus torres rotas se alzan sobre Tortosa como los dedos de un gigante dormido.",
					"La gran puerta está sellada por los escombros desde antes de que naciera nadie del pueblo."],
				["Entre las piedras caídas se cuela un aire frío que huele a humedad... y a algo más antiguo."]])
		"well":
			lines = ["El pozo de la plaza. El agua está helada y tiene un regusto a hierro que antes no tenía."]
		"stall":
			lines = ["El puesto de la plaza está cerrado. Hoy es día de pastoreo y medio pueblo está en el monte."]
		"sign_forest":
			lines = ["«→ Bosque Santo. Que la luz del pacto os guarde.»",
				"Alguien ha grabado debajo con un cuchillo: «Nil estuvo aquí»."]
		"anvil":
			lines = ["El yunque de Maese Roc. Todavía está tibio."]
		"smithy":
			lines = ["La herrería de Maese Roc. Herraduras, azadas y, colgada en la pared, una espada de verdad que nunca vende."]
		"house_kaelen":
			lines = ["La casa de Kaelen. Su padre está en el monte con las cabras; su madre, en el mercado de Val d'Ombra hasta la luna llena."]
		"house_brown":
			lines = ["La casa de los Ferrer, los molineros. Huele a harina y a manzanas asadas."]
		"house_south":
			lines = ["La casa de la Tía Remei. En la ventana, un gato naranja te juzga en silencio."]
		"house_player":
			lines = ["Tu casa. Desde aquí se oye a tu madre cantar mientras friega."]
		"shrine":
			lines = ["La piedra santa. Dicen que aquí, hace siglos, los primeros de Tortosa sellaron el pacto que protege el bosque.",
				"Las runas brillan con una luz azulada... débil, parpadeante, como una vela a punto de apagarse."]
			if GameState.companions["yara"]["in_party"]:
				lines.append(Y("De pequeña brillaban tanto que de noche se veían desde el pueblo. Algo va mal, {name}."))
	if lines.is_empty():
		return
	begin()
	await say(lines)
	end()


func on_npc(id: String) -> void:
	var n = world.npcs.get(id)
	if n and n.mode != NPCScript.Mode.FOLLOW:
		n.face_towards(world.player.position)
	var lines: Array = []
	match id:
		"mother":
			lines = _cycle("mother", [
				[M("¿Todavía aquí? Kaelen va a echar la puerta abajo.")],
				[M("Coge algo de pan de la mesa si tienes hambre.")],
				[M("Y dile a Yara que me guarde unas flores de luna, que me quedan pocas.")]])
		"kaelen":
			lines = _cycle("kaelen", [
				[K("¿Te acuerdas de cuando nos colamos en la Catedral y casi nos aplasta una viga? Mejor día de mi vida.")],
				[K("Algún día me iré de Tortosa. A Puerto Ceniza, al Gremio de Aventureros. Y tú te vienes conmigo.")],
				[K("Yara dice que el bosque está raro. Yara dice muchas cosas. Pero... sí, hoy hay algo raro.")]])
		"yara":
			lines = _cycle("yara", [
				[Y("La flor de luna cura casi cualquier cosa si sabes prepararla. Casi.")],
				[Y("A veces siento el bosque, ¿sabes? Como un latido. Hoy late... más despacio.")],
				[Y("Si Kaelen se mete en líos otra vez, esta vez le dejo con el chichón.")]])
		"bartolo":
			lines = _cycle("bartolo", [
				[{"who": "bartolo", "text": "¿La Catedral? Nadie sabe quién la levantó, muchacho. Mi abuelo decía que ya estaba en ruinas cuando llegaron los primeros de Tortosa."}],
				[{"who": "bartolo", "text": "Algunas noches se oyen campanas ahí dentro. Y esa catedral no tiene campanas desde hace siglos."}],
				[{"who": "bartolo", "text": "Ríete, ríete. Kaelen también se ríe. Ya os reiréis menos."}]])
		"remei":
			lines = _cycle("remei", [
				[{"who": "remei", "text": "El agua del pozo está más fría estos días. Y sabe a hierro. Qué cosas, ¿verdad?"}],
				[{"who": "remei", "text": "Mi gato no quiere salir de casa. Ese bicho no le teme a nada... salvo a lo que yo no veo."}]])
		"roc":
			lines = _cycle("roc", [
				[{"who": "roc", "text": "Han pasado mercaderes del sur contando cosas feas. Pueblos vacíos, bosques que se pudren en una noche."},
					{"who": "roc", "text": "Bah. Cuentos para vender más caro. ...Aunque les compré un hacha, por si acaso."}],
				[{"who": "roc", "text": "Si algún día necesitas una espada de verdad, ven a verme. Pero trae algo más que sonrisas."}]])
		"nil":
			lines = _cycle("nil", [
				[{"who": "nil", "text": "¿Vais al bosque? ¡Mi madre dice que no se puede pasar de la piedra santa!"},
					{"who": "nil", "text": "Yo una vez pasé. Bueno, casi. Bueno, la vi de lejos."}],
				[{"who": "nil", "text": "¡Cuando sea mayor seré más fuerte que Kaelen! ¡Y más listo que Yara! ¡Y más...! Eh... ¡más todo!"}]])
	if lines.is_empty():
		return
	begin()
	await say(lines)
	end()
