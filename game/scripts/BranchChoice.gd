extends CanvasLayer

signal branch_selected(branch: String)

@onready var buttons: Dictionary = {
	"melee": $Panel/VBox/MeleeButton,
	"ranged": $Panel/VBox/RangedButton,
	"dps": $Panel/VBox/DpsButton,
	"healer": $Panel/VBox/HealerButton,
	"tank": $Panel/VBox/TankButton,
}


func _ready() -> void:
	for branch_id in buttons:
		buttons[branch_id].pressed.connect(_on_branch_pressed.bind(branch_id))


func _on_branch_pressed(branch_id: String) -> void:
	GameState.choose_branch(branch_id)
	branch_selected.emit(branch_id)
	queue_free()
