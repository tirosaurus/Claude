extends CanvasLayer
## Caja de diálogo con retrato, nombre, texto con máquina de escribir y
## elecciones. Uso: await Dialogue.say([...]) / var i = await Dialogue.choose([...])

signal _advance
signal _chosen(index: int)

const CHARACTERS := {
	"player": {"name": "", "portrait": "player", "pitch": 1.0},
	"kaelen": {"name": "Kaelen", "portrait": "kaelen", "pitch": 0.85},
	"yara": {"name": "Yara", "portrait": "yara", "pitch": 1.25},
	"mother": {"name": "Madre", "portrait": "mother", "pitch": 1.1},
	"bartolo": {"name": "Bartolo", "portrait": "bartolo", "pitch": 0.7},
	"roc": {"name": "Maese Roc", "portrait": "roc", "pitch": 0.65},
	"nil": {"name": "Nil", "portrait": "nil", "pitch": 1.45},
	"remei": {"name": "Tía Remei", "portrait": "remei", "pitch": 1.05},
	"aelis": {"name": "Aelis", "portrait": "aelis", "pitch": 1.3},
	"brom": {"name": "Brom", "portrait": "brom", "pitch": 0.6},
	"ilvanis": {"name": "Anciana Ilvanis", "portrait": "ilvanis", "pitch": 0.95},
	"elf_guard": {"name": "Guardia élfico", "portrait": "elf_guard", "pitch": 1.0},
	"emissary": {"name": "El Emisario", "portrait": "emissary", "pitch": 0.5},
	"thrall": {"name": "Siervo", "portrait": "thrall", "pitch": 0.45},
	"durgan": {"name": "Capataz Durgan", "portrait": "roc", "pitch": 0.6},
	"selen": {"name": "Selen", "portrait": "villager_f", "pitch": 1.2},
	"kraag": {"name": "General Kraag", "portrait": "thrall", "pitch": 0.4},
}

const TEXT_COLOR := Color(0.24, 0.15, 0.11)
const CHARS_PER_SEC := 48.0

var active := false
var last_close_frame := -100
var _root: Control
var _panel: NinePatchRect
var _portrait_frame: NinePatchRect
var _portrait: TextureRect
var _name_plate: NinePatchRect
var _name_label: Label
var _text: RichTextLabel
var _next: TextureRect
var _choice_panel: NinePatchRect
var _choice_box: VBoxContainer
var _choice_labels: Array[Label] = []
var _choice_cursor: TextureRect

var _typing := false
var _waiting := false
var _choosing := false
var _choice_index := 0
var _visible_f := 0.0
var _blip_counter := 0
var _pitch := 1.0
var _ignore_frame := 0
var _t := 0.0


