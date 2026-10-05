extends "res://scripts/story/route.gd"

func encounters_disabled() -> bool:
	return false

func intro() -> Array:
	return ["Ilvanis os acompaña hasta donde empieza la podredumbre. Luego, sin decir nada, se da la vuelta.",
		"Los árboles se vuelven grises, después violetas. Las raíces se mueven cuando no las miras.",
		Y("Es como caminar dentro de algo vivo, {name}."), "(Hay una hoguera a mitad de camino. Último lugar seguro antes del Corazón.)"]

func sign_text() -> Array:
	return ["Una flecha élfica clavada en un tronco. Han tallado una sola palabra: «Volved»."]
