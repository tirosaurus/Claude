class_name UIKit
extends RefCounted
## Utilidades de interfaz compartidas: botones de madera, marcos y etiquetas.

const TEXT_DARK := Color(0.24, 0.15, 0.11)
const TEXT_LIGHT := Color(1, 0.93, 0.75)


static func _box(state: String) -> StyleBoxTexture:
	var sb := StyleBoxTexture.new()
	sb.texture = load("res://assets/ui/button_%s.png" % state)
	sb.texture_margin_left = 4
	sb.texture_margin_right = 4
	sb.texture_margin_top = 4
	sb.texture_margin_bottom = 4
	sb.content_margin_left = 10
	sb.content_margin_right = 10
	sb.content_margin_top = 3
	sb.content_margin_bottom = 5
	return sb


static func button(text: String, font_size: int = 16) -> Button:
	var b := Button.new()
	b.text = text
	b.focus_mode = Control.FOCUS_ALL
	b.add_theme_font_size_override("font_size", font_size)
	b.add_theme_stylebox_override("normal", _box("normal"))
	b.add_theme_stylebox_override("hover", _box("hover"))
	b.add_theme_stylebox_override("focus", _box("hover"))
	b.add_theme_stylebox_override("pressed", _box("pressed"))
	b.add_theme_stylebox_override("disabled", _box("disabled"))
	for c in ["font_color", "font_focus_color", "font_hover_color", "font_pressed_color", "font_hover_pressed_color"]:
		b.add_theme_color_override(c, TEXT_LIGHT)
	b.add_theme_color_override("font_disabled_color", Color(0.6, 0.55, 0.5))
	b.add_theme_color_override("font_outline_color", Color(0.12, 0.08, 0.08))
	b.add_theme_constant_override("outline_size", 3)
	b.focus_entered.connect(func(): Audio.sfx("select", -16.0))
	return b


static func panel(rect: Rect2, dark: bool = false) -> NinePatchRect:
	var n := NinePatchRect.new()
	n.texture = load("res://assets/ui/panel_dark.png" if dark else "res://assets/ui/panel.png")
	n.patch_margin_left = 8
	n.patch_margin_right = 8
	n.patch_margin_top = 8
	n.patch_margin_bottom = 8
	n.position = rect.position
	n.size = rect.size
	return n


static func label(text: String, size: int = 16, color: Color = TEXT_DARK, outline: int = 0) -> Label:
	var l := Label.new()
	l.text = text
	l.add_theme_font_size_override("font_size", size)
	l.add_theme_color_override("font_color", color)
	if outline > 0:
		l.add_theme_color_override("font_outline_color", Color(0.1, 0.07, 0.1))
		l.add_theme_constant_override("outline_size", outline)
	return l
