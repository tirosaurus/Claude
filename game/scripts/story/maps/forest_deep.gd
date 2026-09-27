extends "res://scripts/story/StoryBase.gd"


var _root: Sprite2D


## Brom ya está peleando cuando llegas: se le ve (y se le oye) desde lejos.
func _stage_brom() -> void:
	var m: Vector2 = world.marker("brom")
	var brom = world.spawn_npc("brom", m, 1)
	var th = world.spawn_npc("thrall", m + Vector2(-30, 14), 2)
	_root = monster("res://assets/enemies/root.png", 2, m + Vector2(34, 10), 0.8, true)
	_brom_idle_fight(brom, th)


func _brom_idle_fight(brom, th) -> void:
	while is_instance_valid(th) and is_instance_valid(brom) and not flag("brom_seen"):
		await wait(randf_range(0.6, 1.1))
		if not is_instance_valid(th) or flag("brom_seen"):
			return
		await th.hop()
		if is_instance_valid(brom):
			await brom.hop()
		if world.player.position.distance_to(brom.position) < 260:
			Audio.sfx("hit", -18.0, randf_range(0.9, 1.2))


func on_map_ready(_m: String) -> void:
	if flag("won_brom_fight") and not flag("brom_event_done"):
		await _elves_arrive()
		return
	if not flag("won_brom_fight"):
		_stage_brom()
	if not flag("deep_seen"):
		setf("deep_seen")
		begin()
		await wait(0.5)
		await say(["Más allá de la piedra santa, el Bosque Santo deja de ser santo.",
			"Los árboles tienen la corteza violácea y blanda como carne. El suelo late bajo los pies.",
			Y("Duele... El bosque está sufriendo. Lo noto aquí, en el pecho."),
			K("Mantened los ojos abiertos. Aquí todo quiere comernos.")])
		if flag("seed_yara"):
			await say(["La Semilla, en las manos de Yara, brilla un poco más fuerte. Ella no parece notarlo."])
		elif flag("seed_kaelen"):
			await say(["Kaelen se lleva la mano al pecho, donde guarda la Semilla, y sonríe sin motivo."])
		elif flag("seed_player"):
			await say(["La Semilla te susurra al oído: «...aquí... más adentro... el Corazón...»"])
		end()


func on_trigger(id: String) -> void:
	if id == "brom_zone" and not flag("won_brom_fight"):
		await _brom()


func _brom() -> void:
	begin()
	setf("brom_seen")
	var brom = npc("brom")
	var th = npc("thrall")
	if brom == null or th == null:
		_stage_brom()
		brom = npc("brom")
		th = npc("thrall")
	await pan_to(world.marker("brom"), 0.9)
	var root := _root
	for k in 3:
		await move_node(th, th.position + Vector2(18, -8), 0.14)
		Audio.sfx("hit", -6.0)
		shake(2.0, 0.2)
		await brom.hop()
		await move_node(th, th.position + Vector2(-18, 8), 0.2)
		Audio.sfx("crit", -8.0, 1.2)
		dust(th.position, Color(0.6, 0.3, 0.7))
	await say([B("¡Atrás, bichos del demonio! ¡Atrás o os parto la cabeza!"),
		"Un enano pelirrojo, atrapado de cintura para abajo entre raíces negras, reparte hachazos a un siervo morongul.",
		"Detrás de él, una raíz reptante se enrosca buscando su cuello.",
		K("¡Hay alguien ahí!"), Y("¡Van a matarlo!")])
	await release_camera(0.3)
	await walk_party_to(world.marker("brom") + Vector2(-60, 0), 80)
	await say([B("¡¿Humanos?! ¡Pues no os quedéis mirando y echad una mano!")])
	await battle("brom_fight", ["thrall", "root", "larva"], "deep", "battle")


