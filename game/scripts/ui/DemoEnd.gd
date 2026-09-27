extends Control


func _ready() -> void:
	var bg := TextureRect.new()
	bg.texture = load("res://assets/bg/title.png")
	bg.size = Vector2(640, 360)
	bg.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
	bg.stretch_mode = TextureRect.STRETCH_SCALE
	bg.modulate = Color(0.45, 0.4, 0.5)
	add_child(bg)

	var title := UIKit.label("Fin de la demo", 34, Color(0.98, 0.86, 0.55), 8)
	title.position = Vector2(0, 22)
	title.size = Vector2(640, 44)
	title.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	add_child(title)

	var panel := UIKit.panel(Rect2(120, 74, 400, 206))
	add_child(panel)
	var s := GameState.player_stats
	var k: Dictionary = GameState.companions["kaelen"]
	var y: Dictionary = GameState.companions["yara"]
	var text := "%s · %s · Nivel %d\nRama elegida: %s\n\n" % [GameState.player_name(), GameState.race_name(), s["level"], GameState.branch_name()]
	text += "Kaelen: %s  (amistad/rivalidad %+d)\n" % [GameState.kaelen_bond_label(), k["rivalry"]]
	text += "Yara: %s  (aprobación %+d)\n\n" % [GameState.yara_bond_label(), y["approval"]]
	text += "Continuará...\nLa savia negra avanza bajo el Bosque Santo. Los Moronguls vienen del sur, y Tortosa aún no lo sabe."
	var l := UIKit.label("", 15)
	l.position = Vector2(24, 18)
	l.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	l.size = Vector2(352, 176)
	l.text = text
	panel.add_child(l)

	var b := UIKit.button("Volver al título", 16)
	b.position = Vector2(245, 296)
	b.size = Vector2(150, 30)
	b.pressed.connect(func(): Transition.go_to_scene("res://scenes/Title.tscn"))
	add_child(b)
	b.grab_focus()
	var credit := UIKit.label("Arte, música, código e historia generados con Claude · Fuente Pixelify Sans (OFL)", 11, Color(0.75, 0.72, 0.8), 3)
	credit.position = Vector2(0, 340)
	credit.size = Vector2(640, 16)
	credit.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	add_child(credit)
	Audio.play_music("title", 1.5)
