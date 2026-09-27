extends Node2D
## Construye un mapa desde assets/maps/<id>.json y gestiona cámara,
## interacción, salidas, zonas, encuentros aleatorios y menús.

const PlayerScript := preload("res://scripts/world/Player.gd")
const NPCScript := preload("res://scripts/world/NPC.gd")
const PauseScript := preload("res://scripts/ui/PauseMenu.gd")
const VIEW := Vector2(320, 180)
const TOD_MAPS := ["village", "bedroom", "housemain", "forest"]

var map_id := ""
var data: Dictionary = {}
var map_size := Vector2.ZERO
var entities: Node2D
var player: CharacterBody2D
var camera: Camera2D
var story: Node
var npcs := {}
var props_by_id := {}
var interact_zones: Array = []
var cutscene := false
var _anim_props: Array = []
var _flicker_lights: Array = []
var _t := 0.0
var _hint: Node2D
var _hud: CanvasLayer
var _banner: Label
var _pause: Control
var _last_exit_block := -10.0
var _steps := 0.0
var _next_encounter := 0.0
var _modulate: CanvasModulate


func _ready() -> void:
	process_mode = Node.PROCESS_MODE_ALWAYS
	map_id = GameState.current_map
	var f := FileAccess.open("res://assets/maps/%s.json" % map_id, FileAccess.READ)
	data = JSON.parse_string(f.get_as_text())
	map_size = Vector2(data["size"][0], data["size"][1])

	var ground := Sprite2D.new()
	ground.texture = load(str(data["ground"]))
	ground.centered = false
	ground.z_index = -10
	add_child(ground)

	_modulate = CanvasModulate.new()
	_modulate.color = _map_modulate()
	add_child(_modulate)

	entities = Node2D.new()
	entities.y_sort_enabled = true
	entities.process_mode = Node.PROCESS_MODE_PAUSABLE
	add_child(entities)

	_build_walls()
	for p in data["props"]:
		_build_prop(p)
	var night := _is_dark()
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
	_build_particles()

	var spawn_pos := Vector2.ZERO
	var markers: Dictionary = data["markers"]
	var face := 0
	if GameState.load_pos != null:
		spawn_pos = GameState.load_pos
		GameState.load_pos = null
	else:
		var sp: String = GameState.current_spawn
		var m: Array = markers.get(sp, markers.values()[0])
		spawn_pos = Vector2(m[0], m[1])
		face = _spawn_facing(sp)
	player = PlayerScript.new()
	entities.add_child(player)
	player.setup(self, spawn_pos, face)
	_snap_camera()
	for id in GameState.party_followers():
		spawn_follower(id)
	_reset_encounter()

	var script_path := "res://scripts/story/maps/%s.gd" % map_id
	var script: Script = load(script_path) if ResourceLoader.exists(script_path) else load("res://scripts/story/StoryBase.gd")
	story = script.new()
	story.world = self
	story.process_mode = Node.PROCESS_MODE_PAUSABLE
	add_child(story)

	Audio.play_music(story.music_override() if story.music_override() != "" else str(data["music"]))
	_show_banner(str(data["display"]))
	story.on_map_ready(map_id)


func _is_dark() -> bool:
	return map_id in TOD_MAPS and GameState.get_meta("tod", "day") in ["night", "dusk"]


func _map_modulate() -> Color:
	var base := Color.WHITE
	if data.has("modulate"):
		var m: Array = data["modulate"]
		base = Color(m[0], m[1], m[2])
	if map_id in TOD_MAPS:
		match str(GameState.get_meta("tod", "day")):
			"dusk":
				return base * Color(1.0, 0.8, 0.66)
			"night":
				return base * Color(0.46, 0.48, 0.72)
			"dawn":
				return base * Color(0.86, 0.8, 0.88)
	return base


func _spawn_facing(spawn_name: String) -> int:
	match spawn_name:
		"home_door", "from_upstairs", "from_downstairs", "from_crypt":
			return 0
		"from_village":
			return 3 if map_id in ["housemain", "cathedral"] else 2
		"from_forest":
			return 1 if map_id.begins_with("village") else 2
		"from_deep", "from_camp", "from_cathedral":
			return 3 if map_id == "heart" else (2 if map_id == "elf_camp" else 1)
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
	spr.name = "Sprite"
	spr.texture = load("res://assets/sprites/%s.png" % p["sprite"])
	var frames: int = int(p.get("frames", 1))
	spr.hframes = frames
	spr.centered = false
	var fw: float = spr.texture.get_width() / float(frames)
	spr.offset = Vector2(-floor(fw / 2.0), -spr.texture.get_height() - float(p.get("lift", 0)))
	spr.flip_h = bool(p.get("flip", false))
	node.add_child(spr)
	entities.add_child(node)
	if frames > 1:
		_anim_props.append({"sprite": spr, "frames": frames, "phase": randf() * 3.0})
	if p.has("col"):
		var c: Array = p["col"]
		var body := StaticBody2D.new()
		body.name = "Body"
		node.add_child(body)
		_add_rect_shape(body, Rect2(c[0], c[1], c[2], c[3]))
	if p.has("id"):
		var pid := str(p["id"])
		props_by_id[pid] = node
		var r: Rect2
		if p.has("col"):
			var c2: Array = p["col"]
			r = Rect2(node.position + Vector2(c2[0], c2[1]), Vector2(c2[2], c2[3]))
		else:
			r = Rect2(node.position - Vector2(8, 8), Vector2(16, 8))
		interact_zones.append({"id": pid, "rect": r.grow(2)})
		if pid.begins_with("chest_") and GameState.has_flag("opened_" + pid):
			spr.modulate = Color(0.55, 0.5, 0.5)
	if p.has("light"):
		var l: Dictionary = p["light"]
		_add_light(node.position + Vector2(0, float(l.get("oy", 0))), l)


