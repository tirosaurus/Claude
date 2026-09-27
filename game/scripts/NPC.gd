extends Area2D
## NPC interactuable. companion_id vincula este NPC con una entrada de
## GameState.companions para llevar su aprobación/vínculo.

@export var companion_id: String = ""
@export var display_name: String = "???"
@export var dialogue_lines: Array[String] = ["..."]
@export var color: Color = Color(0.8, 0.35, 0.4)
## Si no está vacío, se activa como flag de GameState la primera vez
## que se habla con este NPC (útil para hitos narrativos simples).
@export var sets_flag_on_interact: String = ""

@onready var sprite: Sprite2D = $Sprite2D

var _line_index := 0


func _ready() -> void:
	add_to_group("interactable")
	if sprite.texture == null:
		var img := Image.create(16, 24, false, Image.FORMAT_RGBA8)
		img.fill(color)
		sprite.texture = ImageTexture.create_from_image(img)


func interact() -> void:
	var dialogue_box := get_tree().get_first_node_in_group("dialogue_box")
	if dialogue_box == null:
		return
	var line: String = dialogue_lines[_line_index % dialogue_lines.size()]
	_line_index += 1

	var approval := 0
	if companion_id != "" and GameState.companions.has(companion_id):
		approval = GameState.companions[companion_id].approval

	dialogue_box.show_line(display_name, line, companion_id, approval)

	if companion_id != "":
		GameState.change_approval(companion_id, 2)

	if sets_flag_on_interact != "":
		GameState.set_flag(sets_flag_on_interact)
