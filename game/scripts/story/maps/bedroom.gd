extends "res://scripts/story/StoryBase.gd"


func music_override() -> String:
	if GameState.get_meta("tod", "day") == "night":
		return "night"
	return ""


func on_map_ready(_m: String) -> void:
	if not flag("woke_up"):
		setf("woke_up")
		begin()
		world.player.face(0)
		await wait(0.9)
		await say([
			"La luz de la mañana se cuela entre las contraventanas y te da de lleno en la cara.",
			"Desde abajo llega olor a pan recién hecho y el tintineo de los cacharros de tu madre.",
			"Otro día más en Tortosa... o eso crees."])
		end()
		return
	if flag("night_attack") and not flag("night_woke"):
		setf("night_woke")
		begin()
		await wait(0.8)
		Audio.sfx("growl", -2.0, 0.8)
		await wait(0.5)
		await say([
			"Un grito te arranca del sueño.",
			"Luego otro. Y otro más. El cielo, al otro lado de la ventana, es de color naranja.",
			"Huele a humo."])
		world.player.face(2)
		await say([P("¿Qué...? ¡Madre!")])
		end()


func interact_lines(id: String) -> Array:
	var night: bool = GameState.get_meta("tod", "day") == "night"
	match id:
		"bed":
			if night:
				return ["No es momento de dormir. ¡Algo está pasando ahí fuera!"]
			return cycle("bed", [
				["Tu cama. Todavía está calentita...", "...pero ya has dormido más que suficiente. Hoy hace un día precioso."],
				"Ya has dormido bastante. ¡Arriba, que el día no espera!"])
		"nightstand":
			return ["Una vela casi consumida y un libro abierto: «Leyendas del Imperio de Cristal».",
				"Anoche te quedaste leyendo hasta tarde. Otra vez."]
		"wardrobe":
			return ["Tu ropa de siempre y la capa que heredaste de tu padre.", "Nada digno de un héroe... todavía."]
		"chest":
			return cycle("chest", [
				["Tu cofre de tesoros de la infancia.",
					"Una piedra con forma de corazón, una pluma de cuervo y un dibujo hecho por Yara:",
					"vosotros tres delante de la Catedral Vieja, y debajo, en letras torcidas: «LOS GUARDIANES DE TORTOSA».",
					"Sonríes sin querer."],
				"«Los Guardianes de Tortosa». Kaelen insistió en que él era el capitán."])
		"desk":
			return ["Un mapa de Vaelmoor copiado a mano. Tortosa es un puntito en los montes del norte.",
				"El resto del mundo parece enorme... y lleno de nombres que suenan a aventura."]
		"window":
			if night:
				return ["Por la ventana ves llamas sobre los tejados de Tortosa. Sombras deformes corren entre las casas."]
			return ["Desde la ventana se ve la Catedral Vieja, enorme y rota sobre los tejados.",
				"Más allá, el verde oscuro del Bosque Santo se pierde hacia el este."]
		"sword":
			return ["La espada de madera que Kaelen te talló cuando teníais ocho años.",
				"Tiene más muescas que filo. Aun así, nunca has sido capaz de tirarla."]
	return []


func on_interact(id: String) -> void:
	if id == "bed" and flag("council_done") and not flag("night_attack"):
		begin()
		var i := await choose(["Dormir", "Todavía no"], "¿Te acuestas? Mañana será un día largo.")
		if i == 0:
			setf("night_attack")
			GameState.set_meta("tod", "night")
			GameState.heal_all()
			await say(["Te tumbas. Tardas en dormirte: no dejas de pensar en la savia negra, en los ojos del lobo..."])
			end()
			Audio.stop_music(1.2)
			await Transition.fade_out(1.2)
			await wait(1.0)
			GameState.current_map = "bedroom"
			GameState.current_spawn = "start"
			Transition.busy = false
			Transition.go_to_scene("res://scenes/World.tscn", 0.8)
			return
		end()
		return
	await super.on_interact(id)
