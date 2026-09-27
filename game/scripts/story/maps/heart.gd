extends "res://scripts/story/StoryBase.gd"


func music_override() -> String:
	return "heart"


func encounters_disabled() -> bool:
	return flag("final_started")


func on_map_ready(_m: String) -> void:
	if flag("won_kaelen_dark") and not flag("kaelen_resolved"):
		await _after_kaelen_fight()
		return
	if flag("won_horde") and not flag("kaelen_resolved"):
		await _after_horde()
		return
	if flag("won_nhalzur") and not flag("ov_end_chosen"):
		await _after_nhalzur()
		return
	if flag("won_final") and not flag("final_done"):
		await _after_final()
		return
	if not flag("heart_seen"):
		setf("heart_seen")
		begin()
		await wait(0.5)
		await say(["El Corazón del Bosque ya no es un bosque.",
			"Raíces gruesas como torres se retuercen formando pasillos de carne leñosa. Todo late. Todo respira.",
			"Y al fondo, muy arriba, algo enorme palpita con una luz violeta."])
		if has("aelis"):
			await say([A("Aquí jugaba de niña. Aquí aprendí a disparar. Y mirad lo que queda...")])
		if has("brom"):
			await say([B("He visto esto antes. En las grietas bajo Khazgurim. Esa luz... sale de la misma herida.")])
		end()


func on_trigger(id: String) -> void:
	match id:
		"kaelen_zone":
			if not flag("kaelen_event"):
				await _kaelen_event()
		"final_zone":
			if not flag("final_started"):
				await _final()


# ------------------------------------------------------------ Kaelen
func _kaelen_event() -> void:
	setf("kaelen_event")
	begin()
	if flag("kaelen_left"):
		setf("kaelen_dark")
		var kd = world.spawn_npc("kaelen", world.player.position + Vector2(0, -60), 0, 0, Appearance.tex("res://assets/chars/kaelen_dark.png"))
		await wait(0.3)
		await say(["Una figura os espera en mitad del camino. Tiene la piel gris y los ojos violetas.",
			{"who": "Kaelen", "text": "Llegáis tarde. Como siempre llegaba yo, ¿verdad, {name}?"}])
		if flag("seed_stolen"):
			await say([{"who": "Kaelen", "text": "La Semilla me lo explicó todo. La Madre Raíz no es el enemigo. Es poder. Y por una vez, el poder es mío."}])
		else:
			await say([{"who": "Kaelen", "text": "La Madre Raíz me ha escuchado. Por primera vez alguien me escucha a mí."}])
		if has("yara"):
			await say([Y("Kaelen, por favor. Vuelve con nosotros. Esto no eres tú.")])
		await say([{"who": "Kaelen", "text": "Esto soy yo. Por fin. Y si queréis pasar, tendréis que tumbarme."}])
		await battle("kaelen_dark", ["kaelen_dark"], "heart", "boss", true)
		return
	await say(["El suelo tiembla. De las raíces brotan siervos: diez, veinte... demasiados.",
		K("Son demasiados. Si nos quedamos, nos rodean."),
		K("Idos. Yo los contengo en este paso. Es estrecho: puedo aguantar.")])
	var opts := ["Ni hablar. Luchamos juntos.", "Confío en ti, Kaelen. Aguanta."]
	var i := await choose(opts, Y("¡Kaelen, no!"))
	if i == 0:
		setf("fought_horde")
		GameState.change_kaelen_rivalry(-6)
		await say([K("...Siempre tan cabezota. Vale. ¡Juntos!")])
		await battle("horde", ["thrall", "brute", "thrall"], "heart", "boss", true)
		return
	setf("kaelen_held")
	if has("brom"):
		setf("brom_helps_kaelen")
		await say([B("¡Ja! ¿Solo? ¡Ni en sueños, muchacho! ¡Un enano no deja a nadie atrás!"),
			"Brom se planta junto a Kaelen, hacha en mano.", K("...Gracias, barbas."), B("Ya me invitarás a una cerveza.")])
		GameState.leave_party("brom")
		world.remove_npc("brom")
	await say([K("{name}. Planta esa maldita semilla. Por Bartolo, por Tortosa... por nosotros."),
		"Kaelen se vuelve hacia la marea de siervos, con la espada en alto."])
	GameState.leave_party("kaelen")
	world.remove_npc("kaelen")
	setf("kaelen_resolved")
	end()


