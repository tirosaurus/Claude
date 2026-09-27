extends Control
## Combate estilo JRPG con barras de tiempo activo (modo espera).
## GameState.pending_encounter = {enemies: [...], bg, music, boss, can_flee, id}

const BattlerScript := preload("res://scripts/battle/Battler.gd")
const ATB_RATE := 1.15
const PARTY_POS := [Vector2(500, 84), Vector2(530, 126), Vector2(500, 168), Vector2(530, 210)]
const C_TEXT := Color(0.97, 0.97, 1.0)
const C_HI := Color(1.0, 0.86, 0.35)
const C_DIS := Color(0.55, 0.57, 0.68)
const C_HP := Color(0.36, 0.86, 0.42)
const C_MP := Color(0.42, 0.66, 1.0)
const ENEMY_SLOTS := {
	1: [Vector2(180, 168)],
	2: [Vector2(140, 130), Vector2(220, 200)],
	3: [Vector2(110, 118), Vector2(220, 150), Vector2(130, 214)],
	4: [Vector2(100, 112), Vector2(214, 118), Vector2(110, 210), Vector2(224, 214)],
	5: [Vector2(90, 110), Vector2(200, 106), Vector2(150, 164), Vector2(96, 218), Vector2(214, 220)],
}
const EL_ICON := {"fire": "el_fire", "ice": "el_ice", "bolt": "el_bolt", "light": "el_light", "dark": "el_dark",
	"nature": "el_nature"}
const EL_COLOR := {"fire": Color(1, 0.55, 0.2), "ice": Color(0.6, 0.9, 1), "bolt": Color(1, 0.95, 0.4),
	"light": Color(1, 0.97, 0.7), "dark": Color(0.7, 0.35, 0.9), "nature": Color(0.5, 0.95, 0.5), "": Color(1, 1, 1)}

var enc: Dictionary = {}
var stage: Node2D
var party: Array = []
var enemies: Array = []
var state := "idle"
var actor = null
var ready_queue: Array = []
var _busy := false
var _over := false
var _t := 0.0
var _start_snapshot := {}

# UI
var _top_panel: NinePatchRect
var _turn_arrow: Sprite2D
var _sel_bar: NinePatchRect
var _top_label: Label
var _enemy_panel: NinePatchRect
var _enemy_labels: Array = []
var _party_panel: NinePatchRect
var _rows := {}
var _cmd_panel: NinePatchRect
var _cmd_items: Array = []
var _cmd_index := 0
var _sub_panel: NinePatchRect
var _sub_rows: Array = []
var _sub_entries: Array = []
var _sub_index := 0
var _sub_scroll := 0
var _sub_kind := ""
var _hand: Sprite2D
var _hand2: Array = []
var _target_mode := ""
var _target_index := 0
var _target_list: Array = []
var _pending_action := {}
var _auto_label: Label
var _ignore_until := 0

signal _chooser_done(index: int)


func _ready() -> void:
	enc = GameState.pending_encounter
	if enc.is_empty():
		enc = {"enemies": ["wolf"], "bg": "forest"}
	_start_snapshot = GameState.members.duplicate(true)
	_build_scene()
	_build_ui()
	Audio.play_music(str(enc.get("music", "boss" if enc.get("boss", false) else "battle")), 0.3)
	_ignore_until = Engine.get_process_frames() + 10
	_intro.call_deferred()


# ------------------------------------------------------------ Construcción
func _build_scene() -> void:
	var bg := TextureRect.new()
	var bgname := str(enc.get("bg", "forest"))
	var path := "res://assets/bg/battle_%s.png" % bgname
	bg.texture = load(path if ResourceLoader.exists(path) else "res://assets/bg/battle_forest.png")
	bg.size = Vector2(640, 360)
	bg.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
	bg.stretch_mode = TextureRect.STRETCH_SCALE
	add_child(bg)
	stage = Node2D.new()
	add_child(stage)

	var list: Array = enc["enemies"]
	var slots: Array = ENEMY_SLOTS[clampi(list.size(), 1, 5)]
	for i in list.size():
		_spawn_enemy(str(list[i]), slots[i] if list.size() > 1 else (Vector2(170, 160) if enc.get("boss", false) else slots[0]))

	var i := 0
	for id in GameState.party:
		if i >= 4:
			break
		var b := BattlerScript.new()
		b.id = id
		b.key = id
		b.display_name = GameState.member_name(id)
		b.max_stats = GameState.stats(id)
		b.hp = int(GameState.members[id]["hp"])
		b.mp = int(GameState.members[id]["mp"])
		b.home = PARTY_POS[i]
		b.position = b.home
		b.atb = randf_range(20, 60) + b.max_stats["spd"] + (GameState.perk("atb_start") if id == "player" else 0.0)
		_shadow(b, 18, 5, Vector2(0, 30))
		var s := Sprite2D.new()
		s.texture = Appearance.member_battle_sheet(id)
		s.hframes = 9
		s.frame = 0
		s.scale = Vector2(2, 2)
		b.add_child(s)
		b.sprite = s
		b.party_anim = true
		var mb := _mini_bar(Vector2(-18, 36), 36, C_HP)
		b.add_child(mb)
		b.hp_bar = mb
		mb.max_value = b.max_hp()
		mb.value = b.hp
		stage.add_child(b)
		party.append(b)
		_apply_dead_look(b)
		i += 1


func _spawn_enemy(key: String, pos: Vector2) -> Node2D:
	var d: Dictionary = DB.ENEMIES[key]
	var b := BattlerScript.new()
	b.is_enemy = true
	b.key = key
	b.id = key + str(enemies.size())
	b.data = d
	b.display_name = str(d["name"])
	var dm: Array = GameState.diff_mults()
	b.max_stats = {"hp": int(d["hp"] * dm[0]), "mp": 99, "atk": d["atk"] * dm[1], "def": d["def"], "mag": d["mag"] * dm[1],
		"res": d["res"], "spd": d["spd"]}
	b.hp = b.max_hp()
	b.home = pos
	b.position = pos
	b.atb = randf_range(0, 40)
	var s := Sprite2D.new()
	s.texture = Appearance.tex("res://assets/%s.png" % d["sprite"])
	var fr: int = int(d["frames"])
	b.frames = fr
	if fr == 0:
		s.hframes = 8
		s.vframes = 4
		s.frame = 16
	elif fr == -1:
		s.hframes = 9
		s.frame = 0
		s.flip_h = true
	else:
		s.hframes = fr
	var sc: float = float(d["scale"])
	s.scale = Vector2(sc, sc)
	if d.has("tint"):
		var tn: Array = d["tint"]
		s.modulate = Color(tn[0], tn[1], tn[2])
	var h: float = s.texture.get_height() / float(s.vframes) * sc
	_shadow(b, s.texture.get_width() / float(s.hframes) * sc * 0.36, 6, Vector2(0, h * 0.45))
	b.add_child(s)
	b.sprite = s
	var bar := ProgressBar.new()
	bar.show_percentage = false
	bar.size = Vector2(40, 4)
	bar.position = Vector2(-20, h * 0.5 + 4)
	bar.max_value = b.max_hp()
	bar.value = b.hp
	var bgs := StyleBoxFlat.new()
	bgs.bg_color = Color(0.1, 0.05, 0.08, 0.8)
	var fg := StyleBoxFlat.new()
	fg.bg_color = Color(0.85, 0.25, 0.3)
	bar.add_theme_stylebox_override("background", bgs)
	bar.add_theme_stylebox_override("fill", fg)
	b.add_child(bar)
	b.hp_bar = bar
	stage.add_child(b)
	enemies.append(b)
	return b


func _shadow(parent: Node2D, rx: float, ry: float, offset: Vector2) -> void:
	var poly := Polygon2D.new()
	var pts := PackedVector2Array()
	for k in 20:
		var a := TAU * k / 20.0
		pts.append(offset + Vector2(cos(a) * rx, sin(a) * ry))
	poly.polygon = pts
	poly.color = Color(0, 0, 0, 0.35)
	parent.add_child(poly)


func _win(rect: Rect2, red: bool = false) -> NinePatchRect:
	var n := NinePatchRect.new()
	n.texture = load("res://assets/ui/window_red.png" if red else "res://assets/ui/window.png")
	n.patch_margin_left = 6
	n.patch_margin_right = 6
	n.patch_margin_top = 6
	n.patch_margin_bottom = 6
	n.position = rect.position
	n.size = rect.size
	return n


