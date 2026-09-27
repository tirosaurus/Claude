extends Control
## Combate por turnos muy básico: Atacar / Curar / Defender contra un
## único enemigo. Suficiente para el prototipo; se ampliará con
## habilidades por rama más adelante.

@onready var enemy_name_label: Label = $VBox/EnemyNameLabel
@onready var enemy_hp_bar: ProgressBar = $VBox/EnemyHpBar
@onready var player_hp_bar: ProgressBar = $VBox/PlayerHpBar
@onready var player_mana_bar: ProgressBar = $VBox/PlayerManaBar
@onready var log_label: Label = $VBox/LogLabel
@onready var attack_button: Button = $VBox/Buttons/AttackButton
@onready var heal_button: Button = $VBox/Buttons/HealButton
@onready var defend_button: Button = $VBox/Buttons/DefendButton

const HEAL_MANA_COST := 5

var enemy_hp: int
var enemy_max_hp: int
var enemy_attack: int
var enemy_name: String
var xp_reward: int
var _player_defending := false
var _battle_over := false


func _ready() -> void:
	var enemy: Dictionary = GameState.pending_encounter
	enemy_name = enemy.get("name", "Enemigo")
	enemy_max_hp = enemy.get("max_hp", 30)
	enemy_hp = enemy_max_hp
	enemy_attack = enemy.get("attack", 5)
	xp_reward = enemy.get("xp_reward", 40)

	enemy_name_label.text = enemy_name
	attack_button.pressed.connect(_on_attack_pressed)
	heal_button.pressed.connect(_on_heal_pressed)
	defend_button.pressed.connect(_on_defend_pressed)

	_refresh_bars()
	_log("Un %s aparece. ¡Prepárate!" % enemy_name)


func _refresh_bars() -> void:
	var stats: Dictionary = GameState.player_stats
	enemy_hp_bar.max_value = enemy_max_hp
	enemy_hp_bar.value = enemy_hp
	player_hp_bar.max_value = stats.max_hp
	player_hp_bar.value = stats.hp
	player_mana_bar.max_value = stats.max_mana
	player_mana_bar.value = stats.mana


func _log(text: String) -> void:
	log_label.text = text


func _on_attack_pressed() -> void:
	if _battle_over:
		return
	var dmg := randi_range(7, 12)
	enemy_hp = maxi(0, enemy_hp - dmg)
	_log("Atacas y haces %d de daño." % dmg)
	_refresh_bars()
	if enemy_hp <= 0:
		_win()
		return
	_enemy_turn()


func _on_heal_pressed() -> void:
	if _battle_over:
		return
	var stats: Dictionary = GameState.player_stats
	if stats.mana < HEAL_MANA_COST:
		_log("No tienes maná suficiente para curarte.")
		return
	stats.mana -= HEAL_MANA_COST
	var healed := 10
	stats.hp = mini(stats.max_hp, stats.hp + healed)
	_log("Te curas %d de vida." % healed)
	_refresh_bars()
	_enemy_turn()


func _on_defend_pressed() -> void:
	if _battle_over:
		return
	_player_defending = true
	_log("Te preparas para reducir el próximo golpe.")
	_enemy_turn()


func _enemy_turn() -> void:
	var stats: Dictionary = GameState.player_stats
	var dmg := enemy_attack
	if _player_defending:
		dmg = int(dmg / 2.0)
		_player_defending = false
	stats.hp = maxi(0, stats.hp - dmg)
	_log("%s te golpea por %d." % [enemy_name, dmg])
	_refresh_bars()
	if stats.hp <= 0:
		_lose()


func _win() -> void:
	_battle_over = true
	_set_buttons_enabled(false)
	_log("¡Has derrotado a %s! Ganas %d de experiencia." % [enemy_name, xp_reward])
	GameState.add_xp(xp_reward)
	var defeat_flag: String = GameState.pending_encounter.get("defeat_flag", "")
	if defeat_flag != "":
		GameState.set_flag(defeat_flag)

	if GameState.needs_branch_choice():
		var choice_scene := preload("res://scenes/BranchChoice.tscn")
		var choice := choice_scene.instantiate()
		add_child(choice)
		choice.branch_selected.connect(func(_b): _return_to_world())
	else:
		await get_tree().create_timer(1.2).timeout
		_return_to_world()


func _lose() -> void:
	_battle_over = true
	_set_buttons_enabled(false)
	_log("Caes derrotado, pero consigues escapar a tiempo...")
	GameState.player_stats.hp = GameState.player_stats.max_hp
	await get_tree().create_timer(1.5).timeout
	_return_to_world()


func _set_buttons_enabled(enabled: bool) -> void:
	attack_button.disabled = not enabled
	heal_button.disabled = not enabled
	defend_button.disabled = not enabled


func _return_to_world() -> void:
	get_tree().change_scene_to_file(GameState.combat_return_scene)