func _after_kaelen_fight() -> void:
	begin()
	var kd = world.spawn_npc("kaelen", world.player.position + Vector2(0, -40), 0, 0, Appearance.tex("res://assets/chars/kaelen_dark.png"))
	await say(["Kaelen cae de rodillas. La luz violeta de sus ojos parpadea.",
		{"who": "Kaelen", "text": "¿Por qué...? ¿Por qué siempre ganas tú...?"}])
	var opts := ["Porque nunca luché contra ti, Kaelen. Vuelve.", "Vete. No quiero volver a verte.", "Acaba esto. (Rematarlo)"]
	var i := await choose(opts)
	setf("kaelen_resolved")
	match i:
		0:
			if int(GameState.companions["kaelen"]["approval"]) >= 5 or flag("kaelen_oath") or GameState.rivalry() < 70:
				setf("kaelen_redeemed")
				GameState.set_flag("kaelen_dark", false)
				GameState.companions["kaelen"]["rivalry"] = 0
				await say(["Le tiendes la mano. Kaelen la mira mucho rato.",
					K("...Idiota. Siempre fuiste un{o} idiota."),
					"La coge. La luz violeta se apaga en sus ojos. Solo queda un reflejo oscuro, al fondo.",
					K("La Semilla... toma. No debí cogerla nunca.") if flag("seed_stolen") else K("Te debo una. Una muy grande.")])
				if flag("seed_stolen"):
					setf("seed_stolen", false)
					setf("seed_player")
				world.remove_npc("kaelen")
				join("kaelen")
				if has("yara"):
					GameState.change_approval("yara", 8)
					await say([Y("Estás aquí. Estás aquí de verdad."), "Yara abraza a Kaelen. Él no sabe qué hacer con los brazos."])
			else:
				setf("kaelen_lost")
				await say([K("Es tarde, {name}. Demasiado tarde."), "Kaelen se levanta tambaleándose y desaparece entre las raíces."])
				world.remove_npc("kaelen")
		1:
			setf("kaelen_lost")
			await say([K("...Como quieras. Siempre como tú quieras."), "Se aleja cojeando hacia la oscuridad."])
			world.remove_npc("kaelen")
		2:
			setf("kaelen_dead")
			GameState.companions["kaelen"]["alive"] = false
			GameState.change_approval("yara", -30)
			GameState.change_yara_corruption(25)
			await say(["Kaelen no se defiende. Cierra los ojos antes del golpe.", "Cuando todo acaba, el Corazón se queda en silencio."])
			if has("yara"):
				await say([Y("..."), Y("¿Qué has hecho, {name}? ¿Qué HAS HECHO?"), "Yara no vuelve a mirarte en mucho rato."])
			world.remove_npc("kaelen")
	if flag("seed_stolen"):
		setf("seed_stolen", false)
		setf("seed_player")
		await say(["Recoges la Semilla del suelo. Late, furiosa, en tu mano."])
	end()


func _after_horde() -> void:
	begin()
	setf("kaelen_resolved")
	await say(["El último siervo cae. Estáis vivos. Todos.", K("¡Ja! ¿Lo veis? Juntos somos imparables.")])
	end()