func _bar(pos: Vector2, w: float, h: float, col: Color) -> ProgressBar:
	var bar := ProgressBar.new()
	bar.show_percentage = false
	bar.position = pos
	bar.size = Vector2(w, h)
	bar.custom_minimum_size = Vector2(w, h)
	var bgs := StyleBoxFlat.new()
	bgs.bg_color = Color(0.03, 0.03, 0.1, 0.9)
	bgs.border_color = Color(0.0, 0.0, 0.0, 0.9)
	bgs.set_border_width_all(1)
	var fg := StyleBoxFlat.new()
	fg.bg_color = col
	fg.border_color = col.lightened(0.35)
	fg.border_width_top = 1
	bar.add_theme_stylebox_override("background", bgs)
	bar.add_theme_stylebox_override("fill", fg)
	bar.set_meta("fill", fg)
	return bar


func _mini_bar(pos: Vector2, w: float, col: Color) -> ProgressBar:
	var b := _bar(pos, w, 4, col)
	b.z_index = 5
	return b


func _hp_color(r: float) -> Color:
	if r <= 0.25:
		return Color(0.95, 0.3, 0.28)
	if r <= 0.5:
		return Color(0.98, 0.8, 0.3)
	return C_HP


func _build_ui() -> void:
	_top_panel = _win(Rect2(120, 6, 400, 30))
	add_child(_top_panel)
	_top_label = UIKit.label("", 15, C_TEXT, 3)
	_top_label.position = Vector2(8, 5)
	_top_label.size = Vector2(384, 20)
	_top_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	_top_label.clip_text = true
	_top_panel.add_child(_top_label)
	_top_panel.visible = false

	_enemy_panel = _win(Rect2(6, 262, 196, 94))
	add_child(_enemy_panel)
	for k in 5:
		var l := UIKit.label("", 13, C_TEXT, 3)
		l.position = Vector2(12, 6 + k * 16)
		l.size = Vector2(176, 16)
		l.clip_text = true
		_enemy_panel.add_child(l)
		_enemy_labels.append(l)

	_party_panel = _win(Rect2(206, 262, 428, 94))
	add_child(_party_panel)
	var hdr := [["HP", 112], ["PM", 216], ["TIEMPO", 312]]
	for h in hdr:
		var hl := UIKit.label(h[0], 9, Color(0.75, 0.8, 1.0), 2)
		hl.position = Vector2(h[1], 1)
		_party_panel.add_child(hl)
	var y := 10
	for b in party:
		var row := {}
		var por := TextureRect.new()
		por.texture = Appearance.member_portrait(b.id)
		por.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
		por.stretch_mode = TextureRect.STRETCH_KEEP_ASPECT_CENTERED
		por.position = Vector2(6, y - 3)
		por.size = Vector2(24, 24)
		por.custom_minimum_size = Vector2(24, 24)
		_party_panel.add_child(por)
		row["portrait"] = por
		var nm := UIKit.label(b.display_name, 13, C_TEXT, 3)
		nm.position = Vector2(32, y + 1)
		nm.size = Vector2(76, 16)
		nm.clip_text = true
		_party_panel.add_child(nm)
		row["name"] = nm
		var hp := UIKit.label("", 13, C_TEXT, 3)
		hp.position = Vector2(112, y - 2)
		hp.size = Vector2(92, 16)
		hp.horizontal_alignment = HORIZONTAL_ALIGNMENT_RIGHT
		_party_panel.add_child(hp)
		row["hp"] = hp
		var hpb := _bar(Vector2(112, y + 14), 92, 5, C_HP)
		hpb.max_value = b.max_hp()
		hpb.value = b.hp
		_party_panel.add_child(hpb)
		row["hp_bar"] = hpb
		var mp := UIKit.label("", 13, Color(0.8, 0.88, 1.0), 3)
		mp.position = Vector2(216, y - 2)
		mp.size = Vector2(84, 16)
		mp.horizontal_alignment = HORIZONTAL_ALIGNMENT_RIGHT
		_party_panel.add_child(mp)
		row["mp"] = mp
		var mpb := _bar(Vector2(216, y + 14), 84, 5, C_MP)
		mpb.max_value = max(1, b.max_mp())
		mpb.value = b.mp
		_party_panel.add_child(mpb)
		row["mp_bar"] = mpb
		var atb := _bar(Vector2(312, y + 5), 64, 7, Color(0.85, 0.7, 0.25))
		atb.max_value = 100
		_party_panel.add_child(atb)
		row["atb"] = atb
		row["atb_fill"] = atb.get_meta("fill")
		var icons := HBoxContainer.new()
		icons.position = Vector2(380, y + 3)
		icons.add_theme_constant_override("separation", 1)
		_party_panel.add_child(icons)
		row["icons"] = icons
		_rows[b.id] = row
		y += 20

	_cmd_panel = _win(Rect2(6, 262, 150, 94))
	add_child(_cmd_panel)
	_sel_bar = NinePatchRect.new()
	_sel_bar.texture = load("res://assets/ui/select_bar.png")
	_sel_bar.size = Vector2(132, 17)
	_sel_bar.modulate = Color(1, 1, 1, 0.55)
	_cmd_panel.add_child(_sel_bar)
	var cmds := [["Atacar", "cmd_attack", "attack"], ["Habilidades", "cmd_skill", "skill"], ["Objetos", "cmd_item", "item"],
		["Defender", "cmd_defend", "defend"], ["Huir", "cmd_flee", "flee"]]
	for k in cmds.size():
		var ic := TextureRect.new()
		ic.texture = load("res://assets/ui/%s.png" % cmds[k][1])
		ic.position = Vector2(26, 6 + k * 17)
		ic.size = Vector2(16, 16)
		_cmd_panel.add_child(ic)
		var l := UIKit.label(cmds[k][0], 14, C_TEXT, 3)
		l.position = Vector2(46, 5 + k * 17)
		_cmd_panel.add_child(l)
		_cmd_items.append({"label": l, "action": cmds[k][2], "icon": ic})
	_cmd_panel.visible = false

	_sub_panel = _win(Rect2(160, 150, 320, 206))
	add_child(_sub_panel)
	for k in 8:
		var row := {}
		var ic := TextureRect.new()
		ic.position = Vector2(26, 9 + k * 23)
		ic.size = Vector2(16, 16)
		_sub_panel.add_child(ic)
		row["icon"] = ic
		var l := UIKit.label("", 14, C_TEXT, 3)
		l.position = Vector2(46, 8 + k * 23)
		l.size = Vector2(200, 18)
		l.clip_text = true
		_sub_panel.add_child(l)
		row["label"] = l
		var c := UIKit.label("", 14, Color(0.7, 0.82, 1.0), 3)
		c.position = Vector2(246, 8 + k * 23)
		c.size = Vector2(62, 18)
		c.horizontal_alignment = HORIZONTAL_ALIGNMENT_RIGHT
		_sub_panel.add_child(c)
		row["cost"] = c
		_sub_rows.append(row)
	_sub_panel.visible = false

	_hand = Sprite2D.new()
	_hand.texture = load("res://assets/ui/hand.png")
	_hand.z_index = 100
	_hand.visible = false
	add_child(_hand)
	for k in 5:
		var h := Sprite2D.new()
		h.texture = _hand.texture
		h.z_index = 100
		h.visible = false
		add_child(h)
		_hand2.append(h)
	_turn_arrow = Sprite2D.new()
	_turn_arrow.texture = load("res://assets/ui/turn_arrow.png")
	_turn_arrow.scale = Vector2(2, 2)
	_turn_arrow.z_index = 90
	_turn_arrow.visible = false
	add_child(_turn_arrow)

	_auto_label = UIKit.label("AUTO", 14, C_HI, 4)
	_auto_label.position = Vector2(590, 240)
	add_child(_auto_label)
	var tip := UIKit.label("Mayús: auto", 10, Color(0.85, 0.85, 0.95), 3)
	tip.position = Vector2(576, 226)
	add_child(tip)
	_refresh_ui()


# ------------------------------------------------------------ Bucle
func _intro() -> void:
	_busy = true
	var names := {}
	for e in enemies:
		names[e.display_name] = int(names.get(e.display_name, 0)) + 1
	var parts: Array = []
	for n in names:
		parts.append(n if names[n] == 1 else "%d × %s" % [names[n], n])
	await show_top(", ".join(parts), 1.1)
	_busy = false


func _process(delta: float) -> void:
	_t += delta
	for b in party + enemies:
		if is_instance_valid(b):
			b.tick_anim(delta)
	_auto_label.visible = GameState.auto_battle
	if Input.is_action_just_pressed("run") and not _over:
		GameState.auto_battle = not GameState.auto_battle
		Audio.sfx("select", -8.0)
		if GameState.auto_battle and state in ["command", "skill", "item", "target"]:
			_close_menus()
			_auto_act(actor)
	_update_hand()
	if _over:
		return
	match state:
		"idle":
			if not _busy:
				_tick_atb(delta)
		"command":
			_input_command()
		"skill", "item":
			_input_sub()
		"target":
			_input_target()


