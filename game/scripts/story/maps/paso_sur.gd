extends "res://scripts/story/StoryBase.gd"
## Paso del Sur: campamento de guerra morongul. Jefe: General Kraag. Lágrima de Sangre.


func music_override() -> String:
	return "forest" if flag("tear_blood") else ""


func on_map_ready(_m: String) -> void:
	if flag("won_kraag") and not flag("tear_blood"):
		await _after_kraag()
		return
	if flag("tear_blood"):
		for c in ["cage1", "cage2", "cage3"]:
			var node = world.props_by_id.get(c)
			if node:
				node.get_node("Sprite").texture = load("res://assets/sprites/cage_empty.png")
	if not flag("paso_seen"):
		setf("paso_seen")
		begin()
		world.show_banner("Paso del Sur")
		await wait(0.5)
		await say(["Tierra quemada. Estandartes rojos. Tambores de guerra que retumban en el pecho.",
			"Un campamento morongul entero cierra el paso. Y en jaulas, junto a la hoguera... gente.",
			Y("{name}... ¡Esa es la tía Remei! ¡Y más gente de Tortosa!") if flag("bartolo_dead") else Y("{name}... ¡Esos son Bartolo y la tía Remei! ¡De Tortosa!"),
			K("Se los llevaron la noche del ataque. Creía que estaban muertos."),
			"(Abríos paso hasta el general del campamento, al este.)"])
		end()


func on_trigger(id: String) -> void:
	if id == "kraag_zone" and not flag("won_kraag"):
		begin()
		Audio.stop_music(0.8)
		await pan_to(world.marker("kraag"), 1.0)
		await say(["Junto a la hoguera, un morongul enorme con armadura negra y un martillo del tamaño de una puerta.",
			"Colgada al cuello lleva una gema roja que late despacio: la Lágrima de Sangre.",
			N("kraag", "Humanos. Y con la peste de la Semilla encima. La Madre dijo que vendríais."),
			N("kraag", "Soy Kraag. Antes de la raíz fui un hombre. Un campeón de este paso. Ya no recuerdo mi nombre de entonces."),
			N("kraag", "Recuerdo pelear. Eso sí.")])
		await release_camera(0.3)
		await battle("kraag", ["kraag", "soldado"], "heart", "boss", true)


func _after_kraag() -> void:
	begin()
	await say(["Kraag cae de rodillas. El martillo se le escapa de las manos.",
		"La gema roja de su cuello brilla más fuerte... y los ojos del morongul se aclaran por un momento.",
		N("kraag", "...Aldric. Me llamaba Aldric. Defendí este paso hace cien años, antes de que la raíz me encontrara."),
		N("kraag", "Mátame y la Lágrima es tuya. O déjame vivir lo poco que me quede siendo yo.")])
	var i := await choose(["Perdonarle la vida", "Acabar con él"])
	if i == 0:
		setf("kraag_spared")
		await say([N("kraag", "...Gracias. Toma. Esta sangre es mía, la del campeón que fui. No la del monstruo."),
			"Aldric se corta la palma y deja caer una gota sobre la gema. Se vuelve de un rojo limpio, casi dorado.",
			N("kraag", "Un aviso: la Torre Negra guía a la Madre. No sois los primeros héroes que intentan el Corazón."),
			"Aldric se aleja cojeando hacia las montañas. Los soldados moronguls que quedan huyen al verle.",
			"(Tu resistencia aumenta en +2 de forma permanente.)"])
		GameState.change_approval("yara", 3)
		GameState.change_yara_corruption(-3)
	else:
		setf("kraag_killed")
		await say(["Un solo golpe. El general no se defiende.",
			"La Lágrima de Sangre se tiñe de un rojo oscuro, espeso."])
		if has("kaelen"):
			await say([K("Un monstruo menos. No me mires así, Yara.")])
		GameState.change_approval("kaelen", 2)
		GameState.change_yara_corruption(3)
	await say(["Abrís las jaulas.",
		"Los vecinos de Tortosa salen a trompicones, llorando y riendo a la vez." if flag("bartolo_dead") else N("bartolo", "¡Chaval! ¡Sabía que alguien vendría! ¡Os invito a una ronda en cuanto volvamos a Tortosa!"),
		N("remei", "Dios te bendiga, criatura. Toma, lo que les robamos a esos brutos antes de que nos encerraran."),
		"(Recibes: %s.)" % give_items({"pocion_mayor": 3, "gold": 300}),
		"(Obtienes la Lágrima de Sangre. %d de 3.)" % (1 + int(flag("tear_stone")) + int(flag("tear_crystal")))])
	setf("tear_blood")
	Audio.sfx("magic", -4.0)
	for c in ["cage1", "cage2", "cage3"]:
		var node = world.props_by_id.get(c)
		if node:
			node.get_node("Sprite").texture = load("res://assets/sprites/cage_empty.png")
	await say(["(Vuelve al Claro de la Savia cuando tengas las tres lágrimas.)"])
	end()


func interact_lines(id: String) -> Array:
	if id.begins_with("cage"):
		if flag("tear_blood"):
			return ["Una jaula vacía. La puerta cuelga de un gozne."]
		return ["Una jaula de hierro. Dentro, alguien de Tortosa te mira con los ojos muy abiertos: «¡Sácanos de aquí!»"]
	return []
