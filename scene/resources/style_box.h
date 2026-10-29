/**************************************************************************/
/*  style_box.h                                                           */
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

#pragma once

#include "core/io/resource.h"
#include "core/math/math_defs.h"
#include "core/math/rect2.h"
#include "core/math/vector2.h"
#include "core/object/gdvirtual.gen.h"
#include "core/templates/rid.h"

class CanvasItem;

class StyleBox : public Resource {
	GDCLASS(StyleBox, Resource);
	RES_BASE_EXTENSION("stylebox");
	OBJ_SAVE_TYPE(StyleBox);

public:
	StyleBox() = default;

	virtual Size2 get_minimum_size() const;

	// Padding is nonnegative, measured in pixels, and added inside the style's border.
	void set_padding(Side p_side, float p_padding);
	void set_padding_all(float p_padding);
	void set_padding_individual(float p_left, float p_top, float p_right, float p_bottom);
	float get_padding(Side p_side) const;

	// Returns the total layout inset, including the style's border and content padding.
	float get_content_inset(Side p_side) const;
	Point2 get_content_offset() const;

	virtual void draw(RID p_canvas_item, const Rect2 &p_rect) const;
	virtual Rect2 get_draw_rect(const Rect2 &p_rect) const;

	CanvasItem *get_current_item_drawn() const;

	virtual bool test_mask(const Point2 &p_point, const Rect2 &p_rect) const;

protected:
	static void _bind_methods();
	virtual float _get_style_inset(Side p_side) const { return 0; }

	GDVIRTUAL2C_REQUIRED(_draw, RID, Rect2)
	GDVIRTUAL1RC(Rect2, _get_draw_rect, Rect2)
	GDVIRTUAL0RC(Size2, _get_minimum_size)
	GDVIRTUAL2RC(bool, _test_mask, Point2, Rect2)

private:
	float _padding[4] = {};
};

class StyleBoxEmpty : public StyleBox {
	GDCLASS(StyleBoxEmpty, StyleBox);

public:
	void draw(RID p_canvas_item, const Rect2 &p_rect) const override {}
};
