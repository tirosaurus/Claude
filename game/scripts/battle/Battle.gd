extends Control
## Combate por turnos del grupo (jugador, Kaelen, Yara) contra el lobo corrupto.

signal _command(action: String)
signal _branch_done

const ENEMY := {"name": "Lobo corrupto", "max_hp": 92, "atk": 7}

var stage: Node2D
var wolf: Sprite2D
var members := {}          # id -> {sprite, home, bar, label, name}
var enemy_hp := 0
var enemy_atk := 0
var enraged := false
var turn := 0
var defending := false
var _msg: Label
var _enemy_bar: ProgressBar
var _cmd_panel: NinePatchRect
var _cmd_buttons: Array[Button] = []
var _party_rows := {}
var _t := 0.0
var _over := false


func _ready() -> void:
	enemy_hp = ENEMY["max_hp"]
	enemy_atk = ENEMY["atk"]
	_build()
	Audio.play_music("battle", 0.3)
	_run.call_deferred()


# ------------------------------------------------------------ Construcción
func _build() -> void:
	var bg := TextureRect.new()
	bg.texture = load("res://assets/bg/battle_forest.png")
	bg.size = Vector2(640, 360)
	bg.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
	bg.stretch_mode = TextureRect.STRETCH_SCALE
	add_child(bg)
	stage = Node2D.new()
	add_child(stage)

	_shadow(Vector2(186, 222), 70, 10)
	wolf = Sprite2D.new()
	wolf.texture = load("res://assets/sprites/wolf_big.png")
	wolf.hframes = 2
	wolf.scale = Vector2(2, 2)
	wolf.position = Vector2(186, 160)
	stage.add_child(wolf)

	var layout := {"kaelen": Vector2(492, 150), "player": Vector2(462, 196), "yara": Vector2(530, 238)}
	for id in ["kaelen", "player", "yara"]:
		if id != "player" and not GameState.companions[id]["in_party"]:
			continue
		var home: Vector2 = layout[id]
		_shadow(home + Vector2(0, 24), 14, 4)
		var s := Sprite2D.new()
		s.texture = load("res://assets/chars/%s.png" % id)
		s.hframes = 3
		s.vframes = 4
		s.frame = 3
		s.scale = Vector2(2, 2)
		s.position = home
		stage.add_child(s)
		members[id] = {"sprite": s, "home": home}

	# Enemigo: nombre y barra
	var en := UIKit.label(ENEMY["name"], 16, UIKit.TEXT_LIGHT, 4)
	en.position = Vector2(110, 58)
	add_child(en)
	_enemy_bar = _bar(Vector2(110, 80), Vector2(150, 8), Color(0.75, 0.2, 0.25))
	_enemy_bar.max_value = ENEMY["max_hp"]
	_enemy_bar.value = enemy_hp

	# Mensajes
	var mp := UIKit.panel(Rect2(110, 8, 420, 38), true)
	add_child(mp)
	_msg = UIKit.label("", 16, UIKit.TEXT_LIGHT)
	_msg.position = Vector2(10, 8)
	_msg.size = Vector2(400, 24)
	_msg.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	mp.add_child(_msg)

	# Grupo
	var pp := UIKit.panel(Rect2(262, 270, 364, 84))
	add_child(pp)
	var y := 10
	for id in ["player", "kaelen", "yara"]:
		if not members.has(id):
			continue
		var nm := UIKit.label(GameState.player_name() if id == "player" else str(GameState.companions[id]["display_name"]), 14)
		nm.position = Vector2(14, y - 2)
		nm.size = Vector2(90, 20)
		nm.clip_text = true
		pp.add_child(nm)
		var bar := _bar(Vector2(106, y + 4), Vector2(130, 8), Color(0.35, 0.72, 0.35), pp)
		var num := UIKit.label("", 13)
		num.position = Vector2(244, y - 1)
		pp.add_child(num)
		var row := {"bar": bar, "num": num}
		if id == "player":
			var mpbar := _bar(Vector2(300, y + 4), Vector2(50, 8), Color(0.35, 0.5, 0.9), pp)
			row["mp"] = mpbar
		_party_rows[id] = row
		y += 22
	_refresh()

	# Comandos
	_cmd_panel = UIKit.panel(Rect2(14, 270, 240, 84))
	add_child(_cmd_panel)
	var box := VBoxContainer.new()
	box.position = Vector2(14, 8)
	box.size = Vector2(212, 70)
	box.add_theme_constant_override("separation", 1)
	_cmd_panel.add_child(box)
	for pair in [["Atacar", "attack"], ["Defender", "defend"], ["Curar  (5 maná)", "heal"]]:
		var b := UIKit.button(pair[0], 14)
		b.custom_minimum_size = Vector2(212, 22)
		b.pressed.connect(func(): _command.emit(pair[1]))
		box.add_child(b)
		_cmd_buttons.append(b)
	_cmd_panel.visible = false


