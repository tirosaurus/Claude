extends Node
## Estado persistente: personaje, grupo, progreso, inventario, vínculos,
## flags de historia y guardado/carga.

signal leveled_up(new_level: int)

const SAVE_SLOTS := 3
const HAIR_COLORS := [
	["Castaño", Color8(110, 70, 42)], ["Negro", Color8(36, 32, 40)], ["Rubio", Color8(220, 180, 90)],
	["Pelirrojo", Color8(190, 84, 50)], ["Platino", Color8(232, 228, 212)], ["Cobrizo", Color8(160, 90, 40)],
	["Azul noche", Color8(50, 60, 116)], ["Verde bosque", Color8(62, 112, 72)], ["Gris", Color8(150, 150, 160)],
	["Rosa", Color8(206, 112, 152)],
]
const SKIN_TONES := [
	["Clara", Color8(244, 208, 180)], ["Media", Color8(236, 188, 150)], ["Tostada", Color8(198, 142, 102)],
	["Morena", Color8(150, 100, 70)], ["Oscura", Color8(104, 70, 52)],
]
const OUTFITS := [
	["Turquesa", Color8(52, 118, 128)], ["Granate", Color8(140, 46, 52)], ["Verde", Color8(70, 120, 70)],
	["Azul", Color8(60, 80, 150)], ["Marrón", Color8(120, 84, 56)], ["Violeta", Color8(100, 70, 130)],
]
const HAIR_STYLES := [["short", "Corto"], ["spiky", "Rebelde"], ["long", "Largo"], ["ponytail", "Coleta"],
	["bun", "Moño"], ["braids", "Trenzas"], ["shaved", "Rapado"]]

var player_data := {}
var appearance := {}
var level := 1
var xp := 0
var gold := 0
var members := {}
var companions := {}
var party: Array = []
var inventory := {}
var weapon_tier := 0
var armor_tier := 0
var quest_flags := {}
var current_map := "bedroom"
var current_spawn := "start"
var load_pos = null
var return_pos = null
var pending_encounter := {}
var battle_return := {}
var play_time := 0.0
var auto_battle := false


func _ready() -> void:
	process_mode = Node.PROCESS_MODE_ALWAYS
	new_game("Arlen", default_appearance())


func _process(delta: float) -> void:
	if not get_tree().paused:
		play_time += delta


func default_appearance() -> Dictionary:
	return {"race": "human", "sex": "m", "hair_style": 1, "hair_color": 0, "skin": 1, "outfit": 0, "beard": false}


func new_game(player_name: String, app: Dictionary) -> void:
	for m in get_meta_list():
		remove_meta(m)
	player_data = {"name": player_name, "race": app.get("race", "human"), "sex": app.get("sex", "m"), "branch": "", "class": ""}
	appearance = app.duplicate()
	level = 1
	xp = 0
	gold = 30
	weapon_tier = 0
	armor_tier = 0
	inventory = {"pocion": 2}
	quest_flags = {}
	companions = {
		"kaelen": {"approval": 0, "rivalry": 0, "alive": true, "joined": false, "romance": false, "left": false},
		"yara": {"approval": 0, "kaelen_approval": 20, "corruption": 0, "alive": true, "joined": false,
			"romance_target": "none"},
		"aelis": {"approval": 0, "alive": true, "joined": false},
		"brom": {"approval": 0, "alive": true, "joined": false},
	}
	party = ["player"]
	members = {"player": {"hp": 0, "mp": 0}}
	for id in ["kaelen", "yara", "aelis", "brom"]:
		members[id] = {"hp": 0, "mp": 0}
	heal_all()
	current_map = "bedroom"
	current_spawn = "start"
	load_pos = null
	return_pos = null
	play_time = 0.0
	Appearance.clear_cache()


# ------------------------------------------------------------ Flags
func set_flag(flag_name: String, value: bool = true) -> void:
	quest_flags[flag_name] = value