func _tick_atb(delta: float) -> void:
	for b in party + enemies:
		if not b.alive():
			continue
		if b.atb < 100:
			b.atb = minf(100.0, b.atb + delta * (20.0 + b.stat("spd") * 2.2) * ATB_RATE * Audio.battle_speed_mult())
			if b.atb >= 100 and not ready_queue.has(b):
				ready_queue.append(b)
	_refresh_atb()
	if ready_queue.is_empty():
		return
	var b = ready_queue.pop_front()
	if not is_instance_valid(b) or not b.alive():
		return
	_start_turn(b)


func _start_turn(b) -> void:
	_busy = true
	var skip := await _tick_statuses(b)
	if _check_end():
		return
	if skip or not b.alive():
		b.atb = 0
		_busy = false
		return
	if b.is_enemy:
		await _enemy_act(b)
		b.atb = 0
		_busy = false
		_check_end()
		return
	actor = b
	if GameState.auto_battle:
		await _auto_act(b)
		return
	_open_command(b)


func _finish_actor_turn() -> void:
	if actor:
		actor.atb = 0
		actor.set_idle_frame()
	actor = null
	state = "idle"
	_busy = false
	_refresh_ui()
	_check_end()


# ------------------------------------------------------------ Estados
func _tick_statuses(b) -> bool:
	var skip := false
	b.turns += 1
	# Los PM se recuperan poco a poco cada turno (los elfos, algo más).
	if not b.is_enemy and b.alive() and b.max_mp() > 0:
		var regen := maxi(1, int(round(b.max_mp() * 0.05)))
		if b.id == "player":
			regen += int(GameState.perk("mp_regen"))
		if b.id == "player" and GameState.race() == "elf":
			regen += 2
		if b.mp < b.max_mp():
			b.mp = mini(b.max_mp(), b.mp + regen)
	if b.statuses.has("poison"):
		var d := maxi(1, int(b.max_hp() * 0.07))
		b.hp = maxi(0, b.hp - d)
		_float(b, str(d), Color(0.6, 1, 0.4))
		_flash(b.sprite, Color(0.6, 1.6, 0.6))
		await _wait(0.35)
		if not b.alive():
			_on_death(b)
	if b.statuses.has("regen") and b.alive():
		var h := maxi(1, int(b.max_hp() * 0.08))
		b.hp = mini(b.max_hp(), b.hp + h)
		_float(b, "+%d" % h, Color(0.55, 1, 0.55))
		await _wait(0.3)
	if b.statuses.has("stun"):
		skip = true
		await show_top("%s está aturdido." % b.display_name, 0.7)
	for s in b.statuses.keys():
		b.statuses[s] = int(b.statuses[s]) - 1
		if b.statuses[s] <= 0:
			b.statuses.erase(s)
	_refresh_ui()
	return skip


# ------------------------------------------------------------ Menús
func _open_command(b) -> void:
	state = "command"
	_cmd_index = 0
	_cmd_panel.visible = true
	_enemy_panel.visible = false
	_ignore_until = Engine.get_process_frames() + 2
	for it in _cmd_items:
		var dis := false
		if it["action"] == "flee":
			dis = not bool(enc.get("can_flee", not enc.get("boss", false)))
		if it["action"] == "skill":
			dis = GameState.skills_of(b.id).is_empty()
		if it["action"] == "item":
			dis = _usable_items().is_empty()
		it["disabled"] = dis
	_highlight_row()
	Audio.sfx("select", -12.0)


func _close_menus() -> void:
	_cmd_panel.visible = false
	_sub_panel.visible = false
	_enemy_panel.visible = true
	_top_panel.visible = false


func _pressed(action: String) -> bool:
	return Engine.get_process_frames() > _ignore_until and Input.is_action_just_pressed(action)


func _nav() -> int:
	if _pressed("move_up") or _pressed("ui_up"):
		return -1
	if _pressed("move_down") or _pressed("ui_down"):
		return 1
	return 0


func _input_command() -> void:
	var n := _nav()
	if n != 0:
		_cmd_index = (_cmd_index + n + _cmd_items.size()) % _cmd_items.size()
		Audio.sfx("select", -14.0)
		_highlight_row()
	if _pressed("interact"):
		var it: Dictionary = _cmd_items[_cmd_index]
		if it.get("disabled", false):
			Audio.sfx("cancel", -8.0)
			return
		Audio.sfx("confirm", -10.0)
		match it["action"]:
			"attack":
				_pending_action = {"type": "attack"}
				_begin_target("enemy")
			"skill":
				_open_sub("skill")
			"item":
				_open_sub("item")
			"defend":
				_close_menus()
				_do_defend(actor)
			"flee":
				_close_menus()
				_do_flee()


func _highlight_row() -> void:
	for k in _cmd_items.size():
		var l: Label = _cmd_items[k]["label"]
		var dis: bool = _cmd_items[k].get("disabled", false)
		l.add_theme_color_override("font_color", C_DIS if dis else (C_HI if k == _cmd_index else C_TEXT))
		_cmd_items[k]["icon"].modulate = Color(1, 1, 1, 0.45) if dis else Color.WHITE
	_sel_bar.position = Vector2(9, 5 + _cmd_index * 17)


func _usable_items() -> Array:
	var out: Array = []
	for id in GameState.inventory:
		if DB.ITEMS.has(id) and GameState.item_count(id) > 0:
			out.append(id)
	out.sort()
	return out


func _open_sub(kind: String) -> void:
	_sub_kind = kind
	state = kind
	_sub_index = 0
	_sub_scroll = 0
	_sub_entries.clear()
	if kind == "skill":
		for s in GameState.skills_of(actor.id):
			_sub_entries.append(s)
	else:
		_sub_entries = _usable_items()
	_sub_panel.visible = true
	_ignore_until = Engine.get_process_frames() + 2
	_refresh_sub()


func _refresh_sub() -> void:
	for k in _sub_rows.size():
		var row: Dictionary = _sub_rows[k]
		var idx := _sub_scroll + k
		if idx >= _sub_entries.size():
			row["label"].text = ""
			row["cost"].text = ""
			row["icon"].texture = null
			continue
		var id: String = _sub_entries[idx]
		var color := C_TEXT
		if _sub_kind == "skill":
			var sk: Dictionary = DB.SKILLS[id]
			row["label"].text = str(sk["name"])
			row["cost"].text = "%d PM" % int(sk.get("mp", 0))
			var el: String = str(sk.get("el", ""))
			var ic := "el_" + el if el != "" else ("st_regen" if sk["kind"] in ["heal", "revive", "cure"] else ("cmd_defend" if sk["kind"] == "buff" else "cmd_attack"))
			row["icon"].texture = load("res://assets/ui/%s.png" % ic)
			if int(sk.get("mp", 0)) > actor.mp:
				color = C_DIS
		else:
			var it: Dictionary = DB.ITEMS[id]
			row["label"].text = str(it["name"])
			row["cost"].text = "×%d" % GameState.item_count(id)
			row["icon"].texture = load("res://assets/ui/%s.png" % it["icon"])
		row["label"].add_theme_color_override("font_color", C_HI if idx == _sub_index and color != C_DIS else color)
	if _sub_entries.size() > 0:
		var id2: String = _sub_entries[_sub_index]
		var desc: String = str(DB.SKILLS[id2]["desc"]) if _sub_kind == "skill" else str(DB.ITEMS[id2]["desc"])
		_top_label.text = desc
		_top_panel.visible = true


func _input_sub() -> void:
	var n := _nav()
	if n != 0 and _sub_entries.size() > 0:
		_sub_index = (_sub_index + n + _sub_entries.size()) % _sub_entries.size()
		if _sub_index < _sub_scroll:
			_sub_scroll = _sub_index
		if _sub_index >= _sub_scroll + _sub_rows.size():
			_sub_scroll = _sub_index - _sub_rows.size() + 1
		Audio.sfx("select", -14.0)
		_refresh_sub()
	if _pressed("cancel"):
		Audio.sfx("cancel", -10.0)
		_sub_panel.visible = false
		_top_panel.visible = false
		state = "command"
		return
	if _pressed("interact") and _sub_entries.size() > 0:
		var id: String = _sub_entries[_sub_index]
		if _sub_kind == "skill":
			var sk: Dictionary = DB.SKILLS[id]
			if int(sk.get("mp", 0)) > actor.mp:
				Audio.sfx("cancel", -8.0)
				return
			_pending_action = {"type": "skill", "id": id}
			_begin_target(str(sk["target"]))
		else:
			_pending_action = {"type": "item", "id": id}
			_begin_target(str(DB.ITEMS[id]["target"]))
		Audio.sfx("confirm", -10.0)


