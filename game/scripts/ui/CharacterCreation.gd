extends Control
## Creador de personaje: nombre, sexo, raza, peinado, colores y barba.

const RACE_ORDER := ["human", "elf", "dwarf"]
const ROWS := ["name", "sex", "race", "hair_style", "hair_color", "skin", "outfit", "beard", "difficulty", "start"]
const ROW_H := 25
const DIFFS := ["Fácil", "Normal", "Difícil"]
const LABELS := {"name": "Nombre", "sex": "Sexo", "race": "Raza", "hair_style": "Peinado", "hair_color": "Color de pelo",
	"skin": "Piel", "outfit": "Ropa", "beard": "Barba", "difficulty": "Dificultad", "start": ""}

var app := {}
var _row := 0
var _name: LineEdit
var _value_labels := {}
var _row_labels := {}
var _preview: Sprite2D
var _portrait: TextureRect
var _race_desc: Label
var _start: Label
var _cursor: TextureRect
var _t := 0.0
var _dir := 0


func _ready() -> void:
	app = GameState.default_appearance()
	var bg := TextureRect.new()
	bg.texture = load("res://assets/bg/title.png")
	bg.size = Vector2(640, 360)
	bg.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
	bg.stretch_mode = TextureRect.STRETCH_SCALE
	bg.modulate = Color(0.5, 0.45, 0.55)
	add_child(bg)
	var panel := UIKit.panel(Rect2(20, 12, 600, 336))
	add_child(panel)
	var title := UIKit.label("Crea tu personaje", 22, Color(0.35, 0.18, 0.1))
	title.position = Vector2(0, 10)
	title.size = Vector2(600, 28)
	title.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	panel.add_child(title)

	var frame := UIKit.panel(Rect2(22, 44, 200, 262), true)
	panel.add_child(frame)
	var holder := Node2D.new()
	holder.position = Vector2(50, 96)
	frame.add_child(holder)
	_preview = Sprite2D.new()
	_preview.hframes = 8
	_preview.vframes = 4
	_preview.scale = Vector2(4, 4)
	holder.add_child(_preview)
	_portrait = TextureRect.new()
	_portrait.position = Vector2(100, 40)
	_portrait.size = Vector2(88, 88)
	_portrait.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
	_portrait.stretch_mode = TextureRect.STRETCH_SCALE
	frame.add_child(_portrait)
	_race_desc = UIKit.label("", 11, UIKit.TEXT_LIGHT)
	_race_desc.position = Vector2(12, 152)
	_race_desc.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	_race_desc.size = Vector2(176, 100)
	frame.add_child(_race_desc)

	var y := 46
	for key in ROWS:
		if key == "start":
			break
		var l := UIKit.label(LABELS[key], 15)
		l.position = Vector2(250, y)
		panel.add_child(l)
		_row_labels[key] = l
		if key == "name":
			_name = LineEdit.new()
			_name.placeholder_text = "Escribe tu nombre"
			_name.max_length = 14
			_name.position = Vector2(390, y - 4)
			_name.size = Vector2(180, 26)
			_name.add_theme_font_size_override("font_size", 15)
			panel.add_child(_name)
		else:
			var v := UIKit.label("", 15, Color(0.45, 0.15, 0.1))
			v.position = Vector2(390, y)
			v.size = Vector2(180, 20)
			v.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
			panel.add_child(v)
			_value_labels[key] = v
			var la := UIKit.label("<", 15, Color(0.5, 0.35, 0.25))
			la.position = Vector2(380, y)
			panel.add_child(la)
			var ra := UIKit.label(">", 15, Color(0.5, 0.35, 0.25))
			ra.position = Vector2(566, y)
			panel.add_child(ra)
		y += ROW_H
	_start = UIKit.label("¡Comenzar la aventura!", 18, Color(0.45, 0.15, 0.1))
	_start.position = Vector2(250, y + 2)
	_start.size = Vector2(320, 26)
	_start.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	panel.add_child(_start)
	_cursor = TextureRect.new()
	_cursor.texture = load("res://assets/ui/hand.png")
	_cursor.size = Vector2(14, 12)
	panel.add_child(_cursor)
	var hint := UIKit.label("Flechas: elegir y cambiar  ·  Intro: confirmar  ·  Escribe tu nombre en la primera fila", 11, Color(0.45, 0.35, 0.3))
	hint.position = Vector2(0, 314)
	hint.size = Vector2(600, 16)
	hint.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	panel.add_child(hint)
	_name.text_submitted.connect(func(_t):
		_row = 1
		_refresh())
	_name.grab_focus()
	_refresh()
	Audio.play_music("title", 0.5)


