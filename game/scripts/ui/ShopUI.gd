extends Control
## Tienda: compra de objetos y mejoras de equipo.

signal closed

var shop_id := "roc"
var _buttons: Array = []
var _gold: Label
var _desc: Label
var _stock: Array = []


func setup(id: String) -> void:
	shop_id = id


func _ready() -> void:
	process_mode = Node.PROCESS_MODE_ALWAYS
	set_anchors_preset(Control.PRESET_FULL_RECT)
	var dim := ColorRect.new()
	dim.color = Color(0, 0, 0, 0.55)
	dim.set_anchors_preset(Control.PRESET_FULL_RECT)
	add_child(dim)
	var shop: Dictionary = DB.SHOPS[shop_id]
	_stock = shop["stock"]
	var panel := UIKit.panel(Rect2(80, 14, 480, 332))
	add_child(panel)
	var t := UIKit.label(str(shop["name"]), 20, Color(0.35, 0.18, 0.1))
	t.position = Vector2(0, 10)
	t.size = Vector2(480, 28)
	t.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	panel.add_child(t)
	_gold = UIKit.label("", 15, Color(0.55, 0.4, 0.1))
	_gold.position = Vector2(330, 40)
	_gold.size = Vector2(130, 20)
	_gold.horizontal_alignment = HORIZONTAL_ALIGNMENT_RIGHT
	panel.add_child(_gold)
	var scroll := ScrollContainer.new()
	scroll.position = Vector2(24, 62)
	scroll.size = Vector2(440, 224)
	scroll.follow_focus = true
	scroll.horizontal_scroll_mode = ScrollContainer.SCROLL_MODE_DISABLED
	panel.add_child(scroll)
	var box := VBoxContainer.new()
	box.custom_minimum_size = Vector2(426, 0)
	box.add_theme_constant_override("separation", 2)
	scroll.add_child(box)
	for id in _stock:
		var b := UIKit.button("", 13)
		b.custom_minimum_size = Vector2(426, 22)
		b.clip_text = true
		b.alignment = HORIZONTAL_ALIGNMENT_LEFT
		var it: Dictionary = DB.item(id)
		if it.has("icon"):
			b.icon = load("res://assets/ui/%s.png" % it["icon"])
		b.pressed.connect(_buy.bind(id))
		b.focus_entered.connect(func(): _desc.text = str(it["desc"]))
		box.add_child(b)
		_buttons.append(b)
	var close := UIKit.button("Salir de la tienda", 13)
	close.custom_minimum_size = Vector2(160, 24)
	close.pressed.connect(_close)
	box.add_child(close)
	_desc = UIKit.label("", 11)
	_desc.position = Vector2(24, 292)
	_desc.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	_desc.size = Vector2(432, 36)
	_desc.clip_text = true
	_desc.text_overrun_behavior = TextServer.OVERRUN_TRIM_ELLIPSIS
	panel.add_child(_desc)
	_refresh()
	_buttons[0].grab_focus()


func _owned(id: String) -> bool:
	if not DB.UPGRADES.has(id):
		return false
	var u: Dictionary = DB.UPGRADES[id]
	var cur: int = GameState.weapon_tier if u["kind"] == "weapon" else GameState.armor_tier
	return cur >= int(u["tier"])


func _refresh() -> void:
	_gold.text = "%d coronas" % GameState.gold
	for i in _stock.size():
		var id: String = _stock[i]
		var it: Dictionary = DB.item(id)
		var b: Button = _buttons[i]
		if _owned(id):
			b.text = "%s   (ya lo tienes)" % it["name"]
			b.disabled = true
		else:
			var have := "" if DB.UPGRADES.has(id) else "   · tienes %d" % GameState.item_count(id)
			if DB.EQUIP.has(id):
				var users: Array = []
				for m in GameState.party:
					if DB.equip_allowed(GameState.equip_rule(m), id):
						users.append(GameState.member_name(m))
				have += "   · " + (", ".join(users) if not users.is_empty() else "nadie puede usarlo")
			b.text = "%s   —   %d coronas%s" % [it["name"], int(it["price"]), have]
			b.disabled = false


func _buy(id: String) -> void:
	var it: Dictionary = DB.item(id)
	if GameState.gold < int(it["price"]):
		Audio.sfx("cancel", -6.0)
		_desc.text = "No tienes suficientes coronas."
		return
	GameState.gold -= int(it["price"])
	if DB.UPGRADES.has(id):
		if it["kind"] == "weapon":
			GameState.weapon_tier = int(it["tier"])
		else:
			GameState.armor_tier = int(it["tier"])
		_desc.text = "¡Equipo mejorado!"
	else:
		GameState.add_item(id)
		_desc.text = "Compras %s." % it["name"]
	Audio.sfx("buy", -4.0)
	_refresh()


func _process(_d: float) -> void:
	if Input.is_action_just_pressed("cancel"):
		_close()


func _close() -> void:
	closed.emit()
	queue_free()
