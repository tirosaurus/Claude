extends Node
## Autoload global. Guarda el estado persistente de la partida:
## datos del personaje, reputación de facciones y vínculos con compañeros.

signal companion_approval_changed(companion_id: String, new_value: int)
signal companion_died(companion_id: String)
signal faction_reputation_changed(faction_id: String, new_value: int)

var player_data := {
	"name": "",
	"race": "human",
	"clazz": "warrior",
	"background": "",
}

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
	},
	"seraphine": {
		"display_name": "Seraphine",
		"approval": 0,
		"alive": true,
		"romance_active": false,
		"romance_locked_out": false,
		"in_party": true,
	},
}

const APPROVAL_MIN := -100
const APPROVAL_MAX := 100
const ROMANCE_THRESHOLD := 60


func change_approval(companion_id: String, delta: int) -> void:
	if not companions.has(companion_id):
		push_warning("Compañero desconocido: %s" % companion_id)
		return
	var c: Dictionary = companions[companion_id]
	if not c.get("alive", true):
		return
	c.approval = clampi(c.approval + delta, APPROVAL_MIN, APPROVAL_MAX)
	companion_approval_changed.emit(companion_id, c.approval)
	if c.approval >= ROMANCE_THRESHOLD and not c.romance_locked_out:
		c.romance_active = true


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
