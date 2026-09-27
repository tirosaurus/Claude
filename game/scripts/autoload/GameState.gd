extends Node
## Estado persistente de la partida: personaje, grupo, vínculos, progresión,
## flags de historia y datos de transición entre mapas/combates.

signal companion_approval_changed(companion_id: String, new_value: int)
signal kaelen_rivalry_changed(new_value: int)
signal kaelen_left_party(became_enemy: bool)
signal yara_romance_target_changed(target: String)
signal yara_corruption_changed(new_value: int)
signal leveled_up(new_level: int)
signal branch_chosen(branch: String)

const RACES := {
	"human": {"name": "Humano", "desc": "Versátiles y tenaces. El resto de Vaelmoor os ve como iguales... casi siempre."},
	"elf": {"name": "Elfo", "desc": "Longevos y afines a la magia. En los pueblos humanos despertáis admiración y recelo."},
	"dwarf": {"name": "Enano", "desc": "Duros como la roca de Khazgurim. Resistentes y orgullosos."},
	"beastkin": {"name": "Bestial", "desc": "Sangre de lobo, zorro u oso. Sentidos agudos; el Concilio desconfía de los tuyos."},
	"darkblood": {"name": "Sangre-Oscura", "desc": "Descendiente de algo que no era humano. Magia oscura innata; miradas de miedo."},
	"reborn": {"name": "Renacido", "desc": "Volviste de la muerte sin saber cómo. Sin hambre ni sueño... y sin mucha bienvenida."},
}

const BRANCHES := {
	"melee": {"name": "Cuerpo a cuerpo", "desc": "Espada y escudo en primera línea. Más vida y golpes más duros.", "hp": 10, "atk": 3, "mag": 0, "def": 1},
	"ranged": {"name": "A distancia", "desc": "Arco y precisión. Golpeas antes de que te alcancen y rara vez fallas.", "hp": 4, "atk": 2, "mag": 0, "def": 0},
	"dps": {"name": "DPS", "desc": "Todo al daño. Ataques rápidos y críticos devastadores, pero frágil.", "hp": 0, "atk": 5, "mag": 0, "def": -1},
	"healer": {"name": "Sanador", "desc": "Magia de luz. Curas mucho mejor y proteges a los tuyos.", "hp": 4, "atk": 0, "mag": 5, "def": 0},
	"tank": {"name": "Tanque", "desc": "Un muro viviente. Aguantas lo que nadie aguanta y proteges al grupo.", "hp": 16, "atk": 1, "mag": 0, "def": 3},
}

var player_data := {}
var player_stats := {}
var companions := {}
var faction_reputation := {}
var quest_flags := {}

var current_map := "bedroom"
var current_spawn := "start"
var pending_encounter := {}
var battle_return := {}

const APPROVAL_MIN := -100
const APPROVAL_MAX := 100
const ROMANCE_THRESHOLD := 60
const RIVALRY_BREAK_THRESHOLD := 80
const FRIENDSHIP_ROMANCE_THRESHOLD := -60


func _ready() -> void:
	new_game("Viajero", "human")


func new_game(player_name: String, race: String) -> void:
	for m in get_meta_list():
		remove_meta(m)
	player_data = {"name": player_name, "race": race}
	player_stats = {
		"level": 1, "xp": 0, "xp_to_next": 100,
		"max_hp": 34, "hp": 34, "max_mp": 16, "mp": 16,
		"atk": 7, "mag": 6, "def": 1, "branch": "",
	}
	companions = {
		"kaelen": {
			"display_name": "Kaelen", "approval": 0, "alive": true, "in_party": false,
			"romance_active": false, "romance_locked_out": false, "rivalry": 0,
			"max_hp": 38, "hp": 38, "atk": 8, "def": 2,
		},
		"yara": {
			"display_name": "Yara", "approval": 0, "alive": true, "in_party": false,
			"romance_active": false, "romance_locked_out": false, "kaelen_approval": 20,
			"romance_target": "none", "corruption": 0,
			"max_hp": 28, "hp": 28, "atk": 4, "mag": 9, "def": 0,
		},
	}
	faction_reputation = {"aurelia": 0, "savia": 0, "espina": 0, "khazgurim": 0, "torre_negra": 0, "gremio": 0}
	quest_flags = {}
	current_map = "bedroom"
	current_spawn = "start"


func set_flag(flag_name: String, value: bool = true) -> void:
	quest_flags[flag_name] = value


func has_flag(flag_name: String) -> bool:
	return bool(quest_flags.get(flag_name, false))


func player_name() -> String:
	return str(player_data.get("name", "Viajero"))


func race_name() -> String:
	return str(RACES.get(player_data.get("race", "human"), RACES["human"])["name"])


## --- Grupo ---------------------------------------------------------------

func join_party(companion_id: String) -> void:
	if companions.has(companion_id):
		companions[companion_id]["in_party"] = true


func party_followers() -> Array:
	var out: Array = []
	for id in ["kaelen", "yara"]:
		var c: Dictionary = companions[id]
		if c["in_party"] and c["alive"]:
			out.append(id)
	return out


func heal_party() -> void:
	player_stats["hp"] = player_stats["max_hp"]
	player_stats["mp"] = player_stats["max_mp"]
	for id in companions:
		companions[id]["hp"] = companions[id]["max_hp"]


## --- Progresión -----------------------------------------------------------

