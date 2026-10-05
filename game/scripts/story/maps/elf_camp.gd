extends "res://scripts/story/StoryBase.gd"


func encounters_disabled() -> bool:
	return true


func on_map_ready(_m: String) -> void:
	world.spawn_npc("ilvanis", world.marker("ilvanis"), 0)
	world.spawn_npc("elf_guard", world.marker("shop"), 3)
	if flag("brom_prisoner") and not flag("brom_pardoned"):
		world.spawn_npc("brom", Vector2(500, 320), 0)
	if flag("camp_night") and not flag("act4_started"):
		GameState.set_meta("tod", "night")
	if flag("act4_started") and not flag("ready_for_heart"):
		_thorns()
	if not flag("camp_arrived"):
		await _arrival()
	elif flag("act4_started") and not flag("ready_for_heart") and _tears() == 3:
		await _tears_ritual()


func _tears() -> int:
	var n := 0
	for t in ["tear_stone", "tear_crystal", "tear_blood"]:
		if flag(t):
			n += 1
	return n


func _thorns() -> void:
	var s := Sprite2D.new()
	s.texture = load("res://assets/sprites/thorn_wall.png")
	s.centered = false
	s.offset = Vector2(-48, -52)
	s.position = Vector2(288, 30)
	world.entities.add_child(s)


func _tears_ritual() -> void:
	begin()
	await walk_party_to(world.marker("ilvanis") + Vector2(0, 36), 60)
	world.player.face(3)
	await say([N("ilvanis", "Piedra, cristal y sangre. Las Tres Lágrimas... Nunca pensé que volvería a verlas juntas."),
		"Colocas las tres lágrimas en las raíces del Árbol Madre. La luz dorada se vuelve blanca.",
		"Al norte, el Muro de Espinas cruje, se retuerce... y se abre como una flor marchita."])
	Audio.sfx("magic", -2.0)
	await world.set_tint(Color(1.3, 1.3, 1.1), 0.5)
	await world.set_tint(world._map_modulate(), 0.8)
	for c in world.entities.get_children():
		if c is Sprite2D and c.texture and c.texture.resource_path.ends_with("thorn_wall.png"):
			c.queue_free()
	await say([N("ilvanis", "Habéis visto el mundo que hay más allá de este bosque. Enanos, fantasmas, ejércitos."),
		N("ilvanis", "Todo lo que la Madre Raíz toca, se pudre. Y todo lo que habéis salvado por el camino, os mira ahora."),
		N("ilvanis", "El camino al Corazón está abierto. Recordad: la Semilla escucha. Luz... o sombra."),
		"(Consejo: guarda la partida antes de partir. No habrá vuelta atrás fácil.)"])
	setf("ready_for_heart")
	end()