func _shadow(pos: Vector2, rx: float, ry: float) -> void:
	var poly := Polygon2D.new()
	var pts := PackedVector2Array()
	for i in 24:
		var a := TAU * i / 24.0
		pts.append(pos + Vector2(cos(a) * rx, sin(a) * ry))
	poly.polygon = pts
	poly.color = Color(0, 0, 0, 0.35)
	stage.add_child(poly)


func _bar(pos: Vector2, size: Vector2, col: Color, parent: Node = null) -> ProgressBar:
	var b := ProgressBar.new()
	b.show_percentage = false
	b.position = pos
	b.size = size
	b.custom_minimum_size = size
	var bg := StyleBoxFlat.new()
	bg.bg_color = Color(0.12, 0.08, 0.1)
	bg.border_color = Color(0.08, 0.05, 0.07)
	bg.set_border_width_all(1)
	var fg := StyleBoxFlat.new()
	fg.bg_color = col
	fg.border_color = col.lightened(0.3)
	fg.border_width_top = 1
	b.add_theme_stylebox_override("background", bg)
	b.add_theme_stylebox_override("fill", fg)
	(parent if parent else self).add_child(b)
	return b


func _refresh() -> void:
	var s := GameState.player_stats
	for id in _party_rows:
		var row: Dictionary = _party_rows[id]
		var hp: int
		var mx: int
		if id == "player":
			hp = int(s["hp"])
			mx = int(s["max_hp"])
			row["mp"].max_value = s["max_mp"]
			row["mp"].value = s["mp"]
		else:
			hp = int(GameState.companions[id]["hp"])
			mx = int(GameState.companions[id]["max_hp"])
		row["bar"].max_value = mx
		row["bar"].value = hp
		row["num"].text = "%d/%d" % [hp, mx]
		var fill: StyleBoxFlat = row["bar"].get_theme_stylebox("fill")
		fill.bg_color = Color(0.35, 0.72, 0.35) if hp > mx * 0.5 else (Color(0.9, 0.7, 0.2) if hp > mx * 0.25 else Color(0.85, 0.25, 0.2))
	_enemy_bar.value = enemy_hp


func _process(delta: float) -> void:
	_t += delta
	if wolf and not _over:
		wolf.frame = int(_t * 2.0) % 2


# ------------------------------------------------------------ Utilidades
func message(text: String, hold: float = 0.9) -> void:
	_msg.text = text.replace("{name}", GameState.player_name())
	await get_tree().create_timer(hold).timeout


func _float_text(pos: Vector2, text: String, col: Color, big: bool = false) -> void:
	var l := UIKit.label(text, 22 if big else 18, col, 5)
	l.position = pos + Vector2(-20, -10)
	l.size = Vector2(40, 24)
	l.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	add_child(l)
	var tw := create_tween().set_parallel(true)
	tw.tween_property(l, "position:y", l.position.y - 26, 0.8).set_ease(Tween.EASE_OUT).set_trans(Tween.TRANS_QUAD)
	tw.tween_property(l, "modulate:a", 0.0, 0.4).set_delay(0.5)
	tw.chain().tween_callback(l.queue_free)


func _flash(node: CanvasItem, col: Color = Color(6, 6, 6)) -> void:
	var tw := create_tween()
	for i in 2:
		tw.tween_property(node, "self_modulate", col, 0.05)
		tw.tween_property(node, "self_modulate", Color.WHITE, 0.07)


func _shake(amount: float = 5.0) -> void:
	var tw := create_tween()
	for i in 6:
		tw.tween_property(stage, "position", Vector2(randf_range(-amount, amount), randf_range(-amount, amount)), 0.03)
	tw.tween_property(stage, "position", Vector2.ZERO, 0.03)


func _lunge(id: String, dist: float = -28.0) -> void:
	var s: Sprite2D = members[id]["sprite"]
	var home: Vector2 = members[id]["home"]
	s.frame = 4
	var tw := create_tween()
	tw.tween_property(s, "position", home + Vector2(dist, 0), 0.12).set_trans(Tween.TRANS_QUAD)
	await tw.finished
	await get_tree().create_timer(0.05).timeout


func _return(id: String) -> void:
	var s: Sprite2D = members[id]["sprite"]
	var tw := create_tween()
	tw.tween_property(s, "position", members[id]["home"], 0.15)
	await tw.finished
	s.frame = 3


