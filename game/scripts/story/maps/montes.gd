extends "res://scripts/story/StoryBase.gd"
## Montes de Khazgurim: la tierra de los enanos, al este del Claro.


func on_map_ready(_m: String) -> void:
	if not flag("tear_stone"):
		world.spawn_npc("durgan", world.marker("durgan"), 0, 0, Appearance.tex("res://assets/chars/brom.png"))
	if not flag("montes_seen"):
		setf("montes_seen")
		begin()
		world.show_banner("Montes de Khazgurim")
		await wait(0.5)
		await say(["El bosque se acaba de golpe. Delante, montañas grises con las cumbres nevadas y un viento que corta.",
			"En la ladera, la boca de una mina con vigas de roble y farolillos apagados."])
		if has("brom"):
			await say([B("Khazgurim... Hace diez inviernos que no pisaba mi tierra."),
				B("Algo va mal. Las forjas deberían humear. Se oiría el martillo desde aquí.")])
		else:
			await say([Y("Esto está demasiado silencioso para ser una mina enana, ¿no?")])
		end()


func on_npc(id: String) -> void:
	if id == "durgan":
		await _durgan()
		return
	await super.on_npc(id)


func _durgan() -> void:
	begin()
	if flag("durgan_met"):
		await say([N("durgan", "La mina está al norte. Que Thrain guíe tu hacha. O tu lo que sea que lleves.")])
		end()
		return
	setf("durgan_met")
	await say([N("durgan", "¡Alto ahí! ... Bah, no sois moronguls. Los moronguls no huelen a flores."),
		N("durgan", "Soy Durgan, capataz de Khazgurim. O de lo que queda."),
		N("durgan", "Hace una luna, cavando en la veta honda, rompimos una pared. Detrás había savia negra... y un gusano del tamaño de una torre."),
		N("durgan", "Se tragó la galería entera. Media cuadrilla quedó atrapada dentro. Mi hijo entre ellos.")])
	if flag("brom_pardoned") and not has("brom"):
		await say([N("durgan", "Y un tal Brom, que volvió hace poco del bosque hablando de elfos. Entró a buscarlos solo, el muy cabezota.")])
	elif has("brom"):
		await say([N("durgan", "¿Brom? ¿Brom Barbarroja? ¡Te dábamos por muerto, bribón!"),
			B("Casi, Durgan. Casi. Estos chicos me salvaron el pellejo.")])
	await say([P("Buscamos la Lágrima de Piedra."),
		N("durgan", "La Lágrima... está en el santuario de Thrain, al fondo de la mina. Justo donde duerme el gusano."),
		N("durgan", "Matad a esa bestia y es vuestra. Y si podéis... sacad a mis chicos.")])
	end()


func interact_lines(id: String) -> Array:
	match id:
		"sign_montes":
			return ["«Khazgurim. Minas de Thrain. Visitantes: llamad fuerte. Moronguls: dad media vuelta.»"]
	return []
