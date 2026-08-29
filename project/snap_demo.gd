extends Control

const SnapAdapter := preload("res://snap_adapter.gd")

@onready var chip_rv: RecyclerView = %ChipRecyclerView
@onready var card_rv: RecyclerView = %CardRecyclerView


func _ready() -> void:
	# 横向 chip 行：LinearSnapHelper 松手居中最近的 chip。
	var chips := SnapAdapter.new()
	for i in 100:
		chips.items.append("chip %d" % i)
	chip_rv.set_item_size(300)
	chip_rv.set_adapter(chips)
	var chip_layout := LinearLayoutManager.new()
	chip_layout.set_orientation(LinearLayoutManager.HORIZONTAL)
	chip_rv.set_layout(chip_layout)
	var linear := LinearSnapHelper.new()
	linear.attach_to_recycler_view(chip_rv)

	# 横向卡片轮播：PagerSnapHelper 每次 fling 只翻一页（居中）。
	var cards := SnapAdapter.new()
	for i in 100:
		cards.items.append("card %d" % i)
	card_rv.set_item_size(300)
	card_rv.set_adapter(cards)
	var card_layout := LinearLayoutManager.new()
	card_layout.set_orientation(LinearLayoutManager.HORIZONTAL)
	card_rv.set_layout(card_layout)
	var pager := PagerSnapHelper.new()
	pager.attach_to_recycler_view(card_rv)
