extends Area2D
## Encuentro de combate del bosque. Solo se activa si ya te has
## encontrado con Yara (requiere el flag correspondiente).

@export var required_flag: String = "yara_met"
@export var enemy_name: String = "Lobo del Bosque Santo"
@export var enemy_max_hp: int = 40
@export var enemy_attack: int = 6
@export var xp_reward: int = 60
@export var return_scene: String = "res://scenes/Forest.tscn"

@export var defeat_flag: String = "forest_beast_defeated"

@onready var sprite: Sprite2D = $Sprite2D


func _ready() -> void:
	if GameState.has_flag(defeat_flag):
		queue_free()
		return
	add_to_group("interactable")
	if sprite.texture == null:
		var img := Image.create(20, 20, false, Image.FORMAT_RGBA8)
		img.fill(Color(0.5, 0.15, 0.15))
		sprite.texture = ImageTexture.create_from_image(img)
	queue_redraw()


func _draw() -> void:
	var font := ThemeDB.fallback_font
	var w := font.get_string_size(enemy_name, HORIZONTAL_ALIGNMENT_LEFT, -1, 8).x
	draw_string(font, Vector2(-w / 2.0, -14), enemy_name, HORIZONTAL_ALIGNMENT_LEFT, -1, 8, Color(1, 0.6, 0.6))


func interact() -> void:
	if required_flag != "" and not GameState.has_flag(required_flag):
		var dialogue_box = get_tree().get_first_node_in_group("dialogue_box")
		if dialogue_box:
			dialogue_box.show_line("", "Se oye algo moverse entre los árboles, pero de momento sigue su camino.", "", 0)
		return

	GameState.start_encounter({
		"name": enemy_name,
		"max_hp": enemy_max_hp,
		"attack": enemy_attack,
		"xp_reward": xp_reward,
		"defeat_flag": defeat_flag,
	}, return_scene)