func _ready() -> void:
	layer = 50
	process_mode = Node.PROCESS_MODE_ALWAYS
	_root = Control.new()
	_root.set_anchors_preset(Control.PRESET_FULL_RECT)
	_root.mouse_filter = Control.MOUSE_FILTER_IGNORE
	add_child(_root)

	_panel = _nine("res://assets/ui/panel.png", Rect2(14, 268, 612, 84))
	_root.add_child(_panel)

	_portrait_frame = _nine("res://assets/ui/panel_dark.png", Rect2(18, 164, 108, 108))
	_root.add_child(_portrait_frame)
	_portrait = TextureRect.new()
	_portrait.position = Vector2(6, 6)
	_portrait.size = Vector2(96, 96)
	_portrait.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
	_portrait.stretch_mode = TextureRect.STRETCH_SCALE
	_portrait_frame.add_child(_portrait)

	_name_plate = _nine("res://assets/ui/panel_dark.png", Rect2(132, 244, 120, 30))
	_root.add_child(_name_plate)
	_name_label = Label.new()
	_name_label.position = Vector2(0, 3)
	_name_label.size = Vector2(120, 24)
	_name_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	_name_label.add_theme_font_size_override("font_size", 16)
	_name_label.add_theme_color_override("font_color", Color(0.98, 0.86, 0.55))
	_name_plate.add_child(_name_label)

	_text = RichTextLabel.new()
	_text.bbcode_enabled = true
	_text.scroll_active = false
	_text.fit_content = false
	_text.add_theme_font_size_override("normal_font_size", 16)
	_text.add_theme_font_size_override("bold_font_size", 16)
	_text.add_theme_color_override("default_color", TEXT_COLOR)
	_text.add_theme_constant_override("line_separation", 2)
	_root.add_child(_text)

	_next = TextureRect.new()
	_next.texture = load("res://assets/ui/cursor.png")
	_next.size = Vector2(16, 16)
	_next.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
	_next.rotation_degrees = 90
	_root.add_child(_next)

	_choice_panel = _nine("res://assets/ui/panel.png", Rect2(360, 150, 266, 110))
	_root.add_child(_choice_panel)
	_choice_box = VBoxContainer.new()
	_choice_box.position = Vector2(26, 12)
	_choice_box.add_theme_constant_override("separation", 4)
	_choice_panel.add_child(_choice_box)
	_choice_cursor = TextureRect.new()
	_choice_cursor.texture = load("res://assets/ui/cursor.png")
	_choice_cursor.size = Vector2(12, 12)
	_choice_cursor.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
	_choice_panel.add_child(_choice_cursor)

	_hide_all()


func _nine(path: String, r: Rect2) -> NinePatchRect:
	var n := NinePatchRect.new()
	n.texture = load(path)
	n.patch_margin_left = 8
	n.patch_margin_right = 8
	n.patch_margin_top = 8
	n.patch_margin_bottom = 8
	n.position = r.position
	n.size = r.size
	return n


func _hide_all() -> void:
	_panel.visible = false
	_portrait_frame.visible = false
	_name_plate.visible = false
	_text.visible = false
	_next.visible = false
	_choice_panel.visible = false


func say(lines: Array) -> void:
	active = true
	for line in lines:
		await _show_line(line, true)
	_hide_all()
	active = false
	last_close_frame = Engine.get_process_frames()


func choose(options: Array, prompt = null) -> int:
	active = true
	if prompt != null:
		await _show_line(prompt, false)
	_choice_panel.visible = true
	for l in _choice_labels:
		l.queue_free()
	_choice_labels.clear()
	var longest := 0.0
	var font := _text.get_theme_font("normal_font")
	for opt in options:
		var l := Label.new()
		l.text = _fmt(str(opt))
		l.add_theme_font_size_override("font_size", 16)
		l.add_theme_color_override("font_color", TEXT_COLOR)
		_choice_box.add_child(l)
		_choice_labels.append(l)
		longest = max(longest, font.get_string_size(l.text, HORIZONTAL_ALIGNMENT_LEFT, -1, 16).x)
	var h := options.size() * 24 + 20
	var w: float = clamp(longest + 50, 180, 600)
	_choice_panel.size = Vector2(w, h)
	_choice_panel.position = Vector2(626 - w, 240 - h)
	_choice_index = 0
	_choosing = true
	_ignore_frame = Engine.get_process_frames() + 1
	_update_choice()
	var idx: int = await _chosen
	_choosing = false
	_hide_all()
	active = false
	last_close_frame = Engine.get_process_frames()
	Audio.sfx("confirm", -8.0)
	return idx


func _fmt(s: String) -> String:
	return fmt(s)


static func fmt(s: String) -> String:
	var f := GameState.is_female()
	return s.replace("{name}", GameState.player_name()).replace("{o}", "a" if f else "o") \
		.replace("{os}", "as" if f else "os").replace("{hijo}", "hija" if f else "hijo") \
		.replace("{el}", "ella" if f else "él").replace("{e}", "a" if f else "e") \
		.replace("{raza}", {"human": "humana" if f else "humano", "elf": "elfa" if f else "elfo",
			"dwarf": "enana" if f else "enano"}.get(GameState.race(), "humano"))


