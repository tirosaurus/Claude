extends Node2D
## Un combatiente (miembro del grupo o enemigo).

var id := ""
var key := ""
var is_enemy := false
var display_name := ""
var max_stats := {}
var hp := 0
var mp := 0
var statuses := {}
var atb := 0.0
var sprite: Sprite2D
var home := Vector2.ZERO
var data := {}
var frames := 0
var anim_t := 0.0
var turns := 0
var flags := {}
var hp_bar: ProgressBar
var party_anim := false
var _pose_t := 0.0
var victory := false


func alive() -> bool:
	return hp > 0


func stat(k: String) -> float:
	var v := float(max_stats.get(k, 0))
	if k == "atk":
		if statuses.has("rage"):
			v *= 1.35
		if statuses.has("weak"):
			v *= 0.7
	if k == "def":
		if statuses.has("protect"):
			v *= 1.5
		if statuses.has("rage"):
			v *= 0.8
	if k == "res" and statuses.has("protect"):
		v *= 1.3
	if k == "mag" and statuses.has("weak"):
		v *= 0.8
	return v


func max_hp() -> int:
	return int(max_stats["hp"])


func max_mp() -> int:
	return int(max_stats.get("mp", 0))


func set_idle_frame() -> void:
	if sprite == null:
		return
	if is_enemy:
		if frames == 0:
			sprite.frame = 6
		elif frames == -1:
			sprite.frame = int(anim_t * 1.6) % 2
		return
	if _pose_t > 0.0:
		return
	if not alive():
		sprite.frame = 7
	elif victory:
		sprite.frame = 8
	elif float(hp) / float(max_hp()) <= 0.25:
		sprite.frame = 6
	else:
		sprite.frame = int(anim_t * 1.6) % 2


## Muestra una pose durante un tiempo (frames: 2 carga, 3 golpe, 4 magia, 5 herido).
func pose(f: int, t: float) -> void:
	if sprite == null or (not is_enemy and not alive() and f != 7):
		return
	if is_enemy and frames != -1:
		return
	sprite.frame = f
	_pose_t = t


func tick_anim(delta: float) -> void:
	anim_t += delta
	if _pose_t > 0.0:
		_pose_t -= delta
		if _pose_t <= 0.0:
			_pose_t = 0.0
			set_idle_frame()
		return
	if is_enemy and frames > 1 and alive():
		sprite.frame = int(anim_t * 2.0) % frames
	elif (party_anim or frames == -1) and alive():
		set_idle_frame()
