extends Node
## Compone el sprite y el retrato del jugador a partir de las capas en
## grises del creador, tiñéndolas con los colores elegidos.

const SKIN_LEVELS := {228: 0.12, 200: 0.0, 184: -1.0, 160: -0.18}
const HAIR_LEVELS := {236: 0.3, 170: 0.0, 110: -0.3}

var _cache := {}


func clear_cache() -> void:
	_cache.clear()


func _shade(c: Color, t: float) -> Color:
	if t > 0:
		return c.lerp(Color(1, 0.98, 0.92), t)
	return c.lerp(Color(0.07, 0.055, 0.086), -t)


func _skin_map(skin: Color) -> Dictionary:
	return {228: _shade(skin, 0.12), 200: skin, 184: skin.lerp(Color8(230, 120, 120), 0.45), 160: _shade(skin, -0.18)}


func _hair_map(hair: Color) -> Dictionary:
	return {236: _shade(hair, 0.3), 170: hair, 110: _shade(hair, -0.3)}


func _recolor(img: Image, gray_map: Dictionary, outfit: Color, use_outfit: bool) -> void:
	var od := _shade(outfit, -0.3)
	var ol := _shade(outfit, 0.2)
	for y in img.get_height():
		for x in img.get_width():
			var c := img.get_pixel(x, y)
			if c.a == 0.0:
				continue
			var r := c.r8
			if r == c.g8 and r == c.b8:
				if gray_map.has(r):
					var n: Color = gray_map[r]
					img.set_pixel(x, y, Color(n.r, n.g, n.b, c.a))
			elif use_outfit and c.r8 == 255 and c.g8 == 0 and c.b8 == 255:
				img.set_pixel(x, y, Color(outfit.r, outfit.g, outfit.b, c.a))
			elif use_outfit and c.r8 == 170 and c.g8 == 0 and c.b8 == 170:
				img.set_pixel(x, y, Color(od.r, od.g, od.b, c.a))
			elif use_outfit and c.r8 == 255 and c.g8 == 120 and c.b8 == 255:
				img.set_pixel(x, y, Color(ol.r, ol.g, ol.b, c.a))


func _compose(prefix: String, app: Dictionary, gear: Dictionary = {}) -> ImageTexture:
	var race: String = str(app.get("race", "human"))
	var sex: String = str(app.get("sex", "m"))
	var style: String = GameState.HAIR_STYLES[int(app.get("hair_style", 0))][0]
	var look0: Dictionary = gear if not gear.is_empty() else GameState.gear_look("player")
	if str(look0.get("head", "")) in ["helm", "helm_gold", "cap", "hood"] and style in ["spiky", "bun"]:
		style = "short"
	var skin: Color = GameState.SKIN_TONES[int(app.get("skin", 1))][1]
	var hair: Color = GameState.HAIR_COLORS[int(app.get("hair_color", 0))][1]
	var outfit: Color = GameState.OUTFITS[int(app.get("outfit", 0))][1]
	var body_path := "res://assets/creator/%sbody_%s_%s.png" % [prefix, race, sex]
	var base: Image = load(body_path).get_image()
	base.convert(Image.FORMAT_RGBA8)
	_recolor(base, _skin_map(skin), outfit, true)
	var look: Dictionary = gear if not gear.is_empty() else GameState.gear_look("player")
	if prefix == "b":
		base = _with_weapon(base, race, look)
	var hair_img: Image = load("res://assets/creator/%shair_%s_%s_%s.png" % [prefix, race, sex, style]).get_image()
	hair_img.convert(Image.FORMAT_RGBA8)
	_recolor(hair_img, _hair_map(hair), outfit, false)
	base.blend_rect(hair_img, Rect2i(0, 0, hair_img.get_width(), hair_img.get_height()), Vector2i.ZERO)
	if bool(app.get("beard", false)) and sex == "m":
		var beard: Image = load("res://assets/creator/%sbeard_%s_%s.png" % [prefix, race, sex]).get_image()
		beard.convert(Image.FORMAT_RGBA8)
		_recolor(beard, _hair_map(hair), outfit, false)
		base.blend_rect(beard, Rect2i(0, 0, beard.get_width(), beard.get_height()), Vector2i.ZERO)
	if prefix != "p":
		_overlay_gear(base, prefix, race, sex, look)
	return ImageTexture.create_from_image(base)


## Capa de arma (detrás del cuerpo) según tipo, calidad y orbe.
func _with_weapon(body: Image, race: String, look: Dictionary) -> Image:
	var kind: String = str(look.get("weapon_kind", ""))
	if kind == "":
		return body
	var tier: int = clampi(int(look.get("weapon_tier", 1)), 1, 3)
	var orb: String = str(look.get("orb", ""))
	var wpath := "res://assets/creator/weapon_%s_%s_%d%s.png" % [race, kind, tier, ("_" + orb) if orb != "" and kind == "staff" else ""]
	if not ResourceLoader.exists(wpath):
		return body
	var w: Image = load(wpath).get_image().duplicate()
	w.convert(Image.FORMAT_RGBA8)
	w.blend_rect(body, Rect2i(0, 0, body.get_width(), body.get_height()), Vector2i.ZERO)
	return w