func _arrival() -> void:
	setf("camp_arrived")
	begin()
	world.show_banner("Claro de la Savia")
	await wait(0.5)
	await say(["El bosque se abre de golpe en un claro dorado. Aquí la corrupción no ha llegado... todavía.",
		"Tiendas de tela verde, farolillos colgando de las ramas y, en el centro, un árbol inmenso que brilla con luz propia."])
	match GameState.race():
		"elf":
			await say([N("elf_guard", "Por la Savia... ¿Una elfa... un elfo criado entre humanos? Bienvenid{o} a casa, herman{o}.")])
		"dwarf":
			await say([N("elf_guard", "Un enano. En el Claro. La Anciana debe de estar perdiendo el juicio."),
				"Varios elfos te miran de reojo mientras pasas."])
		_:
			await say([N("elf_guard", "Humanos. Hacía cien años que no pisaba uno el Claro.")])
	if flag("elves_hostile"):
		await say([N("elf_guard", "Y con el enano profanador, además. La Anciana os verá. Pero no esperéis sonrisas.")])
	await walk_party_to(world.marker("ilvanis") + Vector2(0, 36), 60)
	world.player.face(3)
	await say([
		N("ilvanis", "Así que sois vosotros. Los niños que despertaron la Semilla."),
		N("ilvanis", "Soy Ilvanis, y he visto este bosque crecer durante seiscientos años. Nunca lo había visto enfermar así."),
		N("ilvanis", "Lo que os ataca no es una plaga. Es una raíz. La Madre Raíz: el primer brote de los Moronguls, que ha anidado en el Corazón del Bosque."),
		N("ilvanis", "Mientras viva, el bosque se pudrirá, y después vuestro pueblo, y después el mundo."),
		Y("¿Y la Semilla?"),
		N("ilvanis", "La Semilla es lo único que puede sanar el Corazón. Hay que plantarla donde late la Madre Raíz, una vez destruida."),
		N("ilvanis", "Pero la Semilla escucha a quien la lleva. Si su portador tiene el corazón turbio... la Semilla se enturbia con él.")])
	if flag("elves_hostile"):
		await say([N("ilvanis", "El ritual requiere la Lágrima de la Savia. No se la daré a quien protege a un profanador."),
			N("ilvanis", "Sin ella, la Semilla pedirá algo más a cambio de sanar. Algo mucho más caro."),
			B("Por mi culpa... Lo siento, chicos.")])
	else:
		setf("ritual_ok")
		await say([N("ilvanis", "Tomad: la Lágrima de la Savia. Cuando plantéis la Semilla, dejadla caer sobre ella."),
			"(Obtienes la Lágrima de la Savia.)"])
	await say([N("ilvanis", "Descansad esta noche en el Claro. Mañana, al alba, os abriré el camino al Corazón."),
		"(Habla con los elfos, compra en el intercambio y descansa junto a la hoguera para pasar la noche.)"])
	end()


func on_interact(id: String) -> void:
	if id == "campfire":
		if flag("camp_arrived") and not flag("camp_night"):
			begin()
			var i := await choose(["Pasar la noche", "Todavía no"], "¿Descansáis hasta mañana?")
			end()
			if i == 0:
				await _night()
			return
		await rest_and_save("La hoguera élfica crepita con llamas verdes.")
		return
	if id == "elf_shop":
		await open_shop("elves")
		return
	await super.on_interact(id)


func on_npc(id: String) -> void:
	match id:
		"elf_guard":
			begin()
			var i := await choose(["Comprar", "Nada"], N("elf_guard", "Intercambio élfico. Aceptamos coronas humanas, qué remedio."))
			end()
			if i == 0:
				await open_shop("elves")
			return
		"ilvanis":
			await _talk_ilvanis()
			return
		"brom":
			if flag("brom_prisoner") and not flag("brom_pardoned"):
				begin()
				await say([B("Vaya, vaya. Los que me vendieron. ¿Venís a ver al enano enjaulado?"),
					B("...Bah. No os guardo rencor. Mucho. Hablad con la vieja, a ver si os escucha a vosotros.")])
				end()
				return
	await super.on_npc(id)


