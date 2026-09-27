extends CanvasLayer
## Fundidos a negro entre escenas y mapas, y destellos para el combate.

var _rect: ColorRect
var busy := false


func _ready() -> void:
	layer = 100
	process_mode = Node.PROCESS_MODE_ALWAYS
	_rect = ColorRect.new()
	_rect.color = Color(0.07, 0.055, 0.086, 0.0)
	_rect.mouse_filter = Control.MOUSE_FILTER_IGNORE
	_rect.set_anchors_preset(Control.PRESET_FULL_RECT)
	add_child(_rect)


func fade_out(duration: float = 0.35, color: Color = Color(0.07, 0.055, 0.086)) -> void:
	_rect.color = Color(color.r, color.g, color.b, _rect.color.a)
	var tw := create_tween()
	tw.tween_property(_rect, "color:a", 1.0, duration)
	await tw.finished


func fade_in(duration: float = 0.35) -> void:
	var tw := create_tween()
	tw.tween_property(_rect, "color:a", 0.0, duration)
	await tw.finished


func go_to_scene(path: String, duration: float = 0.4) -> void:
	if busy:
		return
	busy = true
	await fade_out(duration)
	get_tree().change_scene_to_file(path)
	await get_tree().process_frame
	await get_tree().process_frame
	busy = false
	await fade_in(duration)


func go_to_map(map_id: String, spawn: String, door_sound: bool = false) -> void:
	if busy:
		return
	GameState.current_map = map_id
	GameState.current_spawn = spawn
	if door_sound:
		Audio.sfx("door", -6.0)
	await go_to_scene("res://scenes/World.tscn", 0.3)


func battle_flash() -> void:
	busy = true
	Audio.sfx("battle_start", -2.0)
	for i in 3:
		_rect.color = Color(1, 1, 1, 0.0)
		var tw := create_tween()
		tw.tween_property(_rect, "color:a", 0.85, 0.07)
		tw.tween_property(_rect, "color:a", 0.0, 0.07)
		await tw.finished
	await fade_out(0.35, Color(0, 0, 0))
	get_tree().change_scene_to_file("res://scenes/Battle.tscn")
	await get_tree().process_frame
	await get_tree().process_frame
	busy = false
	await fade_in(0.4)