func _hp(id: String) -> int:
	return int(GameState.player_stats["hp"]) if id == "player" else int(GameState.companions[id]["hp"])


func _set_hp(id: String, v: int) -> void:
	if id == "player":
		GameState.player_stats["hp"] = clampi(v, 0, int(GameState.player_stats["max_hp"]))
	else:
		var c: Dictionary = GameState.companions[id]
		c["hp"] = clampi(v, 0, int(c["max_hp"]))
	var s: Sprite2D = members[id]["sprite"]
	s.modulate = Color(0.45, 0.4, 0.5, 0.8) if _hp(id) <= 0 else Color.WHITE
	s.rotation_degrees = 90.0 if _hp(id) <= 0 else 0.0


func _max_hp(id: String) -> int:
	return int(GameState.player_stats["max_hp"]) if id == "player" else int(GameState.companions[id]["max_hp"])


func _alive(id: String) -> bool:
	return members.has(id) and _hp(id) > 0


func _damage_enemy(amount: int, crit: bool) -> void:
	enemy_hp = maxi(0, enemy_hp - amount)
	Audio.sfx("crit" if crit else "hit", -2.0)
	_flash(wolf)
	_shake(6.0 if crit else 3.0)
	_float_text(wolf.position + Vector2(0, -60), str(amount), Color(1, 0.95, 0.6) if crit else Color.WHITE, crit)
	_refresh()


func _damage_member(id: String, amount: int) -> void:
	_set_hp(id, _hp(id) - amount)
	Audio.sfx("hit", -3.0, 0.8)
	var s: Sprite2D = members[id]["sprite"]
	_flash(s, Color(3, 0.6, 0.6))
	_shake(4.0)
	_float_text(s.position + Vector2(0, -40), str(amount), Color(1, 0.55, 0.5))
	_refresh()


func _heal_member(id: String, amount: int) -> void:
	_set_hp(id, _hp(id) + amount)
	Audio.sfx("heal", -4.0)
	var s: Sprite2D = members[id]["sprite"]
	_flash(s, Color(1.5, 3, 1.5))
	_float_text(s.position + Vector2(0, -40), "+%d" % amount, Color(0.55, 1, 0.55))
	var p := CPUParticles2D.new()
	p.position = s.position
	p.amount = 18
	p.one_shot = true
	p.explosiveness = 0.8
	p.lifetime = 0.8
	p.emission_shape = CPUParticles2D.EMISSION_SHAPE_RECTANGLE
	p.emission_rect_extents = Vector2(14, 20)
	p.direction = Vector2.UP
	p.spread = 20
	p.gravity = Vector2(0, -30)
	p.initial_velocity_min = 10
	p.initial_velocity_max = 30
	p.scale_amount_min = 2
	p.scale_amount_max = 3
	p.color = Color(0.7, 1, 0.6)
	stage.add_child(p)
	p.emitting = true
	get_tree().create_timer(1.2).timeout.connect(p.queue_free)
	_refresh()


func _lowest_ally() -> String:
	var best := ""
	var ratio := 2.0
	for id in members:
		if not _alive(id):
			continue
		var r := float(_hp(id)) / float(_max_hp(id))
		if r < ratio:
			ratio = r
			best = id
	return best


# ------------------------------------------------------------ Flujo
func _run() -> void:
	await get_tree().create_timer(0.5).timeout
	await message("¡Un lobo corrupto os ataca!", 1.2)
	while true:
		turn += 1
		# Jugador
		defending = false
		var first := turn == 1
		if first:
			await message("Tu turno. Kaelen ataca solo; Yara cura a quien lo necesite.", 0.1)
		else:
			await message("¿Qué vas a hacer, {name}?", 0.05)
		var act := await _player_command()
		await _do_player(act)
		if enemy_hp <= 0:
			break
		if _alive("kaelen"):
			await _kaelen_turn()
			if enemy_hp <= 0:
				break
		if _alive("yara"):
			await _yara_turn()
			if enemy_hp <= 0:
				break
		await _wolf_turn()
		if not _alive("player"):
			await _defeat()
			return
	await _victory()


func _player_command() -> String:
	_cmd_panel.visible = true
	_cmd_buttons[2].disabled = int(GameState.player_stats["mp"]) < 5
	_cmd_buttons[0].grab_focus()
	var act: String = await _command
	Audio.sfx("confirm", -8.0)
	_cmd_panel.visible = false
	return act


