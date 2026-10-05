extends "res://scripts/story/StoryBase.gd"
## Ruinas de Cristal: puzle de los tres espejos y jefe Emperatriz de Cristal.


func on_map_ready(_m: String) -> void:
	for k in 3:
		if flag("mirror%d_lit" % (k + 1)):
			_light(k + 1)
	if flag("won_empress") and not flag("tear_crystal"):
		await _after_empress()
		return
	if not flag("ruinas_seen"):
		setf("ruinas_seen")
		begin()
		await wait(0.4)
		await say(["Salas de cristal verde agua, columnas rotas y un silencio de catedral.",
			"En las paredes, reflejos que no son los vuestros: una mujer con dos dagas camina por dentro del cristal.",
			N("selen", "...Tres espejos apagados. Tres verdades que nadie quiso oír. Enciéndelos y te dejaré pasar."),
			"(Mazmorra: Ruinas de Cristal. Enciende los tres espejos.)"])
		end()


func _light(k: int) -> void:
	var node = world.props_by_id.get("mirror%d" % k)
	if node:
		var spr: Sprite2D = node.get_node("Sprite")
		spr.texture = load("res://assets/sprites/mirror_on.png")


func _lit() -> int:
	var n := 0
	for k in 3:
		if flag("mirror%d_lit" % (k + 1)):
			n += 1
	return n


func on_interact(id: String) -> void:
	if id.begins_with("mirror"):
		await _mirror(int(id.substr(6)))
		return
	if id == "chest_ruina3" and not flag("won_empress"):
		begin()
		await say(["Un sello de cristal cubre el cofre. La figura del trono te observa."])
		end()
		return
	await super.on_interact(id)


func _mirror(k: int) -> void:
	begin()
	if flag("mirror%d_lit" % k):
		await say(["El espejo brilla con luz propia."])
		end()
		return
	var q := [
		["«Mi emperador bebió de la savia negra para vivir para siempre. ¿Qué hice?»",
			["Lo maté para salvar a mi pueblo.", "Huí y lo dejé reinar.", "Bebí con él."], 0],
		["«Después, el pueblo me llamó traidora. ¿Qué sentí?»",
			["Orgullo.", "Culpa, aunque había hecho lo correcto.", "Nada."], 1],
		["«¿Qué hace falta para cerrar una herida en el mundo?»",
			["Poder.", "Olvido.", "Un corazón dispuesto a romperse."], 2],
	]
	var d: Array = q[k - 1]
	await say([N("selen", d[0])])
	var i := await choose(d[1])
	if i == int(d[2]):
		setf("mirror%d_lit" % k)
		Audio.sfx("magic", -6.0)
		_light(k)
		await say(["El espejo se enciende con una luz blanca y fría. (Espejos: %d/3)" % _lit()])
		if _lit() == 3:
			await say([N("selen", "...Sí. Esa es la verdad. Pasa. Pero te advierto: ya no soy solo yo la que habita el trono.")])
	else:
		Audio.sfx("cancel", -4.0)
		await say([N("selen", "No. Eso es lo que dijeron ellos. Piénsalo otra vez.")])
	end()


func on_trigger(id: String) -> void:
	if id == "empress_zone" and not flag("won_empress"):
		if _lit() < 3:
			begin()
			await say(["Una barrera de cristal cierra el paso al trono. Los espejos siguen apagados. (%d/3)" % _lit()])
			world.player.position += Vector2(0, 14)
			end()
			return
		begin()
		Audio.stop_music(0.8)
		await say(["En el trono, una figura de cristal abre los ojos. Tiene el rostro de Selen, pero la savia negra le corre por dentro como sangre.",
			N("selen", "Huid... Nhal'Zur me ha encontrado. Lleva siglos susurrándome. Y yo... ya no sé decirle que no."),
			"La figura se levanta. Su voz se vuelve doble, fría: «INTRUSOS EN MI IMPERIO.»"])
		await battle("empress", ["emperatriz"], "crypt", "boss", true)


func _after_empress() -> void:
	begin()
	await say(["La Emperatriz se agrieta. De sus ojos cae una sola lágrima que se endurece en el aire: cristal puro.",
		N("selen", "Gracias... El hambre se ha ido. Pero sigo atada a este cristal."),
		N("selen", "Puedes romperme y quedarte lo que queda de mi poder. O dejarme ir. Ambas cosas cuestan algo.")])
	var i := await choose(["Liberar su alma", "Romper el cristal y absorber su poder"])
	if i == 0:
		setf("selen_freed")
		await say(["Tocas el cristal con suavidad. Se deshace en polvo de luz que sube hacia el techo.",
			N("selen", "Dile a Ysolde, si la ves, que al final también yo lloré por el mundo."),
			Y("...Qué triste. Y qué bonito.")])
		GameState.change_approval("yara", 3)
		GameState.change_yara_corruption(-3)
	else:
		setf("selen_shattered")
		await say(["Descargas el golpe. El cristal estalla y una corriente helada te entra por los brazos.",
			"Te sientes más fuerte. Y, por un instante, oyes un susurro que no es de Selen.",
			"(Todo el grupo gana +3 de magia de forma permanente.)"])
		GameState.change_yara_corruption(5)
		GameState.change_approval("kaelen", 2)
	await say(["(Obtienes la Lágrima de Cristal. %d de 3.)" % (1 + int(flag("tear_stone")) + int(flag("tear_blood")))])
	setf("tear_crystal")
	Audio.sfx("magic", -4.0)
	place_waystone()
	await say(["Junto a ti, una piedra rúnica se enciende con luz azul. (Piedra de retorno: te lleva al Claro de la Savia cuando quieras.)"])
	end()


func interact_lines(id: String) -> Array:
	if id == "empress_throne":
		return ["Un trono de cristal vacío. En el respaldo, grabado: «Aquí lloró Selen».", ] if flag("won_empress") else ["Un trono de cristal. Algo te observa desde dentro."]
	return []
