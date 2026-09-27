extends Node
## Base del guion de cada mapa: atajos de diálogo, interacciones comunes
## (cofres, hogueras, tiendas) y ganchos que World invoca.

const NPCScript := preload("res://scripts/world/NPC.gd")
const ShopScript := preload("res://scripts/ui/ShopUI.gd")
const SaveScript := preload("res://scripts/ui/SaveUI.gd")

var world: Node


# ------------------------------------------------------------ Atajos
func P(t: String) -> Dictionary:
	return {"who": "player", "text": t}


func K(t: String) -> Dictionary:
	return {"who": "kaelen", "text": t}


func Y(t: String) -> Dictionary:
	return {"who": "yara", "text": t}


func M(t: String) -> Dictionary:
	return {"who": "mother", "text": t}


func A(t: String) -> Dictionary:
	return {"who": "aelis", "text": t}


func B(t: String) -> Dictionary:
	return {"who": "brom", "text": t}


func N(who: String, t: String) -> Dictionary:
	return {"who": who, "text": t}


func say(lines: Array) -> void:
	await Dialogue.say(lines)


func choose(options: Array, prompt = null) -> int:
	return await Dialogue.choose(options, prompt)


func begin() -> void:
	world.cutscene = true


func end() -> void:
	world.cutscene = false


func wait(sec: float) -> void:
	await get_tree().create_timer(sec).timeout


func flag(f: String) -> bool:
	return GameState.has_flag(f)


func setf(f: String, v: bool = true) -> void:
	GameState.set_flag(f, v)


func has(id: String) -> bool:
	return GameState.in_party(id)


func npc(id: String):
	return world.npcs.get(id)


func cycle(key: String, options: Array) -> Array:
	var n: int = int(GameState.get_meta("talk_" + key, 0))
	GameState.set_meta("talk_" + key, n + 1)
	var size := options.size()
	var idx := n
	if n >= size:
		idx = 1 + (n - 1) % (size - 1) if size > 1 else 0
	var pick = options[idx]
	return pick if pick is Array else [pick]


func battle(id: String, enemies: Array, bg: String, music: String = "", boss: bool = false) -> void:
	var enc := {"id": id, "enemies": enemies, "bg": bg, "boss": boss, "can_flee": not boss}
	if music != "":
		enc["music"] = music
	await world.start_battle(enc)


func join(id: String) -> void:
	GameState.join_party(id)
	world.make_follower(id)


func walk_party_to(pos: Vector2, speed: float = 60.0) -> void:
	await world.player.walk_to(pos, speed)


func rest_and_save(title: String = "Descansáis junto al fuego.") -> void:
	begin()
	var i := await choose(["Descansar y guardar", "Solo descansar", "Seguir"], title)
	if i <= 1:
		await world.set_tint(Color(0.3, 0.3, 0.4), 0.5)
		GameState.heal_all()
		Audio.sfx("heal", -4.0)
		await wait(0.6)
		world.set_tint(world._map_modulate(), 0.5)
		await say(["El grupo recupera todas sus fuerzas."])
		if i == 0:
			await open_save()
	end()


func open_save() -> void:
	var s := SaveScript.new()
	s.setup("save", world.player.position)
	world._hud.add_child(s)
	get_tree().paused = true
	await s.closed
	get_tree().paused = false


func open_shop(id: String) -> void:
	begin()
	var s := ShopScript.new()
	s.setup(id)
	world._hud.add_child(s)
	get_tree().paused = true
	await s.closed
	get_tree().paused = false
	end()


func give_items(items: Dictionary) -> String:
	var parts: Array = []
	for k in items:
		if k == "gold":
			GameState.gold += int(items[k])
			parts.append("%d coronas" % int(items[k]))
		else:
			GameState.add_item(k, int(items[k]))
			parts.append("%s ×%d" % [DB.ITEMS[k]["name"], int(items[k])])
	return ", ".join(parts)


# ------------------------------------------------------------ Ganchos (sobrescribir)
func music_override() -> String:
	return ""


func on_map_ready(_map_id: String) -> void:
	pass


func on_trigger(_id: String) -> void:
	pass


func encounters_disabled() -> bool:
	return false


func exit_requirement(_to: String, default_req: String) -> String:
	return default_req


func exit_blocked_message(_to: String, default_msg: String) -> String:
	return default_msg


func before_exit(_to: String) -> bool:
	return true


func is_interactable(id: String) -> bool:
	return not (id.ends_with("_zone"))


