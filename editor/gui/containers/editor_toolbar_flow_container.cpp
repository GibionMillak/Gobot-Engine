/**************************************************************************/
/*  editor_toolbar_flow_container.cpp                                     */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#include "editor_toolbar_flow_container.h"

#include "core/math/math_funcs.h"
#include "core/templates/vector.h"
#include "scene/resources/style_box.h"

void EditorToolbarFlowContainer::set_height_limit(int p_height) {
	if (_height_limit == p_height) {
		return;
	}
	_height_limit = p_height;
	update_minimum_size();
	update_desired_size();
	queue_sort();
}

void EditorToolbarFlowContainer::set_group_separators_enabled(bool p_enabled) {
	if (_group_separators_enabled == p_enabled) {
		return;
	}
	_group_separators_enabled = p_enabled;
	queue_redraw();
}

Size2 EditorToolbarFlowContainer::get_minimum_size() const {
	return _get_layout_size();
}

Size2 EditorToolbarFlowContainer::get_desired_size() const {
	return _get_layout_size();
}

void EditorToolbarFlowContainer::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_DRAW: {
			if (_group_separators_enabled) {
				_draw_group_separators();
			}
		} break;

		case NOTIFICATION_RESIZED:
		case NOTIFICATION_THEME_CHANGED:
		case NOTIFICATION_SORT_CHILDREN: {
			if (_group_separators_enabled) {
				queue_redraw();
			}
		} break;
	}
}

Size2 EditorToolbarFlowContainer::_get_layout_size() const {
	const int h_separation = get_theme_constant(SNAME("h_separation"));
	const int v_separation = get_theme_constant(SNAME("v_separation"));
	Size2i size;
	Size2i column_size;
	bool column_empty = true;

	for (int i = 0; i < get_child_count(); i++) {
		Control *child = as_sortable_control(get_child(i), SortableVisibilityMode::VISIBLE);
		if (!child) {
			continue;
		}
		const Size2i child_size = child->get_bound_desired_size();
		if (!column_empty && _height_limit > 0 && column_size.y + v_separation + child_size.y > _height_limit) {
			size.x += column_size.x + h_separation;
			size.y = MAX(size.y, column_size.y);
			column_size = Size2i();
			column_empty = true;
		}
		if (!column_empty) {
			column_size.y += v_separation;
		}
		column_size.x = MAX(column_size.x, child_size.x);
		column_size.y += child_size.y;
		column_empty = false;
	}

	size.x += column_size.x;
	size.y = MAX(size.y, column_size.y);
	return size;
}

void EditorToolbarFlowContainer::_draw_group_separators() {
	struct GroupColumn {
		Rect2 bounds;
		Vector<Rect2> groups;
	};

	Vector<GroupColumn> columns;
	Rect2 previous_rect;
	for (int i = 0; i < get_child_count(); i++) {
		Control *child = as_sortable_control(get_child(i));
		if (!child) {
			continue;
		}
		const Rect2 rect = child->get_rect();
		if (!rect.has_area()) {
			continue;
		}
		// A new column restarts the downward flow, including when columns grow to the left.
		if (columns.is_empty() || rect.position.y < previous_rect.get_end().y) {
			GroupColumn column;
			column.bounds = rect;
			columns.push_back(column);
		}
		GroupColumn &column = columns.write[columns.size() - 1];
		column.bounds = column.bounds.merge(rect);
		column.groups.push_back(rect);
		previous_rect = rect;
	}

	const Ref<StyleBox> horizontal_style = get_theme_stylebox(SNAME("separator"), SNAME("HSeparator"));
	const Ref<StyleBox> vertical_style = get_theme_stylebox(SNAME("separator"), SNAME("VSeparator"));
	const real_t horizontal_thickness = horizontal_style->get_minimum_size().y;
	const real_t vertical_thickness = vertical_style->get_minimum_size().x;

	for (int i = 0; i < columns.size(); i++) {
		const GroupColumn &column = columns[i];
		for (int j = 1; j < column.groups.size(); j++) {
			const real_t gap_begin = column.groups[j - 1].get_end().y;
			const real_t gap_end = column.groups[j].position.y;
			if (gap_end > gap_begin) {
				const real_t y = Math::floor((gap_begin + gap_end - horizontal_thickness) * 0.5f);
				horizontal_style->draw(get_canvas_item(), Rect2(column.bounds.position.x, y, column.bounds.size.x, horizontal_thickness));
			}
		}

		if (i == 0) {
			continue;
		}
		const Rect2 &previous_bounds = columns[i - 1].bounds;
		const bool grows_right = column.bounds.position.x > previous_bounds.position.x;
		const real_t gap_begin = grows_right ? previous_bounds.get_end().x : column.bounds.get_end().x;
		const real_t gap_end = grows_right ? column.bounds.position.x : previous_bounds.position.x;
		if (gap_end > gap_begin) {
			const real_t x = Math::floor((gap_begin + gap_end - vertical_thickness) * 0.5f);
			const real_t y = MIN(previous_bounds.position.y, column.bounds.position.y);
			const real_t end_y = MAX(previous_bounds.get_end().y, column.bounds.get_end().y);
			vertical_style->draw(get_canvas_item(), Rect2(x, y, vertical_thickness, end_y - y));
		}
	}
}
