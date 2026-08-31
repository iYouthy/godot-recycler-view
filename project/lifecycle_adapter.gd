class_name LifecycleAdapter
extends Adapter

# Adapter for lifecycle_demo: records every lifecycle dispatch (Android
# onViewAttachedToWindow / onViewDetachedFromWindow / onViewRecycled /
# onFailedToRecycleView) into a log, and marks one item as non-recyclable so
# the "failed to recycle" consultation is visible.

# The item that declares itself non-recyclable at bind time. Scrolling past it
# consults _on_failed_to_recycle_view: with force=false it stays attached (a
# "ghost" pinned to its slot); with force=true it is recycled anyway.
const STUBBORN_POS := 40

var count := 200
var created := 0
var attached := 0
var detached := 0
var recycled := 0
var failed := 0
# Toggle from the demo UI: force-recycle the stubborn item when consulted.
var force := false
# Event log, newest last. The demo shows the tail of this in a panel.
var events: Array[String] = []
# holder instance id -> times bound: the ↺-style reuse counter on each view.
var _bind_counts := {}


func _get_item_count() -> int:
	return count


func _create_item(parent: Control, view_type: int) -> ViewHolder:
	created += 1
	var vh := ViewHolder.new()
	var label := Label.new()
	label.set_size(Vector2(360, 40))
	vh.set_control(label)
	return vh


func _bind_item(holder: ViewHolder, position: int) -> void:
	var label: Label = holder.get_control()
	if position == STUBBORN_POS:
		holder.set_is_recyclable(false)
		label.text = "★ 顽固项 #%d（不可回收）" % position
		label.add_theme_color_override("font_color", Color(0.95, 0.35, 0.35))
	else:
		label.text = "Item %d" % position
		label.add_theme_color_override("font_color", Color.WHITE)
	# Reuse counter: how many times this exact view has been bound. Views that
	# were recycled (data cleared) and later bound to a different item show >1,
	# which is what makes the virtualized list cheap.
	var id := holder.get_instance_id()
	_bind_counts[id] = _bind_counts.get(id, 0) + 1
	if _bind_counts[id] > 1:
		label.text += "   复用×%d" % _bind_counts[id]


func _on_view_attached(holder: ViewHolder) -> void:
	attached += 1
	events.append("attach    #%d 进屏" % holder.get_position())


func _on_view_detached(holder: ViewHolder) -> void:
	detached += 1
	events.append("detach    #%d 出屏" % holder.get_position())


func _on_item_recycled(holder: ViewHolder) -> void:
	recycled += 1
	events.append("recycled  #%d 数据清空 → 池" % holder.get_position())


func _on_failed_to_recycle_view(holder: ViewHolder) -> bool:
	failed += 1
	events.append("拒绝回收  #%d → %s" % [holder.get_position(), "强制回收" if force else "保留在树上"])
	return force
