extends Control
## Ranuras de guardado/carga. mode = "save" | "load"

signal closed(result: bool)

const MAP_NAMES := {
	"bedroom": "Tu habitación", "housemain": "Casa", "village": "Tortosa", "forest": "Bosque Santo",
	"village_night": "Tortosa en llamas", "village_dawn": "Tortosa, al alba", "cathedral": "La Catedral Vieja",
	"crypt": "La Cripta", "forest_deep": "Bosque Profundo", "elf_camp": "Claro de la Savia", "heart": "Corazón del Bosque",
}

var mode := "save"
var save_pos := Vector2.ZERO
var _buttons: Array = []
var _msg: Label


func setup(m: String, pos: Vector2 = Vector2.ZERO) -> void:
	mode = m
	save_pos = pos


func _ready() -> void:
	process_mode = Node.PROCESS_MODE_ALWAYS
	set_anchors_preset(Control.PRESET_FULL_RECT)
	var dim := ColorRect.new()
	dim.color = Color(0, 0, 0, 0.6)
	dim.set_anchors_preset(Control.PRESET_FULL_RECT)
	add_child(dim)
	var panel := UIKit.panel(Rect2(90, 50, 460, 260))
	add_child(panel)
	var t := UIKit.label("Guardar partida" if mode == "save" else "Cargar partida", 22, Color(0.35, 0.18, 0.1))
	t.position = Vector2(0, 12)
	t.size = Vector2(460, 30)
	t.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	panel.add_child(t)
	var box := VBoxContainer.new()
	box.position = Vector2(30, 50)
	box.size = Vector2(400, 180)
	box.add_theme_constant_override("separation", 6)
	panel.add_child(box)
	for i in GameState.SAVE_SLOTS:
		var b := UIKit.button(_slot_text(i), 13)
		b.custom_minimum_size = Vector2(400, 44)
		b.alignment = HORIZONTAL_ALIGNMENT_LEFT
		b.pressed.connect(_on_slot.bind(i))
		if mode == "load" and GameState.slot_info(i).is_empty():
			b.disabled = true
		box.add_child(b)
		_buttons.append(b)
	var back := UIKit.button("Volver", 14)
	back.custom_minimum_size = Vector2(120, 26)
	back.pressed.connect(func(): _close(false))
	box.add_child(back)
	_buttons.append(back)
	_msg = UIKit.label("", 13, Color(0.3, 0.45, 0.2))
	_msg.position = Vector2(0, 232)
	_msg.size = Vector2(460, 20)
	_msg.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	panel.add_child(_msg)
	for b in _buttons:
		if not b.disabled:
			b.grab_focus()
			break


func _slot_text(i: int) -> String:
	var info := GameState.slot_info(i)
	if info.is_empty():
		return "Ranura %d  —  vacía" % (i + 1)
	var pd: Dictionary = info["player_data"]
	var mapname: String = MAP_NAMES.get(str(info["map"]), str(info["map"]))
	return "Ranura %d  —  %s · Nivel %d · %s\n%s · %s" % [i + 1, pd["name"], int(info["level"]), mapname,
		GameState.format_time(float(info.get("play_time", 0))), str(info.get("saved_at", "")).replace("T", " ")]


func _on_slot(i: int) -> void:
	if mode == "save":
		if GameState.save_game(i, save_pos):
			Audio.sfx("save", -4.0)
			_buttons[i].text = _slot_text(i)
			_msg.text = "Partida guardada en la ranura %d." % (i + 1)
		else:
			_msg.text = "No se pudo guardar."
	else:
		if GameState.load_game(i):
			Audio.sfx("confirm", -4.0)
			get_tree().paused = false
			_close(true)
			Transition.go_to_scene("res://scenes/World.tscn", 0.6)


func _process(_d: float) -> void:
	if Input.is_action_just_pressed("cancel"):
		_close(false)


func _close(result: bool) -> void:
	closed.emit(result)
	queue_free()