func _talk_ilvanis() -> void:
	begin()
	var opts := ["¿Qué es exactamente la Madre Raíz?", "¿Quién era la discípula Ysolde?"]
	if flag("brom_prisoner") and not flag("brom_pardoned") and not flag("asked_pardon"):
		opts.append("Te pido clemencia para Brom.")
	opts.append("Nada más.")
	var i := await choose(opts, N("ilvanis", "¿Qué deseas saber?"))
	var sel: String = opts[i]
	if sel.begins_with("¿Qué es"):
		await say([N("ilvanis", "Cuando el Archimago rasgó el Velo, algo se coló por la herida: una podredumbre viva, hambrienta."),
			N("ilvanis", "Crece como una raíz. Cada Morongul es un brote suyo. La Madre Raíz es la primera, la más antigua."),
			N("ilvanis", "Y alguien la ha despertado. Sospecho de la Torre Negra... siempre están donde huele a poder.")])
	elif sel.begins_with("¿Quién"):
		await say([N("ilvanis", "Ysolde fue mi maestra. Humana. Plantó la Semilla para coser el Velo y murió al hacerlo."),
			N("ilvanis", "La tierra le pidió un corazón. Ella le dio el suyo."), Y("...«La tierra pide un corazón».")])
	elif sel.begins_with("Te pido"):
		setf("asked_pardon")
		var can := GameState.race() == "elf" or int(GameState.companions["yara"]["approval"]) >= 12 or flag("aelis_joined") and int(GameState.companions["aelis"]["approval"]) >= 5
		if can:
			setf("brom_pardoned")
			await say([N("ilvanis", "...Hablas con sinceridad. Y el enano talaba un árbol que ya estaba muerto."),
				N("ilvanis", "Sea. Será libre de volver a su montaña. Pero no con vosotros: su pueblo debe saber lo que ocurre bajo sus minas."),
				"Más tarde, Brom se despide con un abrazo que casi te rompe las costillas.",
				B("¡Os debo la vida dos veces! Si algún día pisáis Khazgurim, preguntad por Brom. ¡Cerveza gratis de por vida!")])
			world.remove_npc("brom")
		else:
			await say([N("ilvanis", "La ley de la Savia no se dobla por unas palabras bonitas, {raza}."),
				N("ilvanis", "Quizá, cuando el Corazón sane, lo reconsidere.")])
	else:
		await say([N("ilvanis", "Descansad. Mañana será el día más largo de vuestras vidas.")])
	end()


func _night() -> void:
	begin()
	setf("camp_night")
	GameState.set_meta("tod", "night")
	await Transition.fade_out(0.8)
	world.set_tint(Color(0.45, 0.48, 0.75), 0.01)
	await wait(0.3)
	await Transition.fade_in(0.8)
	await say(["Cae la noche sobre el Claro. Las luciérnagas se encienden una a una."])
	if has("kaelen"):
		await _kaelen_night()
	if has("yara"):
		await _yara_night()
	if has("aelis"):
		await say([A("No duermes, {raza}. Yo tampoco. El Corazón llama a todos los que llevamos savia en la sangre."),
			A("Mañana, dispara primero y pregunta después.")])
		GameState.change_approval("aelis", 3)
	elif has("brom"):
		await say([B("¿Sabes qué echo de menos? La piedra. Aquí todo es blando y verde y respira."),
			B("Mañana te cubro la espalda. Un enano paga sus deudas.")])
	await say(["Al final, el sueño os vence a todos."])
	await Transition.fade_out(1.0)
	GameState.set_meta("tod", "day")
	GameState.heal_all()
	world.set_tint(world._map_modulate(), 0.01)
	await wait(0.4)
	await Transition.fade_in(1.0)
	await say(["Amanece. Ilvanis os espera junto al gran árbol."])
	await walk_party_to(world.marker("ilvanis") + Vector2(0, 36), 60)
	world.player.face(3)
	await say([N("ilvanis", "Tengo malas noticias. Esta noche la Madre Raíz ha sentido la Semilla... y ha cerrado el Corazón."),
		N("ilvanis", "Un Muro de Espinas se alza al norte. Ni el fuego ni el acero lo atraviesan. Lo he intentado."),
		Y("¿Entonces? ¿Esperamos a que el bosque se muera?"),
		N("ilvanis", "No. Hay una forma. Cuando Ysolde plantó la Semilla, tres pueblos lloraron con ella: las Tres Lágrimas de la Savia."),
		N("ilvanis", "La Lágrima de Piedra, en las Minas de Khazgurim, al este, bajo los montes de los enanos."),
		N("ilvanis", "La Lágrima de Cristal, en las Ruinas del Imperio de Selen, más allá del pantano del sur."),
		N("ilvanis", "Y la Lágrima de Sangre... la llevaba el último campeón del Paso del Sur. Hoy ese paso es un campamento de guerra morongul."),
		N("ilvanis", "Traedme las tres y el Muro caerá."),
		K("Tres mazmorras, tres tesoros. Por fin algo que suena a aventura de verdad.") if has("kaelen") else Y("Tres viajes. Sin Kaelen. Bueno... habrá que apañárselas."),
		"(Acto IV: Las Tres Lágrimas. Salidas nuevas: el este lleva a los Montes; el sur, al Pantano de Selen.)"])
	setf("act4_started")
	_thorns()
	end()


