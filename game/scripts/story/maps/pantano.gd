extends "res://scripts/story/StoryBase.gd"
## Pantano de Selen: al sur del Claro. Lleva a las Ruinas de Cristal (oeste) y al Paso del Sur (este).


func on_map_ready(_m: String) -> void:
	if not flag("pantano_seen"):
		setf("pantano_seen")
		begin()
		world.show_banner("Pantano de Selen")
		await wait(0.5)
		await say(["Agua negra, árboles muertos y una niebla que se te mete en los huesos.",
			"Aquí se alzaba el Imperio de Cristal. Hoy solo quedan juncos... y lo que se esconde entre ellos.",
			Y("Al oeste se ven brillos entre la niebla. Y al este... humo. Mucho humo."),
			"(Oeste: Ruinas de Cristal. Este: Paso del Sur.)"])
		end()


func interact_lines(id: String) -> Array:
	match id:
		"sign_pantano":
			return ["Un poste podrido. Una flecha al oeste: «Selen». Otra al este, tachada con sangre: «Tortosa por el Paso»."]
		"selen_statue":
			if flag("tear_crystal"):
				if flag("selen_freed"):
					return ["La estatua de Selen parece sonreír. Juncos nuevos crecen a sus pies."]
				return ["La estatua de Selen se ha agrietado de arriba abajo."]
			return ["Una mujer encapuchada, con dos dagas cruzadas. Su rostro está erosionado, pero sus mejillas tienen surcos, como de llanto.",
				"«Selen, la Sombra. Mató a su emperador para salvar a su pueblo. Y lloró hasta convertirse en cristal.»"]
	return []
