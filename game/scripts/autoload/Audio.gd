extends Node
## Música con fundido cruzado y efectos de sonido.

const MUSIC_DIR := "res://assets/music/"
const SFX_DIR := "res://assets/sfx/"

var _music_a: AudioStreamPlayer
var _music_b: AudioStreamPlayer
var _current: AudioStreamPlayer
var _current_name := ""
var _sfx_pool: Array[AudioStreamPlayer] = []
var _cache := {}
var music_volume_db := -6.0

# --- Opciones (se guardan en user://settings.cfg) ---
const SETTINGS_PATH := "user://settings.cfg"
var music_level := 8          # 0..10
var sfx_level := 8            # 0..10
var text_speed := 1           # 0 lento, 1 normal, 2 rápido
var battle_speed := 1         # 0 lento, 1 normal, 2 rápido
var fullscreen := false


func _ready() -> void:
	process_mode = Node.PROCESS_MODE_ALWAYS
	_music_a = AudioStreamPlayer.new()
	_music_b = AudioStreamPlayer.new()
	for p in [_music_a, _music_b]:
		p.volume_db = -80.0
		if OS.has_feature("web"):
			# En Safari/iPhone el modo «sample» del audio web no para bien las pistas largas y
			# acaba superponiendo músicas: la música va por el mezclador normal de Godot.
			p.playback_type = AudioServer.PLAYBACK_TYPE_STREAM
		add_child(p)
	_current = _music_a
	for i in 8:
		var s := AudioStreamPlayer.new()
		s.volume_db = -4.0
		add_child(s)
		_sfx_pool.append(s)
	load_settings()


func _level_db(l: int) -> float:
	return -80.0 if l <= 0 else linear_to_db(l / 10.0)


func load_settings() -> void:
	var cf := ConfigFile.new()
	if cf.load(SETTINGS_PATH) == OK:
		music_level = int(cf.get_value("audio", "music", music_level))
		sfx_level = int(cf.get_value("audio", "sfx", sfx_level))
		text_speed = int(cf.get_value("game", "text_speed", text_speed))
		battle_speed = int(cf.get_value("game", "battle_speed", battle_speed))
		fullscreen = bool(cf.get_value("video", "fullscreen", fullscreen))
	apply_settings()


func save_settings() -> void:
	var cf := ConfigFile.new()
	cf.set_value("audio", "music", music_level)
	cf.set_value("audio", "sfx", sfx_level)
	cf.set_value("game", "text_speed", text_speed)
	cf.set_value("game", "battle_speed", battle_speed)
	cf.set_value("video", "fullscreen", fullscreen)
	cf.save(SETTINGS_PATH)


func apply_settings() -> void:
	music_volume_db = -6.0 + _level_db(music_level)
	if _current and _current.playing:
		_current.volume_db = music_volume_db
	if not DisplayServer.get_name() == "headless":
		DisplayServer.window_set_mode(DisplayServer.WINDOW_MODE_FULLSCREEN if fullscreen else DisplayServer.WINDOW_MODE_WINDOWED)


func text_speed_mult() -> float:
	return [0.6, 1.0, 1.8][clampi(text_speed, 0, 2)]


func battle_speed_mult() -> float:
	return [0.75, 1.0, 1.4][clampi(battle_speed, 0, 2)]


func _load(path: String, loop: bool) -> AudioStream:
	if _cache.has(path):
		return _cache[path]
	if not ResourceLoader.exists(path):
		return null
	var stream = load(path)
	if stream is AudioStreamOggVorbis:
		(stream as AudioStreamOggVorbis).loop = loop
	elif stream is AudioStreamWAV and loop:
		var wav := stream as AudioStreamWAV
		wav.loop_mode = AudioStreamWAV.LOOP_FORWARD
		wav.loop_begin = 0
		wav.loop_end = wav.data.size() / 2
	_cache[path] = stream
	return stream


var _music_tw: Tween


## Corta el fundido en curso. Sin esto, dos cambios de música seguidos (frecuentes en móviles
## lentos) dejaban tweens pisándose y una pista vieja sonando a la vez que la nueva.
func _kill_music_tween() -> void:
	if _music_tw and _music_tw.is_valid():
		_music_tw.kill()
	_music_tw = null


func _stop_others(keep: AudioStreamPlayer) -> void:
	for p in [_music_a, _music_b]:
		if p != keep:
			p.stop()


func play_music(track: String, fade: float = 0.8) -> void:
	if track == _current_name:
		return
	if OS.has_feature("web"):
		_play_music_web(track, fade)
		return
	_current_name = track
	_kill_music_tween()
	var old := _current
	var nxt := _music_b if _current == _music_a else _music_a
	var loop := track != "victory"
	var stream := _load(MUSIC_DIR + track + ".ogg", loop)
	if stream == null:
		stream = _load(MUSIC_DIR + track + ".wav", loop)
	if stream == null:
		return
	nxt.stop()
	nxt.stream = stream
	nxt.volume_db = -40.0
	nxt.play()
	var tw := create_tween().set_parallel(true)
	tw.tween_property(nxt, "volume_db", music_volume_db, fade)
	if old.playing:
		tw.tween_property(old, "volume_db", -60.0, fade)
	tw.chain().tween_callback(_stop_others.bind(nxt))
	_music_tw = tw
	_current = nxt


## Navegador (sobre todo Safari en iPhone): el audio web de Godot 4.3 da problemas al cruzar
## dos reproductores o cambiar la pista de uno que suena, y quedaban músicas superpuestas.
## Aquí solo hay UN reproductor: se para del todo, se cambia la pista y entra con un fundido.
func _play_music_web(track: String, fade: float) -> void:
	var loop := track != "victory"
	var stream := _load(MUSIC_DIR + track + ".ogg", loop)
	if stream == null:
		stream = _load(MUSIC_DIR + track + ".wav", loop)
	if stream == null:
		return
	_current_name = track
	_kill_music_tween()
	_music_b.stop()
	_music_b.stream = null
	_music_a.stop()
	_music_a.stream = stream
	_music_a.volume_db = -30.0
	_music_a.play()
	_current = _music_a
	_music_tw = create_tween()
	_music_tw.tween_property(_music_a, "volume_db", music_volume_db, maxf(0.2, fade * 0.6))


func stop_music(fade: float = 0.8) -> void:
	_current_name = ""
	_kill_music_tween()
	if OS.has_feature("web"):
		_music_b.stop()
	var p := _current
	var tw := create_tween()
	tw.tween_property(p, "volume_db", -60.0, fade)
	tw.tween_callback(_stop_others.bind(null))
	_music_tw = tw


func sfx(sound: String, volume_db: float = -4.0, pitch: float = 1.0) -> void:
	var stream := _load(SFX_DIR + sound + ".wav", false)
	if stream == null:
		return
	for p in _sfx_pool:
		if not p.playing:
			p.stream = stream
			p.volume_db = volume_db + _level_db(sfx_level)
			p.pitch_scale = pitch
			p.play()
			return
	_sfx_pool[0].stream = stream
	_sfx_pool[0].play()