func set_prop_sprite(pid: String, sprite_name: String) -> void:
	if not props_by_id.has(pid):
		return
	var spr: Sprite2D = props_by_id[pid].get_node("Sprite")
	spr.texture = load("res://assets/sprites/%s.png" % sprite_name)
	var fw: float = spr.texture.get_width() / float(spr.hframes)
	spr.offset = Vector2(-floor(fw / 2.0), -spr.texture.get_height())


func remove_prop(pid: String) -> void:
	if props_by_id.has(pid):
		props_by_id[pid].queue_free()
		props_by_id.erase(pid)
		interact_zones = interact_zones.filter(func(z): return z["id"] != pid)


func add_prop(sprite_name: String, pos: Vector2, pid: String = "", frames: int = 1) -> Node2D:
	var p := {"sprite": sprite_name, "x": pos.x, "y": pos.y, "frames": frames}
	if pid != "":
		p["id"] = pid
	_build_prop(p)
	return props_by_id.get(pid)


func _add_light(pos: Vector2, l: Dictionary) -> PointLight2D:
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
	return light


func _build_exit(e: Dictionary) -> void:
	var area := Area2D.new()
	var r: Array = e["rect"]
	_add_rect_shape(area, Rect2(r[0], r[1], r[2], r[3]))
	area.collision_mask = 1
	add_child(area)
	area.body_entered.connect(func(b): _on_exit.call_deferred(b, e))


func resolve_target(to: String) -> String:
	match to:
		"village_any":
			if GameState.has_flag("act3_started") or GameState.has_flag("night_over"):
				return "village_dawn"
			if GameState.has_flag("night_attack"):
				return "village_night"
			return "village"
		"village_night_or_dawn":
			return "village_dawn" if GameState.has_flag("night_over") else "village_night"
		"village":
			if GameState.has_flag("night_over"):
				return "village_dawn"
			if GameState.has_flag("night_attack"):
				return "village_night"
	return to


func _on_exit(b: Node, e: Dictionary) -> void:
	if b != player or cutscene or Transition.busy:
		return
	var to := resolve_target(str(e["to"]))
	var req: String = story.exit_requirement(str(e["to"]), str(e.get("requires", "")))
	if req != "" and not GameState.has_flag(req):
		if _t - _last_exit_block < 0.5:
			return
		_last_exit_block = _t
		cutscene = true
		player.position -= Player_DIR(player.facing) * 6.0
		var msg: String = story.exit_blocked_message(str(e["to"]), str(e.get("blocked", "")))
		await Dialogue.say([msg])
		cutscene = false
		return
	if not story.before_exit(to):
		return
	var door: bool = data["interior"] or to in ["housemain", "bedroom", "cathedral", "crypt"]
	Transition.go_to_map(to, str(e["spawn"]), door)


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
		story.on_trigger.call_deferred(id)


