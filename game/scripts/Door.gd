extends Area2D
## Puerta/salida: al entrar el jugador en el área, cambia de escena.
## Si required_flag está puesto y no se cumple, no deja pasar y en su
## lugar muestra blocked_message (si hay una caja de diálogo en escena).

@export var target_scene: String = ""
@export var required_flag: String = ""
@export var blocked_message: String = ""


func _ready() -> void:
	body_entered.connect(_on_body_entered)


func _on_body_entered(body: Node) -> void:
	if not body.is_in_group("player"):
		return
	if required_flag != "" and not GameState.has_flag(required_flag):
		if blocked_message != "":
			var dialogue_box := get_tree().get_first_node_in_group("dialogue_box")
			if dialogue_box:
				dialogue_box.show_line("", blocked_message, "", 0)
		return
	if target_scene != "":
		get_tree().change_scene_to_file(target_scene)
