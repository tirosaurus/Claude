extends Node
## Base del guion de cada mapa: atajos de diálogo, interacciones comunes
## (cofres, hogueras, tiendas) y ganchos que World invoca.

const NPCScript := preload("res://scripts/world/NPC.gd")
const ShopScript := preload("res://scripts/ui/ShopUI.gd")
const SaveScript := preload("res://scripts/ui/SaveUI.gd")

var world: Node


# ------------------------------------------------------------ Atajos
func P(t: String) -> Dictionary:
	return {"who": "player", "text": t}


func K(t: String) -> Dictionary:
	return {"who": "kaelen", "text": t}


func Y(t: String) -> Dictionary:
	return {"who": "yara", "text": t}


func M(t: String) -> Dictionary:
	return {"who": "mother", "text": t}


func A(t: String) -> Dictionary:
	return {"who": "aelis", "text": t}


func B(t: String) -> Dictionary:
	return {"who": "brom", "text": t}


func N(who: String, t: String) -> Dictionary:
	return {"who": who, "text": t}


func say(lines: Array) -> void:
	await Dialogue.say(lines)


func choose(options: Array, prompt = null) -> int:
	return await Dialogue.choose(options, prompt)


# ------------------------------------------------------------ Cinemáticas
## Mueve la cámara a un punto y espera.
func pan_to(pos: Vector2, wait_s: float = 1.2) -> void:
	world.camera_focus = pos
	await wait(wait_s)


func release_camera(wait_s: float = 0.6) -> void:
	world.camera_focus = null
	await wait(wait_s)


func shake(amount: float = 4.0, t: float = 0.4) -> void:
	var tw := create_tween()
	var n := int(t / 0.04)
	for k in n:
		tw.tween_property(world.camera, "offset", Vector2(randf_range(-amount, amount), randf_range(-amount, amount)), 0.04)
	tw.tween_property(world.camera, "offset", Vector2.ZERO, 0.05)


## Sprite animado de monstruo para escenas (larvas, bruto, etc.).
func monster(path: String, frames: int, pos: Vector2, scale_f: float = 1.0, flip: bool = false) -> Sprite2D:
	var s := Sprite2D.new()
	s.texture = load(path)
	s.hframes = frames
	s.centered = false
	var fw: float = s.texture.get_width() / float(frames)
	s.offset = Vector2(-fw / 2.0, -s.texture.get_height())
	s.scale = Vector2(scale_f, scale_f)
	s.flip_h = flip
	s.position = pos
	world.entities.add_child(s)
	_animate(s, frames)
	return s


func _animate(s: Sprite2D, frames: int) -> void:
	var k := 0
	while is_instance_valid(s) and s.is_inside_tree():
		s.frame = k % frames
		k += 1
		await get_tree().create_timer(0.22).timeout


## Mueve un nodo (monstruo de escena) rodeando obstáculos; t = duración total aproximada.
func move_node(n: Node2D, to: Vector2, t: float, direct: bool = false) -> void:
	var pts := PackedVector2Array([to])
	if not direct and n.position.distance_to(to) > 24.0:
		pts = world.find_path(n.position, to)
	var total := 0.0
	var prev := n.position
	for p in pts:
		total += prev.distance_to(p)
		prev = p
	prev = n.position
	for p in pts:
		if not is_instance_valid(n):
			return
		var seg := prev.distance_to(p)
		var tw := create_tween()
		tw.tween_property(n, "position", p, maxf(0.05, t * seg / maxf(total, 1.0)))
		if n is Sprite2D and absf(p.x - prev.x) > 2.0:
			(n as Sprite2D).flip_h = p.x < prev.x if not n.has_meta("faces_left") else p.x > prev.x
		await tw.finished
		prev = p


## Un aldeano cae (herido) tumbado en el suelo.
func knock_down(n: Node2D) -> void:
	var spr: Sprite2D = n.sprite if "sprite" in n else n
	var tw := create_tween().set_parallel(true)
	tw.tween_property(spr, "rotation_degrees", 90.0, 0.25)
	tw.tween_property(spr, "modulate", Color(0.8, 0.6, 0.6), 0.25)
	Audio.sfx("hit", -6.0)
	if "busy" in n:
		n.busy = true