func has_flag(flag_name: String) -> bool:
	return bool(quest_flags.get(flag_name, false))


func player_name() -> String:
	return str(player_data.get("name", "Arlen"))


func race() -> String:
	return str(player_data.get("race", "human"))


func race_name() -> String:
	return str(DB.RACES[race()]["name"])


func is_female() -> bool:
	return str(player_data.get("sex", "m")) == "f"


## Devuelve la forma masculina o femenina según el sexo del jugador.
func g(masc: String, fem: String) -> String:
	return fem if is_female() else masc


# ------------------------------------------------------------ Grupo
func join_party(id: String) -> void:
	if not companions.has(id):
		return
	companions[id]["joined"] = true
	if not party.has(id):
		party.append(id)
	var st := stats(id)
	members[id] = {"hp": st["hp"], "mp": st["mp"]}


func leave_party(id: String) -> void:
	party.erase(id)
	if companions.has(id):
		companions[id]["joined"] = false


func in_party(id: String) -> bool:
	return party.has(id)


func party_followers() -> Array:
	var out: Array = []
	for id in party:
		if id != "player":
			out.append(id)
	return out


func member_name(id: String) -> String:
	if id == "player":
		return player_name()
	return str(DB.MEMBERS[id]["name"])


func member_role(id: String) -> String:
	if id == "player":
		if str(player_data["class"]) != "":
			return str(DB.CLASSES[player_data["class"]]["name"])
		if str(player_data["branch"]) != "":
			return str(DB.BRANCHES[player_data["branch"]]["name"])
		return "Aventurer" + g("o", "a")
	if id == "yara" and yara_is_witch():
		return "Bruja"
	if id == "kaelen" and has_flag("kaelen_dark"):
		return "Caído"
	return str(DB.MEMBERS[id]["role"])


func yara_is_witch() -> bool:
	return int(companions["yara"]["corruption"]) >= 40


func stats(id: String) -> Dictionary:
	var base: Dictionary
	var grow: Dictionary
	if id == "player":
		base = DB.RACES[race()]["base"]
		grow = DB.RACES[race()]["grow"]
	else:
		base = DB.MEMBERS[id]["base"]
		grow = DB.MEMBERS[id]["grow"]
	var s := {}
	for k in ["hp", "mp", "atk", "def", "mag", "res", "spd"]:
		s[k] = int(base[k] + grow[k] * (level - 1))
	if id == "player":
		var b: String = str(player_data["branch"])
		if b != "":
			for k in DB.BRANCHES[b]["bonus"]:
				s[k] += int(DB.BRANCHES[b]["bonus"][k])
		var c: String = str(player_data["class"])
		if c != "":
			for k in DB.CLASSES[c]["bonus"]:
				s[k] += int(DB.CLASSES[c]["bonus"][k])
	var wt: int = [0, 4, 9][weapon_tier]
	var at: int = [0, 4, 9][armor_tier]
	s["atk"] += wt
	s["mag"] += wt
	s["def"] += at
	s["res"] += at
	return s


func skills_of(id: String) -> Array:
	var out: Array = []
	if id == "player":
		var b: String = str(player_data["branch"])
		if b == "healer":
			out.append("luz_sanadora")
		else:
			out.append("aux")
			if b != "":
				out.append(DB.BRANCHES[b]["skill"])
		var c: String = str(player_data["class"])
		if c != "":
			var cs: Dictionary = DB.CLASSES[c]["skills"]
			var lv := cs.keys()
			lv.sort()
			for l in lv:
				if level >= int(l):
					out.append(cs[l])
		return out
	var table: Dictionary = DB.MEMBERS[id]["skills"]
	if id == "yara" and yara_is_witch():
		table = DB.MEMBERS[id]["dark_skills"]
	var levels := table.keys()
	levels.sort()
	for l in levels:
		if level >= int(l):
			out.append(table[l])
	if id == "kaelen" and has_flag("kaelen_redeemed"):
		out.append("tajo_sombrio")
	return out


