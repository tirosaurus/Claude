extends Node2D
## Construye un mapa a partir de assets/maps/<id>.json y gestiona cámara,
## interacción, salidas, zonas de evento y el menú de pausa.

const PlayerScript := preload("res://scripts/world/Player.gd")
const NPCScript := preload("res://scripts/world/NPC.gd")
const StoryScript := preload("res://scripts/story/Story.gd")
const VIEW := Vector2(320, 180)

var data: Dictionary = {}
var map_size := Vector2.ZERO
var entities: Node2D
var player: CharacterBody2D
var camera: Camera2D
var story: Node
var npcs := {}
var props_by_id := {}
var interact_zones: Array = []   # {id, rect}
var cutscene := false
var _anim_props: Array = []
var _flicker_lights: Array = []
var _t := 0.0
var _hint: Node2D
var _hud: CanvasLayer
var _banner: Label
var _pause: Control
var _last_exit_block := -10.0


func _ready() -> void:
	var map_id: String = GameState.current_map
	var f := FileAccess.open("res://assets/maps/%s.json" % map_id, FileAccess.READ)
	data = JSON.parse_string(f.get_as_text())
	map_size = Vector2(data["size"][0], data["size"][1])

	var ground := Sprite2D.new()
	ground.texture = load(str(data["ground"]))
	ground.centered = false
	ground.z_index = -10
	add_child(ground)

	if data.has("modulate"):
		var cm := CanvasModulate.new()
		var m: Array = data["modulate"]
		cm.color = Color(m[0], m[1], m[2])
		add_child(cm)

	process_mode = Node.PROCESS_MODE_ALWAYS
	entities = Node2D.new()
	entities.y_sort_enabled = true
	entities.process_mode = Node.PROCESS_MODE_PAUSABLE
	add_child(entities)

	_build_walls()
	for p in data["props"]:
		_build_prop(p)
	for l in data["lights"]:
		_add_light(Vector2(l["x"], l["y"]), l)
	for e in data["exits"]:
		_build_exit(e)
	for t in data["triggers"]:
		_build_trigger(t)

	camera = Camera2D.new()
	camera.zoom = Vector2(2, 2)
	add_child(camera)
	camera.make_current()

	_build_hud()
	_build_particles(map_id)

	var spawn_name: String = GameState.current_spawn
	var markers: Dictionary = data["markers"]
	var spawn_pos := Vector2(markers.get(spawn_name, markers.values()[0])[0], markers.get(spawn_name, markers.values()[0])[1])
	player = PlayerScript.new()
	entities.add_child(player)
	player.setup(self, spawn_pos, _spawn_facing(spawn_name))
	_snap_camera()

	for id in GameState.party_followers():
		spawn_follower(id)

	story = StoryScript.new()
	story.world = self
	story.process_mode = Node.PROCESS_MODE_PAUSABLE
	add_child(story)

	Audio.play_music(str(data["music"]))
	_show_banner(str(data["display"]))
	story.on_map_ready(map_id)


func _spawn_facing(spawn_name: String) -> int:
	match spawn_name:
		"home_door", "from_upstairs", "from_downstairs":
			return 0
		"from_village":
			return 3 if GameState.current_map == "housemain" else 2
		"from_forest":
			return 1
	return 0


func marker(name: String) -> Vector2:
	var m: Array = data["markers"][name]
	return Vector2(m[0], m[1])


# ------------------------------------------------------------ Construcción
func _build_walls() -> void:
	var body := StaticBody2D.new()
	add_child(body)
	var w := map_size.x
	var h := map_size.y
	for r in [Rect2(-32, -32, w + 64, 32), Rect2(-32, h, w + 64, 32), Rect2(-32, 0, 32, h), Rect2(w, 0, 32, h)]:
		_add_rect_shape(body, r)
	for s in data["solids"]:
		_add_rect_shape(body, Rect2(s[0], s[1], s[2], s[3]))


func _add_rect_shape(body: CollisionObject2D, r: Rect2) -> void:
	var cs := CollisionShape2D.new()
	var shape := RectangleShape2D.new()
	shape.size = r.size
	cs.shape = shape
	cs.position = r.position + r.size / 2.0
	body.add_child(cs)


