extends "res://scripts/story/route.gd"

func intro() -> Array:
	return ["El bosque élfico se abre poco a poco. Los robles dejan paso a pinos altos y el aire huele a nieve.",
		B("¡Eso es aire de montaña! Por fin algo que se puede respirar.") if has("brom") else Y("Hace frío... y se oyen cosas moviéndose entre las rocas.")]

func sign_text() -> Array:
	return ["«Oeste: Claro de la Savia. Este: Montes de Khazgurim. Cuidado con las salamandras: muerden y queman.»"]