## Al subir de nivel: media vida y una cuarta parte del maná (el maná es un recurso a gestionar).
func level_restore() -> void:
	for id in members:
		var st := stats(id)
		var m: Dictionary = members[id]
		m["hp"] = mini(int(st["hp"]), maxi(1, int(m["hp"])) + int(st["hp"] * 0.5))
		m["mp"] = mini(int(st["mp"]), int(m["mp"]) + int(ceil(st["mp"] * 0.25)))


func heal_all() -> void:
	for id in members:
		var st := stats(id)
		members[id]["hp"] = st["hp"]
		members[id]["mp"] = st["mp"]


# ------------------------------------------------------------ Progresión
## Devuelve una lista de {level, learned: [[miembro, habilidad]]}
func add_xp(amount: int) -> Array:
	var result: Array = []
	xp += amount
	while xp >= DB.xp_to_next(level):
		xp -= DB.xp_to_next(level)
		var before := {}
		for id in party:
			before[id] = skills_of(id)
		level += 1
		var learned: Array = []
		for id in party:
			for s in skills_of(id):
				if not before[id].has(s):
					learned.append([id, s])
		level_restore()
		result.append({"level": level, "learned": learned})
		leveled_up.emit(level)
	return result


func needs_branch_choice() -> bool:
	return level >= 2 and str(player_data["branch"]) == ""


func needs_class_choice() -> bool:
	return level >= 5 and str(player_data["branch"]) != "" and str(player_data["class"]) == ""


func choose_branch(b: String) -> void:
	player_data["branch"] = b
	heal_all()


func choose_class(c: String) -> void:
	player_data["class"] = c
	heal_all()


func branch_name() -> String:
	var b: String = str(player_data.get("branch", ""))
	return "Sin rama" if b == "" else str(DB.BRANCHES[b]["name"])


func class_name_str() -> String:
	var c: String = str(player_data.get("class", ""))
	return "Sin clase" if c == "" else str(DB.CLASSES[c]["name"])


# ------------------------------------------------------------ Inventario
func add_item(id: String, qty: int = 1) -> void:
	inventory[id] = int(inventory.get(id, 0)) + qty


func remove_item(id: String, qty: int = 1) -> void:
	inventory[id] = max(0, int(inventory.get(id, 0)) - qty)
	if inventory[id] <= 0:
		inventory.erase(id)


func item_count(id: String) -> int:
	return int(inventory.get(id, 0))


# ------------------------------------------------------------ Vínculos
func change_approval(id: String, delta: int) -> void:
	if companions.has(id):
		companions[id]["approval"] = clampi(int(companions[id]["approval"]) + delta, -100, 100)
		if id == "yara":
			_recalc_yara()


func change_kaelen_rivalry(delta: int) -> void:
	var k: Dictionary = companions["kaelen"]
	k["rivalry"] = clampi(int(k["rivalry"]) + delta, -100, 100)
	_recalc_yara()


func change_yara_kaelen_approval(delta: int) -> void:
	var y: Dictionary = companions["yara"]
	y["kaelen_approval"] = clampi(int(y["kaelen_approval"]) + delta, -100, 100)
	_recalc_yara()


func change_yara_corruption(delta: int) -> void:
	var y: Dictionary = companions["yara"]
	y["corruption"] = clampi(int(y["corruption"]) + delta, -100, 100)


func rivalry() -> int:
	return int(companions["kaelen"]["rivalry"])


func corruption() -> int:
	return int(companions["yara"]["corruption"])


func _recalc_yara() -> void:
	var y: Dictionary = companions["yara"]
	var k: Dictionary = companions["kaelen"]
	var player_wants: bool = int(y["approval"]) >= 25 or has_flag("romance_yara")
	var kaelen_wants: bool = int(y["kaelen_approval"]) >= 45 and bool(k["alive"])
	var target := "none"
	if player_wants and kaelen_wants and rivalry() <= -30 and has_flag("trio_ok"):
		target = "both"
	elif player_wants and has_flag("romance_yara"):
		target = "player"
	elif kaelen_wants and not has_flag("romance_yara"):
		target = "kaelen"
	y["romance_target"] = target