func _elves_arrive() -> void:
	begin()
	var brom = world.spawn_npc("brom", world.marker("brom"), 1)
	await say([B("¡Ja! ¡Así se hace! Brom, hijo de Thrain, de Khazgurim. Os debo una, humanos."),
		"Antes de que podáis responder, tres flechas se clavan en el suelo, a un palmo de vuestros pies."])
	var aelis = world.spawn_npc("aelis", world.marker("brom") + Vector2(40, -30), 1)
	var guard = world.spawn_npc("elf_guard", world.marker("brom") + Vector2(56, 0), 1)
	await say([A("Ni un paso más."),
		"Una elfa de pelo claro os apunta con un arco. Detrás, un guardia élfico hace lo mismo.",
		A("Soy Aelis, de la Corte de la Savia. Ese enano taló árboles sagrados en la linde de nuestras tierras."),
		B("¡Talamos UN árbol! ¡Uno! Y estaba muerto, podrido por esa porquería negra."),
		N("elf_guard", "La ley es la ley. El enano viene con nosotros. La Anciana decidirá. Probablemente, la horca."),
		A("Y vosotros... ¿qué hacen unos humanos tan dentro del bosque?")])
	match GameState.race():
		"elf":
			await say([A("Un momento. Tú... tienes sangre de los nuestros. ¿De qué rama vienes, herman{o}?"),
				P("De ninguna que conozcas. Crecí en Tortosa."), A("Curioso. Muy curioso.")])
		"dwarf":
			await say([A("Y otr{o} enan{o}. Genial. ¿Venís en manada ahora?"),
				B("¡Eh! ¡Un paisano! ¡Por las barbas de mi abuelo!")])
	if GameState.has_flag("seed_kaelen") or GameState.has_flag("seed_player") or GameState.has_flag("seed_yara"):
		await say([Y("Llevamos la Semilla del Pacto. Necesitamos a vuestra Anciana."),
			"Aelis baja el arco un palmo. Solo un palmo.", A("...La Semilla. Así que es cierto. Se ha despertado.")])
	var opts := ["Brom viene con nosotros. Nos ha ayudado.", "Lleváoslo. Vuestra ley, vuestras normas."]
	if GameState.race() == "elf":
		opts.append("(Elf{o}) Como sangre de la Savia, respondo por él.")
	if GameState.race() == "dwarf":
		opts.append("(Enan{o}) Brom, cuéntales qué viste bajo las minas.")
	var i := await choose(opts, A("¿Y bien? El enano. ¿Vais a interponeros?"))
	setf("brom_event_done")
	if i == 0:
		setf("brom_joined")
		setf("elves_hostile")
		GameState.change_approval("kaelen", 3)
		await say([A("Como queráis. Pero la Anciana sabrá esto."), "Aelis y el guardia se retiran entre los árboles.",
			B("¡Por mi hacha! Humanos con agallas. Mi hacha es vuestra hasta que la deuda esté pagada."), "Brom se une al grupo."])
		world.remove_npc("aelis")
		world.remove_npc("elf_guard")
		join("brom")
	elif i == 1:
		setf("aelis_joined")
		setf("brom_prisoner")
		GameState.change_approval("yara", -3)
		await say([B("¡¿Qué?! ¡Traidores! ¡Os salvé el pellejo!"), "El guardia ata a Brom y se lo lleva hacia el este.",
			A("Sabia decisión. La Anciana os recibirá. Y yo os acompañaré: la Semilla es asunto de mi pueblo."), "Aelis se une al grupo."])
		world.remove_npc("brom")
		world.remove_npc("elf_guard")
		join("aelis")
	else:
		setf("brom_joined")
		setf("elves_friendly")
		await say([A("...Está bien. Por esta vez."),
			A("Pero si el enano toca una sola rama, responderás tú."),
			B("¡Ja! ¡Trato hecho!") if GameState.race() == "elf" else B("Bajo las minas de Khazgurim hay grietas que no deberían existir. De ahí salió la savia negra. Cavamos demasiado hondo."),
			A("...Eso cambia las cosas. La Anciana debe oírlo."), "Brom se une al grupo. Aelis os guiará hasta el campamento."])
		world.remove_npc("aelis")
		world.remove_npc("elf_guard")
		join("brom")
		GameState.change_approval("aelis", 10)
	end()


func interact_lines(id: String) -> Array:
	return []
