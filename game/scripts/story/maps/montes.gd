extends "res://scripts/story/StoryBase.gd"
## Montes de Khazgurim: la tierra de los enanos, al este del Claro.


const DWARVES := {
	"hilda": {"name": "Hilda, la minera", "tint": Color(1.0, 0.85, 0.75), "face": 0},
	"torbin": {"name": "Viejo Torbin", "tint": Color(0.8, 0.8, 0.85), "face": 1},
	"bori": {"name": "Bori", "tint": Color(1.1, 1.0, 0.9), "face": 0},
	"kazrik": {"name": "Kazrik, guardia de la mina", "tint": Color(0.75, 0.8, 1.0), "face": 0},
}


func D(id: String, t: String) -> Dictionary:
	return {"who": DWARVES[id]["name"], "text": t}


func on_map_ready(_m: String) -> void:
	var brom_tex := Appearance.tex("res://assets/chars/brom.png")
	world.spawn_npc("durgan", world.marker("durgan"), 0, 0, brom_tex)
	for id in DWARVES:
		var n = world.spawn_npc(id, world.marker(id), int(DWARVES[id]["face"]), 0, brom_tex)
		n.sprite.modulate = DWARVES[id]["tint"]
		if id == "bori":
			n.scale = Vector2(0.8, 0.8)
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
	if DWARVES.has(id):
		var n = npc(id)
		if n:
			n.face_towards(world.player.position)
		begin()
		await say(_dwarf_lines(id))
		end()
		return
	await super.on_npc(id)


func _dwarf_lines(id: String) -> Array:
	var done := flag("tear_stone")
	var saved := flag("dwarves_saved")
	match id:
		"hilda":
			if not done:
				return [D(id, "Mi pico se quedó dentro, junto a mi hermano. El pico me da igual. Mi hermano no."),
					D(id, "Si bajas, ve por la galería del este. Los golpes que se oyen son nuestros chicos.")]
			if saved:
				return [D(id, "¡Mi hermano está vivo! Toma, no es mucho, pero es lo mejor que tengo: un abrazo de enana. Cuidado con las costillas.")]
			return [D(id, "...Sellaste la galería. Lo sé, lo sé. La savia. Era lo correcto."), D(id, "Déjame sola un rato, por favor.")]
		"torbin":
			if not done:
				return [D(id, "Ochenta años picando piedra y nunca vi una veta sangrar. Eso que hay ahí abajo no es de este mundo, muchacho."),
					D(id, "Thrain, el fundador, decía que bajo Khazgurim duerme una lágrima de la tierra. Siempre creí que era un cuento.")]
			return [D(id, "Así que el cuento era verdad. La Lágrima de Piedra... Mi abuelo estaría orgulloso de haberla visto."),
				D(id, "Ve con cuidado, y no gastes la suerte enana toda de golpe.")]
		"bori":
			if not done:
				return [D(id, "¿Tú vas a matar al gusano gigante? ¿De verdad? ¿Me traes un diente?"),
					D(id, "Mi papá dice que los humanos son blandos como el pan. ¡Tú no pareces de pan!")]
			if saved:
				return [D(id, "¡Mi papá ha vuelto! ¡Dice que eres la persona menos de pan que ha visto nunca!")]
			return [D(id, "Mi papá todavía no ha vuelto de la mina... ¿Tú sabes cuándo vuelve?")]
		"kazrik":
			if not done:
				if has("brom"):
					return [D(id, "¿Brom Barbarroja? ¡Por las barbas de Thrain! Pasa, pasa. Y tus amigos también, supongo."),
						B("Kazrik, sigues teniendo cara de yunque."), D(id, "Y tú de cerveza rancia. Bienvenido a casa.")]
				return [D(id, "Alto. Nadie entra en la mina sin... bah. Nadie quiere entrar en la mina. Adelante, valientes.")]
			return [D(id, "Desde que cayó el gusano, la montaña ha dejado de temblar. Volvemos a oír el martillo. Gracias, forastero.")]
	return []


func interact_lines(id: String) -> Array:
	match id:
		"sign_montes":
			return ["«Khazgurim. Minas de Thrain. Visitantes: llamad fuerte. Moronguls: dad media vuelta.»"]
		"dwarf_forge":
			return ["Una forja enana. Aunque la mina esté en peligro, aquí nadie deja que se apague el fuego."]
	return []


func _durgan() -> void:
	begin()
	if flag("tear_stone"):
		if flag("dwarves_saved"):
			await say([N("durgan", "¡Mi hijo está en casa gracias a ti! Khazgurim no olvida. Si alguna vez necesitas un hacha, aquí tienes cien.")])
		else:
			await say([N("durgan", "Hiciste lo que tenías que hacer. La montaña sigue en pie. No te lo reprocho... pero no me pidas que sonría.")])
		end()
		return
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


