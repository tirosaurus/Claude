extends Control
## Menú de pausa: grupo, objetos, guardar y salir.

const SaveUIScript := preload("res://scripts/ui/SaveUI.gd")

var world: Node
var _content: Control
var _tabs: Array = []
var _sub_open := false
var _opened_frame := -1


func _ready() -> void:
	process_mode = Node.PROCESS_MODE_ALWAYS
	set_anchors_preset(Control.PRESET_FULL_RECT)
	visible = false


func open() -> void:
	if visible:
		return
	for c in get_children():
		c.queue_free()
	_tabs.clear()
	visible = true
	_opened_frame = Engine.get_process_frames()
	get_tree().paused = true
	Audio.sfx("select", -8.0)
	var dim := ColorRect.new()
	dim.color = Color(0, 0, 0, 0.55)
	dim.set_anchors_preset(Control.PRESET_FULL_RECT)
	add_child(dim)
	var side := UIKit.panel(Rect2(12, 10, 150, 266))
	add_child(side)
	var box := VBoxContainer.new()
	box.position = Vector2(14, 14)
	box.size = Vector2(122, 240)
	box.add_theme_constant_override("separation", 4)
	side.add_child(box)
	for pair in [["Grupo", _show_party], ["Objetos", _show_items], ["Equipo", _show_equip], ["Talentos", _show_talents],
			["Guardar", _show_save], ["Opciones", _show_options], ["Continuar", close], ["Título", _to_title]]:
		var b := UIKit.button(pair[0], 14)
		b.custom_minimum_size = Vector2(122, 26)
		b.pressed.connect(pair[1])
		box.add_child(b)
		_tabs.append(b)
	var info := UIKit.panel(Rect2(12, 276, 150, 70), true)
	add_child(info)
	var l := UIKit.label("", 13, UIKit.TEXT_LIGHT)
	l.position = Vector2(12, 8)
	l.text = "%d coronas\nTiempo %s" % [GameState.gold, GameState.format_time(GameState.play_time)]
	info.add_child(l)
	_content = Control.new()
	add_child(_content)
	_show_party()
	_tabs[0].grab_focus()


func close() -> void:
	visible = false
	get_tree().paused = false
	for c in get_children():
		c.queue_free()


func _process(_d: float) -> void:
	if not visible or _sub_open:
		return
	# la misma pulsación que abre el menú no debe cerrarlo
	if Engine.get_process_frames() <= _opened_frame + 1:
		return
	if Input.is_action_just_pressed("pause") or Input.is_action_just_pressed("cancel"):
		close()


func _clear() -> void:
	for c in _content.get_children():
		c.queue_free()


func _show_party() -> void:
	_clear()
	var panel := UIKit.panel(Rect2(170, 20, 458, 326))
	_content.add_child(panel)
	var y := 10
	for id in GameState.party:
		var st := GameState.stats(id)
		var m: Dictionary = GameState.members[id]
		var por := TextureRect.new()
		por.texture = Appearance.member_portrait(id)
		por.position = Vector2(12, y)
		por.size = Vector2(48, 48)
		panel.add_child(por)
		var txt := "%s  ·  %s  ·  Nv %d\nVida %d/%d   Maná %d/%d\nAtq %d  Def %d  Mag %d  Res %d  Vel %d" % [
			GameState.member_name(id), GameState.member_role(id), GameState.level, int(m["hp"]), st["hp"],
			int(m["mp"]), st["mp"], st["atk"], st["def"], st["mag"], st["res"], st["spd"]]
		var l := UIKit.label(txt, 13)
		l.position = Vector2(68, y)
		l.size = Vector2(370, 54)
		panel.add_child(l)
		var rel := ""
		if id == "kaelen":
			rel = GameState.kaelen_bond_label()
		elif id == "yara":
			rel = GameState.yara_bond_label()
		elif id == "player":
			rel = GameState.race_name()
		if rel != "":
			var r := UIKit.label(rel, 11, Color(0.5, 0.25, 0.3))
			r.position = Vector2(290, y)
			r.size = Vector2(150, 16)
			r.clip_text = true
			r.horizontal_alignment = HORIZONTAL_ALIGNMENT_RIGHT
			panel.add_child(r)
		y += 62
	var xp_l := UIKit.label("Experiencia: %d / %d para el nivel %d" % [GameState.xp, DB.xp_to_next(GameState.level), GameState.level + 1], 13, Color(0.4, 0.3, 0.2))
	xp_l.position = Vector2(12, 300)
	panel.add_child(xp_l)


