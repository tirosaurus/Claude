extends CanvasLayer
## Controles táctiles para móvil/tableta: cruceta, A (interactuar/aceptar), B (cancelar),
## MENÚ (Esc) y CORRER/AUTO (Mayús). Solo aparecen en pantallas táctiles.

const DPAD_CENTER := Vector2(78, 282)
const DPAD_BTN := 34.0

var _buttons: Array = []
var _rotate_hint: Label


class TouchBtn extends Control:
	var actions: Array = []
	var label := ""
	var shape := "circle"      # circle | up | down | left | right | pill
	var radius := 24.0
	var _touches := {}
	var toggle := false

	func _ready() -> void:
		mouse_filter = Control.MOUSE_FILTER_IGNORE
		focus_mode = Control.FOCUS_NONE

	func set_pressed(on: bool, finger: int) -> void:
		if on:
			if _touches.is_empty():
				_send(true)
			_touches[finger] = true
		else:
			if not _touches.has(finger):
				return
			_touches.erase(finger)
			if _touches.is_empty():
				_send(false)
		queue_redraw()

	func hit(p: Vector2) -> bool:
		return Rect2(position, size).grow(6).has_point(p)

	func _send(pressed: bool) -> void:
		for a in actions:
			if str(a).begins_with("ui_"):
				# un único evento: actualiza el estado de la acción y llega a los menús con foco.
				# (antes se pulsaba dos veces —action_press + evento— y los menús saltaban dos opciones)
				var ev := InputEventAction.new()
				ev.action = a
				ev.pressed = pressed
				ev.strength = 1.0 if pressed else 0.0
				Input.parse_input_event(ev)
			elif pressed:
				Input.action_press(a)
			else:
				Input.action_release(a)

	func _draw() -> void:
		var pressed := not _touches.is_empty()
		var c := size / 2.0
		var fill := Color(1, 1, 1, 0.34 if pressed else 0.16)
		var edge := Color(1, 1, 1, 0.55)
		if shape in ["up", "down", "left", "right"]:
			draw_rect(Rect2(Vector2.ZERO, size), fill)
			draw_rect(Rect2(Vector2.ZERO, size), edge, false, 1.5)
			var d: Vector2 = {"up": Vector2(0, -1), "down": Vector2(0, 1), "left": Vector2(-1, 0), "right": Vector2(1, 0)}[shape]
			var p := Vector2(-d.y, d.x)
			var tip: Vector2 = c + d * 9
			draw_colored_polygon(PackedVector2Array([tip, c - d * 5 + p * 8, c - d * 5 - p * 8]), Color(1, 1, 1, 0.85))
		elif shape == "pill":
			draw_rect(Rect2(Vector2.ZERO, size), fill)
			draw_rect(Rect2(Vector2.ZERO, size), edge, false, 1.5)
		else:
			draw_circle(c, radius, fill)
			draw_arc(c, radius, 0, TAU, 32, edge, 1.5)
		if label != "":
			var f := get_theme_default_font()
			var fs := 13 if shape == "pill" else 16
			var w := f.get_string_size(label, HORIZONTAL_ALIGNMENT_CENTER, -1, fs).x
			draw_string(f, Vector2(c.x - w / 2.0, c.y + fs * 0.35), label, HORIZONTAL_ALIGNMENT_CENTER, -1, fs, Color(1, 1, 1, 0.9))