func dust(pos: Vector2, col: Color = Color(0.7, 0.65, 0.6)) -> void:
	var p := CPUParticles2D.new()
	p.position = pos
	p.amount = 30
	p.one_shot = true
	p.explosiveness = 0.9
	p.lifetime = 0.9
	p.emission_shape = CPUParticles2D.EMISSION_SHAPE_SPHERE
	p.emission_sphere_radius = 10
	p.direction = Vector2.UP
	p.spread = 80
	p.gravity = Vector2(0, 40)
	p.initial_velocity_min = 20
	p.initial_velocity_max = 60
	p.scale_amount_min = 1.5
	p.scale_amount_max = 3.0
	p.color = col
	world.entities.add_child(p)
	p.emitting = true
	get_tree().create_timer(1.6).timeout.connect(p.queue_free)


func begin() -> void:
	world.cutscene = true


func end() -> void:
	world.cutscene = false
	world.camera_focus = null


func wait(sec: float) -> void:
	await get_tree().create_timer(sec).timeout


func flag(f: String) -> bool:
	return GameState.has_flag(f)


func setf(f: String, v: bool = true) -> void:
	GameState.set_flag(f, v)


func has(id: String) -> bool:
	return GameState.in_party(id)


func npc(id: String):
	return world.npcs.get(id)


func cycle(key: String, options: Array) -> Array:
	var n: int = int(GameState.get_meta("talk_" + key, 0))
	GameState.set_meta("talk_" + key, n + 1)
	var size := options.size()
	var idx := n
	if n >= size:
		idx = 1 + (n - 1) % (size - 1) if size > 1 else 0
	var pick = options[idx]
	return pick if pick is Array else [pick]


func battle(id: String, enemies: Array, bg: String, music: String = "", boss: bool = false, extra: Dictionary = {}) -> void:
	var enc := {"id": id, "enemies": enemies, "bg": bg, "boss": boss, "can_flee": not boss}
	enc.merge(extra)
	if music != "":
		enc["music"] = music
	await world.start_battle(enc)


func join(id: String) -> void:
	GameState.join_party(id)
	world.make_follower(id)


func walk_party_to(pos: Vector2, speed: float = 60.0) -> void:
	await world.player.walk_to(pos, speed)


func rest_and_save(title: String = "Descansáis junto al fuego.") -> void:
	begin()
	var i := await choose(["Descansar y guardar", "Solo descansar", "Seguir"], title)
	if i <= 1:
		await world.set_tint(Color(0.3, 0.3, 0.4), 0.5)
		GameState.heal_all()
		Audio.sfx("heal", -4.0)
		await wait(0.6)
		world.set_tint(world._map_modulate(), 0.5)
		await say(["El grupo recupera todas sus fuerzas."])
		if i == 0:
			await open_save()
	end()


func open_save() -> void:
	var s := SaveScript.new()
	s.setup("save", world.player.position)
	world._hud.add_child(s)
	get_tree().paused = true
	await s.closed
	get_tree().paused = false


func open_shop(id: String) -> void:
	begin()
	var s := ShopScript.new()
	s.setup(id)
	world._hud.add_child(s)
	get_tree().paused = true
	await s.closed
	get_tree().paused = false
	end()


func give_items(items: Dictionary) -> String:
	var parts: Array = []
	for k in items:
		if k == "gold":
			GameState.gold += int(items[k])
			parts.append("%d coronas" % int(items[k]))
		else:
			GameState.add_item(k, int(items[k]))
			parts.append("%s ×%d" % [DB.item(k)["name"], int(items[k])])
	return ", ".join(parts)


# ------------------------------------------------------------ Ganchos (sobrescribir)
func music_override() -> String:
	return ""


func on_map_ready(_map_id: String) -> void:
	pass


func on_trigger(_id: String) -> void:
	pass


func encounters_disabled() -> bool:
	return false


func exit_requirement(_to: String, default_req: String) -> String:
	return default_req


func exit_blocked_message(_to: String, default_msg: String) -> String:
	return default_msg


func before_exit(_to: String) -> bool:
	return true


func is_interactable(id: String) -> bool:
	return not (id.ends_with("_zone"))


## Rastro de interacciones para secretos (orden exacto de cosas examinadas).
func _track_secret(id: String) -> void:
	var seq: Array = GameState.get_meta("ov_seq", [])
	seq.append(GameState.current_map + ":" + id)
	while seq.size() > 6:
		seq.pop_front()
	GameState.set_meta("ov_seq", seq)
	var tail4: Array = seq.slice(maxi(0, seq.size() - 4))
	var tail3: Array = seq.slice(maxi(0, seq.size() - 3))
	if not flag("ov1") and not flag("council_done") and tail4 == ["bedroom:bed", "bedroom:wardrobe", "bedroom:bed", "bedroom:chest"]:
		setf("ov1")
		Audio.sfx("magic", -24.0, 0.5)
	elif flag("ov2") and not flag("ov3") and not flag("yara_joined") and tail3 == ["village:well", "village:well", "village:well"]:
		setf("ov3")
		Audio.sfx("magic", -24.0, 0.5)


