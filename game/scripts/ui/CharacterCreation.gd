extends Control

const RACE_ORDER := ["human", "elf", "dwarf", "beastkin", "darkblood", "reborn"]

var _name: LineEdit
var _race_idx := 0
var _race_label: Label
var _race_desc: Label
var _preview: Sprite2D
var _t := 0.0
var _dir := 0


func _ready() -> void:
	var bg := TextureRect.new()
	bg.texture = load("res://assets/bg/title.png")
	bg.size = Vector2(640, 360)
	bg.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
	bg.stretch_mode = TextureRect.STRETCH_SCALE
	bg.modulate = Color(0.55, 0.5, 0.6)
	add_child(bg)

	var panel := UIKit.panel(Rect2(90, 30, 460, 300))
	add_child(panel)
	var title := UIKit.label("Crea tu personaje", 24, Color(0.35, 0.18, 0.1))
	title.position = Vector2(0, 14)
	title.size = Vector2(460, 30)
	title.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	panel.add_child(title)

	var frame := UIKit.panel(Rect2(28, 60, 130, 160), true)
	panel.add_child(frame)
	var holder := Node2D.new()
	holder.position = Vector2(65, 118)
	frame.add_child(holder)
	_preview = Sprite2D.new()
	_preview.texture = load("res://assets/chars/player.png")
	_preview.hframes = 3
	_preview.vframes = 4
	_preview.scale = Vector2(4, 4)
	holder.add_child(_preview)

	var nl := UIKit.label("Nombre", 16)
	nl.position = Vector2(180, 62)
	panel.add_child(nl)
	_name = LineEdit.new()
	_name.placeholder_text = "Escribe tu nombre"
	_name.max_length = 14
	_name.position = Vector2(180, 84)
	_name.size = Vector2(250, 30)
	_name.add_theme_font_size_override("font_size", 16)
	panel.add_child(_name)

	var rl := UIKit.label("Raza", 16)
	rl.position = Vector2(180, 124)
	panel.add_child(rl)
	var prev := UIKit.button("<", 16)
	prev.position = Vector2(180, 146)
	prev.size = Vector2(30, 28)
	prev.pressed.connect(func(): _change_race(-1))
	panel.add_child(prev)
	_race_label = UIKit.label("", 18, Color(0.4, 0.15, 0.1))
	_race_label.position = Vector2(212, 148)
	_race_label.size = Vector2(186, 24)
	_race_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	panel.add_child(_race_label)
	var nxt := UIKit.button(">", 16)
	nxt.position = Vector2(400, 146)
	nxt.size = Vector2(30, 28)
	nxt.pressed.connect(func(): _change_race(1))
	panel.add_child(nxt)
	_race_desc = UIKit.label("", 13)
	_race_desc.position = Vector2(180, 180)
	_race_desc.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	_race_desc.size = Vector2(252, 52)
	panel.add_child(_race_desc)

	var note := UIKit.label("", 11, Color(0.45, 0.33, 0.27))
	note.position = Vector2(20, 228)
	note.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	note.size = Vector2(420, 26)
	note.text = "La raza cambiará cómo te trata el mundo. En esta demo el aspecto aún es humano."
	note.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	panel.add_child(note)

	var start := UIKit.button("Comenzar la aventura", 18)
	start.position = Vector2(130, 256)
	start.size = Vector2(200, 32)
	start.pressed.connect(_on_start)
	panel.add_child(start)

	_name.focus_neighbor_bottom = prev.get_path()
	prev.focus_neighbor_right = nxt.get_path()
	nxt.focus_neighbor_left = prev.get_path()
	prev.focus_neighbor_bottom = start.get_path()
	nxt.focus_neighbor_bottom = start.get_path()
	start.focus_neighbor_top = nxt.get_path()
	_name.text_submitted.connect(func(_t): start.grab_focus())
	_name.grab_focus()
	_change_race(0)


func _change_race(d: int) -> void:
	_race_idx = (_race_idx + d + RACE_ORDER.size()) % RACE_ORDER.size()
	var info: Dictionary = GameState.RACES[RACE_ORDER[_race_idx]]
	_race_label.text = str(info["name"])
	_race_desc.text = str(info["desc"])
	if d != 0:
		Audio.sfx("select", -10.0)


func _process(delta: float) -> void:
	_t += delta
	if int(_t / 1.6) % 4 != _dir:
		_dir = int(_t / 1.6) % 4
	var order := [0, 2, 3, 1]
	_preview.frame = order[_dir] * 3 + [1, 0, 2, 0][int(_t * 6.0) % 4]


func _on_start() -> void:
	var n := _name.text.strip_edges()
	if n == "":
		n = "Arlen"
	GameState.new_game(n, RACE_ORDER[_race_idx])
	Audio.sfx("confirm", -6.0)
	Audio.stop_music(1.0)
	Transition.go_to_scene("res://scenes/Intro.tscn", 0.6)
