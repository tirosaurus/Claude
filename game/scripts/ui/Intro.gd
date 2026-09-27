extends Control

const LINES := [
	"Hace trescientos años, el último Archimago del Imperio de Cristal quiso convertirse en dios.",
	"Fracasó. Y al caer, rasgó el velo que separa los mundos.",
	"Desde entonces, cosas antiguas despiertan en los rincones olvidados de Vaelmoor.",
	"Pero en los montes del norte, lejos de todo, hay un pueblo al que esas historias nunca han llegado.",
]

var _label: Label
var _skip := false
var _next := false


func _process(_delta: float) -> void:
	if Input.is_action_just_pressed("cancel") or Input.is_action_just_pressed("pause"):
		_skip = true
	if Input.is_action_just_pressed("interact"):
		_next = true


func _ready() -> void:
	var bg := ColorRect.new()
	bg.color = Color(0.05, 0.04, 0.07)
	bg.set_anchors_preset(Control.PRESET_FULL_RECT)
	add_child(bg)
	_label = UIKit.label("", 20, Color(0.92, 0.88, 0.8), 4)
	_label.position = Vector2(70, 130)
	_label.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	_label.size = Vector2(500, 100)
	_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	_label.vertical_alignment = VERTICAL_ALIGNMENT_CENTER
	add_child(_label)
	var hint := UIKit.label("E: continuar   ·   X: saltar", 12, Color(0.5, 0.48, 0.55))
	hint.position = Vector2(470, 334)
	add_child(hint)
	Audio.play_music("title", 1.0)
	_run.call_deferred()


func _run() -> void:
	await get_tree().create_timer(0.6).timeout
	for line in LINES:
		if _skip:
			break
		await _show(line, 20)
	if not _skip:
		await _show("Tortosa.", 40)
	Audio.stop_music(1.2)
	GameState.current_map = "bedroom"
	GameState.current_spawn = "start"
	Transition.go_to_scene("res://scenes/World.tscn", 1.0)


func _show(text: String, size: int) -> void:
	_label.text = text
	_label.add_theme_font_size_override("font_size", size)
	_label.modulate.a = 0.0
	var tw := create_tween()
	tw.tween_property(_label, "modulate:a", 1.0, 0.8)
	while tw.is_running() and not _skip:
		await get_tree().process_frame
	var t := 0.0
	_next = false
	while t < 3.4 and not _next and not _skip:
		await get_tree().process_frame
		t += get_process_delta_time()
	var tw2 := create_tween()
	tw2.tween_property(_label, "modulate:a", 0.0, 0.6)
	await tw2.finished