func _ready() -> void:
	layer = 120
	process_mode = Node.PROCESS_MODE_ALWAYS
	var force := OS.get_environment("VAELMOOR_TOUCH") == "1" or OS.has_feature("mobile") \
		or OS.has_feature("web_android") or OS.has_feature("web_ios")
	visible = force or DisplayServer.is_touchscreen_available()
	# Cruceta: movimiento y navegación de menús
	var dirs := {"up": ["move_up", "ui_up"], "down": ["move_down", "ui_down"], "left": ["move_left", "ui_left"],
		"right": ["move_right", "ui_right"]}
	var offs := {"up": Vector2(0, -1), "down": Vector2(0, 1), "left": Vector2(-1, 0), "right": Vector2(1, 0)}
	for d in dirs:
		var b := TouchBtn.new()
		b.actions = dirs[d]
		b.shape = d
		b.size = Vector2(DPAD_BTN, DPAD_BTN)
		b.position = DPAD_CENTER + offs[d] * DPAD_BTN - b.size / 2.0
		add_child(b)
		_buttons.append(b)
	_add_round("A", ["interact", "ui_accept"], Vector2(600, 272), 26)
	_add_round("B", ["cancel", "ui_cancel"], Vector2(548, 312), 21)
	var menu := TouchBtn.new()
	menu.actions = ["pause"]
	menu.shape = "pill"
	menu.label = "MENÚ"
	menu.size = Vector2(58, 24)
	menu.position = Vector2(574, 6)
	add_child(menu)
	_buttons.append(menu)
	var run := TouchBtn.new()
	run.actions = ["run"]
	run.shape = "pill"
	run.label = "CORRER"
	run.size = Vector2(62, 22)
	run.position = Vector2(520, 226)
	add_child(run)
	_buttons.append(run)
	_rotate_hint = Label.new()
	_rotate_hint.text = "Gira el móvil en horizontal para jugar mejor"
	_rotate_hint.add_theme_font_size_override("font_size", 14)
	_rotate_hint.add_theme_color_override("font_outline_color", Color.BLACK)
	_rotate_hint.add_theme_constant_override("outline_size", 4)
	_rotate_hint.position = Vector2(0, 150)
	_rotate_hint.size = Vector2(640, 30)
	_rotate_hint.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	add_child(_rotate_hint)


var _finger := {}   # índice de dedo -> botón
var _last_nav := -100


## Evita que una sola pulsación cuente dos veces en los menús (move_* y ui_* llegan en
## fotogramas distintos). Devuelve true si la navegación debe aplicarse.
func nav_gate() -> bool:
	var f := Engine.get_process_frames()
	if f - _last_nav < 8:
		return false
	_last_nav = f
	return true


func _button_at(p: Vector2):
	for b in _buttons:
		if b.visible and b.hit(p):
			return b
	return null


func _input(event: InputEvent) -> void:
	if not visible:
		return
	if event is InputEventScreenTouch:
		_on_point(event.index, event.position, event.pressed, false)
	elif event is InputEventScreenDrag:
		_on_point(event.index, event.position, true, true)
	elif event is InputEventMouseButton and event.button_index == MOUSE_BUTTON_LEFT \
			and not DisplayServer.is_touchscreen_available():
		_on_point(-1, event.position, event.pressed, false)


func _on_point(idx: int, pos: Vector2, pressed: bool, drag: bool) -> void:
	var b = _button_at(pos)
	var cur = _finger.get(idx)
	if pressed:
		if cur != b:
			if cur != null:
				cur.set_pressed(false, idx)
				_finger.erase(idx)
			if b != null and (not drag or cur != null or true):
				b.set_pressed(true, idx)
				_finger[idx] = b
		if b != null:
			get_viewport().set_input_as_handled()
	else:
		if cur != null:
			cur.set_pressed(false, idx)
			_finger.erase(idx)
			get_viewport().set_input_as_handled()


func _add_round(text: String, actions: Array, center: Vector2, r: float) -> void:
	var b := TouchBtn.new()
	b.actions = actions
	b.label = text
	b.radius = r
	b.size = Vector2(r * 2 + 4, r * 2 + 4)
	b.position = center - b.size / 2.0
	add_child(b)
	_buttons.append(b)


func _physics_process(_d: float) -> void:
	_process(0.0)


func _process(_d: float) -> void:
	if not visible:
		return
	# mantener el movimiento mientras el dedo siga sobre la cruceta
	for b in _buttons:
		if not b._touches.is_empty():
			for a in b.actions:
				if str(a).begins_with("move_") and not Input.is_action_pressed(a):
					Input.action_press(a)
	# en combate la cruceta tapa el menú: se ocultan y se toca directamente la opción
	var cs := get_tree().current_scene
	var in_battle := cs != null and cs.name == "Battle"
	for b in _buttons:
		if b.shape in ["up", "down", "left", "right"] and b.visible == in_battle:
			b.visible = not in_battle
			if in_battle and not b._touches.is_empty():
				for f in b._touches.keys():
					b.set_pressed(false, f)
	var ws := DisplayServer.window_get_size()
	_rotate_hint.visible = ws.y > ws.x