func _build_particles() -> void:
	var p := CPUParticles2D.new()
	p.position = map_size / 2.0
	p.emission_shape = CPUParticles2D.EMISSION_SHAPE_RECTANGLE
	p.emission_rect_extents = map_size / 2.0
	p.gravity = Vector2.ZERO
	p.z_index = 20
	p.process_mode = Node.PROCESS_MODE_PAUSABLE
	var g := Gradient.new()
	match map_id:
		"forest", "elf_camp":
			p.amount = 70
			p.lifetime = 6.0
			p.initial_velocity_min = 3.0
			p.initial_velocity_max = 10.0
			p.direction = Vector2(0.3, -1)
			p.spread = 180
			p.scale_amount_max = 2.0
			g.set_color(0, Color(1, 1, 0.7, 0))
			g.add_point(0.5, Color(0.9, 1, 0.6, 0.9))
			g.set_color(g.get_point_count() - 1, Color(1, 1, 0.7, 0))
			p.color_ramp = g
		"forest_deep", "heart":
			p.amount = 80
			p.lifetime = 6.0
			p.initial_velocity_min = 2.0
			p.initial_velocity_max = 8.0
			p.direction = Vector2(0, -1)
			p.spread = 180
			p.scale_amount_max = 2.0
			g.set_color(0, Color(0.9, 0.5, 1, 0))
			g.add_point(0.5, Color(0.9, 0.5, 1, 0.9))
			g.set_color(g.get_point_count() - 1, Color(0.9, 0.5, 1, 0))
			p.color_ramp = g
		"village_night":
			p.amount = 60
			p.lifetime = 4.0
			p.initial_velocity_min = 10.0
			p.initial_velocity_max = 30.0
			p.direction = Vector2(0.2, -1)
			p.spread = 30
			p.scale_amount_max = 2.0
			g.set_color(0, Color(1, 0.7, 0.3, 0.9))
			g.set_color(g.get_point_count() - 1, Color(1, 0.3, 0.1, 0))
			p.color_ramp = g
		"village", "village_dawn":
			p.amount = 24
			p.lifetime = 8.0
			p.initial_velocity_min = 6.0
			p.initial_velocity_max = 14.0
			p.direction = Vector2(1, 0.4)
			p.spread = 20
			p.color = Color(1, 0.96, 0.85, 0.7) if map_id == "village" else Color(0.7, 0.7, 0.72, 0.6)
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
	_banner = UIKit.label("", 24, Color(1, 0.93, 0.75), 6)
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

	_pause = PauseScript.new()
	_pause.world = self
	_hud.add_child(_pause)


func show_banner(text: String) -> void:
	_show_banner(text, true)


func _show_banner(text: String, force: bool = false) -> void:
	if text == "" or (not force and text == GameState.get_meta("last_banner", "")):
		return
	GameState.set_meta("last_banner", text)
	_banner.text = text
	var tw := create_tween()
	tw.tween_interval(0.4)
	tw.tween_property(_banner, "modulate:a", 1.0, 0.6)
	tw.tween_interval(1.8)
	tw.tween_property(_banner, "modulate:a", 0.0, 0.8)


# ------------------------------------------------------------ NPCs
func spawn_npc(id: String, pos: Vector2, face: int = 0, mode: int = 0, tex: Texture2D = null) -> Node2D:
	if npcs.has(id):
		npcs[id].queue_free()
	var n := NPCScript.new()
	entities.add_child(n)
	n.setup(id, pos, face, mode, tex)
	npcs[id] = n
	return n


func spawn_follower(id: String) -> Node2D:
	var idx_in_party := GameState.party_followers().find(id)
	var gap := 16 * (idx_in_party + 1)
	var idx: int = max(0, player.trail.size() - 1 - gap)
	var n := spawn_npc(id, player.trail[idx], player.facing, NPCScript.Mode.FOLLOW, Appearance.member_sheet(id))
	n.follow_target = player
	n.follow_gap = gap
	return n


func make_follower(id: String) -> void:
	if not npcs.has(id):
		spawn_follower(id)
		return
	var n = npcs[id]
	var idx_in_party := GameState.party_followers().find(id)
	n.follow_target = player
	n.follow_gap = 16 * (max(0, idx_in_party) + 1)
	n.set_mode(NPCScript.Mode.FOLLOW)


func remove_npc(id: String) -> void:
	if npcs.has(id):
		npcs[id].queue_free()
		npcs.erase(id)


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
		if not is_instance_valid(n):
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


# ------------------------------------------------------------ Encuentros
func _reset_encounter() -> void:
	_steps = 0.0
	if data.has("encounters"):
		var rate: float = float(data["encounters"]["rate"])
		_next_encounter = randf_range(rate * 0.6, rate * 1.4) * 16.0
	else:
		_next_encounter = INF


func on_player_moved(dist: float) -> void:
	if not data.has("encounters") or cutscene or Transition.busy or story.encounters_disabled():
		return
	_steps += dist
	if _steps >= _next_encounter:
		_reset_encounter()
		var table: Array = data["encounters"]["table"]
		var formation: Array = table.pick_random()
		start_battle({"enemies": formation, "bg": str(data.get("battle_bg", "forest")), "can_flee": true},
			{"map": map_id, "pos": [player.position.x, player.position.y]})


func start_battle(encounter: Dictionary, ret: Dictionary = {}) -> void:
	GameState.pending_encounter = encounter
	if ret.is_empty():
		ret = {"map": map_id, "pos": [player.position.x, player.position.y]}
	GameState.battle_return = ret
	cutscene = true
	await Transition.battle_flash()


# ------------------------------------------------------------ Bucle
func _process(delta: float) -> void:
	if get_tree().paused:
		return
	_t += delta
	for a in _anim_props:
		if is_instance_valid(a["sprite"]):
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
		_pause.open()


func _update_camera(delta: float) -> void:
	camera.position = camera.position.lerp(_camera_target(), clampf(delta * 8.0, 0, 1))


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


func set_tint(c: Color, duration: float = 1.0) -> void:
	var tw := create_tween()
	tw.tween_property(_modulate, "color", c, duration)
