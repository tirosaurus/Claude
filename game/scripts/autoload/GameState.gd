extends Node
## Autoload global. Guarda el estado persistente de la partida:
## datos del personaje, reputación de facciones y vínculos con compañeros.

signal companion_approval_changed(companion_id: String, new_value: int)
signal companion_died(companion_id: String)
signal faction_reputation_changed(faction_id: String, new_value: int)
signal kaelen_rivalry_changed(new_value: int)
signal kaelen_left_party(became_enemy: bool)
signal yara_romance_target_changed(target: String)
signal xp_gained(amount: int)
signal leveled_up(new_level: int)
signal branch_chosen(branch: String)
signal yara_corruption_changed(new_value: int)

var player_data := {
	"name": "",
	"race": "human",
	"clazz": "warrior",
	"background": "",
}

## Flags narrativos genéricos de un solo bit (quest_flags["kaelen_invited"]
## = true, etc). Evita tener que crear una señal/variable por cada hito.
var quest_flags := {}


func set_flag(flag_name: String, value: bool = true) -> void:
	quest_flags[flag_name] = value


func has_flag(flag_name: String) -> bool:
	return quest_flags.get(flag_name, false)


## --- Progresión del jugador -----------------------------------------

var player_stats := {
	"level": 1,
	"xp": 0,
	"xp_to_next": 100,
	"max_hp": 30,
	"hp": 30,
	"max_mana": 20,
	"mana": 20,
	## "" hasta el primer nivel: se elige entre melee/ranged/dps/healer/tank.
	"branch": "",
}

const BRANCHES := ["melee", "ranged", "dps", "healer", "tank"]


func add_xp(amount: int) -> void:
	player_stats.xp += amount
	xp_gained.emit(amount)
	while player_stats.xp >= player_stats.xp_to_next:
		player_stats.xp -= player_stats.xp_to_next
		player_stats.level += 1
		player_stats.xp_to_next = int(player_stats.xp_to_next * 1.4)
		player_stats.max_hp += 6
		player_stats.hp = player_stats.max_hp
		player_stats.max_mana += 4
		player_stats.mana = player_stats.max_mana
		leveled_up.emit(player_stats.level)


func choose_branch(branch: String) -> void:
	if branch not in BRANCHES:
		push_warning("Rama desconocida: %s" % branch)
		return
	player_stats.branch = branch
	branch_chosen.emit(branch)


func needs_branch_choice() -> bool:
	return player_stats.level > 1 and player_stats.branch == ""


## --- Facciones y compañeros -------------------------------------------

var faction_reputation := {
	"aurelia": 0,
	"savia": 0,
	"espina": 0,
	"khazgurim": 0,
	"torre_negra": 0,
	"gremio": 0,
}

## Cada compañero: approval (-100..100), alive (bool), romance_active (bool),
## romance_locked_out (bool, si ya se cerró esa vía por decisiones pasadas).
var companions := {
	"kaelen": {
		"display_name": "Kaelen",
		"approval": 0,
		"alive": true,
		"romance_active": false,
		"romance_locked_out": false,
		"in_party": true,
		## Eje independiente de la aprobación: -100 amistad total,
		## +100 rivalidad total. Ver DESIGN.md sección 3.
		"rivalry": 0,
	},
	"yara": {
		"display_name": "Yara",
		"approval": 0,
		"alive": true,
		"romance_active": false,
		"romance_locked_out": false,
		"in_party": true,
		## Relación de Yara con Kaelen, llevada aparte de la del jugador.
		"kaelen_approval": 0,
		## "none" | "player" | "kaelen" | "both"
		"romance_target": "none",
		## -100 curandera luminosa total, +100 bruja oscura total.
		## Empieza en 0: Yara es una maga curandera muy eficaz por defecto,
		## pero puede corromperse según las decisiones tomadas con ella
		## y ante la corrupción de los Moronguls.
		"corruption": 0,
	},
}

const APPROVAL_MIN := -100
const APPROVAL_MAX := 100
const ROMANCE_THRESHOLD := 60
const RIVALRY_MIN := -100
const RIVALRY_MAX := 100
## Más allá de este umbral de rivalidad, Kaelen abandona la party.
const RIVALRY_BREAK_THRESHOLD := 80
## Más allá de este umbral de amistad, la ruta de romance con Kaelen
## queda disponible (y se bloquea si ya hay rivalidad alta).
const FRIENDSHIP_ROMANCE_THRESHOLD := -60


