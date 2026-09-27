extends Area2D
## Puerta/salida: al entrar el jugador en el área, cambia de escena.
## Si required_flag está puesto y no se cumple, no deja pasar y en su
## lugar muestra blocked_message (si hay una caja de diálogo en escena).

@export var target_scene: String = ""
@export var required_flag: String = ""
@export var blocked_message: String = ""
@export var label: String = ""
@export var color: Color = Color(0.85, 0.75, 0.4, 0.8)


func _ready() -> void:
	body_entered.connect(_on_body_entered)
	queue_redraw()


func _draw() -> void:
	var shape_node := get_node_or_null("CollisionShape2D") as CollisionShape2D
	if shape_node == null or not shape_node.shape is RectangleShape2D:
		return
	var size: Vector2 = (shape_node.shape as RectangleShape2D).size
	var rect := Rect2(shape_node.position - size / 2.0, size)
	draw_rect(rect, color)
	if label != "":
		var font := ThemeDB.fallback_font
		var text_size := font.get_string_size(label, HORIZONTAL_ALIGNMENT_LEFT, -1, 8)
		draw_string(font, rect.get_center() + Vector2(-text_size.x / 2.0, 3), label, HORIZONTAL_ALIGNMENT_LEFT, -1, 8, Color.BLACK)


func _on_body_entered(body: Node) -> void:
	if not body.is_in_group("player"):
		return
	if required_flag != "" and not GameState.has_flag(required_flag):
		if blocked_message != "":
			var dialogue_box = get_tree().get_first_node_in_group("dialogue_box")
			if dialogue_box:
				dialogue_box.show_line("", blocked_message, "", 0)
		return
	if target_scene != "":
		# No se puede cambiar de escena dentro de un callback de física.
		get_tree().change_scene_to_file.call_deferred(target_scene)