func _begin_target(mode: String) -> void:
	_target_mode = mode
	_target_list.clear()
	match mode:
		"enemy", "enemies", "enemies_random":
			for e in enemies:
				if e.alive():
					_target_list.append(e)
		"ally", "allies":
			for p in party:
				if p.alive():
					_target_list.append(p)
		"ally_dead":
			for p in party:
				if not p.alive():
					_target_list.append(p)
		"self":
			_target_list = [actor]
	if _target_list.is_empty():
		Audio.sfx("cancel", -8.0)
		return
	_target_index = 0
	if mode in ["ally", "ally_dead"]:
		var low := 2.0
		for k in _target_list.size():
			var r := float(_target_list[k].hp) / float(_target_list[k].max_hp())
			if r < low:
				low = r
				_target_index = k
	state = "target"
	_ignore_until = Engine.get_process_frames() + 2


func _input_target() -> void:
	var multi := _target_mode in ["enemies", "allies", "enemies_random", "self"]
	if not multi:
		var n := _nav()
		if _pressed("move_left") or _pressed("move_right"):
			n = 1
		if n != 0:
			_target_index = (_target_index + n + _target_list.size()) % _target_list.size()
			Audio.sfx("select", -14.0)
	if _pressed("cancel"):
		Audio.sfx("cancel", -10.0)
		state = "command" if _pending_action["type"] == "attack" else _sub_kind
		return
	if _pressed("interact"):
		var targets: Array = _target_list.duplicate() if multi else [_target_list[_target_index]]
		_close_menus()
		state = "busy"
		_execute(actor, _pending_action, targets)


func _update_hand() -> void:
	var show := state in ["command", "skill", "item", "target"]
	_hand.visible = show
	for h in _hand2:
		h.visible = false
	if not show:
		return
	var bob := sin(_t * 8.0) * 2.0
	match state:
		"command":
			_hand.position = _cmd_panel.position + Vector2(14 + bob, 13 + _cmd_index * 17)
			_hand.flip_h = false
		"skill", "item":
			_hand.position = _sub_panel.position + Vector2(14 + bob, 17 + (_sub_index - _sub_scroll) * 23)
			_hand.flip_h = false
		"target":
			var multi := _target_mode in ["enemies", "allies", "enemies_random"]
			var list: Array = _target_list if multi else [_target_list[_target_index]]
			for k in list.size():
				var h: Sprite2D = _hand if k == 0 else _hand2[min(k - 1, _hand2.size() - 1)]
				var b = list[k]
				h.visible = true
				if b.is_enemy:
					h.flip_h = false
					h.position = b.position + Vector2(-_half_w(b) - 10 + bob, 0)
				else:
					h.flip_h = true
					h.position = b.position + Vector2(30 - bob, 4)
			var tgt = list[0]
			_top_label.text = tgt.display_name if not multi else ("Todos los enemigos" if tgt.is_enemy else "Todo el grupo")
			_top_panel.visible = true


func _half_w(b) -> float:
	return b.sprite.texture.get_width() / float(b.sprite.hframes) * b.sprite.scale.x * 0.5


# ------------------------------------------------------------ Acciones
func _execute(user, action: Dictionary, targets: Array) -> void:
	match action["type"]:
		"attack":
			await _do_attack(user, targets[0])
		"skill":
			await _do_skill(user, str(action["id"]), targets)
		"item":
			await _do_item(user, str(action["id"]), targets)
	_finish_actor_turn()


func _do_attack(user, target) -> void:
	await _lunge(user)
	if target.alive():
		await _hit(user, target, {"kind": "phys", "power": 1.0}, "")
	await _return(user)


func _do_defend(b) -> void:
	state = "busy"
	b.statuses["protect"] = 1
	Audio.sfx("defend", -4.0)
	_flash(b.sprite, Color(1.5, 1.5, 3))
	await show_top("%s se defiende." % b.display_name, 0.6)
	_finish_actor_turn()


func _do_flee() -> void:
	state = "busy"
	var ps := 0.0
	var es := 0.0
	for p in party:
		ps += p.stat("spd")
	for e in enemies:
		if e.alive():
			es += e.stat("spd")
	ps /= max(1, party.size())
	es /= max(1, _alive(enemies).size())
	var chance := clampf(0.55 + (ps - es) * 0.02, 0.25, 0.9)
	if randf() < chance:
		Audio.sfx("flee", -2.0)
		await show_top("¡Escapáis!", 0.8)
		_over = true
		_write_back()
		_return_to_world()
		return
	await show_top("¡No podéis escapar!", 0.8)
	_finish_actor_turn()


func _do_skill(user, id: String, targets: Array) -> void:
	var sk: Dictionary = DB.SKILLS[id]
	user.mp = maxi(0, user.mp - int(sk.get("mp", 0)))
	_refresh_ui()
	await show_top(str(sk["name"]), 0.45)
	var kind: String = str(sk["kind"])
	var el: String = str(sk.get("el", ""))
	if kind in ["phys"] and user.is_enemy == false:
		await _lunge(user)
	elif kind == "phys" and user.is_enemy:
		await _lunge(user)
	else:
		_cast_pose(user)
		Audio.sfx(_el_sfx(el, kind), -4.0)
		await _wait(0.2)
	var hits: int = int(sk.get("hits", 1))
	if str(sk["target"]) == "enemies_random":
		for h in hits:
			var pool := _alive(enemies if not user.is_enemy else party)
			if pool.is_empty():
				break
			await _hit(user, pool.pick_random(), sk, el)
	else:
		for t in targets:
			if kind in ["phys", "mag", "drain"]:
				for h in hits:
					if t.alive():
						await _hit(user, t, sk, el)
			elif kind == "heal":
				if t.alive():
					_spell_fx(t, "heal")
					var amt := int(user.stat("mag") * float(sk["power"]) + float(sk.get("base", 0)))
					if not user.is_enemy and user.id == "player":
						amt = int(amt * (1.0 + GameState.perk("heal_pow")))
					_heal(t, amt)
					if sk.get("cure", false):
						_cure(t)
					for st in sk.get("add_status", []):
						t.statuses[st] = 4
			elif kind == "revive":
				if not t.alive():
					t.hp = maxi(1, int(t.max_hp() * float(sk["ratio"])))
					_apply_dead_look(t)
					_effect(t, Color(1, 1, 0.7))
					Audio.sfx("heal", -4.0)
					_float(t, "¡Revive!", Color(1, 1, 0.6))
			elif kind == "buff":
				for st in sk.get("status", []):
					t.statuses[st] = int(sk.get("turns", 3))
				_effect(t, Color(0.6, 0.8, 1) if not sk.get("status", []).has("rage") else Color(1, 0.5, 0.4))
				_float(t, DB.STATUS[sk["status"][0]]["name"], Color(0.8, 0.9, 1))
			elif kind == "status":
				await _try_inflict(t, str(sk["inflict"]), float(sk.get("chance", 0.5)))
			elif kind == "cure":
				_cure(t)
				_effect(t, Color(0.8, 1, 0.8))
				_float(t, "Purificado", Color(0.8, 1, 0.8))
			elif kind == "steal":
				await _steal(t)
			elif kind == "summon":
				await _summon(user)
			await _wait(0.08)
	for st in sk.get("self_status", []):
		user.statuses[st] = 3
	if sk.has("heal_party"):
		for p in (party if not user.is_enemy else enemies):
			if p.alive():
				_heal(p, int(user.stat("mag") * float(sk["heal_party"]) + 10))
	await _wait(0.25)
	if kind == "phys":
		await _return(user)
	user.set_idle_frame()
	_refresh_ui()


func _do_item(user, id: String, targets: Array) -> void:
	var it: Dictionary = DB.ITEMS[id]
	GameState.remove_item(id)
	await show_top(str(it["name"]), 0.45)
	_cast_pose(user)
	for t in targets:
		if it.has("revive"):
			if not t.alive():
				t.hp = maxi(1, int(t.max_hp() * float(it["revive"])))
				_apply_dead_look(t)
				_float(t, "¡Revive!", Color(1, 1, 0.6))
				Audio.sfx("heal", -4.0)
			continue
		if it.has("damage"):
			if t.alive():
				Audio.sfx("fire", -2.0)
				_spell_fx(t, "fire")
				var mult := _el_mult(t, str(it.get("el", "")))
				_damage(t, int(float(it["damage"]) * mult * randf_range(0.9, 1.1)), false, mult > 1.0)
			continue
		if not t.alive():
			continue
		if it.has("heal"):
			_heal(t, int(it["heal"]))
		if it.has("mp"):
			t.mp = mini(t.max_mp(), t.mp + int(it["mp"]))
			_float(t, "+%d PM" % int(it["mp"]), Color(0.6, 0.75, 1))
			Audio.sfx("heal", -6.0, 1.3)
		if it.get("cure", false):
			_cure(t)
	await _wait(0.4)
	user.set_idle_frame()
	_refresh_ui()


