extends "res://scripts/story/StoryBase.gd"


func music_override() -> String:
	if flag("night_attack") and not flag("night_over"):
		return "night"
	if flag("night_over"):
		return "sad"
	return ""


func exit_requirement(_to: String, default_req: String) -> String:
	return default_req


func on_map_ready(_m: String) -> void:
	var tod: String = GameState.get_meta("tod", "day")
	if flag("night_over"):
		var mom = world.spawn_npc("mother", Vector2(150, 96), 0)
		if flag("mother_wounded"):
			mom.face(0)
		return
	if flag("night_attack"):
		if flag("mother_hidden"):
			return
		var mom2 = world.spawn_npc("mother", world.marker("mother"), 2)
		if GameState.current_spawn != "from_upstairs":
			return
		begin()
		await wait(0.3)
		await world.player.walk_to(world.player.position + Vector2(0, 14), 60)
		mom2.face_towards(world.player.position)
		await mom2.hop()
		await say([
			M("¡{name}! ¡Gracias a los dioses, estás bien!"),
			M("Algo ha salido del bosque. Cosas... cosas negras. Han prendido fuego a la casa de los Ferrer."),
			M("He oído a Kaelen gritar tu nombre hacia la plaza. Y a Yara, junto al pozo.")])
		var i := await choose(["Escóndete en la bodega. Voy a buscarlos.", "Ven conmigo, no pienso dejarte sola.",
			"Madre... ¿tú sabías que esto podía pasar?"], M("¿Qué hacemos?"))
		match i:
			0:
				await say([M("Ten cuidado, cariño. Y... coge esto."),
					"Te da la vieja espada de tu padre, envuelta en un trapo. Pesa más de lo que recordabas.",
					M("Él habría querido que la tuvieras.")])
			1:
				await say([M("Solo te estorbaría. Soy vieja, no tonta. Me esconderé en la bodega."),
					M("Pero coge esto: era de tu padre."), "La vieja espada familiar. Pesa más de lo que recordabas."])
			2:
				setf("asked_mother")
				await say([M("..."), M("Tu abuela decía que la Catedral guardaba algo. Que si el bosque callaba, vendrían."),
					M("Pensé que eran cuentos de vieja. Toma: la espada de tu padre. Y vete, ¡vete!")])
		setf("mother_hidden")
		GameState.add_item("pocion", 2)
		await say(["Obtienes 2 pociones del armario de la cocina."])
		await mom2.walk_to(Vector2(62, 96), 60)
		await mom2.walk_to(Vector2(62, 136), 60)
		await say(["Tu madre levanta la trampilla de la bodega y baja, sin dejar de mirarte."])
		var tw := create_tween()
		tw.tween_property(mom2, "modulate:a", 0.0, 0.5)
		await tw.finished
		world.remove_npc("mother")
		end()
		return
	var mom3 = world.spawn_npc("mother", world.marker("mother"), 1)
	if tod == "dusk" or flag("mother_talked"):
		return
	if GameState.current_spawn != "from_upstairs":
		return
	begin()
	await wait(0.3)
	await world.player.walk_to(world.player.position + Vector2(0, 14), 50)
	mom3.face_towards(world.player.position)
	await mom3.hop()
	await say([M("¡Mira quién se ha despertado! Ya pensaba que tendría que subir con el cubo de agua fría.")])
	await mom3.walk_to(world.player.position + Vector2(-44, 10), 50)
	mom3.face_towards(world.player.position)
	world.player.face(1)
	await say([M("Hoy hace muy buen día, de esos que ya casi no quedan.")])
	var i2 := await choose(["Como un tronco, madre.", "He tenido un sueño muy raro...", "¿Eso es pan recién hecho?"],
		M("¿Has descansado bien, cariño?"))
	if i2 == 2 and flag("ov1") and not flag("ov2"):
		setf("ov2")
		Audio.sfx("magic", -24.0, 0.5)
	match i2:
		0:
			await say([M("Eso es que tienes la conciencia tranquila. Disfrútalo mientras dure.")])
		1:
			setf("dream_told")
			await say([M("¿Raro? ¿Raro cómo?"),
				"Recuerdas raíces negras creciendo bajo el bosque, latiendo como venas... y una voz que susurraba tu nombre.",
				P("Nada... tonterías. Raíces, oscuridad... ya se me está olvidando."),
				M("Bah, será la cena de anoche. Te dije que no repitieras queso."),
				"Por un instante, a tu madre se le borra la sonrisa. Luego vuelve a remover el guiso como si nada."])
		2:
			await say([M("De hogaza, con la harina del molino de los Ferrer. Coge un trozo, anda, que estás en los huesos.")])
	await say([
		M("Por cierto, Kaelen ha pasado esta mañana preguntando por ti. Tres veces."),
		M("Anda, sal a que te dé el aire. Y volved antes de que anochezca, ¿eh? Últimamente el bosque... bueno, tú vuelve pronto.")])
	setf("mother_talked")
	end()


func interact_lines(id: String) -> Array:
	match id:
		"fireplace":
			if flag("night_attack") and not flag("night_over"):
				return ["El fuego de la chimenea sigue encendido, como si nada pasara ahí fuera."]
			return ["El guiso de tu madre burbujea al fuego. Huele a romero y a tomillo del monte."]
		"kitchen_table":
			if not flag("ate_bread"):
				setf("ate_bread")
				GameState.heal_all()
				GameState.add_item("pan", 1)
				return ["Pan de hogaza, queso de cabra y un tazón de leche.",
					"Comes algo y te guardas un trozo de pan en el zurrón. (Pan de hogaza ×1)"]
			return ["Ya has comido. Si comes más, tu madre te hará subir leña como pago."]
		"cupboard":
			return ["Tarros de miel, hierbas secas y un libro de recetas con la letra apretada de tu abuela.",
				"En la última página, alguien escribió: «Si el bosque calla, cerrad las puertas»."]
	return []


func npc_lines(id: String) -> Array:
	if id != "mother":
		return []
	var tod: String = GameState.get_meta("tod", "day")
	if flag("night_over"):
		if flag("mother_wounded"):
			return cycle("mom_dawn_w", [[M("No es nada, un rasguño... Remei dice que en una semana estaré como nueva."), M("Ve. Haz lo que tengas que hacer. Pero vuelve, ¿me oyes? Vuelve.")],
				[M("Tu padre estaría orgulloso. Y aterrado. Como yo.")]])
		return cycle("mom_dawn", [[M("Estoy bien, cariño. Gracias a ti."), M("Si tenéis que ir al bosque de los elfos... llévate provisiones. Y vuelve.")],
			[M("Tu padre estaría orgulloso de ti. Y aterrado. Como yo.")]])
	if tod == "dusk":
		return cycle("mom_dusk", [[M("¿Una reunión en la plaza? Ay... Bartolo y sus cuentos. Pero ve, ve."), M("Y luego a dormir, que tienes una cara que da miedo.")],
			[M("Tu cama está hecha. Descansa esta noche, ¿vale?")]])
	return cycle("mother", [[M("¿Todavía aquí? Kaelen va a echar la puerta abajo.")],
		[M("Coge algo de pan de la mesa si tienes hambre.")],
		[M("Y dile a Yara que me guarde unas flores de luna, que me quedan pocas.")]])
