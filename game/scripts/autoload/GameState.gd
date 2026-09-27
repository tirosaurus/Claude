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
var equipment := {}   # {miembro: {slot: id_equipo}}
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
	player_data = {"name": player_name, "race": app.get("race", "human"), "sex": app.get("sex", "m"), "branch": "", "class": "", "tree": {},
		"difficulty": int(app.get("difficulty", 1))}
	appearance = app.duplicate()
	appearance.erase("difficulty")
	level = 1
	xp = 0
	gold = 30
	weapon_tier = 0
	armor_tier = 0
	equipment = {"player": {}}
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
	if not equipment.has(id):
		equipment[id] = {}
		for eid in DB.MEMBER_START_GEAR.get(id, []):
			equipment[id][DB.EQUIP[eid]["slot"]] = eid
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
		if b != "" and DB.BRANCHES.has(b):
			for k in DB.BRANCHES[b]["bonus"]:
				s[k] += int(DB.BRANCHES[b]["bonus"][k])
		var c: String = str(player_data["class"])
		if c != "" and DB.CLASSES.has(c):
			for k in DB.CLASSES[c]["bonus"]:
				s[k] += int(DB.CLASSES[c]["bonus"][k])
			var tree: Dictionary = talents()
			for n in DB.CLASSES[c]["tree"]:
				var r: int = int(tree.get(n["id"], 0))
				if r > 0 and n.has("stats"):
					for k in n["stats"]:
						s[k] += int(n["stats"][k]) * r
	for slot in equipment.get(id, {}):
		var eid: String = equipment[id][slot]
		if DB.EQUIP.has(eid):
			var est: Dictionary = DB.EQUIP[eid]["stats"]
			for k in est:
				s[k] += int(est[k])
	for k in s:
		s[k] = maxi(1, int(s[k]))
	return s


# ------------------------------------------------------------ Equipo
func equip_rule(id: String) -> String:
	if id == "player":
		var c: String = str(player_data.get("class", ""))
		return c if c != "" else "none"
	return id


func equipped(id: String, slot: String) -> String:
	return str(equipment.get(id, {}).get(slot, ""))


## Equipa (o quita con eid = "") y ajusta vida/PM actuales a los nuevos máximos.
func equip(id: String, slot: String, eid: String) -> void:
	if not equipment.has(id):
		equipment[id] = {}
	var before := stats(id)
	var cur := equipped(id, slot)
	if cur != "":
		add_item(cur)
		equipment[id].erase(slot)
	if eid != "":
		remove_item(eid)
		equipment[id][slot] = eid
	var after := stats(id)
	if members.has(id):
		var m: Dictionary = members[id]
		m["hp"] = clampi(int(m["hp"]) + int(after["hp"]) - int(before["hp"]), 1 if int(m["hp"]) > 0 else 0, int(after["hp"]))
		m["mp"] = clampi(int(m["mp"]) + int(after["mp"]) - int(before["mp"]), 0, int(after["mp"]))
	Appearance.clear_cache()


## Aspecto visible del equipo de un miembro: {weapon_kind, weapon_tier, orb, head, body, shield}
func gear_look(id: String) -> Dictionary:
	var out := {"weapon_kind": "", "weapon_tier": 1, "orb": "", "head": "", "body": "", "shield": ""}
	var w := equipped(id, "weapon")
	if w != "":
		out["weapon_kind"] = DB.EQUIP[w]["kind"]
		out["weapon_tier"] = int(DB.EQUIP[w]["tier"])
		out["orb"] = str(DB.EQUIP[w].get("orb", ""))
		if out["orb"] == "" and id == "player":
			var c: String = str(player_data.get("class", ""))
			if c != "" and DB.CLASSES.has(c):
				out["orb"] = str(DB.CLASSES[c].get("orb", ""))
	for slot in ["head", "body", "shield"]:
		var e := equipped(id, slot)
		if e != "":
			out[slot] = str(DB.EQUIP[e]["look"])
	return out


func skills_of(id: String) -> Array:
	var out: Array = []
	if id == "player":
		var c: String = str(player_data["class"])
		if c == "" or not DB.CLASSES.has(c):
			out.append("aux")
			return out
		var starter: String = DB.CLASSES[c]["starter"]
		out.append(starter)
		if starter != "aux" and not c in ["cleric"]:
			out.append("aux")
		var tree: Dictionary = talents()
		for n in DB.CLASSES[c]["tree"]:
			if n.has("skill") and int(tree.get(n["id"], 0)) > 0 and not out.has(n["skill"]):
				out.append(n["skill"])
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


# ------------------------------------------------------------ Talentos
func talents() -> Dictionary:
	if not player_data.has("tree"):
		player_data["tree"] = {}
	return player_data["tree"]


## Puntos de habilidad totales: 1 por nivel desde el 2, más uno extra en los niveles 5 y 8.
func skill_points_total() -> int:
	return maxi(0, level - 1) + (1 if level >= 5 else 0) + (1 if level >= 8 else 0)


func skill_points_spent() -> int:
	var n := 0
	for k in talents():
		n += int(talents()[k])
	return n


func skill_points() -> int:
	if str(player_data.get("class", "")) == "":
		return 0
	return skill_points_total() - skill_points_spent()


