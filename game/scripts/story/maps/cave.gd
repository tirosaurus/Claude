extends "res://scripts/story/StoryBase.gd"
## Cueva de los Susurros: mazmorra opcional con tesoros legendarios.


func on_map_ready(_m: String) -> void:
	if flag("won_cave_guardian") and not flag("cave_guardian_done"):
		await _after_guardian()
		return
	if not flag("cave_seen"):
		setf("cave_seen")
		begin()
		await wait(0.5)
		await say(["Una cueva húmeda, iluminada por cristales que laten como luciérnagas.",
			"Desde el fondo llegan susurros. Palabras en una lengua que nadie habla desde hace siglos.",
			Y("Aquí dentro hay algo muy antiguo, {name}. Y no creo que quiera visitas."),
			K("Lo antiguo suele ser valioso. Y lo valioso, peligroso. Me encanta."),
			"(Mazmorra opcional: enemigos duros y tesoros legendarios. Se recomienda nivel 8.)"])
		end()


func on_trigger(id: String) -> void:
	if id == "guardian_zone" and not flag("won_cave_guardian"):
		begin()
		Audio.stop_music(0.8)
		await say(["La estatua del fondo gira la cabeza. Piedra contra piedra.",
			"«Ermengol selló el Velo. Thrain abrió la montaña. Selen lloró a su emperador.»",
			"«Sus armas duermen aquí. Solo quien me venza podrá despertarlas.»"])
		await battle("cave_guardian", ["custodian", "spectre", "spectre"], "crypt", "boss", true)


func _after_guardian() -> void:
	begin()
	setf("cave_guardian_done")
	await say(["El guardián se desmorona en un montón de cristal apagado.",
		"Los susurros callan. Por primera vez en siglos, la cueva está en silencio.",
		K("Bueno. Ahora sí que me merezco lo que haya en esos cofres.")])
	end()


func on_interact(id: String) -> void:
	if id in ["chest_cave4", "chest_cave5"] and not flag("won_cave_guardian"):
		begin()
		await say(["Un sello de cristal rodea el cofre. Algo lo protege... la estatua del fondo te observa."])
		end()
		return
	await super.on_interact(id)


func exit_requirement(to: String, default_req: String) -> String:
	if to.begins_with("abismo"):
		return "" if flag("won_cave_guardian") and GameState.level >= DB.MAX_LEVEL else "abyss_open"
	return default_req


func exit_blocked_message(to: String, default_msg: String) -> String:
	if to.begins_with("abismo"):
		if not flag("won_cave_guardian"):
			return "Unas escaleras bajan hacia una oscuridad que susurra. El guardián del fondo te impide acercarte."
		return "Los susurros te rechazan: «Vuelve cuando hayas alcanzado tu máximo poder». (Necesitas nivel %d. Tienes nivel %d.)" % [DB.MAX_LEVEL, GameState.level]
	return default_msg


func before_exit(to: String) -> bool:
	if to.begins_with("abismo"):
		GameState.set_meta("abyss_floor", 1)
		GameState.set_meta("abyss_opened", [])
		GameState.set_meta("abyss_banner_floor", 0)
		for k in range(1, 11):
			GameState.set_meta("abyss_chests_%d" % k, [])
		Transition.go_to_map(["abismo_a", "abismo_b", "abismo_c", "abismo_d"].pick_random(), "from_above", true)
		return false
	return true


func interact_lines(id: String) -> Array:
	if id == "statue_broken":
		return ["Una estatua sin rostro. En su base, tres nombres: Ermengol, Thrain, Selen."]
	return []