# ------------------------------------------------------------ Final
func _final() -> void:
	setf("final_started")
	begin()
	await walk_party_to(world.marker("final_spot") + Vector2(0, 40), 50)
	world.player.face(3)
	await say(["La Madre Raíz ocupa toda la caverna: un corazón monstruoso atravesado por raíces, con ojos que se abren y se cierran.",
		"Cada latido os golpea en el pecho."])
	if flag("ov_done") and not flag("ov_e3"):
		setf("ov_e3")
		Audio.sfx("dark", -4.0, 0.6)
		await say(["El arma del Soberano vibra como un animal que huele la sangre.",
			"«Aquí late. Debajo de la Madre Raíz, detrás del Velo. Nhal'Zur.»",
			"(Tercer eco del Soberano: 3/3)" if flag("ov_e1") and flag("ov_e2") else "«Te faltan ecos, heredero. No estás preparad" + GameState.g("o", "a") + " para verle la cara.»"])
		if flag("ov_e1") and flag("ov_e2"):
			setf("ov_ready")
	var em = world.spawn_npc("emissary", world.player.position + Vector2(-50, -10), 2)
	em.sprite.modulate.a = 0.0
	var tw := create_tween()
	tw.tween_property(em.sprite, "modulate:a", 1.0, 0.8)
	await tw.finished
	await say([
		{"who": "emissary", "text": "Os dije que volveríamos a hablar."},
		{"who": "emissary", "text": "Mirad a esa cosa. No podéis vencerla. Nadie puede... salvo la Torre Negra."},
		{"who": "emissary", "text": "Dadme la Semilla y la Madre Raíz se dormirá esta misma noche. Tortosa vivirá. Vuestras familias vivirán."},
		{"who": "emissary", "text": "Solo pedimos una cosa a cambio: que Tortosa, y su Catedral, nos pertenezcan. Un precio pequeño, ¿no?"}])
	var i := await choose(["Jamás. Tortosa no está en venta.", "...Trato hecho. Toma la Semilla."], {"who": "emissary", "text": "¿Y bien?"})
	if i == 1:
		setf("pact_accepted")
		await say(["Le entregas la Semilla. Sus dedos están helados.",
			{"who": "emissary", "text": "Sabia decisión. La Torre Negra recompensa a los sabios."},
			"Con un gesto, la Madre Raíz se encoge, gime, y se hunde en la tierra como un animal apaleado.",
			"El silencio que queda es peor que cualquier rugido."])
		end()
		_go_ending()
		return
	await say([{"who": "emissary", "text": "Qué lástima. Veremos cuánto dura vuestra luz."}])
	var tw2 := create_tween()
	tw2.tween_property(em.sprite, "modulate:a", 0.0, 0.6)
	await tw2.finished
	world.remove_npc("emissary")
	if has("yara") and GameState.corruption() >= 60:
		await _yara_turns()
	await say(["La Madre Raíz abre todos sus ojos a la vez.", "Es ahora o nunca."])
	await battle("final", ["mother_root"], "heart", "final", true)


func _yara_turns() -> void:
	await say([Y("Espera... La oigo. La Madre Raíz. Dice que puede darme todo el poder que necesito."),
		"La piel de Yara se vuelve pálida. Sus manos gotean luz violeta.",
		Y("Podría curar a todo el mundo, {name}. A todos. Solo tengo que dejarla entrar.")])
	var i := await choose(["Yara, mírame. Vuelve conmigo.", "Haz lo que tengas que hacer."])
	var ap: int = int(GameState.companions["yara"]["approval"])
	if i == 0 and (ap >= 15 or flag("romance_yara")):
		GameState.companions["yara"]["corruption"] = 30
		setf("yara_saved")
		await say(["Te mira. Tiembla. Y poco a poco, la luz violeta se retira de sus manos.",
			Y("...Estás aquí. Siempre estás aquí. Gracias.")])
	else:
		setf("yara_lost")
		GameState.leave_party("yara")
		world.remove_npc("yara")
		await say([Y("Adiós, {name}. Lo siento."), "Yara se funde con las raíces y desaparece en la oscuridad del Corazón."])


func _after_final() -> void:
	begin()
	setf("final_done")
	if flag("ov_ready"):
		await _nhalzur()
		return
	await say(["La Madre Raíz se estremece por última vez. Sus ojos se apagan uno a uno.",
		"Las raíces se agrietan. Por las grietas se cuela, por primera vez, un rayo de luz verde.",
		"La Semilla late con fuerza. Sabe que ha llegado su momento."])
	var carrier := "tú"
	if flag("seed_yara") and has("yara"):
		carrier = "Yara"
	elif flag("seed_kaelen") and has("kaelen"):
		carrier = "Kaelen"
	var opts := ["Plantar la Semilla y purificar el Corazón.", "Absorber el poder de la Semilla.", "Fundirte con la Semilla, como Ysolde."]
	var i := await choose(opts, "La Semilla espera. (La lleva %s.)" % carrier)
	match i:
		0:
			var pure := flag("ritual_ok") or (has("yara") and GameState.corruption() < 30)
			var tainted := (flag("seed_yara") and GameState.corruption() >= 40) or (flag("seed_kaelen") and GameState.rivalry() >= 40)
			if pure and not tainted:
				setf("ending_purify")
				await say(["La Semilla se hunde en la tierra negra." if not flag("ritual_ok") else "Dejas caer la Lágrima de la Savia sobre la Semilla y la hundes en la tierra negra.",
					"Un brote verde atraviesa la podredumbre. Luego otro. Y otro más.",
					"La luz se extiende como una ola por todo el Corazón del Bosque."])
			else:
				await say(["Plantas la Semilla... pero la tierra no la acepta. La luz se apaga, vacilante.",
					"Oyes la voz de Ysolde, lejana: «La tierra pide un corazón.»"])
				var opts2 := ["Darle el mío.", "No. No así."]
				if has("yara") and GameState.corruption() >= 60 and not flag("yara_saved"):
					opts2 = ["Darle el mío.", "No. No así."]
				var j := await choose(opts2)
				if j == 0:
					setf("ending_sacrifice")
				else:
					setf("ending_withered")
		1:
			setf("ending_king")
			await say(["Aprietas la Semilla contra tu pecho. El cristal se hunde en tu carne como si siempre hubiera sido tuyo.",
				"El poder te inunda. Las raíces se inclinan ante ti. La Madre Raíz no ha muerto: ahora eres tú."])
		2:
			setf("ending_sacrifice")
	end()
	_go_ending()


