extends "res://scripts/story/StoryBase.gd"


func _dusk() -> bool:
	return GameState.get_meta("tod", "day") == "dusk"


func exit_requirement(to: String, default_req: String) -> String:
	if to == "forest" and _dusk():
		return "never"
	return default_req


func exit_blocked_message(to: String, default_msg: String) -> String:
	if to == "forest" and _dusk():
		return "Ya anochece. Nadie en su sano juicio entraría ahora en el bosque."
	return default_msg


func on_map_ready(_m: String) -> void:
	if _dusk():
		await _dusk_ready()
		return
	var bart = world.spawn_npc("bartolo", world.marker("bartolo"), 3)
	world.spawn_npc("remei", world.marker("remei"), 1)
	world.spawn_npc("roc", world.marker("roc"), 1)
	var nil = world.spawn_npc("nil", world.marker("nil"), 0, NPCScript.Mode.WANDER)
	nil.wander_radius = 60.0
	bart.face(3)
	if flag("kaelen_joined"):
		return
	var kae = world.spawn_npc("kaelen", world.marker("kaelen_start"), 1)
	if GameState.current_spawn != "home_door":
		return
	begin()
	await wait(0.5)
	kae.face_towards(world.player.position)
	await kae.hop()
	await say([K("¡{name}! ¡Eh, {name}! ¡Aquí!")])
	await kae.walk_to(Vector2(world.player.position.x + 26, world.player.position.y + 18), 80)
	kae.face(1)
	world.player.face(2)
	await say([
		K("¡Por fin! Llevo media mañana tirando piedras al río para no morirme de aburrimiento."),
		K("En este pueblo nunca pasa nada. Nada. El viejo Bartolo dice que la Catedral está maldita, pero lleva cuarenta años diciendo lo mismo."),
		K("Yara se ha ido al Bosque Santo a por sus flores. Vamos a buscarla.")])
	var i := await choose(["¡Claro! Como en los viejos tiempos.", "¿Otra vez? Siempre acabas metiéndonos en líos.",
		"Vale, pero esta vez no te pierdas."], K("¿Vienes o te vas a quedar ahí plantad{o}?"))
	match i:
		0:
			GameState.change_kaelen_rivalry(-8)
			GameState.change_approval("kaelen", 5)
			await say([K("¡Esa es la actitud! Sabía que no me fallarías.")])
		1:
			GameState.change_kaelen_rivalry(6)
			await say([K("Y siempre salimos de ellos, ¿no? Bueno... casi siempre."),
				K("No seas aguafiestas. Si pasa algo, yo te cubro. Como siempre."),
				"Lo dice sonriendo, pero hay algo de pique en su voz."])
		2:
			GameState.change_approval("kaelen", 3)
			GameState.change_kaelen_rivalry(-3)
			await say([K("¡Fue UNA vez! Y tenía diez años. Y la niebla era muy espesa."), K("...Y no se lo cuentes a Yara. Otra vez.")])
	await say([K("El sendero del bosque sale por el este, pasado el cartel. ¡Vamos!"), "Kaelen se une al grupo."])
	setf("kaelen_joined")
	join("kaelen")
	end()


func _dusk_ready() -> void:
	world.spawn_npc("remei", world.marker("remei"), 1)
	world.spawn_npc("roc", world.marker("roc"), 1)
	var nil = world.spawn_npc("nil", Vector2(300, 330), 0, NPCScript.Mode.WANDER)
	nil.wander_radius = 40.0
	if flag("council_done"):
		world.spawn_npc("bartolo", Vector2(350, 290), 0)
		world.spawn_npc("mother", Vector2(180, 380), 3)
		return
	world.spawn_npc("bartolo", Vector2(368, 292), 0)
	var mom = world.spawn_npc("mother", Vector2(330, 300), 2)
	begin()
	world.show_banner("Tortosa, al anochecer")
	await wait(0.6)
	await walk_party_to(Vector2(380, 330), 70)
	world.player.face(3)
	await say([
		"En la plaza se ha reunido medio pueblo. Kaelen enseña a todos un frasco con la savia negra del lobo.",
		N("bartolo", "Savia podrida... Así empezó, según contaba mi abuelo. Primero los animales. Luego el bosque. Luego..."),
		N("roc", "Luego nada, Bartolo. Es un lobo enfermo. Mañana lo quemamos y en paz."),
		N("bartolo", "¿Y las runas de la piedra santa? Las he visto apagarse desde mi ventana, noche tras noche."),
		Y("Es verdad. Hoy apenas brillaban."),
		N("bartolo", "Escuchadme bien, muchachos. La Catedral no está en ruinas por casualidad. Guarda algo: la Semilla del Pacto."),
		N("bartolo", "Los primeros de Tortosa la custodiaron bajo el altar. Mientras esté a salvo, el bosque aguanta. Si algo la busca..."),
		K("¿Algo como qué?"),
		N("bartolo", "Como lo que vive en las Tierras del Sur. Los viejos los llamaban Moronguls. Los que pudren todo lo que tocan.")])
	var i := await choose(["Te creo, Bartolo. Deberíamos vigilar la Catedral.", "Son cuentos, abuelo. No asustes a la gente.",
		"Sea lo que sea, estaremos preparados."], N("bartolo", "¿Tú qué dices, {hijo} de Elena?"))
	match i:
		0:
			setf("believed_bartolo")
			GameState.change_approval("yara", 3)
			await say([N("bartolo", "Por fin alguien con sesera."), K("Genial. Ahora tendremos guardia en unas ruinas.")])
		1:
			GameState.change_approval("yara", -2)
			GameState.change_kaelen_rivalry(-2)
			await say([N("bartolo", "Ojalá tengas razón, criatura. Ojalá."), K("¡Eso digo yo!")])
		2:
			GameState.change_approval("kaelen", 2)
			await say([N("roc", "Así me gusta. Mi herrería está abierta: si queréis pociones o algo mejor, pasad antes de dormir.")])
	mom.face_towards(world.player.position)
	await say([
		M("Ya basta de historias por hoy. Todos a casa. {name}, tú también, que mañana hay que ir al molino."),
		Y("Buenas noches, {name}. Nos vemos mañana... si Bartolo no tiene razón."),
		K("Si mañana hay Moronguls, me pido el más feo."),
		"Kaelen y Yara se marchan a sus casas."])
	GameState.leave_party("kaelen")
	GameState.leave_party("yara")
	world.remove_npc("kaelen")
	world.remove_npc("yara")
	setf("council_done")
	await say(["(Puedes comprar en la herrería de Maese Roc antes de irte a dormir. Tu cama te espera en casa.)"])
	end()


