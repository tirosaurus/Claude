extends "res://scripts/story/StoryBase.gd"
## Rutas entre zonas: un tramo de viaje con enemigos, una hoguera y un par de cofres.

func intro() -> Array:
	return []


func sign_text() -> Array:
	return []


func on_map_ready(map_id: String) -> void:
	if flag("seen_" + map_id):
		return
	setf("seen_" + map_id)
	var lines := intro()
	if lines.is_empty():
		return
	begin()
	await wait(0.5)
	await say(lines)
	end()


func interact_lines(id: String) -> Array:
	if id == "sign_route":
		return sign_text()
	return []
