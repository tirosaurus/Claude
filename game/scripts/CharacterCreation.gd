extends Control

@onready var name_edit: LineEdit = $VBox/NameEdit
@onready var race_option: OptionButton = $VBox/RaceOption
@onready var start_button: Button = $VBox/StartButton

const RACE_IDS := ["human", "elf", "dwarf", "beastkin", "darkblood", "reborn"]
const RACE_LABELS := [
	"Humano",
	"Elfo",
	"Enano",
	"Bestial",
	"Sangre-Oscura",
	"Renacido (no-muerto)",
]


func _ready() -> void:
	for label in RACE_LABELS:
		race_option.add_item(label)
	race_option.selected = 0
	start_button.pressed.connect(_on_start_pressed)


func _on_start_pressed() -> void:
	var chosen_name := name_edit.text.strip_edges()
	GameState.player_data.name = chosen_name if chosen_name != "" else "Viajero"
	GameState.player_data.race = RACE_IDS[race_option.selected]
	get_tree().change_scene_to_file("res://scenes/Bedroom.tscn")
