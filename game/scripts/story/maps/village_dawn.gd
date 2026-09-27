extends "res://scripts/story/StoryBase.gd"


func encounters_disabled() -> bool:
	return true


func exit_requirement(to: String, default_req: String) -> String:
	if to == "cathedral":
		return "never"
	return default_req


func exit_blocked_message(to: String, default_msg: String) -> String:
	if to == "cathedral":
		return "No hay nada más que hacer ahí dentro."
	return default_msg


func on_map_ready(_m: String) -> void:
	world.remove_prop("rubble")
	world.set_prop_sprite("cathedral", "cathedral_open")
	world.spawn_npc("remei", Vector2(398, 290), 1)
	world.spawn_npc("roc", Vector2(214, 262), 1)
	world.spawn_npc("mother", Vector2(330, 300), 2)
	if flag("nil_alive") or flag("chose_home"):
		var nil = world.spawn_npc("nil", Vector2(300, 330), 0, NPCScript.Mode.WANDER)
		nil.wander_radius = 30.0
	if flag("bartolo_alive"):
		world.spawn_npc("bartolo", Vector2(368, 250), 0)
	if flag("dawn_done"):
		return
	await _aftermath()


func _aftermath() -> void:
	begin()
	world.show_banner("Tortosa, al alba")
	await wait(0.5)
	await walk_party_to(Vector2(360, 300), 70)
	world.player.face(3)
	await say(["El sol sale sobre un pueblo herido.",
		"Media Tortosa está negra de hollín. Los vecinos apagan las últimas brasas en silencio."])
	var lost: Array = []
	if flag("bartolo_dead"):
		lost.append("Bartolo")
	if flag("nil_dead"):
		lost.append("Nil")
	if not lost.is_empty():
		await say(["Al pie de la Catedral, alguien ha cubierto con una manta a %s." % " y ".join(lost),
			"Tu madre deja una flor de luna sobre la manta. Luego te abraza muy fuerte, sin decir nada."])
	if flag("nil_dead"):
		await say([K("Fue culpa mía. Si hubiera ido yo a por el carro..."), Y("Kaelen...")])
		var i := await choose(["No fue culpa tuya. Fue mía. Yo elegí.", "Nadie tiene la culpa. Solo esas cosas.",
			"Llorar no le devolverá."], K("..."))
		match i:
			0:
				GameState.change_kaelen_rivalry(-6)
				GameState.change_approval("yara", 3)
				await say([K("...No lo cargues tú solo, ¿vale? Lo cargamos entre los dos.")])
			1:
				GameState.change_kaelen_rivalry(-2)
				await say([Y("Tiene razón. Los Moronguls hicieron esto. Nosotros no.")])
			2:
				GameState.change_kaelen_rivalry(8)
				GameState.change_approval("yara", -5)
				GameState.change_yara_corruption(4)
				await say([K("...Qué frío te has vuelto, {name}."), "Yara te mira como si no te reconociera."])
	elif flag("bartolo_dead"):
		await say([Y("Bartolo nos contaba cuentos a los tres, ¿os acordáis? Del ciervo blanco. Del Archimago."),
			K("Y nunca le creímos. Ni una sola vez.")])
	if flag("mother_wounded"):
		await say([M("Solo es un rasguño. No me mires así."), "Tiene el brazo vendado hasta el hombro."])
	var sage := "bartolo" if flag("bartolo_alive") else "remei"
	await say([
		N(sage, "La Semilla sola no basta. Hay que plantarla en el Corazón del Bosque, donde nació el pacto."),
		N(sage, "Y solo los elfos de la Corte de la Savia conocen el ritual. Tienen un campamento más allá de la piedra santa, en lo profundo."),
		K("¿Los elfos? ¿Existen de verdad?"),
		Y("Existen. Mi abuela comerciaba con ellos: hierbas por miel."),
		N(sage, "El bosque profundo está corrompido. Id preparados. Roc os venderá lo que podáis pagar.")])
	var j := await choose(["Partimos ya. No hay tiempo que perder.", "Primero me despido de mi madre.",
		"¿Y si nos quedamos a proteger el pueblo?"], K("¿Qué hacemos, {name}?"))
	match j:
		0:
			GameState.change_approval("kaelen", 3)
		1:
			GameState.change_approval("yara", 3)
			await say([M("Ven aquí."), "Tu madre te coloca bien la capa, como cuando eras pequeñ{o}.",
				M("Toma. Las flores de luna que me quedaban. Y vuelve, {name}. Vuelve."), "(Flor de luna ×2)"])
			GameState.add_item("flor_luna_item", 2)
		2:
			GameState.change_kaelen_rivalry(4)
			await say([K("¿Y esperar a la próxima noche? Volverán. Y serán más."),
				Y("Si no plantamos la Semilla, no habrá pueblo que proteger, {name}."), P("...Tenéis razón. Vamos.")])
	if j != 1:
		GameState.add_item("flor_luna_item", 1)
		await say([M("Llevaos esto, al menos. (Flor de luna ×1)")])
	GameState.add_item("pocion", 2)
	await say(["Los vecinos os dan lo poco que tienen: dos pociones y muchos abrazos.",
		"(El camino al bosque profundo sigue desde el este del Bosque Santo. Puedes comprar en la herrería de Roc.)"])
	setf("dawn_done")
	setf("act3_started")
	GameState.set_meta("tod", "day")
	end()


func on_npc(id: String) -> void:
	if id == "roc":
		begin()
		var n = npc("roc")
		if n:
			n.face_towards(world.player.position)
		var line := "Tengo el brazo hecho polvo, pero aún puedo venderos cosas." if flag("roc_wounded") else "Lo prometido: os vendo lo que necesitéis. Casi a precio de coste."
		var i := await choose(["Comprar", "Nada"], N("roc", line))
		end()
		if i == 0:
			await open_shop("roc")
		return
	await super.on_npc(id)


func npc_lines(id: String) -> Array:
	match id:
		"mother":
			return [M("Ve con cuidado. Y come algo por el camino, que te conozco.")]
		"remei":
			return cycle("remei_d", [[N("remei", "Mi gato ha vuelto. Con la cola chamuscada, pero ha vuelto. Como vosotros.")],
				[N("remei", "Los elfos no son malos, pero no les gustan las prisas. Ni los enanos.")]])
		"nil":
			if flag("chose_home") and not flag("nil_alive"):
				return [N("nil", "Roc me sacó... Tiene el brazo muy mal. Por mi culpa."), N("nil", "¡Cuando sea mayor seré herrero y le haré un brazo de hierro!")]
			return [N("nil", "¡Me salvasteis! ¡Cuando sea mayor seré un Guardián como vosotros!")]
		"bartolo":
			return [N("bartolo", "Fe, Memoria, Sacrificio. La última es la que más cuesta, muchachos. Recordadlo.")]
	return []


func interact_lines(id: String) -> Array:
	match id:
		"cathedral":
			return ["La Catedral, abierta por primera vez en siglos. Parece más pequeña a la luz del día."]
		"sign_forest":
			return ["«→ Bosque Santo.» Alguien ha añadido con carbón: «Volved pronto»."]
	return []