func _steal(t) -> void:
	if t.flags.get("stolen", false):
		await show_top("No le queda nada que robar.", 0.7)
		return
	if randf() < 0.65:
		t.flags["stolen"] = true
		var drops: Array = t.data.get("drops", [])
		if not drops.is_empty() and randf() < 0.6:
			var it: String = drops[0][0]
			GameState.add_item(it)
			await show_top("¡Robas %s!" % DB.ITEMS[it]["name"], 0.9)
		else:
			var g := int(t.data.get("gold", 10)) + randi_range(5, 20)
			GameState.gold += g
			await show_top("¡Robas %d coronas!" % g, 0.9)
		Audio.sfx("buy", -6.0)
	else:
		await show_top("Fallas el robo.", 0.7)


func _summon(user) -> void:
	var alive_n := _alive(enemies).size()
	if alive_n >= 4:
		return
	var slots := [Vector2(90, 110), Vector2(96, 230), Vector2(250, 240)]
	for k in 2:
		if _alive(enemies).size() >= 3:
			break
		var pos: Vector2 = slots[(user.turns + k) % slots.size()]
		var e := _spawn_enemy("larva", pos)
		e.sprite.modulate.a = 0.0
		var tw := create_tween()
		tw.tween_property(e.sprite, "modulate:a", 1.0, 0.4)
	Audio.sfx("dark", -4.0)
	await show_top("¡De la raíz brotan larvas!", 0.9)
	_refresh_ui()


func _el_sfx(el: String, kind: String) -> String:
	match el:
		"fire":
			return "fire"
		"ice":
			return "ice"
		"bolt":
			return "bolt"
		"dark":
			return "dark"
	if kind in ["heal", "revive", "cure"]:
		return "heal"
	return "magic"


func _el_mult(t, el: String) -> float:
	var weak: Array = t.data.get("weak", []) if t.is_enemy else []
	var resist: Array = t.data.get("resist", []) if t.is_enemy else []
	if el != "" and weak.has(el):
		return 1.5
	if el != "" and resist.has(el):
		return 0.5
	if el == "" and resist.has("phys"):
		return 0.5
	return 1.0


func _hit(user, target, sk: Dictionary, el: String) -> void:
	var kind: String = str(sk.get("kind", "phys"))
	var power: float = float(sk.get("power", 1.0))
	var dmg := 0.0
	if kind == "phys":
		dmg = user.stat("atk") * power * 1.25 - target.stat("def") * 0.6
	else:
		dmg = user.stat("mag") * power * 1.35 - target.stat("res") * 0.5
	if sk.has("vs_poison") and target.statuses.has("poison"):
		dmg *= float(sk["vs_poison"])
	var mult := _el_mult(target, el)
	dmg *= mult
	var crit := false
	if kind == "phys":
		var cc := 0.05 + float(sk.get("crit", 0))
		if not user.is_enemy and user.id == "player":
			if GameState.race() == "human":
				cc += 0.1
			cc += GameState.perk("crit")
		if randf() < cc:
			crit = true
			dmg *= 1.7
	if not target.is_enemy and target.id == "player" and GameState.race() == "dwarf" and kind == "phys":
		dmg *= 0.9
	if not user.is_enemy and user.id == "player":
		dmg *= 1.0 + GameState.perk("dmg_phys" if kind == "phys" else "dmg_mag")
	if not target.is_enemy and target.id == "player":
		dmg *= maxf(0.5, 1.0 - GameState.perk("dmg_red"))
	if target.statuses.has("wall"):
		dmg *= 0.5
	dmg *= randf_range(0.9, 1.1)
	var amount := maxi(1, int(dmg))
	if kind == "phys":
		Audio.sfx("crit" if crit else "hit", -2.0)
	else:
		_spell_fx(target, el)
	_damage(target, amount, crit, mult > 1.0)
	if kind == "drain":
		_heal(user, int(amount * 0.5))
	elif kind == "phys" and not user.is_enemy and user.id == "player" and GameState.perk("lifesteal") > 0:
		_heal(user, maxi(1, int(amount * GameState.perk("lifesteal"))))
	if sk.has("inflict") and target.alive():
		await _try_inflict(target, str(sk["inflict"]), float(sk.get("chance", 0.5)))
	await _wait(0.22)


func _try_inflict(t, st: String, chance: float) -> void:
	if not t.alive():
		return
	if st == "poison" and not t.is_enemy and t.id == "player" and GameState.race() == "dwarf":
		_float(t, "Inmune", Color(0.8, 0.8, 0.8))
		return
	if t.is_enemy and t.data.get("boss", false) and st == "stun":
		chance *= 0.3
	if randf() < chance:
		t.statuses[st] = 3 if st != "stun" else 1
		_float(t, DB.STATUS[st]["name"], Color(1, 0.9, 0.5))
		_refresh_ui()
	else:
		_float(t, "Resiste", Color(0.8, 0.8, 0.8))
	await _wait(0.15)


func _damage(t, amount: int, crit: bool, weak: bool) -> void:
	t.hp = maxi(0, t.hp - amount)
	if not t.is_enemy:
		t.pose(5, 0.4)
		var kb := create_tween()
		kb.tween_property(t.sprite, "position:x", 6.0, 0.05)
		kb.tween_property(t.sprite, "position:x", 0.0, 0.12)
	_flash(t.sprite, Color(6, 6, 6) if t.is_enemy else Color(3, 0.6, 0.6))
	_shake(6.0 if crit else 3.0)
	_float(t, str(amount), Color(1, 0.95, 0.55) if crit else (Color(1, 0.55, 0.5) if not t.is_enemy else Color.WHITE), crit)
	if weak:
		_float(t, "¡Débil!", Color(1, 0.7, 0.3), false, -22)
	if t.statuses.has("stun") and randf() < 0.3:
		t.statuses.erase("stun")
	_refresh_ui()
	if not t.alive():
		_on_death(t)


func _heal(t, amount: int) -> void:
	t.hp = mini(t.max_hp(), t.hp + amount)
	Audio.sfx("heal", -6.0)
	_flash(t.sprite, Color(1.5, 3, 1.5))
	_float(t, "+%d" % amount, Color(0.55, 1, 0.55))
	_refresh_ui()


func _cure(t) -> void:
	for st in ["poison", "stun", "weak"]:
		t.statuses.erase(st)
	_refresh_ui()


func _on_death(t) -> void:
	t.statuses.clear()
	t.atb = 0
	ready_queue.erase(t)
	if t.is_enemy:
		Audio.sfx("dark", -8.0, 1.4)
		var tw := create_tween().set_parallel(true)
		tw.tween_property(t.sprite, "modulate", Color(1, 0.4, 1, 0), 0.6)
		tw.tween_property(t.sprite, "scale", t.sprite.scale * Vector2(1.2, 0.6), 0.6)
		if t.hp_bar:
			t.hp_bar.visible = false
	else:
		_apply_dead_look(t)
	_refresh_ui()


func _apply_dead_look(b) -> void:
	if b.is_enemy:
		return
	b.sprite.rotation_degrees = 0
	b.sprite.position = Vector2.ZERO
	b.sprite.modulate = Color.WHITE if b.alive() else Color(0.75, 0.7, 0.8)
	b.set_idle_frame()