func _show_line(line, wait_input: bool) -> void:
	var who := ""
	var text := ""
	if line is Dictionary:
		who = str(line.get("who", ""))
		text = str(line.get("text", ""))
	else:
		text = str(line)
	text = _fmt(text)
	_panel.visible = true
	_text.visible = true
	var has_portrait := who != "" and CHARACTERS.has(who)
	_portrait_frame.visible = has_portrait
	_name_plate.visible = who != ""
	if has_portrait:
		var info: Dictionary = CHARACTERS[who]
		if who in ["player", "kaelen", "yara", "aelis", "brom"]:
			_portrait.texture = Appearance.member_portrait(who)
		else:
			var pname: String = str(info["portrait"])
			if pname == "mother" and GameState.race() != "human":
				pname = "mother_" + GameState.race()
			_portrait.texture = Appearance.tex("res://assets/portraits/%s.png" % pname)
		_name_label.text = GameState.player_name() if who == "player" else str(info["name"])
		_pitch = float(info["pitch"])
		_name_plate.position = Vector2(132, 244)
		_text.position = Vector2(140, 280)
		_text.size = Vector2(474, 66)
	else:
		_pitch = 0.9
		_text.position = Vector2(32, 280)
		_text.size = Vector2(582, 66)
		if who != "":
			_name_label.text = who
			_name_plate.position = Vector2(24, 244)
	var italic := not has_portrait and who == ""
	_text.text = ("[i]%s[/i]" % text) if italic else text
	_text.visible_characters = 0
	_visible_f = 0.0
	_blip_counter = 0
	_typing = true
	_waiting = false
	_next.visible = false
	_ignore_frame = Engine.get_process_frames() + 1
	while _typing:
		await get_tree().process_frame
	if not wait_input:
		return
	_waiting = true
	_next.visible = true
	await _advance
	_waiting = false
	_next.visible = false


func _process(delta: float) -> void:
	_t += delta
	if _next.visible:
		_next.position = Vector2(610, 334 + sin(_t * 6.0) * 2.0)
	if not active:
		return
	var pressed := Input.is_action_just_pressed("interact") and Engine.get_process_frames() > _ignore_frame
	if _typing:
		var total := _text.get_total_character_count()
		var speed := CHARS_PER_SEC * Audio.text_speed_mult() * (3.0 if Input.is_action_pressed("cancel") else 1.0)
		_visible_f += delta * speed
		var shown := int(_visible_f)
		if shown != _text.visible_characters:
			_text.visible_characters = shown
			_blip_counter += 1
			if _blip_counter % 3 == 0:
				Audio.sfx("blip", -18.0, _pitch + randf_range(-0.05, 0.05))
		if pressed:
			shown = total
			_text.visible_characters = total
		if shown >= total:
			_text.visible_characters = -1
			_typing = false
		return
	if _choosing:
		if Input.is_action_just_pressed("move_up") or Input.is_action_just_pressed("ui_up"):
			_choice_index = (_choice_index - 1 + _choice_labels.size()) % _choice_labels.size()
			Audio.sfx("select", -14.0)
			_update_choice()
		elif Input.is_action_just_pressed("move_down") or Input.is_action_just_pressed("ui_down"):
			_choice_index = (_choice_index + 1) % _choice_labels.size()
			Audio.sfx("select", -14.0)
			_update_choice()
		elif pressed:
			_chosen.emit(_choice_index)
		return
	if _waiting and pressed:
		Audio.sfx("blip", -14.0, 1.4)
		_advance.emit()


func _update_choice() -> void:
	for i in _choice_labels.size():
		var l := _choice_labels[i]
		l.add_theme_color_override("font_color", Color(0.55, 0.2, 0.12) if i == _choice_index else TEXT_COLOR)
	_choice_cursor.position = Vector2(10, 16 + _choice_index * 24)
