extends CharacterBody2D

@export var speed: float = 120.0

@onready var sprite: Sprite2D = $Sprite2D
@onready var interact_area: Area2D = $InteractArea

var _interactables: Array[Node] = []


func _ready() -> void:
	if sprite.texture == null:
		sprite.texture = _make_placeholder_texture(Color(0.3, 0.55, 0.9))


func _make_placeholder_texture(color: Color) -> ImageTexture:
	var img := Image.create(16, 24, false, Image.FORMAT_RGBA8)
	img.fill(color)
	return ImageTexture.create_from_image(img)


func _physics_process(_delta: float) -> void:
	var input_dir := Vector2(
		Input.get_action_strength("move_right") - Input.get_action_strength("move_left"),
		Input.get_action_strength("move_down") - Input.get_action_strength("move_up")
	)
	velocity = input_dir.normalized() * speed if input_dir != Vector2.ZERO else Vector2.ZERO
	move_and_slide()

	if input_dir.x != 0:
		sprite.flip_h = input_dir.x < 0

	if Input.is_action_just_pressed("interact") and not _interactables.is_empty():
		var target := _interactables[0]
		if target.has_method("interact"):
			target.interact()


func _on_interact_area_area_entered(area: Area2D) -> void:
	if area.is_in_group("interactable"):
		_interactables.append(area)


func _on_interact_area_area_exited(area: Area2D) -> void:
	_interactables.erase(area)
