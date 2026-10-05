extends "res://scripts/story/StoryBase.gd"
## Abismo de los Susurros: mazmorra post-juego de 10 pisos aleatorios bajo la Cueva de los Susurros.
## Cada piso elige al azar uno de los mapas abismo_*, qué cofres aparecen y qué contienen.

const FLOORS := 10
const VARIANTS := ["abismo_a", "abismo_b", "abismo_c", "abismo_d"]
const RARITY_BY_FLOOR := [
	{"common": 5, "rare": 4, "very_rare": 1},
	{"rare": 5, "very_rare": 3, "divine": 1},
	{"very_rare": 4, "divine": 3, "rare": 2},
]


func floor_n() -> int:
	return int(GameState.get_meta("abyss_floor", 1))


func on_map_ready(_m: String) -> void:
	_layout_chests()
	if flag("won_abyss_boss") and GameState.get_meta("abyss_boss_pending", false):
		await _after_boss()
		return
	if GameState.get_meta("abyss_banner_floor", 0) != floor_n():
		GameState.set_meta("abyss_banner_floor", floor_n())
		world.show_banner("Abismo de los Susurros · Piso %d/%d" % [floor_n(), FLOORS])
		if floor_n() == 1:
			begin()
			await wait(0.4)
			await say(["Las escaleras bajan y bajan. El aire huele a hierro y a tormenta.",
				"Los susurros de la cueva vienen de aquí abajo. Dicen tu nombre. Dicen otro nombre: «Vael».",
				"(Abismo de los Susurros: 10 pisos que cambian en cada visita. Cuanto más bajes, más duros los enemigos y mejor el botín.",
				"Salir por donde has entrado te devuelve a la superficie, pero el descenso empezará de nuevo.)"])
			end()
		elif floor_n() == FLOORS:
			begin()
			await say(["Último piso. Al fondo, junto a la escalera, algo dorado respira despacio."])
			end()


## Cada visita a un piso muestra entre 3 y 5 cofres, elegidos al azar.
func _layout_chests() -> void:
	var key := "abyss_chests_%d" % floor_n()
	var keep: Array = GameState.get_meta(key, [])
	if keep.is_empty():
		var ids: Array = []
		for pid in world.props_by_id.keys():
			if str(pid).begins_with("chest_ab"):
				ids.append(pid)
		ids.shuffle()
		keep = ids.slice(0, randi_range(3, mini(5, ids.size())))
		GameState.set_meta(key, keep)
	var opened: Array = GameState.get_meta("abyss_opened", [])
	for pid in world.props_by_id.keys().duplicate():
		if str(pid).begins_with("chest_ab") and not keep.has(pid):
			world.remove_prop(pid)
		elif opened.has("%d:%s" % [floor_n(), pid]):
			world.props_by_id[pid].get_node("Sprite").modulate = Color(0.55, 0.5, 0.5)


func open_chest(id: String) -> void:
	if not id.begins_with("chest_ab"):
		await super.open_chest(id)
		return
	var tag := "%d:%s" % [floor_n(), id]
	var opened: Array = GameState.get_meta("abyss_opened", [])
	begin()
	if opened.has(tag):
		await say(["El cofre está vacío."])
		end()
		return
	opened.append(tag)
	GameState.set_meta("abyss_opened", opened)
	Audio.sfx("chest", -4.0)
	world.props_by_id[id].get_node("Sprite").modulate = Color(0.55, 0.5, 0.5)
	var loot := _roll_loot()
	await say(["Abres el cofre: %s." % give_items(loot)])
	for k in loot:
		if DB.LEGENDARIES.has(k):
			Audio.sfx("levelup", -2.0, 0.8)
			await say(["¡Un objeto legendario! (%d de %d legendarios)" % [DB.LEGENDARIES.size() - GameState.missing_legendaries().size(), DB.LEGENDARIES.size()]])
	end()


