class_name TestListItem extends BoxContainer

@onready var label: Label = $Label

func refresh(content: String) -> void:
	label.text = content