func _do_player(act: String) -> void:
	var s := GameState.player_stats
	match act:
		"attack":
			await _lunge("player", -40.0)
			var crit := randf() < 0.12 or (str(s["branch"]) == "dps" and randf() < 0.25)
			var dmg := int(s["atk"]) + randi_range(1, 4)
			if crit:
				dmg = int(dmg * 1.8)
			_damage_enemy(dmg, crit)
			await message("¡Golpe crítico! %d de daño." % dmg if crit else "Atacas al lobo: %d de daño." % dmg, 0.8)
			await _return("player")
		"defend":
			defending = true
			s["mp"] = mini(int(s["mp"]) + 2, int(s["max_mp"]))
			Audio.sfx("defend", -4.0)
			_flash(members["player"]["sprite"], Color(1.5, 1.5, 3))
			_refresh()
			await message("Te cubres y recuperas el aliento (+2 maná).", 0.9)
		"heal":
			s["mp"] = int(s["mp"]) - 5
			var target := _lowest_ally()
			var amount := 12 + int(s["mag"]) / 2 + randi_range(0, 3)
			_heal_member(target, amount)
			var who := "a ti" if target == "player" else "a " + str(GameState.companions[target]["display_name"])
			await message("Una luz cálida cura %s: +%d vida." % [who, amount], 0.9)


func _kaelen_turn() -> void:
	await get_tree().create_timer(0.2).timeout
	await _lunge("kaelen", -44.0)
	var crit := randf() < 0.1
	var dmg := int(GameState.companions["kaelen"]["atk"]) + randi_range(0, 3)
	if crit:
		dmg = int(dmg * 1.8)
	_damage_enemy(dmg, crit)
	var lines := ["Kaelen embiste con una rama gruesa: %d.", "Kaelen: «¡Toma esto!» %d de daño.", "Kaelen golpea con todas sus fuerzas: %d."]
	await message(lines[turn % lines.size()] % dmg, 0.8)
	await _return("kaelen")


func _yara_turn() -> void:
	await get_tree().create_timer(0.2).timeout
	var target := _lowest_ally()
	var ratio := float(_hp(target)) / float(_max_hp(target))
	if ratio < 0.6:
		var amount := int(GameState.companions["yara"]["mag"]) + randi_range(3, 7)
		members["yara"]["sprite"].frame = 4
		_heal_member(target, amount)
		var who := "te cura" if target == "player" else ("cura a Kaelen" if target == "kaelen" else "se cura")
		await message("Yara %s con flor de luna: +%d." % [who, amount], 0.9)
		members["yara"]["sprite"].frame = 3
	else:
		await _lunge("yara", -16.0)
		var dmg := randi_range(4, 7)
		_damage_enemy(dmg, false)
		await message("Yara lanza un destello de luz: %d." % dmg, 0.8)
		await _return("yara")


func _wolf_turn() -> void:
	await get_tree().create_timer(0.25).timeout
	if not enraged and enemy_hp < ENEMY["max_hp"] / 2:
		enraged = true
		enemy_atk += 2
		Audio.sfx("growl", -2.0)
		_shake(3.0)
		wolf.self_modulate = Color(1.2, 0.8, 1.2)
		await message("¡El lobo aúlla! Las venas negras de su lomo palpitan...", 1.3)
	var alive: Array = []
	var weights := {"player": 45, "kaelen": 35, "yara": 20}
	var total := 0
	for id in members:
		if _alive(id):
			alive.append(id)
			total += int(weights[id])
	var r := randi_range(1, total)
	var target: String = alive[0]
	for id in alive:
		r -= int(weights[id])
		if r <= 0:
			target = id
			break
	var fierce := turn % 3 == 0
	var tw := create_tween()
	tw.tween_property(wolf, "position", wolf.position + Vector2(60, 0), 0.14).set_trans(Tween.TRANS_QUAD)
	await tw.finished
	var dmg := enemy_atk + randi_range(0, 3)
	if fierce:
		dmg += 5
	var def := int(GameState.player_stats["def"]) if target == "player" else int(GameState.companions[target]["def"])
	dmg = maxi(1, dmg - def)
	if target == "player" and defending:
		dmg = maxi(1, dmg / 2)
	_damage_member(target, dmg)
	var tname := "a ti" if target == "player" else "a " + str(GameState.companions[target]["display_name"])
	var txt := ""
	if fierce:
		txt = "¡Mordisco feroz %s! %d de daño." % [tname, dmg]
	elif target == "player":
		txt = "El lobo te muerde: %d de daño." % dmg
	else:
		txt = "El lobo muerde %s: %d de daño." % [tname, dmg]
	if defending and target == "player":
		txt += " (bloqueas parte)"
	var tw2 := create_tween()
	tw2.tween_property(wolf, "position", Vector2(186, 160), 0.2)
	await message(txt, 1.0)
	if not _alive(target) and target != "player":
		await message("¡%s cae al suelo, sin fuerzas!" % GameState.companions[target]["display_name"], 1.0)