func _kaelen_night() -> void:
	var r := GameState.rivalry()
	await say(["Kaelen está solo junto a la hoguera, afilando una espada élfica que no es suya."])
	if r >= 15:
		await say([K("¿Sabes qué me dijo la Semilla hoy? No, da igual.") if not flag("seed_kaelen") else K("La Semilla me habla, ¿sabes? Dice que soy fuerte. Que no necesito ir detrás de nadie."),
			K("Toda la vida igual. {name} decide, {name} elige, {name} es el héroe. Y Kaelen... Kaelen lleva las bolsas."),
			K("Ni siquiera Yara me mira ya.")])
		var i := await choose(["Tienes razón. Lo siento. Sin ti no habría llegado aquí.", "Deja de compadecerte. Esto no va de ti.",
			"Si tanto te molesta, puedes irte."], K("¿Nada que decir?"))
		match i:
			0:
				GameState.change_kaelen_rivalry(-18)
				GameState.change_approval("kaelen", 8)
				await say([K("..."), K("...Idiota. No hacía falta que lo dijeras. Pero... gracias."), "Por primera vez en días, Kaelen sonríe de verdad."])
			1:
				GameState.change_kaelen_rivalry(14)
				await say([K("No. Claro. Nunca va de mí.")])
			2:
				GameState.change_kaelen_rivalry(25)
				await say([K("...Puede que lo haga.")])
		if GameState.rivalry() >= 40:
			setf("kaelen_left")
			GameState.companions["kaelen"]["left"] = true
			GameState.leave_party("kaelen")
			world.remove_npc("kaelen")
			await say(["Esa noche, cuando despiertas, el sitio de Kaelen junto al fuego está vacío.",
				"Sus huellas se pierden hacia el norte. Hacia el Corazón."])
			if flag("seed_kaelen"):
				setf("seed_stolen")
				await say(["Y la Semilla se ha ido con él.", Y("No... Kaelen, ¿qué has hecho?")])
	elif r <= -20:
		await say([K("¿Te acuerdas del juramento de los Guardianes? Lo inventamos a los nueve años, en la Catedral."),
			K("«Juntos hasta el final, aunque el final sea feo.»")])
		var i2 := await choose(["Juntos hasta el final, hermano.", "Éramos unos críos.", "Espero que el final no sea tan feo."])
		match i2:
			0:
				setf("kaelen_oath")
				GameState.change_kaelen_rivalry(-10)
				await say([K("Hasta el final."), "Chocáis los antebrazos, como cuando erais niños. Algo pesado se os quita de encima a los dos."])
			1:
				GameState.change_kaelen_rivalry(4)
				await say([K("Ya. Supongo.")])
			2:
				setf("kaelen_oath")
				await say([K("Ja. Si lo es, al menos lo será con estilo.")])
	else:
		await say([K("Mañana vamos a pelear contra una raíz gigante. Con espadas. Si Bartolo nos viera..."),
			K("Oye. Pase lo que pase... me alegro de que fueras tú. Y no otr{o}.")])
		GameState.change_kaelen_rivalry(-4)


