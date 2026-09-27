extends "res://scripts/story/StoryBase.gd"

const RUNES := {
	"rune_1": ["Fe", "La runa de la Fe se enciende con una luz azul.",
		"Grabado en la piedra: «Los primeros de Tortosa juraron guardar lo que la discípula enterró, aunque nadie recordara por qué.»"],
	"rune_2": ["Memoria", "La runa de la Memoria despierta. Las paredes susurran.",
		"Grabado: «El Archimago Velmoth rasgó el Velo buscando ser dios. Su discípula, Ysolde, plantó la Semilla para coser la herida.»"],
	"rune_3": ["Sacrificio", "La runa del Sacrificio arde, fría y blanca.",
		"Grabado: «Toda semilla pide tierra. Y la tierra, a veces, pide un corazón.»"],
}


func on_map_ready(_m: String) -> void:
	for r in RUNES:
		if flag("lit_" + r):
			world.set_prop_sprite(r, "rune_on")
	if flag("runes_done"):
		world.remove_prop("crypt_rubble")
	if not flag("cathedral_seen"):
		setf("cathedral_seen")
		begin()
		await wait(0.6)
		await say(["Por dentro, la Catedral es aún más grande de lo que parecía.",
			"Vidrieras de colores imposibles filtran la luz de las llamas. Huele a piedra húmeda, a incienso antiguo... y a savia.",
			K("Nunca habíamos entrado tan lejos. De críos no pasábamos de la puerta."),
			Y("Algo se mueve ahí dentro. Con cuidado."),
			"(Busca las tres runas del suelo, que brillan en azul: junto a la pared izquierda, junto a la derecha y en la esquina de abajo a la izquierda. Examínalas con E. Hay cofres escondidos.)"])
		end()


func on_interact(id: String) -> void:
	if RUNES.has(id):
		await _rune(id)
		return
	await super.on_interact(id)


func _rune(id: String) -> void:
	begin()
	if flag("lit_" + id):
		await say(["La runa de %s brilla con calma." % RUNES[id][0]])
		end()
		return
	setf("lit_" + id)
	world.set_prop_sprite(id, "rune_on")
	Audio.sfx("magic", -4.0)
	await say([RUNES[id][1], RUNES[id][2]])
	if id == "rune_3" and has("yara"):
		await say([Y("«La tierra pide un corazón»... No me gusta cómo suena eso.")])
	var count := 0
	for r in RUNES:
		if flag("lit_" + r):
			count += 1
	if count == 3:
		setf("runes_done")
		Audio.sfx("growl", -4.0, 0.5)
		await wait(0.4)
		world.remove_prop("crypt_rubble")
		await say(["Las tres runas brillan a la vez. Un temblor recorre la Catedral.",
			"En la esquina del fondo, los escombros se deslizan y dejan al descubierto unas escaleras que bajan a la oscuridad."])
	end()


func interact_lines(id: String) -> Array:
	match id:
		"crypt_rubble":
			var missing: Array = []
			var where := {"rune_1": "Fe (pared izquierda)", "rune_2": "Memoria (pared derecha)", "rune_3": "Sacrificio (abajo a la izquierda)"}
			for r in RUNES:
				if not flag("lit_" + r):
					missing.append(where[r])
			return ["Escombros que tapan unas escaleras. Tienen tallado el símbolo de tres runas.",
				"Faltan por encender: %s." % ", ".join(missing)]
		"altar":
			return ["Un altar de piedra cubierto con un paño rojo que el tiempo no ha logrado pudrir.",
				"En el centro hay una marca con forma de semilla, como si algo hubiera descansado aquí durante siglos."]
		"statue":
			return ["La estatua de una mujer con una semilla entre las manos. La placa está casi borrada: «Y...olde».",
				"A la otra estatua, rota, le falta la cabeza. Quizá era el Archimago."]
	return []