# ------------------------------------------------------------ IA
func _enemy_act(e) -> void:
	var d: Dictionary = e.data
	var ai: Array = d["ai"]
	var skill_id := ""
	var ratio := float(e.hp) / float(e.max_hp())
	match e.key:
		"wolf_alpha", "brute":
			if ratio < 0.5 and not e.flags.get("enraged", false):
				e.flags["enraged"] = true
				skill_id = "e_aullido" if e.key == "wolf_alpha" else "e_rugido"
				Audio.sfx("growl", -2.0)
				await show_top("¡%s se enfurece! Las venas negras palpitan..." % e.display_name, 1.0)
		"custodian":
			if ratio < 0.6 and not e.statuses.has("protect") and randf() < 0.5:
				skill_id = "e_escudo"
		"kaelen_dark":
			if ratio < 0.5 and not e.flags.get("taunt", false):
				e.flags["taunt"] = true
				await show_top("Kaelen: «¿Por qué siempre tú? ¡¿Por qué nunca yo?!»", 1.4)
		"mother_root":
			if ratio < 0.5 and not e.flags.get("phase2", false):
				e.flags["phase2"] = true
				e.max_stats["atk"] = int(e.max_stats["atk"]) + 6
				e.max_stats["mag"] = int(e.max_stats["mag"]) + 6
				_shake(8.0)
				Audio.sfx("growl", 0.0, 0.7)
				await show_top("¡La Madre Raíz despierta del todo! El suelo late...", 1.4)
				skill_id = "e_invocar"
			elif e.flags.get("phase2", false) and e.turns % 5 == 0 and _alive(enemies).size() < 2:
				skill_id = "e_invocar"
			elif e.turns % 5 == 0:
				skill_id = "e_regenerar"
	if skill_id == "":
		var total := 0
		for pair in ai:
			total += int(pair[1])
		var r := randi_range(1, total)
		for pair in ai:
			r -= int(pair[1])
			if r <= 0:
				skill_id = str(pair[0])
				break
	var sk: Dictionary = DB.SKILLS[skill_id]
	var targets: Array = []
	var alive_party := _alive(party)
	if alive_party.is_empty():
		return
	match str(sk["target"]):
		"enemy":
			var taunters: Array = alive_party.filter(func(p): return p.statuses.has("taunt"))
			if not taunters.is_empty() and randf() < 0.8:
				targets = [taunters.pick_random()]
			else:
				targets = [alive_party.pick_random()]
		"enemies", "enemies_random":
			targets = alive_party
		"self":
			targets = [e]
		_:
			targets = [e]
	await _do_skill(e, skill_id, targets)
	if not e.alive():
		pass


func _auto_act(b) -> void:
	state = "busy"
	var skills: Array = GameState.skills_of(b.id)
	var low = null
	var low_r := 1.0
	for p in _alive(party):
		var r := float(p.hp) / float(p.max_hp())
		if r < low_r:
			low_r = r
			low = p
	var dead := party.filter(func(p): return not p.alive())
	if not dead.is_empty() and GameState.item_count("pluma") > 0 and randf() < 0.7:
		await _execute(b, {"type": "item", "id": "pluma"}, [dead[0]])
		return
	for s in skills:
		var sk: Dictionary = DB.SKILLS[s]
		if int(sk.get("mp", 0)) > b.mp:
			continue
		if sk["kind"] == "revive" and not dead.is_empty():
			await _execute(b, {"type": "skill", "id": s}, [dead[0]])
			return
	if low != null and low_r < 0.45:
		for s in skills:
			var sk: Dictionary = DB.SKILLS[s]
			if sk["kind"] == "heal" and int(sk.get("mp", 0)) <= b.mp:
				await _execute(b, {"type": "skill", "id": s}, [low] if sk["target"] == "ally" else _alive(party))
				return
		if GameState.item_count("pocion") > 0 and low_r < 0.3:
			await _execute(b, {"type": "item", "id": "pocion"}, [low])
			return
	var foes := _alive(enemies)
	if foes.is_empty():
		_finish_actor_turn()
		return
	foes.sort_custom(func(a, c): return a.hp < c.hp)
	var best := ""
	var best_score := 0.0
	for s in skills:
		var sk: Dictionary = DB.SKILLS[s]
		if not sk["kind"] in ["phys", "mag", "drain"] or int(sk.get("mp", 0)) > b.mp:
			continue
		var score := float(sk.get("power", 1)) * int(sk.get("hits", 1))
		if str(sk["target"]).begins_with("enemies"):
			score *= foes.size() * 0.8
		var el: String = str(sk.get("el", ""))
		if el != "" and foes[0].data.get("weak", []).has(el):
			score *= 1.5
		if b.mp < b.max_mp() * 0.3:
			score *= 0.6
		if score > best_score and score > 1.2:
			best_score = score
			best = s
	if best != "" and randf() < 0.7:
		var sk2: Dictionary = DB.SKILLS[best]
		await _execute(b, {"type": "skill", "id": best}, foes if str(sk2["target"]).begins_with("enemies") else [foes[0]])
		return
	await _execute(b, {"type": "attack"}, [foes[0]])


# ------------------------------------------------------------ Efectos visuales
func show_top(text: String, hold: float) -> void:
	_top_label.text = text.replace("{name}", GameState.player_name())
	_top_panel.visible = true
	await _wait(hold)
	if state in ["idle", "busy"]:
		_top_panel.visible = false


func _wait(s: float) -> void:
	await get_tree().create_timer(s).timeout


func _float(b, text: String, col: Color, big: bool = false, dy: float = 0) -> void:
	var l := UIKit.label(text, 22 if big else 17, col, 5)
	l.position = b.position + Vector2(-40, -_sprite_h(b) * 0.5 - 8 + dy)
	l.size = Vector2(80, 24)
	l.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	l.z_index = 50
	l.pivot_offset = Vector2(40, 12)
	l.scale = Vector2(1.6, 1.6) if big else Vector2(1.25, 1.25)
	add_child(l)
	var y0 := l.position.y
	var tw := create_tween()
	tw.set_parallel(true)
	tw.tween_property(l, "scale", Vector2.ONE, 0.18).set_trans(Tween.TRANS_BACK).set_ease(Tween.EASE_OUT)
	tw.tween_property(l, "position:y", y0 - 14, 0.16).set_ease(Tween.EASE_OUT).set_trans(Tween.TRANS_QUAD)
	tw.chain().tween_property(l, "position:y", y0 - 6, 0.14).set_ease(Tween.EASE_IN).set_trans(Tween.TRANS_QUAD)
	tw.chain().tween_property(l, "position:y", y0 - 20, 0.5).set_delay(0.25)
	tw.parallel().tween_property(l, "modulate:a", 0.0, 0.35).set_delay(0.45)
	tw.chain().tween_callback(l.queue_free)


func _slash(pos: Vector2) -> void:
	var ln := Line2D.new()
	ln.width = 3
	ln.default_color = Color(1, 1, 0.9, 0.95)
	ln.points = PackedVector2Array([pos + Vector2(10, -16), pos + Vector2(-10, 14)])
	ln.z_index = 60
	add_child(ln)
	var tw := create_tween()
	tw.tween_property(ln, "modulate:a", 0.0, 0.18)
	tw.tween_callback(ln.queue_free)


func _sprite_h(b) -> float:
	return b.sprite.texture.get_height() / float(b.sprite.vframes) * b.sprite.scale.y


func _flash(node: CanvasItem, col: Color) -> void:
	var tw := create_tween()
	for k in 2:
		tw.tween_property(node, "self_modulate", col, 0.05)
		tw.tween_property(node, "self_modulate", Color.WHITE, 0.07)


func _shake(amount: float) -> void:
	var tw := create_tween()
	for k in 6:
		tw.tween_property(stage, "position", Vector2(randf_range(-amount, amount), randf_range(-amount, amount)), 0.03)
	tw.tween_property(stage, "position", Vector2.ZERO, 0.03)


func _effect(t, col: Color) -> void:
	_particles(t.position, col, 24, 14, Vector2(0, -40), 60)


func _particles(pos: Vector2, col: Color, amount: int, radius: float, gravity: Vector2, speed: float,
		life: float = 0.7, size: float = 3.0) -> void:
	var p := CPUParticles2D.new()
	p.position = pos
	p.amount = amount
	p.one_shot = true
	p.explosiveness = 0.85
	p.lifetime = life
	p.emission_shape = CPUParticles2D.EMISSION_SHAPE_SPHERE
	p.emission_sphere_radius = radius
	p.direction = Vector2.UP
	p.spread = 180
	p.gravity = gravity
	p.initial_velocity_min = speed * 0.3
	p.initial_velocity_max = speed
	p.scale_amount_min = size * 0.6
	p.scale_amount_max = size * 1.3
	var grad := Gradient.new()
	grad.set_color(0, col.lightened(0.4))
	grad.set_color(1, Color(col.r, col.g, col.b, 0.0))
	p.color_ramp = grad
	p.z_index = 40
	stage.add_child(p)
	p.emitting = true
	get_tree().create_timer(life + 0.6).timeout.connect(p.queue_free)


func _screen_flash(col: Color, t: float = 0.18) -> void:
	var r := ColorRect.new()
	r.color = col
	r.size = Vector2(640, 360)
	r.mouse_filter = Control.MOUSE_FILTER_IGNORE
	r.z_index = 80
	add_child(r)
	var tw := create_tween()
	tw.tween_property(r, "modulate:a", 0.0, t)
	tw.tween_callback(r.queue_free)


