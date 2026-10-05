extends Control
## Menú de opciones: volumen, velocidades y pantalla completa.

signal closed

var _rows: Array = []
var _labels: Array = []
var _values: Array = []
var _index := 0
var _ignore := 0


func _ready() -> void:
	process_mode = Node.PROCESS_MODE_ALWAYS
	set_anchors_preset(Control.PRESET_FULL_RECT)
	var dim := ColorRect.new()
	dim.color = Color(0, 0, 0, 0.6)
	dim.set_anchors_preset(Control.PRESET_FULL_RECT)
	add_child(dim)
	var panel := UIKit.panel(Rect2(150, 60, 340, 240))
	add_child(panel)
	var t := UIKit.label("Opciones", 22, Color(0.35, 0.18, 0.1))
	t.position = Vector2(0, 10)
	t.size = Vector2(340, 30)
	t.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	panel.add_child(t)
	_rows = ["music", "sfx", "text", "battle", "full", "back"]
	for k in _rows.size():
		var l := UIKit.label("", 16)
		l.position = Vector2(30, 50 + k * 28)
		l.size = Vector2(280, 22)
		panel.add_child(l)
		_labels.append(l)
		var v := UIKit.label("", 16)
		v.position = Vector2(150, 50 + k * 28)
		v.size = Vector2(170, 22)
		panel.add_child(v)
		_values.append(v)
	var hint := UIKit.label("Izq./Der.: cambiar  ·  Esc: volver", 12, Color(0.45, 0.35, 0.3))
	hint.position = Vector2(0, 214)
	hint.size = Vector2(340, 18)
	hint.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	panel.add_child(hint)
	_ignore = Engine.get_process_frames() + 2
	get_viewport().gui_release_focus()
	_refresh()


func _bar(v: int) -> String:
	return "[" + "|".repeat(v) + ".".repeat(10 - v) + "]"


func _refresh() -> void:
	var speeds := ["Lenta", "Normal", "Rápida"]
	var names := {"music": "Música", "sfx": "Efectos", "text": "Texto", "battle": "Combate", "full": "Pantalla",
		"back": "Volver"}
	var vals := {
		"music": _bar(Audio.music_level),
		"sfx": _bar(Audio.sfx_level),
		"text": speeds[Audio.text_speed],
		"battle": speeds[Audio.battle_speed],
		"full": "Completa" if Audio.fullscreen else "Ventana",
		"back": "",
	}
	for k in _rows.size():
		var l: Label = _labels[k]
		var col := Color(0.6, 0.2, 0.1) if k == _index else UIKit.TEXT_DARK
		l.text = ("> " if k == _index else "   ") + names[_rows[k]]
		l.add_theme_color_override("font_color", col)
		_values[k].text = vals[_rows[k]]
		_values[k].add_theme_color_override("font_color", col)


func _change(d: int) -> void:
	match _rows[_index]:
		"music":
			Audio.music_level = clampi(Audio.music_level + d, 0, 10)
		"sfx":
			Audio.sfx_level = clampi(Audio.sfx_level + d, 0, 10)
		"text":
			Audio.text_speed = clampi(Audio.text_speed + d, 0, 2)
		"battle":
			Audio.battle_speed = clampi(Audio.battle_speed + d, 0, 2)
		"full":
			Audio.fullscreen = not Audio.fullscreen
	Audio.apply_settings()
	Audio.sfx("select", -8.0)
	_refresh()


func _process(_d: float) -> void:
	if Engine.get_process_frames() <= _ignore:
		return
	var nav_up := Input.is_action_just_pressed("ui_up") or Input.is_action_just_pressed("move_up")
	var nav_dn := Input.is_action_just_pressed("ui_down") or Input.is_action_just_pressed("move_down")
	var nav_l := Input.is_action_just_pressed("ui_left") or Input.is_action_just_pressed("move_left")
	var nav_r := Input.is_action_just_pressed("ui_right") or Input.is_action_just_pressed("move_right")
	if (nav_up or nav_dn or nav_l or nav_r) and not TouchControls.nav_gate():
		return
	if nav_up:
		_index = (_index - 1 + _rows.size()) % _rows.size()
		Audio.sfx("select", -14.0)
		_refresh()
	elif nav_dn:
		_index = (_index + 1) % _rows.size()
		Audio.sfx("select", -14.0)
		_refresh()
	elif nav_l:
		_change(-1)
	elif nav_r:
		_change(1)
	elif Input.is_action_just_pressed("ui_accept"):
		if _rows[_index] == "back":
			_close()
		else:
			_change(1)
	elif Input.is_action_just_pressed("ui_cancel"):
		_close()


func _close() -> void:
	Audio.save_settings()
	Audio.sfx("cancel", -8.0)
	closed.emit()
	queue_free()
