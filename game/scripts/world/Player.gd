extends CharacterBody2D
## Jugador: movimiento 4 direcciones, animación, interacción y rastro para
## que el grupo le siga.

const WALK_SPEED := 72.0
const RUN_SPEED := 118.0
const DIR_VEC := [Vector2.DOWN, Vector2.LEFT, Vector2.RIGHT, Vector2.UP]

var facing := 0
var sprite: Sprite2D
var world: Node
var trail: Array[Vector2] = []
var _anim_t := 0.0
var _step_count := 0
var locked := false


func setup(w: Node, pos: Vector2, face: int = 0) -> void:
	world = w
	position = pos
	facing = face
	add_to_group("player")
	collision_layer = 1
	collision_mask = 1
	motion_mode = CharacterBody2D.MOTION_MODE_FLOATING
	var shape := CollisionShape2D.new()
	var rect := RectangleShape2D.new()
	rect.size = Vector2(10, 6)
	shape.shape = rect
	shape.position = Vector2(0, -3)
	add_child(shape)
	sprite = Sprite2D.new()
	sprite.texture = Appearance.sheet()
	sprite.hframes = 8
	sprite.vframes = 4
	sprite.centered = false
	sprite.offset = Vector2(-10, -29)
	add_child(sprite)
	reset_trail()
	_update_frame(-1)


## Rellena el rastro en línea recta detrás del jugador (para que los compañeros
## aparezcan escalonados detrás y no amontonados en el mismo punto).
func reset_trail() -> void:
	trail.clear()
	var back: Vector2 = [Vector2(0, -1), Vector2(1, 0), Vector2(-1, 0), Vector2(0, 1)][clampi(facing, 0, 3)]
	for i in range(80, 0, -1):
		trail.append(position + back * i * 1.5)
	trail.append(position)


func _physics_process(delta: float) -> void:
	var can_move: bool = not locked and not Dialogue.active and not Transition.busy and not world.cutscene
	var input := Vector2.ZERO
	if can_move:
		input = Vector2(
			Input.get_action_strength("move_right") - Input.get_action_strength("move_left"),
			Input.get_action_strength("move_down") - Input.get_action_strength("move_up"))
	if input.length() > 0.2:
		if absf(input.x) > absf(input.y):
			facing = 2 if input.x > 0 else 1
		else:
			facing = 0 if input.y > 0 else 3
		var running := Input.is_action_pressed("run")
		velocity = input.normalized() * (RUN_SPEED if running else WALK_SPEED)
		var before := position
		move_and_slide()
		var moved := position.distance_to(before)
		if moved > 0.05:
			world.on_player_moved(moved)
			_anim_t += delta * (11.0 if running else 7.5)
			_record_trail()
		_update_frame(2 + int(_anim_t * 1.5) % 6)
		var step_now := int(_anim_t * 1.5 / 3.0)
		if step_now != _step_count and moved > 0.05:
			_step_count = step_now
			Audio.sfx("step", -20.0, randf_range(0.9, 1.1))
	else:
		velocity = Vector2.ZERO
		_anim_t = 0.0
		_update_frame(-1)
	var fresh_press: bool = Engine.get_process_frames() - Dialogue.last_close_frame > 8
	if can_move and fresh_press and Input.is_action_just_pressed("interact"):
		world.try_interact(self)


func walk_to(target: Vector2, speed: float = 60.0, direct: bool = false) -> void:
	locked = true
	var pts := PackedVector2Array([target])
	if not direct and world and world.has_method("find_path") and position.distance_to(target) > 20.0:
		pts = world.find_path(position, target)
	for goal in pts:
		while position.distance_to(goal) > 1.0:
			var d := goal - position
			if absf(d.x) > absf(d.y):
				facing = 2 if d.x > 0 else 1
			else:
				facing = 0 if d.y > 0 else 3
			var step := d.normalized() * speed * get_physics_process_delta_time()
			if step.length() > d.length():
				step = d
			position += step
			_record_trail()
			_anim_t += get_physics_process_delta_time() * 7.5
			_update_frame(2 + int(_anim_t * 1.5) % 6)
			await get_tree().physics_frame
	_update_frame(-1)
	locked = false


func face(dir: int) -> void:
	facing = dir
	_update_frame(-1)


func probe_point() -> Vector2:
	return position + DIR_VEC[facing] * 11.0 + Vector2(0, -4)


func _record_trail() -> void:
	if trail.is_empty() or trail[trail.size() - 1].distance_to(position) >= 1.5:
		trail.append(position)
		if trail.size() > 120:
			trail.pop_front()


## f = columna de la hoja (2..7 caminar); -1 = reposo con respiración.
func _update_frame(f: int) -> void:
	if f < 0:
		f = int(Time.get_ticks_msec() / 700.0) % 2
	sprite.frame = facing * 8 + f