func _value_text(key: String) -> String:
	match key:
		"sex":
			return "Hombre" if app["sex"] == "m" else "Mujer"
		"race":
			return str(DB.RACES[app["race"]]["name"])
		"hair_style":
			return GameState.HAIR_STYLES[app["hair_style"]][1]
		"hair_color":
			return GameState.HAIR_COLORS[app["hair_color"]][0]
		"skin":
			return GameState.SKIN_TONES[app["skin"]][0]
		"outfit":
			return GameState.OUTFITS[app["outfit"]][0]
		"difficulty":
			return DIFFS[int(app.get("difficulty", 1))]
		"beard":
			if app["sex"] == "f":
				return "—"
			return "Sí" if app["beard"] else "No"
	return ""


func _refresh() -> void:
	for key in _value_labels:
		_value_labels[key].text = _value_text(key)
	for i in ROWS.size():
		var key: String = ROWS[i]
		var sel := i == _row
		if _row_labels.has(key):
			_row_labels[key].add_theme_color_override("font_color", Color(0.6, 0.2, 0.1) if sel else UIKit.TEXT_DARK)
	_start.add_theme_color_override("font_color", Color(0.75, 0.25, 0.1) if _row == ROWS.size() - 1 else Color(0.45, 0.3, 0.2))
	var target_y := 46 + _row * ROW_H + 4 if _row < ROWS.size() - 1 else 46 + (ROWS.size() - 1) * ROW_H + 8
	_cursor.position = Vector2(232, target_y)
	var r: Dictionary = DB.RACES[app["race"]]
	_race_desc.text = "%s\n%s" % [r["desc"], r["trait"]]
	if ROWS[_row] == "difficulty":
		_race_desc.text = ["Fácil: enemigos más débiles y más experiencia. Para disfrutar de la historia.",
			"Normal: equilibrado. Hay que usar habilidades y objetos con cabeza.",
			"Difícil: enemigos más duros y agresivos. Para veteranos de los JRPG."][int(app.get("difficulty", 1))]
	_preview.texture = Appearance.sheet(app)
	_portrait.texture = Appearance.portrait(app)
	if _row == 0:
		if not _name.has_focus():
			_name.grab_focus()
	elif _name.has_focus():
		_name.release_focus()


func _change(key: String, d: int) -> void:
	match key:
		"sex":
			app["sex"] = "f" if app["sex"] == "m" else "m"
			if app["sex"] == "f":
				app["beard"] = false
				if app["hair_style"] in [1, 6]:
					app["hair_style"] = 2
			else:
				if app["hair_style"] in [2, 5]:
					app["hair_style"] = 1
		"race":
			var i := RACE_ORDER.find(app["race"])
			app["race"] = RACE_ORDER[(i + d + RACE_ORDER.size()) % RACE_ORDER.size()]
			if app["race"] == "dwarf" and app["sex"] == "m":
				app["beard"] = true
		"hair_style":
			app["hair_style"] = (int(app["hair_style"]) + d + GameState.HAIR_STYLES.size()) % GameState.HAIR_STYLES.size()
		"hair_color":
			app["hair_color"] = (int(app["hair_color"]) + d + GameState.HAIR_COLORS.size()) % GameState.HAIR_COLORS.size()
		"skin":
			app["skin"] = (int(app["skin"]) + d + GameState.SKIN_TONES.size()) % GameState.SKIN_TONES.size()
		"outfit":
			app["outfit"] = (int(app["outfit"]) + d + GameState.OUTFITS.size()) % GameState.OUTFITS.size()
		"beard":
			if app["sex"] == "m":
				app["beard"] = not app["beard"]
		"difficulty":
			app["difficulty"] = clampi(int(app.get("difficulty", 1)) + d, 0, 2)
	Audio.sfx("select", -12.0)
	_refresh()


func _process(delta: float) -> void:
	_t += delta
	_dir = int(_t / 1.6) % 4
	var order := [0, 2, 3, 1]
	_preview.frame = order[_dir] * 8 + 2 + int(_t * 9.0) % 6
	if Transition.busy:
		return
	var key: String = ROWS[_row]
	if Input.is_action_just_pressed("ui_up"):
		_row = (_row - 1 + ROWS.size()) % ROWS.size()
		Audio.sfx("select", -14.0)
		_refresh()
	elif Input.is_action_just_pressed("ui_down"):
		_row = (_row + 1) % ROWS.size()
		Audio.sfx("select", -14.0)
		_refresh()
	elif key != "name" and key != "start" and Input.is_action_just_pressed("ui_left"):
		_change(key, -1)
	elif key != "name" and key != "start" and Input.is_action_just_pressed("ui_right"):
		_change(key, 1)
	elif key != "name" and Input.is_action_just_pressed("ui_accept"):
		if key == "start":
			_on_start()
		else:
			_change(key, 1)


func _on_start() -> void:
	var n := _name.text.strip_edges()
	if n == "":
		n = "Arlen" if app["sex"] == "m" else "Alda"
	GameState.new_game(n, app)
	Audio.sfx("confirm", -6.0)
	Audio.stop_music(1.0)
	Transition.go_to_scene("res://scenes/Intro.tscn", 0.6)
