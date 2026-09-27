extends Node2D
## Personaje no jugador: puede estar quieto, pasear, seguir al jugador o
## moverse por guion en escenas.

enum Mode { IDLE, WANDER, FOLLOW }

const DIR_VEC := [Vector2.DOWN, Vector2.LEFT, Vector2.RIGHT, Vector2.UP]

var char_id := ""
var mode := Mode.IDLE
var facing := 0
var sprite: Sprite2D
var body: StaticBody2D
var follow_target: Node = null
var follow_gap := 18
var home := Vector2.ZERO
var wander_radius := 40.0
var busy := false
var _anim_t := 0.0
var _wander_target := Vector2.ZERO
var _wander_wait := 1.0
var _last_pos := Vector2.ZERO


func setup(id: String, pos: Vector2, face: int = 0, m: int = Mode.IDLE) -> void:
	char_id = id
	position = pos
	home = pos
	facing = face
	sprite = Sprite2D.new()
	sprite.texture = load("res://assets/chars/%s.png" % id)
	sprite.hframes = 3
	sprite.vframes = 4
	sprite.centered = false
	sprite.offset = Vector2(-8, -23)
	add_child(sprite)
	body = StaticBody2D.new()
	var shape := CollisionShape2D.new()
	var rect := RectangleShape2D.new()
	rect.size = Vector2(10, 6)
	shape.shape = rect
	shape.position = Vector2(0, -3)
	body.add_child(shape)
	add_child(body)
	set_mode(m)
	_last_pos = position
	_frame(0)


func set_mode(m: int) -> void:
	mode = m
	if body:
		body.process_mode = Node.PROCESS_MODE_DISABLED if m == Mode.FOLLOW else Node.PROCESS_MODE_INHERIT
		body.collision_layer = 0 if m == Mode.FOLLOW else 1


func _physics_process(delta: float) -> void:
	if busy:
		return
	match mode:
		Mode.FOLLOW:
			_follow(delta)
		Mode.WANDER:
			_wander(delta)


func _follow(delta: float) -> void:
	if follow_target == null:
		return
	var trail: Array = follow_target.trail
	var idx: int = max(0, trail.size() - 1 - follow_gap)
	var target: Vector2 = trail[idx]
	var d := target - position
	if d.length() > 0.5:
		position = position.lerp(target, clampf(delta * 14.0, 0, 1))
		_face_dir(d)
		_anim_t += delta * 7.5
		_frame([1, 0, 2, 0][int(_anim_t) % 4])
	else:
		_anim_t = 0
		_frame(0)


func _wander(delta: float) -> void:
	if Dialogue.active:
		_frame(0)
		return
	if _wander_wait > 0:
		_wander_wait -= delta
		_frame(0)
		if _wander_wait <= 0:
			_wander_target = home + Vector2(randf_range(-1, 1), randf_range(-1, 1)) * wander_radius
		return
	var d := _wander_target - position
	if d.length() < 1.5:
		_wander_wait = randf_range(1.2, 3.5)
		return
	var step := d.normalized() * 34.0 * delta
	var col := body.move_and_collide(step, true) if body else null
	if col:
		_wander_wait = randf_range(0.5, 1.5)
		return
	position += step
	_face_dir(d)
	_anim_t += delta * 6.0
	_frame([1, 0, 2, 0][int(_anim_t) % 4])


func walk_to(target: Vector2, speed: float = 60.0) -> void:
	busy = true
	while position.distance_to(target) > 1.0:
		var d := target - position
		_face_dir(d)
		var step := d.normalized() * speed * get_physics_process_delta_time()
		if step.length() > d.length():
			step = d
		position += step
		_anim_t += get_physics_process_delta_time() * 7.5
		_frame([1, 0, 2, 0][int(_anim_t) % 4])
		await get_tree().physics_frame
	position = target
	_frame(0)
	busy = false


func face(dir: int) -> void:
	facing = dir
	_frame(0)


func face_towards(p: Vector2) -> void:
	_face_dir(p - position)
	_frame(0)


func hop() -> void:
	var tw := create_tween()
	tw.tween_property(sprite, "position:y", -5.0, 0.1)
	tw.tween_property(sprite, "position:y", 0.0, 0.1)
	await tw.finished


func _face_dir(d: Vector2) -> void:
	if d.length() < 0.01:
		return
	if absf(d.x) > absf(d.y):
		facing = 2 if d.x > 0 else 1
	else:
		facing = 0 if d.y > 0 else 3


func _frame(f: int) -> void:
	sprite.frame = facing * 3 + f
