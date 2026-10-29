/**************************************************************************/
/*  style_box.cpp                                                         */
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

#include "style_box.h"

#include "core/object/class_db.h"
#include "scene/main/canvas_item.h"

Size2 StyleBox::get_minimum_size() const {
	Size2 min_size = Size2(get_content_inset(SIDE_LEFT) + get_content_inset(SIDE_RIGHT), get_content_inset(SIDE_TOP) + get_content_inset(SIDE_BOTTOM));
	Size2 custom_size;
	GDVIRTUAL_CALL(_get_minimum_size, custom_size);

	if (min_size.x < custom_size.x) {
		min_size.x = custom_size.x;
	}
	if (min_size.y < custom_size.y) {
		min_size.y = custom_size.y;
	}

	return min_size;
}

void StyleBox::set_padding(Side p_side, float p_padding) {
	ERR_FAIL_INDEX((int)p_side, 4);

	_padding[p_side] = MAX(0.0f, p_padding);
	emit_changed();
}

void StyleBox::set_padding_all(float p_padding) {
	set_padding_individual(p_padding, p_padding, p_padding, p_padding);
}

void StyleBox::set_padding_individual(float p_left, float p_top, float p_right, float p_bottom) {
	_padding[SIDE_LEFT] = MAX(0.0f, p_left);
	_padding[SIDE_TOP] = MAX(0.0f, p_top);
	_padding[SIDE_RIGHT] = MAX(0.0f, p_right);
	_padding[SIDE_BOTTOM] = MAX(0.0f, p_bottom);
	emit_changed();
}

float StyleBox::get_padding(Side p_side) const {
	ERR_FAIL_INDEX_V((int)p_side, 4, 0.0);

	return _padding[p_side];
}

float StyleBox::get_content_inset(Side p_side) const {
	ERR_FAIL_INDEX_V((int)p_side, 4, 0.0);

	return _get_style_inset(p_side) + _padding[p_side];
}

Point2 StyleBox::get_content_offset() const {
	return Point2(get_content_inset(SIDE_LEFT), get_content_inset(SIDE_TOP));
}

void StyleBox::draw(RID p_canvas_item, const Rect2 &p_rect) const {
	GDVIRTUAL_CALL(_draw, p_canvas_item, p_rect);
}

Rect2 StyleBox::get_draw_rect(const Rect2 &p_rect) const {
	Rect2 ret;
	if (GDVIRTUAL_CALL(_get_draw_rect, p_rect, ret)) {
		return ret;
	}
	return p_rect;
}

CanvasItem *StyleBox::get_current_item_drawn() const {
	return CanvasItem::get_current_item_drawn();
}

bool StyleBox::test_mask(const Point2 &p_point, const Rect2 &p_rect) const {
	bool ret = true;
	GDVIRTUAL_CALL(_test_mask, p_point, p_rect, ret);
	return ret;
}

void StyleBox::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_minimum_size"), &StyleBox::get_minimum_size);

	ClassDB::bind_method(D_METHOD("set_padding", "side", "padding"), &StyleBox::set_padding);
	ClassDB::bind_method(D_METHOD("get_padding", "side"), &StyleBox::get_padding);
	ClassDB::bind_method(D_METHOD("set_padding_all", "padding"), &StyleBox::set_padding_all);

	ClassDB::bind_method(D_METHOD("get_content_inset", "side"), &StyleBox::get_content_inset);
	ClassDB::bind_method(D_METHOD("get_content_offset"), &StyleBox::get_content_offset);

	ClassDB::bind_method(D_METHOD("draw", "canvas_item", "rect"), &StyleBox::draw);
	ClassDB::bind_method(D_METHOD("get_current_item_drawn"), &StyleBox::get_current_item_drawn);

	ClassDB::bind_method(D_METHOD("test_mask", "point", "rect"), &StyleBox::test_mask);

	ADD_GROUP("Padding", "padding_");
	ADD_PROPERTYI(PropertyInfo(Variant::FLOAT, "padding_left", PROPERTY_HINT_RANGE, "0,2048,1,suffix:px"), "set_padding", "get_padding", SIDE_LEFT);
	ADD_PROPERTYI(PropertyInfo(Variant::FLOAT, "padding_top", PROPERTY_HINT_RANGE, "0,2048,1,suffix:px"), "set_padding", "get_padding", SIDE_TOP);
	ADD_PROPERTYI(PropertyInfo(Variant::FLOAT, "padding_right", PROPERTY_HINT_RANGE, "0,2048,1,suffix:px"), "set_padding", "get_padding", SIDE_RIGHT);
	ADD_PROPERTYI(PropertyInfo(Variant::FLOAT, "padding_bottom", PROPERTY_HINT_RANGE, "0,2048,1,suffix:px"), "set_padding", "get_padding", SIDE_BOTTOM);

	GDVIRTUAL_BIND(_draw, "to_canvas_item", "rect")
	GDVIRTUAL_BIND(_get_draw_rect, "rect")
	GDVIRTUAL_BIND(_get_minimum_size)
	GDVIRTUAL_BIND(_test_mask, "point", "rect")
}