func kaelen_bond_label() -> String:
	var r := rivalry()
	if not bool(companions["kaelen"]["alive"]):
		return "Caído"
	if r <= -40:
		return "Hermanos de alma"
	if r <= -10:
		return "Mejores amigos"
	if r < 10:
		return "Amigos de la infancia"
	if r < 40:
		return "Pique constante"
	return "Rivales"


func yara_bond_label() -> String:
	var a: int = int(companions["yara"]["approval"])
	if has_flag("romance_yara"):
		return "Enamorada de ti"
	if a >= 20:
		return "Muy unida a ti"
	if a >= 8:
		return "Cómplice"
	if a > -8:
		return "Amiga de siempre"
	return "Dolida contigo"


# ------------------------------------------------------------ Guardado
func _slot_path(slot: int) -> String:
	return "user://save_%d.json" % slot


func save_game(slot: int, pos: Vector2) -> bool:
	var data := {
		"version": 2, "player_data": player_data, "appearance": appearance, "level": level, "xp": xp, "gold": gold,
		"members": members, "companions": companions, "party": party, "inventory": inventory,
		"weapon_tier": weapon_tier, "armor_tier": armor_tier, "flags": quest_flags,
		"map": current_map, "pos": [pos.x, pos.y], "play_time": play_time,
		"saved_at": Time.get_datetime_string_from_system(false, true),
		"meta": _meta_dict(),
	}
	var f := FileAccess.open(_slot_path(slot), FileAccess.WRITE)
	if f == null:
		return false
	f.store_string(JSON.stringify(data))
	f.close()
	return true


func _meta_dict() -> Dictionary:
	var d := {}
	for m in get_meta_list():
		var v = get_meta(m)
		if v is int or v is String or v is float or v is bool:
			d[m] = v
	return d


func slot_info(slot: int) -> Dictionary:
	if not FileAccess.file_exists(_slot_path(slot)):
		return {}
	var f := FileAccess.open(_slot_path(slot), FileAccess.READ)
	var d = JSON.parse_string(f.get_as_text())
	if not d is Dictionary:
		return {}
	return d


func any_save() -> int:
	var best := -1
	var best_time := ""
	for i in SAVE_SLOTS:
		var info := slot_info(i)
		if info.is_empty():
			continue
		var t: String = str(info.get("saved_at", ""))
		if best == -1 or t > best_time:
			best = i
			best_time = t
	return best


func load_game(slot: int) -> bool:
	var d := slot_info(slot)
	if d.is_empty():
		return false
	for m in get_meta_list():
		remove_meta(m)
	player_data = d["player_data"]
	appearance = d["appearance"]
	for k in ["hair_style", "hair_color", "skin", "outfit"]:
		appearance[k] = int(appearance[k])
	level = int(d["level"])
	xp = int(d["xp"])
	gold = int(d["gold"])
	members = d["members"]
	companions = d["companions"]
	party = d["party"]
	inventory = {}
	for k in d["inventory"]:
		inventory[k] = int(d["inventory"][k])
	weapon_tier = int(d["weapon_tier"])
	armor_tier = int(d["armor_tier"])
	quest_flags = d["flags"]
	current_map = str(d["map"])
	current_spawn = ""
	load_pos = Vector2(d["pos"][0], d["pos"][1])
	play_time = float(d.get("play_time", 0))
	var meta: Dictionary = d.get("meta", {})
	for k in meta:
		set_meta(k, meta[k])
	Appearance.clear_cache()
	return true


func format_time(t: float) -> String:
	var s := int(t)
	return "%d:%02d:%02d" % [s / 3600, (s / 60) % 60, s % 60]