## "" si se puede aprender, o el motivo por el que no.
func talent_block_reason(nid: String) -> String:
	var c: String = str(player_data.get("class", ""))
	var n: Dictionary = DB.tree_node(c, nid)
	if n.is_empty():
		return "?"
	var r: int = int(talents().get(nid, 0))
	if r >= int(n["max"]):
		return "Rango máximo"
	var need: int = [0, 0, 3, 6][int(n["tier"])]
	if skill_points_spent() < need:
		return "Requiere %d puntos gastados" % need
	if skill_points() <= 0:
		return "Sin puntos"
	return ""


func learn_talent(nid: String) -> bool:
	if talent_block_reason(nid) != "":
		return false
	var before := stats("player")
	talents()[nid] = int(talents().get(nid, 0)) + 1
	var after := stats("player")
	var m: Dictionary = members["player"]
	m["hp"] = int(m["hp"]) + int(after["hp"]) - int(before["hp"])
	m["mp"] = int(m["mp"]) + int(after["mp"]) - int(before["mp"])
	return true


## Suma de una ventaja pasiva del árbol (crit, heal_pow, mp_regen, dmg_phys, dmg_mag, dmg_red, lifesteal, atb_start).
func perk(key: String) -> float:
	var c: String = str(player_data.get("class", ""))
	if c == "" or not DB.CLASSES.has(c):
		return 0.0
	var v := 0.0
	var tree: Dictionary = talents()
	for n in DB.CLASSES[c]["tree"]:
		if n.has("perk") and n["perk"][0] == key:
			v += float(n["perk"][1]) * int(tree.get(n["id"], 0))
	return v


## Al subir de nivel: media vida y una cuarta parte del maná (el maná es un recurso a gestionar).
func level_restore() -> void:
	for id in members:
		var st := stats(id)
		var m: Dictionary = members[id]
		m["hp"] = mini(int(st["hp"]), maxi(1, int(m["hp"])) + int(st["hp"] * 0.5))
		m["mp"] = mini(int(st["mp"]), int(m["mp"]) + int(ceil(st["mp"] * 0.25)))


func difficulty() -> int:
	return int(player_data.get("difficulty", 1)) if player_data else 1


## Multiplicadores de enemigos según dificultad: [vida, ataque/magia, experiencia/oro]
func diff_mults() -> Array:
	return [[0.7, 0.75, 1.25], [0.85, 0.87, 1.1], [1.15, 1.1, 1.0]][clampi(difficulty(), 0, 2)]


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
	return level >= 2 and str(player_data["branch"]) != "" and str(player_data["class"]) == ""


func choose_branch(b: String) -> void:
	player_data["branch"] = b
	heal_all()


func choose_class(c: String) -> void:
	player_data["class"] = c
	player_data["branch"] = DB.class_branch(c)
	talents()
	# equipo inicial de la clase (lo que no pueda llevar vuelve a la bolsa)
	for slot in equipment.get("player", {}).keys():
		if not DB.equip_allowed(c, equipment["player"][slot]):
			equip("player", slot, "")
	for eid in DB.CLASS_STARTER_GEAR.get(c, []):
		add_item(eid)
		if equipped("player", DB.EQUIP[eid]["slot"]) == "":
			equip("player", DB.EQUIP[eid]["slot"], eid)
	Appearance.clear_cache()
	heal_all()


func branch_name() -> String:
	var b: String = str(player_data.get("branch", ""))
	return "Sin senda" if b == "" or not DB.BRANCHES.has(b) else str(DB.BRANCHES[b]["name"])


func class_name_str() -> String:
	var c: String = str(player_data.get("class", ""))
	return "Sin clase" if c == "" or not DB.CLASSES.has(c) else str(DB.CLASSES[c]["name"])


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
		"weapon_tier": weapon_tier, "armor_tier": armor_tier, "flags": quest_flags, "equipment": equipment,
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
	# migración de partidas antiguas (ramas/clases de la v2)
	var oc: String = str(player_data.get("class", ""))
	var ob: String = str(player_data.get("branch", ""))
	if oc != "" and not DB.CLASSES.has(oc):
		player_data["class"] = DB.LEGACY_CLASSES.get(oc, "")
	if ob != "" and not DB.BRANCHES.has(ob):
		player_data["branch"] = DB.LEGACY_BRANCHES.get(ob, "")
	if str(player_data["class"]) != "":
		player_data["branch"] = DB.class_branch(str(player_data["class"]))
	if not player_data.has("tree"):
		player_data["tree"] = {}
	var tr: Dictionary = player_data["tree"]
	for k in tr.keys():
		tr[k] = int(tr[k])
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
	equipment = d.get("equipment", {})
	if not d.has("equipment"):
		# partidas antiguas: equipo inicial para todos y el nivel de mejoras compradas como objetos
		equipment = {"player": {}}
		var c0: String = str(player_data.get("class", ""))
		for eid in DB.CLASS_STARTER_GEAR.get(c0, []):
			equipment["player"][DB.EQUIP[eid]["slot"]] = eid
		for mid in party:
			if mid != "player":
				equipment[mid] = {}
				for eid in DB.MEMBER_START_GEAR.get(mid, []):
					equipment[mid][DB.EQUIP[eid]["slot"]] = eid
		if weapon_tier >= 1:
			for eid in ["espada_acero", "baston_cristal", "arco_largo", "hacha_guerra"]:
				inventory[eid] = int(inventory.get(eid, 0)) + 1
		if armor_tier >= 1:
			for eid in ["cota_malla", "tunica_mago", "jubon_cuero"]:
				inventory[eid] = int(inventory.get(eid, 0)) + 1
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
