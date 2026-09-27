extends Control

const SaveUIScript := preload("res://scripts/ui/SaveUI.gd")

var _logo: Label
var _t := 0.0
var _first: Button


func _ready() -> void:
	get_tree().paused = false
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
	_logo.position = Vector2(0, 40)
	_logo.size = Vector2(640, 70)
	_logo.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	add_child(_logo)
	var sub := UIKit.label("Crónicas del Velo Roto", 18, Color(0.85, 0.8, 0.95), 5)
	sub.position = Vector2(0, 106)
	sub.size = Vector2(640, 24)
	sub.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	add_child(sub)

	var box := VBoxContainer.new()
	box.position = Vector2(230, 180)
	box.size = Vector2(180, 150)
	box.add_theme_constant_override("separation", 6)
	add_child(box)
	var last := GameState.any_save()
	if last >= 0:
		var cont := UIKit.button("Continuar", 17)
		cont.pressed.connect(func():
			if GameState.load_game(last):
				Audio.sfx("confirm", -6.0)
				Transition.go_to_scene("res://scenes/World.tscn", 0.6))
		box.add_child(cont)
		_first = cont
	var start := UIKit.button("Nueva partida", 17)
	start.pressed.connect(_on_start)
	box.add_child(start)
	if _first == null:
		_first = start
	var load_b := UIKit.button("Cargar partida", 17)
	load_b.disabled = last < 0
	load_b.pressed.connect(_on_load)
	box.add_child(load_b)
	var quit := UIKit.button("Salir", 17)
	quit.pressed.connect(func(): get_tree().quit())
	box.add_child(quit)
	_first.grab_focus()

	var hint := UIKit.label("WASD/flechas: mover · E: interactuar · Mayús: correr · Esc: menú y guardar", 12, Color(0.8, 0.78, 0.9), 3)
	hint.position = Vector2(0, 338)
	hint.size = Vector2(640, 20)
	hint.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	add_child(hint)
	Audio.play_music("title", 1.0)


func _process(delta: float) -> void:
	_t += delta
	_logo.position.y = 40 + sin(_t * 1.4) * 3.0


func _on_start() -> void:
	Audio.sfx("confirm", -6.0)
	Transition.go_to_scene("res://scenes/CharacterCreation.tscn")


func _on_load() -> void:
	var s := SaveUIScript.new()
	s.setup("load")
	add_child(s)
	s.closed.connect(func(_r):
		if is_instance_valid(_first):
			_first.grab_focus())