func _victory() -> void:
	_over = true
	Audio.play_music("victory", 0.2)
	var tw := create_tween().set_parallel(true)
	tw.tween_property(wolf, "position:x", -140.0, 0.9).set_trans(Tween.TRANS_QUAD).set_ease(Tween.EASE_IN)
	tw.tween_property(wolf, "modulate:a", 0.0, 0.9)
	await message("¡El lobo huye malherido hacia la espesura!", 1.6)
	for id in members:
		if _hp(id) <= 0:
			_set_hp(id, 1)
	_refresh()
	var levels := GameState.add_xp(120)
	await message("Ganáis 120 puntos de experiencia.", 1.4)
	if levels > 0:
		Audio.sfx("levelup", -2.0)
		for id in members:
			_set_hp(id, _max_hp(id))
			_flash(members[id]["sprite"], Color(2.5, 2.5, 1.2))
		_refresh()
		await message("¡{name} sube al nivel %d! El grupo se recupera." % GameState.player_stats["level"], 1.8)
	if GameState.needs_branch_choice():
		_show_branch_choice()
		await _branch_done
		await message("Has elegido la rama: %s." % GameState.branch_name(), 1.6)
	GameState.set_flag("wolf_defeated")
	var ret: Dictionary = GameState.battle_return
	Transition.go_to_map(str(ret.get("map", "forest")), str(ret.get("spawn", "party_vs_wolf")))


func _defeat() -> void:
	_over = true
	Audio.stop_music(0.8)
	await message("Todo se vuelve negro...", 1.6)
	var panel := UIKit.panel(Rect2(220, 130, 200, 90))
	add_child(panel)
	var l := UIKit.label("Habéis caído.", 16)
	l.position = Vector2(0, 12)
	l.size = Vector2(200, 24)
	l.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	panel.add_child(l)
	var b := UIKit.button("Reintentar")
	b.position = Vector2(50, 46)
	b.size = Vector2(100, 28)
	panel.add_child(b)
	b.grab_focus()
	await b.pressed
	GameState.heal_party()
	Transition.go_to_scene("res://scenes/Battle.tscn", 0.4)


# ------------------------------------------------------------ Elección de rama
func _show_branch_choice() -> void:
	var root := Control.new()
	root.set_anchors_preset(Control.PRESET_FULL_RECT)
	add_child(root)
	var dim := ColorRect.new()
	dim.color = Color(0, 0, 0, 0.6)
	dim.set_anchors_preset(Control.PRESET_FULL_RECT)
	root.add_child(dim)
	var panel := UIKit.panel(Rect2(110, 36, 420, 290))
	root.add_child(panel)
	var title := UIKit.label("Elige tu camino", 22, Color(0.35, 0.18, 0.1))
	title.position = Vector2(0, 12)
	title.size = Vector2(420, 30)
	title.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	panel.add_child(title)
	var sub := UIKit.label("Marcará cómo luchas. Más adelante se abrirá en clases.", 13)
	sub.position = Vector2(20, 42)
	sub.size = Vector2(380, 20)
	sub.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	panel.add_child(sub)
	var desc := UIKit.label("", 14)
	desc.position = Vector2(24, 232)
	desc.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	desc.size = Vector2(372, 48)
	desc.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	panel.add_child(desc)
	var box := VBoxContainer.new()
	box.position = Vector2(110, 68)
	box.size = Vector2(200, 160)
	box.add_theme_constant_override("separation", 3)
	panel.add_child(box)
	var first: Button = null
	for id in ["melee", "ranged", "dps", "healer", "tank"]:
		var info: Dictionary = GameState.BRANCHES[id]
		var b := UIKit.button(str(info["name"]))
		b.icon = load("res://assets/ui/icon_%s.png" % id)
		b.alignment = HORIZONTAL_ALIGNMENT_LEFT
		b.custom_minimum_size = Vector2(200, 28)
		b.focus_entered.connect(func(): desc.text = str(info["desc"]))
		b.mouse_entered.connect(func(): desc.text = str(info["desc"]))
		b.pressed.connect(func():
			GameState.choose_branch(id)
			Audio.sfx("levelup", -6.0)
			root.queue_free()
			_branch_done.emit())
		box.add_child(b)
		if first == null:
			first = b
	first.grab_focus()
	desc.text = str(GameState.BRANCHES["melee"]["desc"])
