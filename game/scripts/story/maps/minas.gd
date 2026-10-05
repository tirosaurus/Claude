extends "res://scripts/story/StoryBase.gd"
## Minas de Khazgurim: mazmorra de la Lágrima de Piedra. Jefe: Gusano de Savia.


func music_override() -> String:
	return "mountain" if flag("tear_stone") else ""


func on_map_ready(_m: String) -> void:
	if flag("won_worm") and not flag("tear_stone"):
		await _after_worm()
		return
	if not flag("minas_seen"):
		setf("minas_seen")
		begin()
		await wait(0.4)
		await say(["Galerías de piedra tallada, vías de vagoneta y forjas que aún guardan brasas.",
			"El suelo tiembla de vez en cuando. Algo enorme se mueve bajo vuestros pies.",
			"(Mazmorra: Minas de Khazgurim. Busca el santuario de Thrain, al norte.)"])
		end()


func on_trigger(id: String) -> void:
	if id == "worm_zone" and not flag("won_worm"):
		begin()
		Audio.stop_music(0.8)
		shake(6.0, 1.0)
		await say(["El santuario de Thrain. Un altar de piedra con una gema parda, lisa como una lágrima.",
			"La roca detrás del altar revienta. Una boca redonda llena de dientes, goteando savia negra.",
			K("Vale. Eso es un gusano. Un gusano muy, muy grande.") if has("kaelen") else Y("Eso... es un gusano. Un gusano enorme. {name}, ¡cuidado!")])
		if has("brom"):
			await say([B("¡Por Thrain y por Khazgurim! ¡Hoy como gusano a la brasa!")])
		await battle("worm", ["gusano"], "crypt", "boss", true)


func _after_worm() -> void:
	begin()
	await say(["El gusano se desploma con un gemido que hace temblar la montaña entera.",
		"Por la galería del este se oyen golpes. Voces. «¡Aquí! ¡Estamos aquí!»",
		"Los mineros atrapados. Pero el techo cruje: la savia del gusano sigue manando por la grieta y se extiende por las vetas."])
	var i := await choose(["Sacar a los mineros primero (el túnel puede ceder)", "Sellar la grieta ya (los mineros quedarán dentro)"],
		"La savia avanza. No hay tiempo para las dos cosas.")
	if i == 0:
		setf("dwarves_saved")
		await say(["Apartáis roca a roca mientras la savia os quema los tobillos.",
			"Uno a uno salen los mineros: sucios, hambrientos, vivos."])
		if flag("brom_pardoned") and not has("brom"):
			await say([B("¡Sabía que vendríais! ¡Os lo dije, cerveza gratis de por vida!"),
				B("Y esta vez voy con vosotros. Khazgurim puede apañárselas sin mí un poco más.")])
			join("brom")
		await say(["Al final, la grieta se cierra sola, taponada por el cuerpo del gusano. Habéis tenido suerte."])
		GameState.change_approval("yara", 3)
		GameState.change_yara_corruption(-3)
	else:
		setf("dwarves_sealed")
		await say(["Derrumbáis la galería del este sobre la grieta. El polvo lo cubre todo.",
			"Los golpes al otro lado se hacen más débiles... y callan."])
		if has("yara"):
			await say([Y("...Había gente ahí, {name}.")])
		if has("kaelen"):
			await say([K("La savia habría llegado a la montaña entera. Era lo correcto. Creo.")])
		GameState.change_approval("yara", -3)
		GameState.change_approval("kaelen", 2)
		GameState.change_yara_corruption(3)
	await say(["Tomas la gema del altar. Pesa como una montaña y tiembla como un corazón.",
		"(Obtienes la Lágrima de Piedra. 1 de 3.)"])
	setf("tear_stone")
	Audio.sfx("magic", -4.0)
	place_waystone()
	await say(["Junto a ti, una piedra rúnica se enciende con luz azul. (Piedra de retorno: te lleva al Claro de la Savia cuando quieras.)"])
	end()


func on_interact(id: String) -> void:
	if id == "chest_mina3" and not flag("won_worm"):
		begin()
		await say(["El cofre está enterrado bajo escombros... y algo enorme respira detrás de la pared."])
		end()
		return
	if id == "trapped_dwarves":
		begin()
		if flag("dwarves_saved"):
			await say(["La galería está despejada. Los mineros se han ido a casa."])
		elif flag("dwarves_sealed"):
			await say(["Roca y silencio."])
		else:
			await say(["Una galería derrumbada. Se oyen golpes débiles al otro lado: «¡Socorro!»",
				"Imposible abrirla con el gusano haciendo temblar la montaña."])
		end()
		return
	if id == "forge":
		await rest_and_save("Las brasas de la forja aún calientan.")
		return
	await super.on_interact(id)