## Ruta del Soberano: ecos del arma Overlord.
func _sovereign_echo(id: String) -> bool:
	if not flag("ov_done"):
		return false
	var map := GameState.current_map
	if map == "cathedral" and id == "altar" and not flag("ov_e1"):
		setf("ov_e1")
		await _echo(["El arma del Soberano arde en tu mano. El altar se tiñe de rojo.",
			"Ves a un hombre con una corona de cristal negro y a un monje con una túnica blanca, espalda contra espalda.",
			"«Soy Vael, el Primer Soberano. Esta tierra lleva mi nombre, aunque nadie recuerda por qué.»",
			"«Bajo este altar, Ermengol y yo cerramos la puerta. Él puso la luz. Yo puse mi alma en el acero que ahora empuñas.»",
			"«Lo que encerramos no era un monstruo. Era un hambre. Se llama Nhal'Zur.»",
			"(Primer eco del Soberano: 1/3)"])
		return true
	if map == "elf_camp" and id == "sacred_tree" and not flag("ov_e2"):
		setf("ov_e2")
		await _echo(["Rozas el Árbol Sagrado con el arma. La corteza sangra luz roja.",
			"«La druida Ysolde plantó la Semilla para que el bosque sujetara el Velo con sus raíces.»",
			"«Pero Nhal'Zur aprendió a hablar a través de ellas. La Madre Raíz es su voz. La Torre Negra, sus manos.»",
			"«Si la Madre Raíz cae, el Velo se abrirá del todo. Y él saldrá a comer.»",
			"«Por eso te elegí, heredero. Solo mi acero puede herir lo que no tiene cuerpo.»",
			"(Segundo eco del Soberano: 2/3)"])
		return true
	return false


func _echo(lines: Array) -> void:
	begin()
	Audio.sfx("dark", -4.0, 0.6)
	await world.set_tint(Color(0.6, 0.18, 0.25), 0.8)
	await say(lines)
	await world.set_tint(world._map_modulate(), 0.8)
	end()


# ------------------------------------------------------------ Piedras de retorno (Acto IV)
## Al conseguir cada lágrima aparece una piedra rúnica que lleva al Claro de la Savia (y desde el Claro, de vuelta).
const WAYSTONES := {
	"minas": {"pos": Vector2(320, 120), "flag": "tear_stone", "name": "Minas de Khazgurim"},
	"ruinas": {"pos": Vector2(340, 124), "flag": "tear_crystal", "name": "Ruinas de Cristal"},
	"paso_sur": {"pos": Vector2(490, 200), "flag": "tear_blood", "name": "Paso del Sur"},
}
const CAMP_WAYSTONE := Vector2(360, 150)


func place_waystone() -> void:
	if world.props_by_id.has("waystone"):
		return
	var map := GameState.current_map
	var pos := Vector2.INF
	if WAYSTONES.has(map) and flag(WAYSTONES[map]["flag"]):
		pos = WAYSTONES[map]["pos"]
	elif map == "elf_camp" and flag("act4_started") and (flag("tear_stone") or flag("tear_crystal") or flag("tear_blood")):
		pos = CAMP_WAYSTONE
	if pos == Vector2.INF:
		return
	world.add_prop("rune_on", pos, "waystone", 1, {"col": [-6, -6, 12, 6],
		"light": {"r": 56, "color": [0.55, 0.85, 1.0], "e": 0.9, "flicker": true, "oy": -10}})


func _use_waystone() -> void:
	begin()
	var map := GameState.current_map
	if map == "elf_camp":
		var dests: Array = []
		var opts: Array = []
		for k in WAYSTONES:
			if flag(WAYSTONES[k]["flag"]):
				dests.append(k)
				opts.append("Viajar a: " + str(WAYSTONES[k]["name"]))
		opts.append("Quedarme en el Claro")
		var i := await choose(opts, "La piedra de retorno zumba con luz azul.")
		end()
		if i < dests.size():
			Audio.sfx("magic", -4.0)
			Transition.go_to_map(dests[i], "from_waystone")
		return
	var j := await choose(["Viajar al Claro de la Savia", "Quedarme aquí"], "La piedra de retorno zumba con luz azul.")
	end()
	if j == 0:
		Audio.sfx("magic", -4.0)
		Transition.go_to_map("elf_camp", "from_waystone")