# ------------------------------------------------------------ Ruta del Soberano
func _nhalzur() -> void:
	Audio.stop_music(1.0)
	await say(["La Madre Raíz se estremece por última vez... y en lugar de morir, se abre como una flor podrida.",
		"Detrás de ella no hay tierra. Hay un desgarrón en el aire, negro y rojo, que cruje como hielo.",
		"El Velo."])
	shake(6.0, 1.0)
	var em = world.spawn_npc("emissary", world.player.position + Vector2(-40, -30), 2)
	await say([{"who": "emissary", "text": "Por fin. Mil años esperando a que alguien fuera lo bastante fuerte para matar a la Madre."},
		{"who": "emissary", "text": "Gracias, heredero de Vael. Me has abierto la puerta."},
		"La capucha del Emisario se hunde hacia dentro, como si detrás no hubiera cara. Su cuerpo se deshace en humo y entra en el desgarrón."])
	world.remove_npc("emissary")
	await world.set_tint(Color(0.5, 0.12, 0.22), 1.0)
	await say(["Del Velo sale una cabeza coronada de cristal negro. Luego unos brazos. Luego ojos, demasiados ojos.",
		"«NHAL'ZUR», dice la voz, y no es una voz: es hambre con forma de palabra.",
		"El arma del Soberano grita en tu mano: «¡AHORA, HEREDERO! ¡TERMINA LO QUE EMPEZAMOS!»"])
	if has("yara"):
		await say([Y("{name}... ¿Qué es eso? ¿Qué es esa arma?"), P("Luego te lo explico. Si hay un luego.")])
	end()
	await battle("nhalzur", ["nhalzur"], "heart", "soberano", true)


func _after_nhalzur() -> void:
	begin()
	await say(["Nhal'Zur se retuerce. Sus ojos se apagan uno a uno, y su grito hace temblar el mundo entero.",
		"El desgarrón del Velo empieza a cerrarse... pero no del todo. Algo lo mantiene abierto: el arma que llevas.",
		"«Heredero», susurra el Soberano. «Ya lo has visto. Hay dos caminos.»",
		"«Puedes quedarte conmigo. Ocupar mi trono. Con este acero, nadie volverá a amenazar Vaelmoor... nunca. Ni nadie te dirá nunca que no.»",
		"«O puedes romperme contra el Velo. Mi alma lo sellará para siempre, y yo descansaré por fin. Pero el arma desaparecerá contigo... o sin ti.»"])
	var i := await choose(["Ocupar el trono del Soberano.", "Romper el arma y sellar el Velo para siempre."], "El arma espera.")
	setf("ov_end_chosen")
	if i == 0:
		setf("ending_sovereign")
		await say(["Levantas el arma. La corona de cristal negro se forma sola sobre tu cabeza.",
			"Las raíces se inclinan. El Velo se cierra... a tu voluntad. Todo en Vaelmoor sabe, de golpe, quién manda."])
	else:
		setf("ending_sealed")
		var w := GameState.equipped("player", "weapon")
		if w.begins_with("overlord_"):
			GameState.equipment["player"].erase("weapon")
			Appearance.clear_cache()
		await say(["Clavas el arma en el desgarrón. El acero se agrieta, brilla en blanco... y estalla.",
			"«Gracias, heredero», dice Vael, y por primera vez su voz suena en paz.",
			"El Velo se cierra con un sonido de campana. En el Corazón del Bosque, la Semilla echa raíces sola.",
			"Y todo, por fin, se queda en silencio."])
	end()
	_go_ending()


func _go_ending() -> void:
	Audio.stop_music(1.5)
	await wait(0.5)
	Transition.go_to_scene("res://scenes/Ending.tscn", 1.4)