## Armadura, casco y escudo visibles.
func _overlay_gear(img: Image, prefix: String, race: String, sex: String, look: Dictionary) -> void:
	var layers: Array = []
	if str(look.get("body", "")) != "":
		layers.append("%seqbody_%s_%s_%s" % [prefix, look["body"], race, sex])
	if str(look.get("head", "")) != "":
		layers.append("%seqhead_%s_%s_%s" % [prefix, look["head"], race, sex])
	if prefix == "b" and str(look.get("shield", "")) != "":
		layers.append("beqshield_%s_%s_%s" % [look["shield"], race, sex])
	for l in layers:
		var path := "res://assets/creator/%s.png" % l
		if ResourceLoader.exists(path):
			var o: Image = load(path).get_image()
			o.convert(Image.FORMAT_RGBA8)
			if o.get_size() == img.get_size():
				img.blend_rect(o, Rect2i(Vector2i.ZERO, o.get_size()), Vector2i.ZERO)


func _key(prefix: String, app: Dictionary) -> String:
	return prefix + JSON.stringify(app) + JSON.stringify(GameState.gear_look("player"))


func sheet(app = null) -> Texture2D:
	var a: Dictionary = app if app != null else GameState.appearance
	var k := _key("s", a)
	if not _cache.has(k):
		_cache[k] = _compose("", a)
	return _cache[k]


func portrait(app = null) -> Texture2D:
	var a: Dictionary = app if app != null else GameState.appearance
	var k := _key("p", a)
	if not _cache.has(k):
		_cache[k] = _compose("p", a)
	return _cache[k]


func weapon_type() -> String:
	var c: String = str(GameState.player_data.get("class", "")) if GameState.player_data else ""
	if c != "" and DB.CLASSES.has(c):
		return str(DB.CLASSES[c]["weapon"])
	return "sword"


func battle_sheet(app = null) -> Texture2D:
	var a: Dictionary = app if app != null else GameState.appearance
	var k := _key("b", a)
	if not _cache.has(k):
		_cache[k] = _compose("b", a)
	return _cache[k]


## Hoja de poses de combate (9 frames de 32x32) para un miembro.
func member_battle_sheet(id: String) -> Texture2D:
	if id == "player":
		return battle_sheet()
	var look := GameState.gear_look(id)
	var k := "mb_" + _member_base_name(id) + JSON.stringify(look)
	if not _cache.has(k):
		var path := "res://assets/battle/%s_nw.png" % _member_base_name(id)
		if not ResourceLoader.exists(path):
			return load("res://assets/battle/%s.png" % _member_base_name(id))
		var img: Image = load(path).get_image().duplicate()
		img.convert(Image.FORMAT_RGBA8)
		var rs: Array = MEMBER_BODY.get(id, ["human", "m"])
		if id == "yara" and GameState.yara_is_witch() and str(look.get("orb", "")) == "":
			look["orb"] = "dark"
		img = _with_weapon(img, rs[0], look)
		_overlay_gear(img, "b", rs[0], rs[1], look)
		_cache[k] = ImageTexture.create_from_image(img)
	return _cache[k]


## Textura de hoja de sprites para cualquier miembro del grupo.
const MEMBER_BODY := {"kaelen": ["human", "m"], "yara": ["human", "f"], "aelis": ["elf", "f"], "brom": ["dwarf", "m"]}


func _member_base_name(id: String) -> String:
	if id == "yara" and GameState.yara_is_witch():
		return "yara_dark"
	if id == "kaelen" and GameState.has_flag("kaelen_dark"):
		return "kaelen_dark"
	return id


func member_sheet(id: String) -> Texture2D:
	if id == "player":
		return sheet()
	var look := GameState.gear_look(id)
	var k := "m_" + _member_base_name(id) + JSON.stringify(look)
	if not _cache.has(k):
		var img: Image = load("res://assets/chars/%s.png" % _member_base_name(id)).get_image().duplicate()
		img.convert(Image.FORMAT_RGBA8)
		var rs: Array = MEMBER_BODY.get(id, ["human", "m"])
		_overlay_gear(img, "", rs[0], rs[1], look)
		_cache[k] = ImageTexture.create_from_image(img)
	return _cache[k]


func member_portrait(id: String) -> Texture2D:
	if id == "player":
		return portrait()
	if id == "yara" and GameState.yara_is_witch():
		return load("res://assets/portraits/yara_dark.png")
	if id == "kaelen" and GameState.has_flag("kaelen_dark"):
		return load("res://assets/portraits/kaelen_dark.png")
	return load("res://assets/portraits/%s.png" % id)
