extends Control
## Árbol de talentos de la clase del jugador: 3 niveles x 3 talentos.

signal closed

var _buttons: Array = []
var _desc: Label
var _points: Label
var _ignore := 0


func _ready() -> void:
	process_mode = Node.PROCESS_MODE_ALWAYS
	set_anchors_preset(Control.PRESET_FULL_RECT)
	var panel := UIKit.panel(Rect2(170, 20, 458, 326))
	add_child(panel)
	var c: String = str(GameState.player_data.get("class", ""))
	var title := UIKit.label("", 18, Color(0.35, 0.18, 0.1))
	title.position = Vector2(16, 8)
	title.size = Vector2(300, 24)
	panel.add_child(title)
	_points = UIKit.label("", 14, Color(0.55, 0.3, 0.1))
	_points.position = Vector2(250, 12)
	_points.size = Vector2(192, 20)
	_points.horizontal_alignment = HORIZONTAL_ALIGNMENT_RIGHT
	panel.add_child(_points)
	_desc = UIKit.label("", 13)
	_desc.position = Vector2(16, 262)
	_desc.size = Vector2(426, 56)
	_desc.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	panel.add_child(_desc)
	if c == "" or not DB.CLASSES.has(c):
		title.text = "Talentos"
		_desc.text = "Al llegar al nivel 2 elegirás tu senda y tu clase. Entonces podrás gastar aquí tus puntos de talento."
		_ignore = Engine.get_process_frames() + 2
		return
	title.text = "Talentos · %s" % DB.CLASSES[c]["name"]
	for t in 3:
		var h := UIKit.label(["Nivel I", "Nivel II  (3 puntos)", "Nivel III  (6 puntos)"][t], 12, Color(0.45, 0.3, 0.2))
		h.position = Vector2(16 + t * 144, 36)
		panel.add_child(h)
	var grid := GridContainer.new()
	grid.columns = 3
	grid.position = Vector2(14, 54)
	grid.add_theme_constant_override("h_separation", 6)
	grid.add_theme_constant_override("v_separation", 8)
	panel.add_child(grid)
	# ordenar por fila: la cuadrícula se rellena por filas, así que intercalamos los tiers
	var by_tier := {1: [], 2: [], 3: []}
	for n in DB.CLASSES[c]["tree"]:
		by_tier[int(n["tier"])].append(n)
	for row in 3:
		for t in [1, 2, 3]:
			var n: Dictionary = by_tier[t][row] if row < by_tier[t].size() else {}
			var b := UIKit.button("", 12)
			b.custom_minimum_size = Vector2(138, 58)
			b.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
			if n.is_empty():
				b.disabled = true
			else:
				b.focus_entered.connect(_show.bind(n))
				b.pressed.connect(_learn.bind(n))
				if n.has("skill"):
					var el: String = str(DB.SKILLS[n["skill"]].get("el", ""))
					b.icon = load("res://assets/ui/%s.png" % ("el_" + el if el != "" else "cmd_skill"))
				else:
					b.icon = load("res://assets/ui/st_rage.png" if n.has("perk") else "res://assets/ui/cmd_defend.png")
			grid.add_child(b)
			_buttons.append({"b": b, "n": n})
	_refresh()
	_ignore = Engine.get_process_frames() + 2
	get_viewport().gui_release_focus()
	_buttons[0]["b"].grab_focus()


func _refresh() -> void:
	_points.text = "Puntos: %d" % GameState.skill_points()
	var tree: Dictionary = GameState.talents()
	for e in _buttons:
		var n: Dictionary = e["n"]
		if n.is_empty():
			continue
		var r: int = int(tree.get(n["id"], 0))
		e["b"].text = "%s\n%d/%d" % [n["name"], r, int(n["max"])]
		var reason := GameState.talent_block_reason(n["id"])
		var col := Color(1, 0.93, 0.6) if r > 0 else (UIKit.TEXT_LIGHT if reason == "" else Color(0.65, 0.6, 0.55))
		for k in ["font_color", "font_focus_color", "font_hover_color"]:
			e["b"].add_theme_color_override(k, col)


func _show(n: Dictionary) -> void:
	var txt := ""
	if n.has("skill"):
		var sk: Dictionary = DB.SKILLS[n["skill"]]
		txt = "%s — %s (%d PM)" % [sk["name"], sk["desc"], int(sk.get("mp", 0))]
	else:
		txt = "%s — %s" % [n["name"], n.get("desc", "")]
	var reason := GameState.talent_block_reason(n["id"])
	if reason != "":
		txt += "\n[%s]" % reason
	else:
		txt += "\nPulsa para aprender."
	_desc.text = txt


func _learn(n: Dictionary) -> void:
	if GameState.learn_talent(n["id"]):
		Audio.sfx("levelup", -8.0)
	else:
		Audio.sfx("cancel", -8.0)
	_refresh()
	_show(n)


func _process(_d: float) -> void:
	if Engine.get_process_frames() <= _ignore:
		return
	if Input.is_action_just_pressed("ui_cancel"):
		Audio.sfx("cancel", -8.0)
		closed.emit()
		queue_free()