func _build_prop(p: Dictionary) -> void:
	var node := Node2D.new()
	node.position = Vector2(p["x"], p["y"])
	var spr := Sprite2D.new()
	spr.texture = load("res://assets/sprites/%s.png" % p["sprite"])
	var frames: int = int(p.get("frames", 1))
	spr.hframes = frames
	spr.centered = false
	var fw: float = spr.texture.get_width() / float(frames)
	spr.offset = Vector2(-floor(fw / 2.0), -spr.texture.get_height())
	spr.flip_h = bool(p.get("flip", false))
	node.add_child(spr)
	entities.add_child(node)
	if frames > 1:
		_anim_props.append({"sprite": spr, "frames": frames, "phase": randf() * 3.0})
	if p.has("col"):
		var c: Array = p["col"]
		var body := StaticBody2D.new()
		node.add_child(body)
		_add_rect_shape(body, Rect2(c[0], c[1], c[2], c[3]))
	if p.has("id"):
		props_by_id[str(p["id"])] = node
		var r: Rect2
		if p.has("col"):
			var c2: Array = p["col"]
			r = Rect2(node.position + Vector2(c2[0], c2[1]), Vector2(c2[2], c2[3]))
		else:
			r = Rect2(node.position - Vector2(8, 8), Vector2(16, 8))
		interact_zones.append({"id": str(p["id"]), "rect": r.grow(2)})
	if p.has("light"):
		var l: Dictionary = p["light"]
		_add_light(node.position + Vector2(0, float(l.get("oy", 0))), l)


func _add_light(pos: Vector2, l: Dictionary) -> void:
	var light := PointLight2D.new()
	light.texture = load("res://assets/ui/light.png")
	light.position = pos
	light.texture_scale = float(l.get("r", 64)) / 64.0
	var c: Array = l.get("color", [1, 1, 1])
	light.color = Color(c[0], c[1], c[2])
	light.energy = float(l.get("e", 0.6))
	light.blend_mode = Light2D.BLEND_MODE_ADD
	add_child(light)
	if l.get("flicker", false):
		_flicker_lights.append({"light": light, "base": light.energy, "seed": randf() * 10.0})


func _build_exit(e: Dictionary) -> void:
	var area := Area2D.new()
	var r: Array = e["rect"]
	_add_rect_shape(area, Rect2(r[0], r[1], r[2], r[3]))
	area.collision_mask = 1
	add_child(area)
	area.body_entered.connect(_on_exit.bind(e))


func _on_exit(b: Node, e: Dictionary) -> void:
	if b != player or cutscene or Transition.busy:
		return
	if e.has("requires") and not GameState.has_flag(str(e["requires"])):
		if _t - _last_exit_block < 0.5:
			return
		_last_exit_block = _t
		cutscene = true
		var back: Vector2 = -Player_DIR(player.facing) * 6.0
		player.position += back
		await Dialogue.say([str(e["blocked"])])
		cutscene = false
		return
	var interior_switch: bool = data["interior"] or str(e["to"]) in ["housemain", "bedroom"]
	Transition.go_to_map(str(e["to"]), str(e["spawn"]), interior_switch)


func Player_DIR(f: int) -> Vector2:
	return [Vector2.DOWN, Vector2.LEFT, Vector2.RIGHT, Vector2.UP][f]


func _build_trigger(t: Dictionary) -> void:
	var r: Array = t["rect"]
	var rect := Rect2(r[0], r[1], r[2], r[3])
	interact_zones.append({"id": str(t["id"]), "rect": rect})
	var area := Area2D.new()
	_add_rect_shape(area, rect)
	area.collision_mask = 1
	add_child(area)
	area.body_entered.connect(_on_trigger_body.bind(str(t["id"])))


func _on_trigger_body(b: Node, id: String) -> void:
	if b == player and not cutscene and not Transition.busy:
		story.on_trigger(id)


func _build_particles(map_id: String) -> void:
	var p := CPUParticles2D.new()
	p.position = map_size / 2.0
	p.emission_shape = CPUParticles2D.EMISSION_SHAPE_RECTANGLE
	p.emission_rect_extents = map_size / 2.0
	p.gravity = Vector2.ZERO
	p.z_index = 20
	match map_id:
		"forest":
			p.amount = 70
			p.lifetime = 6.0
			p.initial_velocity_min = 3.0
			p.initial_velocity_max = 10.0
			p.direction = Vector2(0.3, -1)
			p.spread = 180
			p.scale_amount_min = 1.0
			p.scale_amount_max = 2.0
			p.color = Color(0.85, 1.0, 0.6, 0.8)
			var g := Gradient.new()
			g.set_color(0, Color(1, 1, 0.7, 0))
			g.add_point(0.5, Color(0.9, 1, 0.6, 0.9))
			g.set_color(g.get_point_count() - 1, Color(1, 1, 0.7, 0))
			p.color_ramp = g
		"village":
			p.amount = 24
			p.lifetime = 8.0
			p.initial_velocity_min = 6.0
			p.initial_velocity_max = 14.0
			p.direction = Vector2(1, 0.4)
			p.spread = 20
			p.color = Color(1, 0.96, 0.85, 0.7)
		_:
			p.amount = 18
			p.lifetime = 7.0
			p.initial_velocity_min = 1.0
			p.initial_velocity_max = 4.0
			p.direction = Vector2(0.2, 1)
			p.spread = 180
			p.color = Color(1, 0.95, 0.8, 0.45)
	add_child(p)


