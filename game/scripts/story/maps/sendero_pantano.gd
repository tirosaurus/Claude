extends "res://scripts/story/route.gd"

func intro() -> Array:
	return ["El sendero baja y baja. Cada paso, el suelo es más blando y la luz más gris.",
		K("Si mis botas se llenan de barro, que conste que fue idea tuya.") if has("kaelen") else Y("Huele a agua quieta. No me gusta.")]

func sign_text() -> Array:
	return ["«Norte: Claro de la Savia. Sur: Pantano de Selen.» Alguien ha añadido debajo, con prisas: «No sigáis las luces»."]
