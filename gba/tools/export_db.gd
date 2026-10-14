extends SceneTree
func _initialize() -> void:
	_run.call_deferred()
func _run() -> void:
	var DBs = load("res://scripts/data/DB.gd")
	var d := {}
	for k in ["RACES","BRANCHES","CLASSES","MEMBERS","SKILLS","STATUS","ITEMS","EQUIP","RARITY","SLOT_NAMES","STAT_NAMES","EQUIP_RULES","CLASS_STARTER_GEAR","MEMBER_START_GEAR","UPGRADES","SHOPS","CHESTS","ENEMIES","MAX_LEVEL","LEGENDARIES","VAEL_SET"]:
		d[k] = DBs.get(k) if DBs.get(k) != null else DBs.new().get(k)
	var g = root.get_node("GameState")
	for k in ["HAIR_COLORS","SKIN_TONES","OUTFITS","HAIR_STYLES"]:
		var arr = []
		for e in g.get(k):
			var row = []
			for v in e:
				row.append([v.r8, v.g8, v.b8] if v is Color else v)
			arr.append(row)
		d[k] = arr
	var rar = {}
	for k in d["RARITY"]:
		var c: Color = d["RARITY"][k]["color"]
		rar[k] = {"name": d["RARITY"][k]["name"], "color": [c.r8, c.g8, c.b8]}
	d["RARITY"] = rar
	var f = FileAccess.open("/home/user/Claude/gba/build/db.json", FileAccess.WRITE)
	f.store_string(JSON.stringify(d, "  ", false))
	f.close()
	print("ok")
	quit()