func _ring(pos: Vector2, col: Color, r0: float, r1: float, t: float = 0.35) -> void:
	var ln := Line2D.new()
	ln.width = 2
	ln.default_color = col
	ln.z_index = 45
	var pts := PackedVector2Array()
	for k in 25:
		var a := TAU * k / 24.0
		pts.append(Vector2(cos(a), sin(a) * 0.45))
	ln.points = pts
	ln.position = pos
	ln.scale = Vector2(r0, r0)
	stage.add_child(ln)
	var tw := create_tween().set_parallel(true)
	tw.tween_property(ln, "scale", Vector2(r1, r1), t).set_ease(Tween.EASE_OUT)
	tw.tween_property(ln, "modulate:a", 0.0, t)
	tw.chain().tween_callback(ln.queue_free)


## Efecto visual de hechizo según elemento.
func _spell_fx(t, el: String) -> void:
	var pos: Vector2 = t.position
	match el:
		"fire":
			_particles(pos + Vector2(0, 10), Color(1, 0.55, 0.15), 40, 16, Vector2(0, -120), 50, 0.7, 4)
			_particles(pos, Color(1, 0.9, 0.4), 16, 8, Vector2(0, -60), 30, 0.5, 3)
			_ring(pos + Vector2(0, 14), Color(1, 0.6, 0.2), 4, 36)
			_screen_flash(Color(1, 0.5, 0.1, 0.18))
		"ice":
			for k in 6:
				var shard := Polygon2D.new()
				shard.polygon = PackedVector2Array([Vector2(0, -7), Vector2(3, 0), Vector2(0, 7), Vector2(-3, 0)])
				shard.color = Color(0.75, 0.95, 1, 0.95)
				shard.z_index = 45
				var off := Vector2(randf_range(-22, 22), randf_range(-14, 14))
				shard.position = pos + off + Vector2(0, -70)
				stage.add_child(shard)
				var tw := create_tween()
				tw.tween_interval(k * 0.04)
				tw.tween_property(shard, "position", pos + off, 0.16).set_ease(Tween.EASE_IN)
				tw.tween_property(shard, "modulate:a", 0.0, 0.25)
				tw.tween_callback(shard.queue_free)
			_particles(pos, Color(0.7, 0.95, 1), 24, 18, Vector2(0, 30), 40, 0.8, 2)
			_screen_flash(Color(0.6, 0.9, 1, 0.15))
		"bolt":
			var ln := Line2D.new()
			ln.width = 3
			ln.default_color = Color(1, 1, 0.75)
			ln.z_index = 60
			var pts := PackedVector2Array()
			var y := -20.0
			while y < pos.y:
				pts.append(Vector2(pos.x + randf_range(-10, 10), y))
				y += randf_range(14, 24)
			pts.append(pos)
			ln.points = pts
			add_child(ln)
			var tw2 := create_tween()
			tw2.tween_property(ln, "modulate:a", 0.0, 0.25)
			tw2.tween_callback(ln.queue_free)
			_particles(pos, Color(1, 1, 0.6), 20, 6, Vector2.ZERO, 90, 0.4, 2)
			_screen_flash(Color(1, 1, 0.8, 0.35), 0.15)
		"light":
			var col := ColorRect.new()
			col.color = Color(1, 0.97, 0.75, 0.6)
			col.size = Vector2(30, pos.y + 30)
			col.position = Vector2(pos.x - 15, -10)
			col.z_index = 44
			add_child(col)
			var tw3 := create_tween().set_parallel(true)
			tw3.tween_property(col, "size:x", 4.0, 0.45)
			tw3.tween_property(col, "position:x", pos.x - 2, 0.45)
			tw3.tween_property(col, "modulate:a", 0.0, 0.45)
			tw3.chain().tween_callback(col.queue_free)
			_particles(pos, Color(1, 1, 0.8), 30, 16, Vector2(0, -30), 40, 0.8, 2)
		"dark":
			_particles(pos, Color(0.6, 0.25, 0.85), 40, 30, Vector2.ZERO, 10, 0.6, 3)
			_ring(pos, Color(0.7, 0.3, 0.9), 40, 4, 0.4)
			_screen_flash(Color(0.25, 0.05, 0.35, 0.25))
		"nature":
			_particles(pos + Vector2(0, 16), Color(0.5, 0.95, 0.4), 30, 20, Vector2(0, -50), 40, 0.9, 3)
			_ring(pos + Vector2(0, 16), Color(0.5, 0.9, 0.4), 6, 30)
		"heal":
			_particles(pos + Vector2(0, 20), Color(0.6, 1, 0.65), 28, 16, Vector2(0, -70), 20, 0.9, 2)
			_ring(pos + Vector2(0, 24), Color(0.6, 1, 0.6), 4, 26)
			_ring(pos + Vector2(0, 24), Color(1, 1, 0.8), 2, 18, 0.5)
		_:
			_particles(pos, Color(1, 1, 1), 18, 12, Vector2(0, -30), 50)


func _cast_pose(b) -> void:
	if not b.is_enemy:
		b.pose(4, 0.6)
		_effect(b, Color(1, 1, 0.8, 0.8))


func _lunge(b) -> void:
	var dir := 1.0 if b.is_enemy else -1.0
	var humanoid: bool = not b.is_enemy or b.frames == -1
	if humanoid:
		b.pose(2, 0.2)
	var tw := create_tween()
	tw.tween_property(b, "position", b.home + Vector2(dir * 44, 0), 0.14).set_trans(Tween.TRANS_QUAD)
	await tw.finished
	if humanoid:
		b.pose(3, 0.35)
		_slash(b.position + Vector2(dir * 34, 0))


func _return(b) -> void:
	var tw := create_tween()
	tw.tween_property(b, "position", b.home, 0.15)
	await tw.finished
	b.set_idle_frame()
	if not b.is_enemy:
		_apply_dead_look(b)


func _alive(list: Array) -> Array:
	return list.filter(func(b): return is_instance_valid(b) and b.alive())


# ------------------------------------------------------------ UI
func _refresh_ui() -> void:
	var alive_e := _alive(enemies)
	var counted := {}
	for e in alive_e:
		counted[e.display_name] = int(counted.get(e.display_name, 0)) + 1
	var names: Array = counted.keys()
	for k in _enemy_labels.size():
		var l: Label = _enemy_labels[k]
		if k < names.size():
			var n: String = names[k]
			l.text = n if counted[n] == 1 else "%s  ×%d" % [n, counted[n]]
		else:
			l.text = ""
	for b in party:
		var row: Dictionary = _rows[b.id]
		row["hp"].text = "%d/%d" % [b.hp, b.max_hp()]
		var r := float(b.hp) / float(b.max_hp())
		row["hp"].add_theme_color_override("font_color", Color(1, 0.45, 0.4) if r <= 0.25 else (Color(1, 0.88, 0.45) if r <= 0.5 else C_TEXT))
		_tween_bar(row["hp_bar"], b.hp, b.max_hp())
		row["hp_bar"].get_meta("fill").bg_color = _hp_color(r)
		row["mp"].text = "%d/%d" % [b.mp, b.max_mp()]
		_tween_bar(row["mp_bar"], b.mp, max(1, b.max_mp()))
		row["name"].add_theme_color_override("font_color", C_HI if b == actor else (C_DIS if not b.alive() else C_TEXT))
		row["portrait"].modulate = Color(0.45, 0.45, 0.55) if not b.alive() else Color.WHITE
		if b.hp_bar:
			_tween_bar(b.hp_bar, b.hp, b.max_hp())
			b.hp_bar.get_meta("fill").bg_color = _hp_color(r)
			b.hp_bar.visible = b.alive()
		var icons: HBoxContainer = row["icons"]
		for c in icons.get_children():
			c.queue_free()
		for st in b.statuses:
			var ic := TextureRect.new()
			ic.texture = load("res://assets/ui/%s.png" % DB.STATUS[st]["icon"])
			ic.custom_minimum_size = Vector2(12, 12)
			ic.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
			ic.stretch_mode = TextureRect.STRETCH_SCALE
			icons.add_child(ic)
		b.set_idle_frame()
	for e in enemies:
		if e.hp_bar:
			_tween_bar(e.hp_bar, e.hp, e.max_hp())
	_refresh_atb()


func _tween_bar(bar: ProgressBar, value: float, maxv: float) -> void:
	bar.max_value = maxv
	if absf(bar.value - value) < 0.5:
		return
	var tw := bar.create_tween()
	tw.tween_property(bar, "value", value, 0.35).set_trans(Tween.TRANS_QUAD).set_ease(Tween.EASE_OUT)


func _refresh_atb() -> void:
	for b in party:
		var row: Dictionary = _rows[b.id]
		row["atb"].value = b.atb if b.alive() else 0
		var full: bool = b.atb >= 100 and b.alive()
		row["atb_fill"].bg_color = Color(1, 0.97, 0.6).lerp(Color(1, 0.75, 0.3), 0.5 + 0.5 * sin(_t * 10.0)) if full else Color(0.85, 0.68, 0.22)
	if actor != null and is_instance_valid(actor) and state in ["command", "skill", "item", "target"]:
		_turn_arrow.visible = true
		_turn_arrow.position = actor.position + Vector2(0, -42 + sin(_t * 6.0) * 2.0)
	else:
		_turn_arrow.visible = false