func on_interact(id: String) -> void:
	if id.begins_with("chest_"):
		await open_chest(id)
		return
	if id == "campfire":
		await rest_and_save()
		return
	var lines: Array = interact_lines(id)
	if lines.is_empty():
		return
	begin()
	await say(lines)
	end()


func interact_lines(_id: String) -> Array:
	return []


func open_chest(id: String) -> void:
	if flag("opened_" + id):
		begin()
		await say(["El cofre está vacío."])
		end()
		return
	setf("opened_" + id)
	Audio.sfx("chest", -4.0)
	var node = world.props_by_id.get(id)
	if node:
		node.get_node("Sprite").modulate = Color(0.55, 0.5, 0.5)
	begin()
	await say(["Abres el cofre: %s." % give_items(DB.CHESTS.get(id, {"pocion": 1}))])
	end()


func on_npc(id: String) -> void:
	var n = npc(id)
	if n and n.mode != NPCScript.Mode.FOLLOW:
		n.face_towards(world.player.position)
	var lines: Array = npc_lines(id)
	if lines.is_empty():
		lines = companion_banter(id)
	if lines.is_empty():
		return
	begin()
	await say(lines)
	end()


func npc_lines(_id: String) -> Array:
	return []


## Charla genérica con compañeros según el acto.
func companion_banter(id: String) -> Array:
	var act := 1
	if flag("act2_started"):
		act = 2
	if flag("act3_started"):
		act = 3
	if flag("ready_for_heart"):
		act = 4
	var key := "%s_%d" % [id, act]
	match key:
		"kaelen_1":
			return cycle(key, [[K("¿Te acuerdas de cuando nos colamos en la Catedral y casi nos aplasta una viga? Mejor día de mi vida.")],
				[K("Algún día me iré de Tortosa. A Puerto Ceniza, al Gremio de Aventureros. Y tú te vienes conmigo.")]])
		"kaelen_2":
			return cycle(key, [[K("No dejo de pensar en esas cosas... los siervos. Algunos llevaban ropa de pastor. Eran personas, {name}.")],
				[K("Si hubiera sido más rápido... No. No pienso en eso. Ahora no.")]])
		"kaelen_3":
			if GameState.rivalry() >= 30:
				return cycle(key, [[K("Tú decides, tú eliges, tú, tú, tú. ¿Te has dado cuenta de que ya nunca me preguntas?")]])
			return cycle(key, [[K("Los elfos me miran como si fuera un tronco con patas. Me caen bien, supongo.")],
				[K("Pase lo que pase ahí dentro, cúbreme el flanco izquierdo. Siempre se me olvida.")]])
		"kaelen_4":
			return [K("Ya casi estamos. Oye, {name}... gracias por no dejarme atrás.")] if GameState.rivalry() < 20 else [K("Terminemos con esto.")]
		"yara_1":
			return cycle(key, [[Y("La flor de luna cura casi cualquier cosa si sabes prepararla. Casi.")],
				[Y("A veces siento el bosque, ¿sabes? Como un latido. Hoy late... más despacio.")]])
		"yara_2":
			return cycle(key, [[Y("He curado tantas heridas esta noche que me tiemblan las manos.")],
				[Y("¿Por qué la Semilla me mira así? ...Olvídalo, digo tonterías.")]])
		"yara_3", "yara_4":
			if GameState.yara_is_witch():
				return cycle(key, [[Y("La oscuridad no es tan fría como dicen, {name}. Es... cálida. Poderosa.")],
					[Y("Mis flores ya no brillan. Ahora mis manos sí.")]])
			if flag("romance_yara"):
				return cycle(key, [[Y("Cuando esto acabe, quiero ver el mar contigo. ¿Me lo prometes?")],
					[Y("Deja de mirarme así, que me desconcentras.")]])
			return cycle(key, [[Y("Los elfos saben tanto de plantas... Ilvanis me ha enseñado tres ungüentos nuevos.")],
				[Y("Pase lo que pase, seguimos siendo los Guardianes de Tortosa. ¿Vale?")]])
		"aelis_3", "aelis_4":
			return cycle(key, [[A("No confío en los humanos. Pero confío en mis flechas, y te han visto luchar.")],
				[A("El Corazón fue lo primero que plantaron mis ancestros. Verlo podrido... duele.")]])
		"brom_3", "brom_4":
			return cycle(key, [[B("¡Ja! Otro bosque, otra pelea. En Khazgurim al menos los túneles no te muerden.")],
				[B("Esas larvas salen de abajo, lo sé. De donde cavamos demasiado hondo. Nuestra culpa, en parte.")]])
	return []