func _build_hud() -> void:
	_hud = CanvasLayer.new()
	_hud.layer = 10
	add_child(_hud)
	_banner = Label.new()
	_banner.add_theme_font_size_override("font_size", 24)
	_banner.add_theme_color_override("font_color", Color(1, 0.93, 0.75))
	_banner.add_theme_color_override("font_outline_color", Color(0.1, 0.07, 0.1))
	_banner.add_theme_constant_override("outline_size", 6)
	_banner.position = Vector2(0, 26)
	_banner.size = Vector2(640, 40)
	_banner.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	_banner.modulate.a = 0.0
	_hud.add_child(_banner)

	_hint = Node2D.new()
	_hint.z_index = 50
	_hint.visible = false
	add_child(_hint)
	var bubble := Label.new()
	bubble.text = "E"
	bubble.add_theme_font_size_override("font_size", 8)
	bubble.add_theme_color_override("font_color", Color(0.24, 0.15, 0.11))
	var sb := StyleBoxFlat.new()
	sb.bg_color = Color(0.95, 0.9, 0.78)
	sb.border_color = Color(0.12, 0.09, 0.12)
	sb.set_border_width_all(1)
	sb.set_corner_radius_all(2)
	sb.content_margin_left = 3
	sb.content_margin_right = 3
	bubble.add_theme_stylebox_override("normal", sb)
	bubble.position = Vector2(-5, -8)
	_hint.add_child(bubble)

	_pause = _build_pause()
	_hud.add_child(_pause)


func _show_banner(text: String) -> void:
	if text == "" or text == GameState.get_meta("last_banner", ""):
		return
	GameState.set_meta("last_banner", text)
	_banner.text = text
	var tw := create_tween()
	tw.tween_interval(0.4)
	tw.tween_property(_banner, "modulate:a", 1.0, 0.6)
	tw.tween_interval(1.8)
	tw.tween_property(_banner, "modulate:a", 0.0, 0.8)


# ------------------------------------------------------------ NPCs
func spawn_npc(id: String, pos: Vector2, face: int = 0, mode: int = 0) -> Node2D:
	var n := NPCScript.new()
	entities.add_child(n)
	n.setup(id, pos, face, mode)
	npcs[id] = n
	return n


func spawn_follower(id: String) -> Node2D:
	if npcs.has(id):
		return npcs[id]
	var gap := 16 if id == "kaelen" else 32
	var idx: int = max(0, player.trail.size() - 1 - gap)
	var n := spawn_npc(id, player.trail[idx], player.facing, NPCScript.Mode.FOLLOW)
	n.follow_target = player
	n.follow_gap = gap
	return n


# ------------------------------------------------------------ Interacción
func try_interact(p: CharacterBody2D) -> void:
	var target = _find_target(p)
	if target == null:
		return
	if target is Dictionary:
		story.on_interact(str(target["id"]))
	else:
		story.on_npc(target.char_id)


func _find_target(p: CharacterBody2D):
	var probe: Vector2 = p.probe_point()
	var best = null
	var best_d := 14.0
	for id in npcs:
		var n: Node2D = npcs[id]
		if n.mode == NPCScript.Mode.FOLLOW and not story.follower_talkable(id):
			continue
		var d := probe.distance_to(n.position + Vector2(0, -4))
		if d < best_d:
			best_d = d
			best = n
	for z in interact_zones:
		var r: Rect2 = z["rect"]
		var closest := Vector2(clampf(probe.x, r.position.x, r.end.x), clampf(probe.y, r.position.y, r.end.y))
		var d := probe.distance_to(closest)
		if d < best_d and story.is_interactable(str(z["id"])):
			best_d = d
			best = z
	return best


