extends CanvasLayer

@onready var panel: Panel = $Panel
@onready var name_label: Label = $Panel/VBox/NameLabel
@onready var line_label: Label = $Panel/VBox/LineLabel
@onready var approval_label: Label = $Panel/VBox/ApprovalLabel

var _hide_timer: Timer


func _ready() -> void:
	add_to_group("dialogue_box")
	panel.visible = false
	_hide_timer = Timer.new()
	_hide_timer.one_shot = true
	_hide_timer.wait_time = 3.5
	add_child(_hide_timer)
	_hide_timer.timeout.connect(_on_hide_timeout)


func show_line(speaker: String, line: String, companion_id: String, approval: int) -> void:
	name_label.text = speaker
	line_label.text = line
	if companion_id != "":
		approval_label.text = "Vínculo: %d" % approval
		approval_label.visible = true
	else:
		approval_label.visible = false
	panel.visible = true
	_hide_timer.start()


func _on_hide_timeout() -> void:
	panel.visible = false