func change_approval(companion_id: String, delta: int) -> void:
	if not companions.has(companion_id):
		push_warning("Compañero desconocido: %s" % companion_id)
		return
	var c: Dictionary = companions[companion_id]
	if not c.get("alive", true):
		return
	c.approval = clampi(c.approval + delta, APPROVAL_MIN, APPROVAL_MAX)
	companion_approval_changed.emit(companion_id, c.approval)

	if companion_id == "yara":
		_recalculate_yara_romance_target()
	elif c.approval >= ROMANCE_THRESHOLD and not c.romance_locked_out:
		c.romance_active = true


## Mueve el eje amistad/rivalidad de Kaelen. delta negativo = más amistad,
## delta positivo = más rivalidad.
func change_kaelen_rivalry(delta: int) -> void:
	var k: Dictionary = companions["kaelen"]
	if not k.get("alive", true) or not k.get("in_party", false):
		return
	k.rivalry = clampi(k.rivalry + delta, RIVALRY_MIN, RIVALRY_MAX)
	kaelen_rivalry_changed.emit(k.rivalry)

	if k.rivalry >= RIVALRY_BREAK_THRESHOLD:
		_kaelen_breaks_away(true)
	elif k.rivalry <= FRIENDSHIP_ROMANCE_THRESHOLD and not k.romance_locked_out:
		k.romance_active = true


## Kaelen abandona la party. became_enemy=true si fue por rivalidad extrema
## (puede reaparecer más adelante como antagonista); false si fue por una
## ruptura de otro tipo (decisión narrativa concreta).
func _kaelen_breaks_away(became_enemy: bool) -> void:
	var k: Dictionary = companions["kaelen"]
	if not k.get("in_party", false):
		return
	k.in_party = false
	k.romance_active = false
	k.romance_locked_out = true
	kaelen_left_party.emit(became_enemy)


## Actualiza la relación de Yara con Kaelen (independiente de la del
## jugador) y recalcula quién es el objetivo de su romance.
func change_yara_kaelen_approval(delta: int) -> void:
	var y: Dictionary = companions["yara"]
	if not y.get("alive", true):
		return
	y.kaelen_approval = clampi(y.kaelen_approval + delta, APPROVAL_MIN, APPROVAL_MAX)
	_recalculate_yara_romance_target()


func _recalculate_yara_romance_target() -> void:
	var y: Dictionary = companions["yara"]
	var k: Dictionary = companions["kaelen"]

	var player_wants := y.approval >= ROMANCE_THRESHOLD
	var kaelen_wants := y.kaelen_approval >= ROMANCE_THRESHOLD and k.get("in_party", false)
	## El trío consentido requiere además una ruta de amistad alta entre
	## jugador y Kaelen (no tendría sentido con rivalidad de por medio).
	var trio_possible := player_wants and kaelen_wants and k.rivalry <= FRIENDSHIP_ROMANCE_THRESHOLD

	var new_target := "none"
	if trio_possible:
		new_target = "both"
	elif player_wants:
		new_target = "player"
	elif kaelen_wants:
		new_target = "kaelen"

	if new_target != y.romance_target:
		y.romance_target = new_target
		y.romance_active = new_target == "player" or new_target == "both"
		yara_romance_target_changed.emit(new_target)


## Mueve la corrupción de Yara. delta negativo = más luz/curandera,
## delta positivo = más bruja oscura.
func change_yara_corruption(delta: int) -> void:
	var y: Dictionary = companions["yara"]
	if not y.get("alive", true):
		return
	y.corruption = clampi(y.corruption + delta, -100, 100)
	yara_corruption_changed.emit(y.corruption)


func kill_companion(companion_id: String) -> void:
	if not companions.has(companion_id):
		return
	var c: Dictionary = companions[companion_id]
	c.alive = false
	c.in_party = false
	c.romance_active = false
	companion_died.emit(companion_id)


func remove_from_party(companion_id: String) -> void:
	if not companions.has(companion_id):
		return
	companions[companion_id].in_party = false
	companions[companion_id].romance_locked_out = true
	companions[companion_id].romance_active = false


func change_reputation(faction_id: String, delta: int) -> void:
	if not faction_reputation.has(faction_id):
		push_warning("Facción desconocida: %s" % faction_id)
		return
	faction_reputation[faction_id] = clampi(faction_reputation[faction_id] + delta, -100, 100)
	faction_reputation_changed.emit(faction_id, faction_reputation[faction_id])


## --- Combate ------------------------------------------------------------

var pending_encounter := {}
var combat_return_scene := ""


func start_encounter(enemy: Dictionary, return_scene: String) -> void:
	pending_encounter = enemy
	combat_return_scene = return_scene
	get_tree().change_scene_to_file("res://scenes/Combat.tscn")