func _show_items() -> void:
	_clear()
	var panel := UIKit.panel(Rect2(170, 20, 458, 326))
	_content.add_child(panel)
	var desc := UIKit.label("", 13)
	desc.position = Vector2(16, 290)
	desc.size = Vector2(426, 24)
	panel.add_child(desc)
	var box := VBoxContainer.new()
	box.position = Vector2(16, 12)
	box.size = Vector2(426, 270)
	box.add_theme_constant_override("separation", 3)
	panel.add_child(box)
	var ids: Array = GameState.inventory.keys()
	ids.sort()
	if ids.is_empty():
		desc.text = "No llevas objetos."
		return
	for id in ids:
		var it: Dictionary = DB.ITEMS.get(id, {})
		if it.is_empty():
			continue
		var b := UIKit.button("%s  ×%d" % [it["name"], GameState.item_count(id)], 13)
		b.icon = load("res://assets/ui/%s.png" % it["icon"])
		b.alignment = HORIZONTAL_ALIGNMENT_LEFT
		b.custom_minimum_size = Vector2(426, 24)
		b.focus_entered.connect(func(): desc.text = str(it["desc"]))
		b.pressed.connect(_pick_target.bind(id))
		box.add_child(b)
	if box.get_child_count() > 0:
		box.get_child(0).grab_focus()


func _pick_target(item_id: String) -> void:
	var it: Dictionary = DB.ITEMS[item_id]
	if str(it["target"]) == "enemies":
		return
	_clear()
	var panel := UIKit.panel(Rect2(170, 20, 458, 326))
	_content.add_child(panel)
	var t := UIKit.label("¿Sobre quién usas %s?" % it["name"], 15)
	t.position = Vector2(16, 12)
	panel.add_child(t)
	var box := VBoxContainer.new()
	box.position = Vector2(16, 44)
	box.size = Vector2(426, 240)
	box.add_theme_constant_override("separation", 4)
	panel.add_child(box)
	var targets: Array = GameState.party if str(it["target"]) != "allies" else ["__all"]
	for id in targets:
		var label := "Todo el grupo" if id == "__all" else "%s   %d/%d vida" % [GameState.member_name(id), int(GameState.members[id]["hp"]), GameState.stats(id)["hp"]]
		var b := UIKit.button(label, 13)
		b.custom_minimum_size = Vector2(426, 26)
		b.alignment = HORIZONTAL_ALIGNMENT_LEFT
		b.pressed.connect(func():
			_apply_item(item_id, id)
			_show_items())
		box.add_child(b)
	var back := UIKit.button("Volver", 13)
	back.custom_minimum_size = Vector2(120, 24)
	back.pressed.connect(_show_items)
	box.add_child(back)
	box.get_child(0).grab_focus()


func _apply_item(item_id: String, target: String) -> void:
	var it: Dictionary = DB.ITEMS[item_id]
	var ids: Array = GameState.party if target == "__all" else [target]
	var used := false
	for id in ids:
		var m: Dictionary = GameState.members[id]
		var st := GameState.stats(id)
		if it.has("revive"):
			if int(m["hp"]) <= 0:
				m["hp"] = int(st["hp"] * float(it["revive"]))
				used = true
			continue
		if int(m["hp"]) <= 0:
			continue
		if it.has("heal") and int(m["hp"]) < int(st["hp"]):
			m["hp"] = mini(int(st["hp"]), int(m["hp"]) + int(it["heal"]))
			used = true
		if it.has("mp") and int(m["mp"]) < int(st["mp"]):
			m["mp"] = mini(int(st["mp"]), int(m["mp"]) + int(it["mp"]))
			used = true
		if it.get("cure", false):
			used = true
	if used:
		GameState.remove_item(item_id)
		Audio.sfx("heal", -6.0)
	else:
		Audio.sfx("cancel", -8.0)


func _show_save() -> void:
	# En Difícil no se puede guardar en mitad de una escena o momento decisivo.
	if world and world.cutscene and GameState.difficulty() >= 2:
		Audio.sfx("cancel", -6.0)
		_clear()
		var panel := UIKit.panel(Rect2(170, 20, 458, 80))
		_content.add_child(panel)
		var l := UIKit.label("Modo Difícil: no puedes guardar en un momento decisivo.\nSí puedes usar objetos y cambiar el equipo.", 14)
		l.position = Vector2(16, 14)
		l.size = Vector2(426, 50)
		panel.add_child(l)
		return
	_sub_open = true
	var s := SaveUIScript.new()
	s.setup("save", world.player.position if world else Vector2.ZERO)
	add_child(s)
	s.closed.connect(func(_r):
		_sub_open = false
		if _tabs.size() > 2:
			_tabs[2].grab_focus())


func _sub(script_path: String, tab: int) -> void:
	_clear()
	_sub_open = true
	var o = load(script_path).new()
	_content.add_child(o)
	o.closed.connect(func():
		_sub_open = false
		if _tabs.size() > tab:
			_tabs[tab].grab_focus())


func _show_talents() -> void:
	_sub("res://scripts/ui/TalentUI.gd", 3)


func _show_equip() -> void:
	_sub("res://scripts/ui/EquipUI.gd", 2)


func _show_options() -> void:
	_sub_open = true
	var o = load("res://scripts/ui/OptionsUI.gd").new()
	add_child(o)
	o.closed.connect(func():
		_sub_open = false
		if _tabs.size() > 5:
			_tabs[5].grab_focus())


func _to_title() -> void:
	close()
	Transition.go_to_scene("res://scenes/Title.tscn")