func interact_lines(id: String) -> Array:
	match id:
		"cathedral":
			if _dusk():
				return ["La Catedral Vieja se recorta contra el cielo rojo. Por un instante te parece ver una luz azulada entre los escombros."]
			return cycle("cathedral", [
				["La Catedral Vieja.", "Sus torres rotas se alzan sobre Tortosa como los dedos de un gigante dormido.",
					"La gran puerta está sellada por los escombros desde antes de que naciera nadie del pueblo."],
				["Entre las piedras caídas se cuela un aire frío que huele a humedad... y a algo más antiguo."]])
		"well":
			return ["El pozo de la plaza. El agua está helada y tiene un regusto a hierro que antes no tenía."]
		"stall":
			return ["El puesto de la plaza está cerrado. Hoy medio pueblo está en el monte."]
		"sign_forest":
			return ["«→ Bosque Santo. Que la luz del pacto os guarde.»", "Alguien ha grabado debajo: «Nil estuvo aquí»."]
		"anvil":
			return ["El yunque de Maese Roc. Todavía está tibio."]
		"smithy":
			return ["La herrería de Maese Roc. Herraduras, azadas y, colgada en la pared, una espada de verdad que nunca vende."]
		"house_kaelen":
			return ["La casa de Kaelen. Su padre está en el monte con las cabras."]
		"house_brown":
			return ["La casa de los Ferrer, los molineros. Huele a harina y a manzanas asadas."]
		"house_south":
			return ["La casa de la Tía Remei. En la ventana, un gato naranja te juzga en silencio."]
		"house_player":
			return ["Tu casa."]
	return []


func on_npc(id: String) -> void:
	if id == "roc" and flag("act2_started"):
		begin()
		var n = npc("roc")
		if n:
			n.face_towards(world.player.position)
		var i := await choose(["Comprar", "Hablar", "Nada"], N("roc", "¿Qué te pongo? Pociones, pan, y si tienes coronas, acero de verdad."))
		end()
		if i == 0:
			await open_shop("roc")
		elif i == 1:
			begin()
			await say([N("roc", "Los mercaderes del sur hablaban de pueblos vacíos. Bah. ...Aunque les compré un hacha, por si acaso.")])
			end()
		return
	await super.on_npc(id)


func npc_lines(id: String) -> Array:
	match id:
		"bartolo":
			if _dusk():
				return [N("bartolo", "Esta noche no pienso dormir. Me quedaré mirando la Catedral.")]
			return cycle("bartolo", [
				[N("bartolo", "¿La Catedral? Nadie sabe quién la levantó. Mi abuelo decía que ya estaba en ruinas cuando llegaron los primeros de Tortosa.")],
				[N("bartolo", "Algunas noches se oyen campanas ahí dentro. Y esa catedral no tiene campanas desde hace siglos.")]])
		"remei":
			return cycle("remei", [[N("remei", "El agua del pozo está más fría estos días. Y sabe a hierro. Qué cosas.")],
				[N("remei", "Mi gato no quiere salir de casa. Ese bicho no le teme a nada... salvo a lo que yo no veo.")]])
		"roc":
			return cycle("roc", [[N("roc", "Han pasado mercaderes del sur contando cosas feas. Pueblos vacíos, bosques que se pudren en una noche.")],
				[N("roc", "Si algún día necesitas una espada de verdad, ven a verme. Pero trae algo más que sonrisas.")]])
		"nil":
			return cycle("nil", [[N("nil", "¿Vais al bosque? ¡Mi madre dice que no se puede pasar de la piedra santa!")],
				[N("nil", "¡Cuando sea mayor seré más fuerte que Kaelen! ¡Y más listo que Yara! ¡Y más...! ¡Más todo!")]])
		"mother":
			return [M("Vamos, a casa. Que ya es de noche.")]
	return []
