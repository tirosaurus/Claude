extends "res://scripts/story/StoryBase.gd"


func music_override() -> String:
	return "cathedral"


func encounters_disabled() -> bool:
	return flag("seed_taken")


func on_map_ready(_m: String) -> void:
	if flag("seed_taken"):
		world.set_prop_sprite("seed", "seed_empty")
		return
	if flag("won_custodian"):
		await _seed_scene()
		return
	if not flag("crypt_seen"):
		setf("crypt_seen")
		begin()
		await wait(0.5)
		await say(["Las escaleras bajan hasta una cripta helada. Sarcófagos sin nombre flanquean el pasillo.",
			"Al fondo, una luz azul late despacio. Como un corazón.", Y("Es ella. La Semilla. Puedo sentirla.")])
		end()


func on_trigger(id: String) -> void:
	match id:
		"custodian_zone":
			if not flag("won_custodian"):
				await _custodian()
		"seed_zone":
			if flag("won_custodian") and not flag("seed_taken"):
				await _seed_scene()


func _custodian() -> void:
	begin()
	Audio.stop_music(0.6)
	await wait(0.3)
	world.player.face(0)
	Audio.sfx("magic", 0.0, 0.5)
	await say(["Los cristales de la sala se encienden todos a la vez.",
		"Del suelo, junto al pedestal, se levanta una figura enorme de piedra y cristal.",
		{"who": "Custodio", "text": "INTRUSOS. LA SEMILLA DUERME. NADIE LA DESPIERTA."},
		K("¡No somos ladrones! ¡Venimos a protegerla!"),
		{"who": "Custodio", "text": "TODOS DICEN LO MISMO."},
		Y("Tiene un núcleo de cristal en el pecho. Si le damos con rayos o luz...")])
	await battle("custodian", ["custodian"], "crypt", "boss", true)


func _seed_scene() -> void:
	begin()
	setf("seed_taken")
	await wait(0.4)
	await walk_party_to(Vector2(160, 440), 60)
	world.player.face(3)
	await say(["El Custodio se deshace en polvo brillante. El silencio vuelve a la cripta.",
		"Sobre el pedestal flota una semilla de cristal del tamaño de un puño. Late. Respira.",
		"Cuando te acercas, oyes una voz lejana dentro de tu cabeza: «...por fin...»"])
	var em = world.spawn_npc("emissary", Vector2(260, 400), 1)
	em.sprite.modulate.a = 0.0
	var tw := create_tween()
	tw.tween_property(em.sprite, "modulate:a", 1.0, 1.0)
	Audio.sfx("dark", -2.0)
	await tw.finished
	await say([
		{"who": "emissary", "text": "Qué conmovedor. Tres niños de pueblo custodiando el mayor secreto del norte."},
		K("¿Quién eres tú? ¡Aléjate de ella!"),
		{"who": "emissary", "text": "Tranquilo, espadachín. No he venido a pelear. Solo a mirar quién la lleva."},
		{"who": "emissary", "text": "La Torre Negra se interesa por las cosas que duran. Y esa semilla ha durado trescientos años."},
		{"who": "emissary", "text": "Volveremos a hablar. Cuando la Madre Raíz os tenga contra las cuerdas... sabréis dónde encontrarme."}])
	var tw2 := create_tween()
	tw2.tween_property(em.sprite, "modulate:a", 0.0, 0.8)
	await tw2.finished
	world.remove_npc("emissary")
	await say([Y("¿La Madre Raíz?"), K("Ni idea. Pero no me ha gustado nada ese tipo."),
		Y("Alguien tiene que llevar la Semilla. Quema al tocarla... y susurra.")])
	var opts := ["Yo la llevaré.", "Kaelen, llévala tú.", "Yara, tú la entiendes mejor que nadie."]
	var i := await choose(opts, K("¿Quién la lleva?"))
	match i:
		0:
			setf("seed_player")
			await say(["Coges la Semilla. Está fría como el hielo y caliente como una brasa a la vez.",
				"La voz vuelve, más clara: «...{name}...». Sabe tu nombre.", Y("¿Estás bien? Te has puesto pálid{o}."),
				P("Estoy bien. Vámonos.")])
		1:
			setf("seed_kaelen")
			GameState.change_kaelen_rivalry(-4)
			GameState.change_approval("kaelen", 6)
			await say([K("¿Yo? ...Vale. Vale. No te fallaré."),
				"Kaelen guarda la Semilla contra el pecho. Por un momento, sus ojos brillan con un reflejo violeta.",
				K("Susurra, es verdad. Dice que soy fuerte. Ja. Por fin alguien que lo nota.")])
		2:
			setf("seed_yara")
			GameState.change_approval("yara", 5)
			GameState.change_yara_corruption(12)
			await say([Y("Es... preciosa. Y está sufriendo. Puedo sentirlo."),
				"Yara acuna la Semilla entre las manos. La luz azul se tiñe, apenas, de un matiz oscuro.",
				Y("Dice que puede curarlo todo. Que solo necesita... que le haga caso.")])
	setf("night_over")
	GameState.set_meta("tod", "dawn")
	GameState.heal_all()
	var i := await choose(["Volver a la superficie ahora.", "Explorar la cripta antes de salir."],
		K("La Semilla está a salvo. ¿Nos vamos?") if has("kaelen") else "La Semilla está a salvo. ¿Qué hacéis?")
	if i == 1:
		await say(["(Tómate tu tiempo: abre cofres y explora. Cuando quieras salir, sube por las escaleras del norte.)"])
		end()
		return
	await say(["Subís de nuevo a la Catedral. Fuera, el cielo empieza a clarear."])
	end()
	Transition.go_to_map("village_dawn", "cathedral_front")


func interact_lines(id: String) -> Array:
	match id:
		"seed":
			return ["El pedestal está vacío. Todavía se nota el calor de la Semilla en la piedra."] if flag("seed_taken") else []
	return []
