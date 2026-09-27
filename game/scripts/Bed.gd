extends Area2D
## La cama del jugador. Solo da un poco de sabor inicial: ya se ha
## dormido bastante, hora de bajar.

@onready var sprite: Sprite2D = $Sprite2D


func _ready() -> void:
	add_to_group("interactable")
	if sprite.texture == null:
		var img := Image.create(48, 32, false, Image.FORMAT_RGBA8)
		img.fill(Color(0.55, 0.4, 0.75))
		sprite.texture = ImageTexture.create_from_image(img)


func interact() -> void:
	GameState.set_flag("slept", true)
	var dialogue_box := get_tree().get_first_node_in_group("dialogue_box")
	if dialogue_box:
		dialogue_box.show_line("", "Ya has dormido bastante por hoy. Mejor bajar.", "", 0)