func _roll_loot() -> Dictionary:
	var f := floor_n()
	var out := {}
	var missing := GameState.missing_legendaries()
	if f >= 5 and not missing.is_empty() and randf() < 0.035 * f:
		out[missing.pick_random()] = 1
		return out
	if randf() < 0.75:
		var e := _random_equip(f)
		if e != "":
			out[e] = 1
	var cons := ["pocion_mayor", "eter", "pluma", "flor_luna_item", "bomba"]
	out[cons.pick_random()] = randi_range(1, 2)
	out["gold"] = randi_range(40, 90) * f
	return out


func _random_equip(f: int) -> String:
	var weights: Dictionary = RARITY_BY_FLOOR[clampi((f - 1) / 4, 0, 2)]
	var total := 0
	for r in weights:
		total += int(weights[r])
	var roll := randi() % total
	var rar := ""
	for r in weights:
		roll -= int(weights[r])
		if roll < 0:
			rar = r
			break
	var pool: Array = []
	for eid in DB.EQUIP:
		if str(DB.EQUIP[eid].get("rarity", "")) == rar:
			pool.append(eid)
	return pool.pick_random() if not pool.is_empty() else ""


func before_exit(to: String) -> bool:
	if to == "abismo_next":
		if floor_n() >= FLOORS:
			world.player.position += Vector2(0, 14)
			_boss.call_deferred()
			return false
		GameState.set_meta("abyss_floor", floor_n() + 1)
		GameState.set_meta("abyss_chests_%d" % floor_n(), [])
		Transition.go_to_map(VARIANTS.pick_random(), "from_above", true)
		return false
	if to == "cave":
		world.player.position += Vector2(0, -16)
		_leave_prompt.call_deferred()
		return false
	return true


func _leave_prompt() -> void:
	begin()
	var i := await choose(["Seguir explorando", "Volver a la superficie (el descenso se reinicia)"], "¿Abandonas el Abismo?")
	end()
	if i == 1:
		_reset_run()
		Transition.go_to_map("cave", "from_abyss", true)


func _reset_run() -> void:
	GameState.set_meta("abyss_floor", 1)
	GameState.set_meta("abyss_opened", [])
	GameState.set_meta("abyss_banner_floor", 0)
	for k in range(1, FLOORS + 1):
		GameState.set_meta("abyss_chests_%d" % k, [])


func _boss() -> void:
	begin()
	Audio.stop_music(0.8)
	await say(["Junto a la escalera se alza una figura de cristal dorado, con una corona rota sobre la frente.",
		"«Soy el Heraldo de Vael. Guardo lo que el Primer Soberano dejó atrás antes de entregar su alma al acero.»",
		"«Demuéstrame que eres digno de su legado.»"])
	setf("won_abyss_boss", false)
	GameState.set_meta("abyss_boss_pending", true)
	await battle("abyss_boss", ["heraldo", "wisp", "wisp"], "crypt", "boss", true)


func _after_boss() -> void:
	begin()
	GameState.set_meta("abyss_boss_pending", false)
	var missing := GameState.missing_legendaries()
	var vael_missing: Array = missing.filter(func(e): return DB.VAEL_SET.has(e))
	await say(["El Heraldo se arrodilla y se deshace en polvo de oro.",
		"«El legado de Vael es tuyo... por ahora.»"])
	var loot := {"gold": 1500, "pocion_mayor": 2}
	if not missing.is_empty():
		loot[(vael_missing if not vael_missing.is_empty() else missing).pick_random()] = 1
	await say(["Recibes: %s." % give_items(loot)])
	missing = GameState.missing_legendaries()
	if missing.is_empty():
		Audio.sfx("levelup", 0.0, 0.7)
		await say(["Los catorce legendarios vibran a la vez. Por un instante, ves tu reflejo en el cristal... con una corona de luz.",
			"(Tienes todos los objetos legendarios.)" if flag("ov_done") else "(Tienes todos los objetos legendarios. Algo ha cambiado en ti. Quizá al final del camino lo descubras.)"])
	else:
		await say(["(Legendarios: %d de %d. Vuelve a bajar para buscar el resto.)" % [DB.LEGENDARIES.size() - missing.size(), DB.LEGENDARIES.size()]])
	_reset_run()
	end()
	Transition.go_to_map("cave", "from_abyss", true)
