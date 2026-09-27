extends Control
## Equipo del grupo: arma, escudo, cabeza, cuerpo y anillo para cada miembro,
## con vista previa del personaje y comparación de estadísticas.

signal closed

const SLOTS := ["weapon", "shield", "head", "body", "ring"]
const STAT_ORDER := ["hp", "mp", "atk", "def", "mag", "res", "spd"]

var _member_idx := 0
var _panel: NinePatchRect
var _title: Label
var _preview: TextureRect
var _stat_labels := {}
var _list_box: VBoxContainer
var _desc: Label
var _mode := "slots"      # slots | items
var _slot := ""
var _ignore := 0


func _ready() -> void:
	process_mode = Node.PROCESS_MODE_ALWAYS
	set_anchors_preset(Control.PRESET_FULL_RECT)
	_panel = UIKit.panel(Rect2(170, 20, 458, 326))
	add_child(_panel)
	_title = UIKit.label("", 17, Color(0.35, 0.18, 0.1))
	_title.position = Vector2(12, 8)
	_title.size = Vector2(434, 22)
	_title.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	_panel.add_child(_title)
	var frame := UIKit.panel(Rect2(12, 34, 140, 110), true)
	_panel.add_child(frame)
	_preview = TextureRect.new()
	_preview.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
	_preview.stretch_mode = TextureRect.STRETCH_KEEP_ASPECT_CENTERED
	_preview.texture_filter = CanvasItem.TEXTURE_FILTER_NEAREST
	_preview.position = Vector2(14, 4)
	_preview.size = Vector2(112, 102)
	frame.add_child(_preview)
	for i in STAT_ORDER.size():
		var k: String = STAT_ORDER[i]
		var l := UIKit.label("", 13)
		l.position = Vector2(16, 150 + i * 15)
		l.size = Vector2(136, 16)
		_panel.add_child(l)
		_stat_labels[k] = l
	_list_box = VBoxContainer.new()
	_list_box.position = Vector2(162, 36)
	_list_box.size = Vector2(284, 220)
	_list_box.add_theme_constant_override("separation", 3)
	_panel.add_child(_list_box)
	var desc_bg := ColorRect.new()
	desc_bg.color = Color(0.3, 0.2, 0.12, 0.12)
	desc_bg.position = Vector2(160, 214)
	desc_bg.size = Vector2(288, 104)
	_panel.add_child(desc_bg)
	_desc = UIKit.label("", 11, Color(0.35, 0.25, 0.2))
	_desc.position = Vector2(164, 216)
	_desc.size = Vector2(280, 100)
	_desc.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	_desc.clip_text = true
	_desc.text_overrun_behavior = TextServer.OVERRUN_TRIM_ELLIPSIS
	_panel.add_child(_desc)
	get_viewport().gui_release_focus()
	_ignore = Engine.get_process_frames() + 2
	_show_slots()


func _member() -> String:
	return GameState.party[_member_idx]


func _clear_list() -> void:
	for c in _list_box.get_children():
		c.queue_free()


func _refresh_header() -> void:
	var id := _member()
	_title.text = "<  %s · %s  >" % [GameState.member_name(id), GameState.member_role(id)]
	var tex: Texture2D = Appearance.member_battle_sheet(id)
	var at := AtlasTexture.new()
	at.atlas = tex
	at.region = Rect2(0, 0, 32, 32)
	_preview.texture = at


func _show_stats(preview: Dictionary = {}) -> void:
	var cur := GameState.stats(_member())
	for k in STAT_ORDER:
		var l: Label = _stat_labels[k]
		var txt := "%s  %d" % [DB.STAT_NAMES[k], int(cur[k])]
		var col := UIKit.TEXT_DARK
		if not preview.is_empty():
			var d: int = int(preview[k]) - int(cur[k])
			if d != 0:
				txt += "  → %d" % int(preview[k])
				col = Color(0.15, 0.5, 0.15) if d > 0 else Color(0.65, 0.15, 0.1)
		l.text = txt
		l.add_theme_color_override("font_color", col)


func _show_slots() -> void:
	_mode = "slots"
	_refresh_header()
	_show_stats()
	_clear_list()
	var id := _member()
	var first: Button = null
	for slot in SLOTS:
		var eid := GameState.equipped(id, slot)
		var b := UIKit.button("%s:  %s" % [DB.SLOT_NAMES[slot], DB.EQUIP[eid]["name"] if eid != "" else "—"], 13)
		b.alignment = HORIZONTAL_ALIGNMENT_LEFT
		b.custom_minimum_size = Vector2(284, 26)
		b.clip_text = true
		if eid != "":
			b.icon = load("res://assets/ui/%s.png" % DB.equip_icon(eid))
			_rarity_color(b, eid)
		b.focus_entered.connect(func():
			_desc.text = (DB.item(eid)["desc"] if eid != "" else "Hueco vacío.") + "\nIzq./Der.: cambiar de personaje.")
		b.pressed.connect(_show_items.bind(slot))
		_list_box.add_child(b)
		if first == null:
			first = b
	var auto := UIKit.button("Equipar lo mejor", 13)
	auto.custom_minimum_size = Vector2(284, 26)
	auto.focus_entered.connect(func(): _desc.text = "Equipa automáticamente lo que más estadísticas dé.")
	auto.pressed.connect(_auto_equip)
	_list_box.add_child(auto)
	(func(): first.grab_focus()).call_deferred()