func on_interact(id: String) -> void:
	if id == "waystone":
		await _use_waystone()
		return
	_track_secret(id)
	if await _sovereign_echo(id):
		return
	if id.begins_with("chest_"):
		await open_chest(id)
		return
	if id == "campfire":
		await rest_and_save()
		return
	var lines: Array = interact_lines(id)
	if lines.is_empty():
		return
	begin()
	await say(lines)
	end()


func interact_lines(_id: String) -> Array:
	return []


func open_chest(id: String) -> void:
	if flag("opened_" + id):
		begin()
		await say(["El cofre está vacío."])
		end()
		return
	setf("opened_" + id)
	Audio.sfx("chest", -4.0)
	var node = world.props_by_id.get(id)
	if node:
		node.get_node("Sprite").modulate = Color(0.55, 0.5, 0.5)
	begin()
	await say(["Abres el cofre: %s." % give_items(DB.CHESTS.get(id, {"pocion": 1}))])
	end()


func on_npc(id: String) -> void:
	var n = npc(id)
	if n and n.mode != NPCScript.Mode.FOLLOW:
		n.face_towards(world.player.position)
	var lines: Array = npc_lines(id)
	if lines.is_empty():
		lines = companion_banter(id)
	if lines.is_empty():
		return
	begin()
	await say(lines)
	end()


func npc_lines(_id: String) -> Array:
	return []


## Charla genérica con compañeros según el acto.
func companion_banter(id: String) -> Array:
	var act := 1
	if flag("act2_started"):
		act = 2
	if flag("act3_started"):
		act = 3
	if flag("ready_for_heart"):
		act = 4
	var key := "%s_%d" % [id, act]
	match key:
		"kaelen_1":
			return cycle(key, [[K("¿Te acuerdas de cuando nos colamos en la Catedral y casi nos aplasta una viga? Mejor día de mi vida.")],
				[K("Algún día me iré de Tortosa. A Puerto Ceniza, al Gremio de Aventureros. Y tú te vienes conmigo.")]])
		"kaelen_2":
			return cycle(key, [[K("No dejo de pensar en esas cosas... los siervos. Algunos llevaban ropa de pastor. Eran personas, {name}.")],
				[K("Si hubiera sido más rápido... No. No pienso en eso. Ahora no.")]])
		"kaelen_3":
			if GameState.rivalry() >= 30:
				return cycle(key, [[K("Tú decides, tú eliges, tú, tú, tú. ¿Te has dado cuenta de que ya nunca me preguntas?")]])
			return cycle(key, [[K("Los elfos me miran como si fuera un tronco con patas. Me caen bien, supongo.")],
				[K("Pase lo que pase ahí dentro, cúbreme el flanco izquierdo. Siempre se me olvida.")]])
		"kaelen_4":
			return [K("Ya casi estamos. Oye, {name}... gracias por no dejarme atrás.")] if GameState.rivalry() < 20 else [K("Terminemos con esto.")]
		"yara_1":
			return cycle(key, [[Y("La flor de luna cura casi cualquier cosa si sabes prepararla. Casi.")],
				[Y("A veces siento el bosque, ¿sabes? Como un latido. Hoy late... más despacio.")]])
		"yara_2":
			return cycle(key, [[Y("He curado tantas heridas esta noche que me tiemblan las manos.")],
				[Y("¿Por qué la Semilla me mira así? ...Olvídalo, digo tonterías.")]])
		"yara_3", "yara_4":
			if GameState.yara_is_witch():
				return cycle(key, [[Y("La oscuridad no es tan fría como dicen, {name}. Es... cálida. Poderosa.")],
					[Y("Mis flores ya no brillan. Ahora mis manos sí.")]])
			if flag("romance_yara"):
				return cycle(key, [[Y("Cuando esto acabe, quiero ver el mar contigo. ¿Me lo prometes?")],
					[Y("Deja de mirarme así, que me desconcentras.")]])
			return cycle(key, [[Y("Los elfos saben tanto de plantas... Ilvanis me ha enseñado tres ungüentos nuevos.")],
				[Y("Pase lo que pase, seguimos siendo los Guardianes de Tortosa. ¿Vale?")]])
		"aelis_3", "aelis_4":
			return cycle(key, [[A("No confío en los humanos. Pero confío en mis flechas, y te han visto luchar.")],
				[A("El Corazón fue lo primero que plantaron mis ancestros. Verlo podrido... duele.")]])
		"brom_3", "brom_4":
			return cycle(key, [[B("¡Ja! Otro bosque, otra pelea. En Khazgurim al menos los túneles no te muerden.")],
				[B("Esas larvas salen de abajo, lo sé. De donde cavamos demasiado hondo. Nuestra culpa, en parte.")]])
	return []