# ------------------------------------------------------------ Bucle
func _process(delta: float) -> void:
	if get_tree().paused:
		if Input.is_action_just_pressed("pause") or Input.is_action_just_pressed("cancel"):
			_toggle_pause()
		return
	_t += delta
	for a in _anim_props:
		a["sprite"].frame = int(_t * 6.0 + a["phase"]) % int(a["frames"])
	for fl in _flicker_lights:
		var l: PointLight2D = fl["light"]
		l.energy = fl["base"] * (0.88 + 0.12 * sin(_t * 9.0 + fl["seed"]) + 0.06 * sin(_t * 23.0 + fl["seed"] * 2.0))
	if player:
		_update_camera(delta)
		var can_hint: bool = not cutscene and not Dialogue.active and not Transition.busy
		var target = _find_target(player) if can_hint else null
		_hint.visible = target != null
		if target != null:
			if target is Dictionary:
				var r: Rect2 = target["rect"]
				_hint.position = Vector2(r.get_center().x, r.position.y - 6)
			else:
				_hint.position = target.position + Vector2(0, -28)
			_hint.position.y += sin(_t * 5.0) * 1.5
	if Input.is_action_just_pressed("pause") and not Dialogue.active and not cutscene and not Transition.busy:
		_toggle_pause()


func _update_camera(delta: float) -> void:
	var target := _camera_target()
	camera.position = camera.position.lerp(target, clampf(delta * 8.0, 0, 1))


func _snap_camera() -> void:
	camera.position = _camera_target()
	camera.reset_smoothing()


func _camera_target() -> Vector2:
	var p := player.position + Vector2(0, -8)
	var half := VIEW / 2.0
	var t := Vector2.ZERO
	t.x = map_size.x / 2.0 if map_size.x <= VIEW.x else clampf(p.x, half.x, map_size.x - half.x)
	t.y = map_size.y / 2.0 if map_size.y <= VIEW.y else clampf(p.y, half.y, map_size.y - half.y)
	return t.round()


# ------------------------------------------------------------ Pausa
var _pause_info: Label
var _pause_buttons: Array[Button] = []


func _build_pause() -> Control:
	var root := Control.new()
	root.set_anchors_preset(Control.PRESET_FULL_RECT)
	root.visible = false
	root.process_mode = Node.PROCESS_MODE_ALWAYS
	var dim := ColorRect.new()
	dim.color = Color(0, 0, 0, 0.55)
	dim.set_anchors_preset(Control.PRESET_FULL_RECT)
	root.add_child(dim)
	var panel := NinePatchRect.new()
	panel.texture = load("res://assets/ui/panel.png")
	for side in ["patch_margin_left", "patch_margin_right", "patch_margin_top", "patch_margin_bottom"]:
		panel.set(side, 8)
	panel.position = Vector2(150, 34)
	panel.size = Vector2(340, 292)
	root.add_child(panel)
	var title := Label.new()
	title.text = "Pausa"
	title.add_theme_font_size_override("font_size", 22)
	title.add_theme_color_override("font_color", Color(0.35, 0.18, 0.1))
	title.position = Vector2(0, 12)
	title.size = Vector2(340, 30)
	title.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	panel.add_child(title)
	_pause_info = Label.new()
	_pause_info.add_theme_font_size_override("font_size", 14)
	_pause_info.add_theme_color_override("font_color", Color(0.24, 0.15, 0.11))
	_pause_info.position = Vector2(24, 46)
	_pause_info.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	_pause_info.size = Vector2(292, 150)
	panel.add_child(_pause_info)
	var box := VBoxContainer.new()
	box.position = Vector2(90, 212)
	box.size = Vector2(160, 70)
	box.add_theme_constant_override("separation", 6)
	panel.add_child(box)
	for pair in [["Continuar", _toggle_pause], ["Volver al título", _to_title]]:
		var b := UIKit.button(pair[0])
		b.pressed.connect(pair[1])
		box.add_child(b)
		_pause_buttons.append(b)
	return root


func _toggle_pause() -> void:
	var show := not _pause.visible
	_pause.visible = show
	get_tree().paused = show
	if show:
		Audio.sfx("select", -8.0)
		var s := GameState.player_stats
		var txt := "%s  ·  %s  ·  Nivel %d\nRama: %s\nVida %d/%d   Maná %d/%d\n" % [
			GameState.player_name(), GameState.race_name(), s["level"], GameState.branch_name(),
			s["hp"], s["max_hp"], s["mp"], s["max_mp"]]
		if GameState.companions["kaelen"]["in_party"]:
			txt += "\nKaelen: %s" % GameState.kaelen_bond_label()
		if GameState.companions["yara"]["in_party"]:
			txt += "\nYara: %s" % GameState.yara_bond_label()
		txt += "\n\nMover: WASD / flechas\nCorrer: Mayús   ·   Hablar: E   ·   Pausa: Esc"
		_pause_info.text = txt
		_pause_buttons[0].grab_focus()


func _to_title() -> void:
	get_tree().paused = false
	_pause.visible = false
	Transition.go_to_scene("res://scenes/Title.tscn")