func add_xp(amount: int) -> int:
	var levels := 0
	player_stats["xp"] = int(player_stats["xp"]) + amount
	while int(player_stats["xp"]) >= int(player_stats["xp_to_next"]):
		player_stats["xp"] = int(player_stats["xp"]) - int(player_stats["xp_to_next"])
		player_stats["level"] = int(player_stats["level"]) + 1
		player_stats["xp_to_next"] = int(int(player_stats["xp_to_next"]) * 1.5)
		player_stats["max_hp"] = int(player_stats["max_hp"]) + 6
		player_stats["max_mp"] = int(player_stats["max_mp"]) + 3
		player_stats["atk"] = int(player_stats["atk"]) + 1
		player_stats["mag"] = int(player_stats["mag"]) + 1
		player_stats["hp"] = player_stats["max_hp"]
		player_stats["mp"] = player_stats["max_mp"]
		for id in companions:
			var c: Dictionary = companions[id]
			c["max_hp"] = int(c["max_hp"]) + 5
			c["hp"] = c["max_hp"]
		levels += 1
		leveled_up.emit(int(player_stats["level"]))
	return levels


func needs_branch_choice() -> bool:
	return int(player_stats["level"]) > 1 and str(player_stats["branch"]) == ""


func choose_branch(branch: String) -> void:
	if not BRANCHES.has(branch):
		push_warning("Rama desconocida: %s" % branch)
		return
	var b: Dictionary = BRANCHES[branch]
	player_stats["branch"] = branch
	player_stats["max_hp"] = int(player_stats["max_hp"]) + int(b["hp"])
	player_stats["hp"] = player_stats["max_hp"]
	player_stats["atk"] = int(player_stats["atk"]) + int(b["atk"])
	player_stats["mag"] = int(player_stats["mag"]) + int(b["mag"])
	player_stats["def"] = int(player_stats["def"]) + int(b["def"])
	branch_chosen.emit(branch)


func branch_name() -> String:
	var b: String = str(player_stats.get("branch", ""))
	if b == "":
		return "Sin rama"
	return str(BRANCHES[b]["name"])


## --- Vínculos -------------------------------------------------------------

func change_approval(companion_id: String, delta: int) -> void:
	if not companions.has(companion_id):
		return
	var c: Dictionary = companions[companion_id]
	if not c["alive"]:
		return
	c["approval"] = clampi(int(c["approval"]) + delta, APPROVAL_MIN, APPROVAL_MAX)
	companion_approval_changed.emit(companion_id, c["approval"])
	if companion_id == "yara":
		_recalculate_yara_romance_target()
	elif int(c["approval"]) >= ROMANCE_THRESHOLD and not c["romance_locked_out"]:
		c["romance_active"] = true


## delta negativo = más amistad, positivo = más rivalidad.
func change_kaelen_rivalry(delta: int) -> void:
	var k: Dictionary = companions["kaelen"]
	if not k["alive"]:
		return
	k["rivalry"] = clampi(int(k["rivalry"]) + delta, -100, 100)
	kaelen_rivalry_changed.emit(k["rivalry"])
	if int(k["rivalry"]) >= RIVALRY_BREAK_THRESHOLD and k["in_party"]:
		k["in_party"] = false
		k["romance_active"] = false
		k["romance_locked_out"] = true
		kaelen_left_party.emit(true)
	elif int(k["rivalry"]) <= FRIENDSHIP_ROMANCE_THRESHOLD and not k["romance_locked_out"]:
		k["romance_active"] = true
	_recalculate_yara_romance_target()


func change_yara_kaelen_approval(delta: int) -> void:
	var y: Dictionary = companions["yara"]
	y["kaelen_approval"] = clampi(int(y["kaelen_approval"]) + delta, APPROVAL_MIN, APPROVAL_MAX)
	_recalculate_yara_romance_target()


func change_yara_corruption(delta: int) -> void:
	var y: Dictionary = companions["yara"]
	y["corruption"] = clampi(int(y["corruption"]) + delta, -100, 100)
	yara_corruption_changed.emit(y["corruption"])


func _recalculate_yara_romance_target() -> void:
	var y: Dictionary = companions["yara"]
	var k: Dictionary = companions["kaelen"]
	var player_wants: bool = int(y["approval"]) >= ROMANCE_THRESHOLD
	var kaelen_wants: bool = int(y["kaelen_approval"]) >= ROMANCE_THRESHOLD and bool(k["alive"])
	var trio_possible: bool = player_wants and kaelen_wants and int(k["rivalry"]) <= FRIENDSHIP_ROMANCE_THRESHOLD
	var new_target := "none"
	if trio_possible:
		new_target = "both"
	elif player_wants:
		new_target = "player"
	elif kaelen_wants:
		new_target = "kaelen"
	if new_target != str(y["romance_target"]):
		y["romance_target"] = new_target
		y["romance_active"] = new_target == "player" or new_target == "both"
		yara_romance_target_changed.emit(new_target)


func kaelen_bond_label() -> String:
	var r: int = int(companions["kaelen"]["rivalry"])
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
	if a >= 20:
		return "Muy unida a ti"
	if a >= 8:
		return "Cómplice"
	if a > -8:
		return "Amiga de siempre"
	return "Algo dolida contigo"


func change_reputation(faction_id: String, delta: int) -> void:
	if faction_reputation.has(faction_id):
		faction_reputation[faction_id] = clampi(int(faction_reputation[faction_id]) + delta, -100, 100)
