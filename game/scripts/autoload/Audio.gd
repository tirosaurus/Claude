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
var music_volume_db := -8.0


func _ready() -> void:
	process_mode = Node.PROCESS_MODE_ALWAYS
	_music_a = AudioStreamPlayer.new()
	_music_b = AudioStreamPlayer.new()
	for p in [_music_a, _music_b]:
		p.volume_db = -80.0
		add_child(p)
	_current = _music_a
	for i in 8:
		var s := AudioStreamPlayer.new()
		s.volume_db = -4.0
		add_child(s)
		_sfx_pool.append(s)


func _load(path: String, loop: bool) -> AudioStream:
	if _cache.has(path):
		return _cache[path]
	if not ResourceLoader.exists(path):
		return null
	var stream = load(path)
	if stream is AudioStreamWAV and loop:
		var wav := stream as AudioStreamWAV
		wav.loop_mode = AudioStreamWAV.LOOP_FORWARD
		wav.loop_begin = 0
		wav.loop_end = wav.data.size() / 2
	_cache[path] = stream
	return stream


func play_music(track: String, fade: float = 0.8) -> void:
	if track == _current_name:
		return
	_current_name = track
	var old := _current
	var nxt := _music_b if _current == _music_a else _music_a
	var loop := track != "victory"
	var stream := _load(MUSIC_DIR + track + ".wav", loop)
	if stream == null:
		return
	nxt.stream = stream
	nxt.volume_db = -40.0
	nxt.play()
	var tw := create_tween().set_parallel(true)
	tw.tween_property(nxt, "volume_db", music_volume_db, fade)
	tw.tween_property(old, "volume_db", -60.0, fade)
	tw.chain().tween_callback(old.stop)
	_current = nxt


func stop_music(fade: float = 0.8) -> void:
	_current_name = ""
	var p := _current
	var tw := create_tween()
	tw.tween_property(p, "volume_db", -60.0, fade)
	tw.tween_callback(p.stop)


func sfx(sound: String, volume_db: float = -4.0, pitch: float = 1.0) -> void:
	var stream := _load(SFX_DIR + sound + ".wav", false)
	if stream == null:
		return
	for p in _sfx_pool:
		if not p.playing:
			p.stream = stream
			p.volume_db = volume_db
			p.pitch_scale = pitch
			p.play()
			return
	_sfx_pool[0].stream = stream
	_sfx_pool[0].play()