func _yara_night() -> void:
	await say(["Encuentras a Yara junto al lago de la luna, con los pies en el agua."])
	var c := GameState.corruption()
	if c >= 15:
		await say([Y("¿Puedo contarte un secreto? Oigo la Semilla. Incluso cuando no la tengo cerca."),
			Y("Me dice que puedo curarlo todo. A Bartolo, a Nil, a todos. Que solo tengo que... aceptar lo que me ofrece."),
			"Sus manos brillan con una luz violeta. Por un instante, sus ojos también."])
		var i := await choose(["Esa voz miente, Yara. Tú eres más fuerte que ella.", "¿Y si tiene razón? Podrías salvar a mucha gente.",
			"Si pasa algo, estaré a tu lado."], Y("¿Tú qué harías?"))
		match i:
			0:
				GameState.change_yara_corruption(-22)
				GameState.change_approval("yara", 5)
				await say([Y("...Tienes razón. Gracias por recordármelo."), "La luz violeta se apaga. Yara respira hondo, aliviada."])
			1:
				GameState.change_yara_corruption(22)
				await say([Y("Sí... Sí. Podría. ¿Verdad que podría?"), "Sonríe. No es su sonrisa de siempre."])
			2:
				GameState.change_yara_corruption(-8)
				GameState.change_approval("yara", 6)
				await say([Y("Lo sé. Siempre lo has estado.")])
	var ap: int = int(GameState.companions["yara"]["approval"])
	if ap >= 12 and GameState.corruption() < 50:
		await say([Y("¿Sabes? De pequeña pensaba que me casaría con uno de los dos. Con Kaelen o contigo. Nunca supe con quién."),
			"La luna se refleja en el agua. Yara te mira, esperando."])
		var opts := ["Me alegra que al final sea conmigo. (Besarla)", "Eres mi mejor amiga, Yara. Siempre.", "Kaelen te quiere, ¿lo sabías?"]
		if has("kaelen") and GameState.rivalry() <= -20 and int(GameState.companions["yara"]["kaelen_approval"]) >= 20:
			opts.append("¿Y si no tuvieras que elegir?")
		var j := await choose(opts)
		match j:
			0:
				setf("romance_yara")
				GameState.change_approval("yara", 10)
				GameState.change_yara_corruption(-10)
				GameState.change_kaelen_rivalry(8)
				await say(["Yara se ríe bajito y te besa. Sabe a flor de luna.", Y("Tardabas mucho, {name}.")])
			1:
				GameState.change_approval("yara", 3)
				await say([Y("Siempre. Los Guardianes de Tortosa, ¿no?"), "Apoya la cabeza en tu hombro un rato."])
			2:
				GameState.change_yara_kaelen_approval(30)
				GameState.change_kaelen_rivalry(-6)
				await say([Y("...Lo sé. Ese idiota nunca se atreve a decirlo."), Y("Gracias, {name}. De verdad.")])
			3:
				setf("romance_yara")
				setf("trio_ok")
				GameState.change_approval("yara", 8)
				GameState.change_yara_kaelen_approval(20)
				await say([Y("¿Qué quieres decir...?"), "Kaelen, que lo ha oído todo desde detrás de un árbol, carraspea.",
					K("Eh... Hola. Pasaba por aquí. Casualmente."), Y("...¿Los tres?"), K("Siempre hemos sido tres, ¿no?"),
					"Yara se ríe hasta que le salen lágrimas. Luego os abraza a los dos a la vez."])
	elif ap < 12:
		await say([Y("Mañana... ten cuidado, {name}. Aunque estemos un poco raros tú y yo, no quiero perderte.")])


func npc_lines(id: String) -> Array:
	match id:
		"brom":
			return [B("¡Ja!")]
	return []


func interact_lines(id: String) -> Array:
	match id:
		"sacred_tree":
			if flag("act4_started") and not flag("ready_for_heart"):
				return ["Las raíces del Árbol Madre tienen tres huecos con forma de lágrima. (Lágrimas: %d/3)" % _tears()]
			return ["El Árbol Madre de la Corte de la Savia. Su luz dorada pulsa despacio, como si respirara contigo."]
		"tent1", "tent2", "tent3":
			return ["Una tienda élfica. Huele a resina y a té de menta."]
	return []