# ------------------------------------------------------------ Fin
func _check_end() -> bool:
	if _over:
		return true
	if _alive(enemies).is_empty():
		_over = true
		_victory.call_deferred()
		return true
	if _alive(party).is_empty():
		_over = true
		_defeat.call_deferred()
		return true
	return false


func _write_back() -> void:
	for b in party:
		GameState.members[b.id]["hp"] = maxi(b.hp, 1) if _over and not _alive(party).is_empty() else b.hp
		GameState.members[b.id]["mp"] = b.mp


func _victory() -> void:
	_close_menus()
	state = "end"
	await _wait(0.6)
	Audio.play_music("victory", 0.2)
	var xp_total := 0
	var gold_total := 0
	var drops: Array = []
	for e in enemies:
		xp_total += int(e.data.get("xp", 0) * GameState.diff_mults()[2])
		gold_total += int(e.data.get("gold", 0) * GameState.diff_mults()[2])
		for dpair in e.data.get("drops", []):
			if randf() < float(dpair[1]):
				drops.append(dpair[0])
	for b in party:
		if not b.alive():
			b.hp = 1
			_apply_dead_look(b)
		b.victory = true
		b.set_idle_frame()
	_write_back()
	GameState.gold += gold_total
	for d in drops:
		GameState.add_item(d)
	var panel := UIKit.panel(Rect2(170, 70, 300, 150))
	add_child(panel)
	var title := UIKit.label("¡Victoria!", 24, Color(0.45, 0.2, 0.1))
	title.position = Vector2(0, 10)
	title.size = Vector2(300, 30)
	title.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	panel.add_child(title)
	var txt := "Experiencia: +%d\nCoronas: +%d" % [xp_total, gold_total]
	for d in drops:
		txt += "\nObtienes: %s" % DB.item(d)["name"]
	var info := UIKit.label("", 15)
	info.position = Vector2(24, 44)
	info.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	info.size = Vector2(252, 96)
	info.text = txt
	panel.add_child(info)
	await _wait(1.6)
	var results := GameState.add_xp(xp_total)
	for r in results:
		Audio.sfx("levelup", -2.0)
		var lines := "¡Nivel %d! Recuperáis parte de las fuerzas." % r["level"]
		for l in r["learned"]:
			lines += "\n%s aprende %s." % [GameState.member_name(l[0]), DB.SKILLS[l[1]]["name"]]
		info.text = lines
		for b in party:
			b.max_stats = GameState.stats(b.id)
			b.hp = int(GameState.members[b.id]["hp"])
			b.mp = int(GameState.members[b.id]["mp"])
			_flash(b.sprite, Color(2.5, 2.5, 1.2))
		_refresh_ui()
		await _wait(2.0)
	panel.queue_free()
	if GameState.needs_branch_choice() or GameState.needs_class_choice():
		var opts: Array = []
		for bid in ["melee", "ranged", "support"]:
			var bd: Dictionary = DB.BRANCHES[bid]
			var names: Array = []
			for cid in bd["classes"]:
				names.append(DB.CLASSES[cid]["name"])
			opts.append({"id": bid, "name": bd["name"], "desc": "%s\nClases: %s" % [bd["desc"], ", ".join(names)], "icon": bd["icon"]})
		var idx: int = await choose_panel("Elige tu senda", "Cómo vas a luchar. Luego elegirás tu clase.", opts)
		var branch: String = opts[idx]["id"]
		var opts2: Array = []
		for cid in DB.BRANCHES[branch]["classes"]:
			var cd: Dictionary = DB.CLASSES[cid]
			opts2.append({"id": cid, "name": cd["name"], "desc": cd["desc"], "icon": cd["icon"]})
		var idx2: int = await choose_panel("Elige tu clase", "Cada clase tiene su árbol de talentos.", opts2)
		GameState.choose_class(opts2[idx2]["id"])
		Audio.sfx("levelup", -4.0)
	if GameState.skill_points() > 0:
		await show_top("Tienes %d punto(s) de talento. Ábrelos en el menú (Esc) > Talentos." % GameState.skill_points(), 1.8)
	var after := str(enc.get("id", ""))
	if after != "":
		GameState.set_flag("won_" + after)
	_return_to_world()


func choose_panel(title_text: String, sub_text: String, opts: Array) -> int:
	var root := Control.new()
	root.set_anchors_preset(Control.PRESET_FULL_RECT)
	add_child(root)
	var dim := ColorRect.new()
	dim.color = Color(0, 0, 0, 0.6)
	dim.set_anchors_preset(Control.PRESET_FULL_RECT)
	root.add_child(dim)
	var h := 120 + opts.size() * 32
	var panel := UIKit.panel(Rect2(110, 180 - h / 2, 420, h))
	root.add_child(panel)
	var t := UIKit.label(title_text, 22, Color(0.35, 0.18, 0.1))
	t.position = Vector2(0, 10)
	t.size = Vector2(420, 30)
	t.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	panel.add_child(t)
	var s := UIKit.label(sub_text, 13)
	s.position = Vector2(20, 40)
	s.size = Vector2(380, 18)
	s.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	panel.add_child(s)
	var desc := UIKit.label("", 14)
	desc.position = Vector2(24, h - 50)
	desc.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	desc.size = Vector2(372, 44)
	desc.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	panel.add_child(desc)
	var box := VBoxContainer.new()
	box.position = Vector2(100, 64)
	box.size = Vector2(220, opts.size() * 32)
	box.add_theme_constant_override("separation", 4)
	panel.add_child(box)
	var buttons: Array = []
	for k in opts.size():
		var o: Dictionary = opts[k]
		var b := UIKit.button(str(o["name"]))
		b.icon = load("res://assets/ui/%s.png" % o["icon"])
		b.alignment = HORIZONTAL_ALIGNMENT_LEFT
		b.custom_minimum_size = Vector2(220, 28)
		b.focus_entered.connect(func(): desc.text = str(o["desc"]))
		b.pressed.connect(func(): _chooser_done.emit(k))
		box.add_child(b)
		buttons.append(b)
	buttons[0].grab_focus()
	desc.text = str(opts[0]["desc"])
	var idx: int = await _chooser_done
	root.queue_free()
	return idx


func _defeat() -> void:
	_close_menus()
	state = "end"
	Audio.stop_music(0.8)
	await show_top("Todo se vuelve negro...", 1.6)
	var root := Control.new()
	root.set_anchors_preset(Control.PRESET_FULL_RECT)
	add_child(root)
	var dim := ColorRect.new()
	dim.color = Color(0, 0, 0, 0.7)
	dim.set_anchors_preset(Control.PRESET_FULL_RECT)
	root.add_child(dim)
	var panel := UIKit.panel(Rect2(200, 110, 240, 150))
	root.add_child(panel)
	var l := UIKit.label("Habéis caído", 20, Color(0.45, 0.15, 0.12))
	l.position = Vector2(0, 12)
	l.size = Vector2(240, 26)
	l.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	panel.add_child(l)
	var box := VBoxContainer.new()
	box.position = Vector2(40, 48)
	box.size = Vector2(160, 90)
	box.add_theme_constant_override("separation", 5)
	panel.add_child(box)
	var retry := UIKit.button("Reintentar el combate", 14)
	retry.pressed.connect(func():
		GameState.members = _start_snapshot.duplicate(true)
		GameState.heal_all()
		Transition.go_to_scene("res://scenes/Battle.tscn", 0.4))
	box.add_child(retry)
	var slot := GameState.any_save()
	var load_b := UIKit.button("Cargar partida", 14)
	load_b.disabled = slot < 0
	load_b.pressed.connect(func():
		if GameState.load_game(slot):
			Transition.go_to_scene("res://scenes/World.tscn", 0.5))
	box.add_child(load_b)
	var title := UIKit.button("Volver al título", 14)
	title.pressed.connect(func(): Transition.go_to_scene("res://scenes/Title.tscn", 0.5))
	box.add_child(title)
	retry.grab_focus()


func _return_to_world() -> void:
	var ret: Dictionary = GameState.battle_return
	GameState.current_map = str(ret.get("map", GameState.current_map))
	if ret.has("pos"):
		var p: Array = ret["pos"]
		GameState.load_pos = Vector2(p[0], p[1])
		GameState.current_spawn = ""
	else:
		GameState.current_spawn = str(ret.get("spawn", GameState.current_spawn))
	Transition.go_to_scene("res://scenes/World.tscn", 0.5)
