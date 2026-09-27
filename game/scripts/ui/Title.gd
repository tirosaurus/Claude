extends Control

var _logo: Label
var _t := 0.0


func _ready() -> void:
	var bg := TextureRect.new()
	bg.texture = load("res://assets/bg/title.png")
	bg.size = Vector2(640, 360)
	bg.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
	bg.stretch_mode = TextureRect.STRETCH_SCALE
	add_child(bg)

	var flies := CPUParticles2D.new()
	flies.position = Vector2(320, 300)
	flies.amount = 40
	flies.lifetime = 7.0
	flies.emission_shape = CPUParticles2D.EMISSION_SHAPE_RECTANGLE
	flies.emission_rect_extents = Vector2(320, 60)
	flies.gravity = Vector2(0, -4)
	flies.initial_velocity_min = 2
	flies.initial_velocity_max = 8
	flies.spread = 180
	flies.scale_amount_min = 1.5
	flies.scale_amount_max = 2.5
	var g := Gradient.new()
	g.set_color(0, Color(1, 0.9, 0.5, 0))
	g.add_point(0.5, Color(1, 0.9, 0.5, 1))
	g.set_color(g.get_point_count() - 1, Color(1, 0.9, 0.5, 0))
	flies.color_ramp = g
	add_child(flies)

	_logo = UIKit.label("VAELMOOR", 56, Color(0.98, 0.86, 0.55), 10)
	_logo.position = Vector2(0, 46)
	_logo.size = Vector2(640, 70)
	_logo.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	add_child(_logo)
	var sub := UIKit.label("Crónicas del Velo Roto", 18, Color(0.85, 0.8, 0.95), 5)
	sub.position = Vector2(0, 112)
	sub.size = Vector2(640, 24)
	sub.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	add_child(sub)

	var box := VBoxContainer.new()
	box.position = Vector2(240, 214)
	box.size = Vector2(160, 80)
	box.add_theme_constant_override("separation", 8)
	add_child(box)
	var start := UIKit.button("Nueva partida", 18)
	start.pressed.connect(_on_start)
	box.add_child(start)
	var quit := UIKit.button("Salir", 18)
	quit.pressed.connect(func(): get_tree().quit())
	box.add_child(quit)
	start.grab_focus()

	var hint := UIKit.label("Demo · WASD/flechas para moverse · E para interactuar · Mayús para correr", 12, Color(0.8, 0.78, 0.9), 3)
	hint.position = Vector2(0, 336)
	hint.size = Vector2(640, 20)
	hint.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	add_child(hint)
	Audio.play_music("title", 1.0)


func _process(delta: float) -> void:
	_t += delta
	_logo.position.y = 46 + sin(_t * 1.4) * 3.0


func _on_start() -> void:
	Audio.sfx("confirm", -6.0)
	Transition.go_to_scene("res://scenes/CharacterCreation.tscn")