func _rarity_color(b: Button, eid: String) -> void:
	var rz: String = str(DB.EQUIP[eid].get("rarity", "basic"))
	var col: Color = DB.RARITY[rz]["color"].lightened(0.5)
	if rz == "basic":
		col = UIKit.TEXT_LIGHT
	for k in ["font_color", "font_focus_color", "font_hover_color"]:
		b.add_theme_color_override(k, col)


func _candidates(slot: String) -> Array:
	var out: Array = []
	var rule := GameState.equip_rule(_member())
	for eid in GameState.inventory.keys():
		if DB.EQUIP.has(eid) and str(DB.EQUIP[eid]["slot"]) == slot and GameState.item_count(eid) > 0:
			if DB.equip_allowed(rule, eid):
				out.append(eid)
	out.sort_custom(func(a, b): return _score(a) > _score(b))
	return out


func _score(eid: String) -> int:
	var t := 0
	for k in DB.EQUIP[eid]["stats"]:
		t += int(DB.EQUIP[eid]["stats"][k]) * (1 if k in ["hp", "mp"] else 4)
	return t


func _preview_stats(slot: String, eid: String) -> Dictionary:
	var id := _member()
	var st := GameState.stats(id)
	var cur := GameState.equipped(id, slot)
	var out := st.duplicate()
	if cur != "":
		for k in DB.EQUIP[cur]["stats"]:
			out[k] -= int(DB.EQUIP[cur]["stats"][k])
	if eid != "":
		for k in DB.EQUIP[eid]["stats"]:
			out[k] += int(DB.EQUIP[eid]["stats"][k])
	return out


func _show_items(slot: String) -> void:
	_mode = "items"
	_slot = slot
	_clear_list()
	var cands := _candidates(slot)
	var first: Button = null
	var remove := UIKit.button("(Quitar)", 13)
	remove.custom_minimum_size = Vector2(284, 22)
	remove.focus_entered.connect(func():
		_show_stats(_preview_stats(slot, ""))
		_desc.text = "Deja el hueco vacío.")
	remove.pressed.connect(func(): _do_equip(""))
	_list_box.add_child(remove)
	first = remove
	for eid in cands.slice(0, 5):
		var need: int = int(DB.EQUIP[eid].get("lvl", 1))
		var lvl_txt := "" if GameState.level >= need else "  (Nv %d)" % need
		var b := UIKit.button("%s ×%d%s" % [DB.EQUIP[eid]["name"], GameState.item_count(eid), lvl_txt], 13)
		b.alignment = HORIZONTAL_ALIGNMENT_LEFT
		b.custom_minimum_size = Vector2(284, 22)
		b.clip_text = true
		_rarity_color(b, eid)
		b.disabled = GameState.level < need
		b.icon = load("res://assets/ui/%s.png" % DB.equip_icon(eid))
		b.focus_entered.connect(func():
			_show_stats(_preview_stats(slot, eid))
			_desc.text = DB.item(eid)["desc"])
		b.pressed.connect(_do_equip.bind(eid))
		_list_box.add_child(b)
		if first == remove:
			first = b
	if cands.size() > 5:
		_desc.text = "(Se muestran las 5 mejores piezas de %d.)" % cands.size()
	if cands.is_empty():
		_desc.text = "No tienes nada que %s pueda llevar ahí. Busca en tiendas y cofres." % GameState.member_name(_member())
	(func(): first.grab_focus()).call_deferred()


func _do_equip(eid: String) -> void:
	GameState.equip(_member(), _slot, eid)
	Audio.sfx("defend" if eid != "" else "cancel", -8.0)
	_show_slots()


func _auto_equip() -> void:
	var id := _member()
	for slot in SLOTS:
		var cands := _candidates(slot)
		if cands.is_empty():
			continue
		var cur := GameState.equipped(id, slot)
		var best := ""
		for c in cands:
			if int(DB.EQUIP[c].get("lvl", 1)) <= GameState.level:
				best = c
				break
		if best != "" and (cur == "" or _score(best) > _score(cur)):
			GameState.equip(id, slot, best)
	Audio.sfx("defend", -8.0)
	_show_slots()


func _process(_d: float) -> void:
	if Engine.get_process_frames() <= _ignore:
		return
	if Input.is_action_just_pressed("ui_cancel"):
		Audio.sfx("cancel", -8.0)
		if _mode == "items":
			_show_slots()
		else:
			closed.emit()
			queue_free()
		return
	if _mode == "slots" and GameState.party.size() > 1:
		var d := 0
		if Input.is_action_just_pressed("ui_left"):
			d = -1
		elif Input.is_action_just_pressed("ui_right"):
			d = 1
		if d != 0:
			_member_idx = (_member_idx + d + GameState.party.size()) % GameState.party.size()
			Audio.sfx("select", -10.0)
			_show_slots()
