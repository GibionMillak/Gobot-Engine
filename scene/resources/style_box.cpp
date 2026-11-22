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

#include "core/config/engine.h"
#include "core/error/error_macros.h"
#include "core/math/math_funcs.h"
#include "core/object/callable_mp.h"
#include "core/object/class_db.h"
#include "core/string/ustring.h"
#include "scene/main/canvas_item.h"
#include "scene/resources/atlas_texture.h"
#include "scene/resources/dpi_texture.h"
#include "scene/resources/theme.h"
#include "servers/rendering/rendering_server.h"
#include "servers/text/text_server.h"

#include <cfloat>

void StyleBox::set_texture(Ref<Texture2D> p_texture) {
	const bool override_changed = _mark_theme_override(TEXTURE);
	if (_texture == p_texture) {
		if (override_changed) {
			emit_changed();
		}
		return;
	}
	if (_texture.is_valid()) {
		_texture->disconnect_changed(callable_mp(this, &StyleBox::_texture_changed));
	}
	_texture = p_texture;
	if (_texture.is_valid()) {
		_texture->connect_changed(callable_mp(this, &StyleBox::_texture_changed));
	}
	emit_changed();
	notify_property_list_changed();
}

Ref<Texture2D> StyleBox::get_texture() const {
	return _texture;
}

void StyleBox::set_texture_modulate(const Color &p_modulate) {
	const bool override_changed = _mark_theme_override(TEXTURE_MODULATE);
	if (_texture_modulate == p_modulate) {
		if (override_changed) {
			emit_changed();
		}
		return;
	}
	_texture_modulate = p_modulate;
	emit_changed();
}

Color StyleBox::get_texture_modulate() const {
	return _texture_modulate;
}

void StyleBox::set_texture_slice(Side p_side, float p_size) {
	ERR_FAIL_INDEX((int)p_side, 4);
	_mark_theme_override(TEXTURE_SLICE_LEFT + int(p_side));

	_texture_slice[p_side] = MAX(0.0f, p_size);
	emit_changed();
}

float StyleBox::get_texture_slice(Side p_side) const {
	ERR_FAIL_INDEX_V((int)p_side, 4, 0.0);

	return _texture_slice[p_side];
}

void StyleBox::set_texture_slice_all(float p_size) {
	for (int i = 0; i < 4; i++) {
		_mark_theme_override(TEXTURE_SLICE_LEFT + i);
	}
	for (int i = 0; i < 4; i++) {
		_texture_slice[i] = MAX(0.0f, p_size);
	}
	emit_changed();
}

void StyleBox::set_texture_slice_individual(float p_left, float p_top, float p_right, float p_bottom) {
	for (int i = 0; i < 4; i++) {
		_mark_theme_override(TEXTURE_SLICE_LEFT + i);
	}
	_texture_slice[SIDE_LEFT] = MAX(0.0f, p_left);
	_texture_slice[SIDE_TOP] = MAX(0.0f, p_top);
	_texture_slice[SIDE_RIGHT] = MAX(0.0f, p_right);
	_texture_slice[SIDE_BOTTOM] = MAX(0.0f, p_bottom);
	emit_changed();
}

void StyleBox::set_h_axis_stretch_mode(AxisStretchMode p_mode) {
	ERR_FAIL_INDEX((int)p_mode, 3);
	_mark_theme_override(AXIS_STRETCH_HORIZONTAL);
	_axis_h = p_mode;
	emit_changed();
}

StyleBox::AxisStretchMode StyleBox::get_h_axis_stretch_mode() const {
	return _axis_h;
}

void StyleBox::set_v_axis_stretch_mode(AxisStretchMode p_mode) {
	ERR_FAIL_INDEX((int)p_mode, 3);
	_mark_theme_override(AXIS_STRETCH_VERTICAL);
	_axis_v = p_mode;
	emit_changed();
}

StyleBox::AxisStretchMode StyleBox::get_v_axis_stretch_mode() const {
	return _axis_v;
}

void StyleBox::set_region_rect(const Rect2 &p_region_rect) {
	_mark_theme_override(REGION_RECT);
	if (_region_rect == p_region_rect) {
		return;
	}

	_region_rect = p_region_rect;
	emit_changed();
}

Rect2 StyleBox::get_region_rect() const {
	return _region_rect;
}

void StyleBox::set_background_color(const Color &p_color) {
	_mark_theme_override(BACKGROUND_COLOR);
	_background_color = p_color;
	emit_changed();
}

Color StyleBox::get_background_color() const {
	return _background_color;
}

void StyleBox::set_skew(Vector2 p_skew) {
	_mark_theme_override(SKEW);
	_skew = p_skew;
	emit_changed();
}

Vector2 StyleBox::get_skew() const {
	return _skew;
}

void StyleBox::set_padding(Side p_side, float p_padding) {
	ERR_FAIL_INDEX((int)p_side, 4);

	_mark_theme_override(PADDING_LEFT + int(p_side));
	_padding[p_side] = MAX(0.0f, p_padding);
	emit_changed();
}

float StyleBox::get_padding(Side p_side) const {
	ERR_FAIL_INDEX_V((int)p_side, 4, 0.0);

	return _padding[p_side];
}

void StyleBox::set_padding_all(float p_padding) {
	set_padding_individual(p_padding, p_padding, p_padding, p_padding);
}

void StyleBox::set_padding_individual(float p_left, float p_top, float p_right, float p_bottom) {
	for (int i = 0; i < 4; i++) {
		_mark_theme_override(PADDING_LEFT + i);
	}
	_padding[SIDE_LEFT] = MAX(0.0f, p_left);
	_padding[SIDE_TOP] = MAX(0.0f, p_top);
	_padding[SIDE_RIGHT] = MAX(0.0f, p_right);
	_padding[SIDE_BOTTOM] = MAX(0.0f, p_bottom);
	emit_changed();
}

void StyleBox::set_border_color(const Color &p_color) {
	_mark_theme_override(BORDER_COLOR);
	_border_color = p_color;
	emit_changed();
}

Color StyleBox::get_border_color() const {
	return _border_color;
}

void StyleBox::set_border_blend(bool p_blend) {
	_mark_theme_override(BORDER_BLEND);
	_blend_border = p_blend;
	emit_changed();
}

bool StyleBox::get_border_blend() const {
	return _blend_border;
}

void StyleBox::set_border_width(Side p_side, int p_width) {
	ERR_FAIL_INDEX((int)p_side, 4);
	_mark_theme_override(BORDER_WIDTH_LEFT + int(p_side));
	_border_width[p_side] = MAX(0, p_width);
	emit_changed();
}

int StyleBox::get_border_width(Side p_side) const {
	ERR_FAIL_INDEX_V((int)p_side, 4, 0);
	return _border_width[p_side];
}

void StyleBox::set_border_width_all(int p_size) {
	for (int i = 0; i < 4; i++) {
		_mark_theme_override(BORDER_WIDTH_LEFT + i);
	}
	_border_width[0] = MAX(0, p_size);
	_border_width[1] = MAX(0, p_size);
	_border_width[2] = MAX(0, p_size);
	_border_width[3] = MAX(0, p_size);
	emit_changed();
}

int StyleBox::get_border_width_min() const {
	return MIN(MIN(_border_width[0], _border_width[1]), MIN(_border_width[2], _border_width[3]));
}

void StyleBox::set_corner_radius(const Corner p_corner, const int p_radius) {
	ERR_FAIL_INDEX((int)p_corner, 4);
	_mark_theme_override(CORNER_RADIUS_TOP_LEFT + int(p_corner));
	_corner_radius[p_corner] = MAX(0, p_radius);
	emit_changed();
}

int StyleBox::get_corner_radius(const Corner p_corner) const {
	ERR_FAIL_INDEX_V((int)p_corner, 4, 0);
	return _corner_radius[p_corner];
}

void StyleBox::set_corner_radius_all(int p_radius) {
	for (int i = 0; i < 4; i++) {
		_mark_theme_override(CORNER_RADIUS_TOP_LEFT + i);
		_corner_radius[i] = MAX(0, p_radius);
	}

	emit_changed();
}

void StyleBox::set_corner_radius_individual(const int p_top_left, const int p_top_right, const int p_bottom_right, const int p_bottom_left) {
	for (int i = 0; i < 4; i++) {
		_mark_theme_override(CORNER_RADIUS_TOP_LEFT + i);
	}
	_corner_radius[0] = MAX(0, p_top_left);
	_corner_radius[1] = MAX(0, p_top_right);
	_corner_radius[2] = MAX(0, p_bottom_right);
	_corner_radius[3] = MAX(0, p_bottom_left);

	emit_changed();
}

void StyleBox::set_corner_detail(int p_corner_detail) {
	_mark_theme_override(CORNER_DETAIL);
	_corner_detail = CLAMP(p_corner_detail, 1, 20);
	emit_changed();
}

int StyleBox::get_corner_detail() const {
	return _corner_detail;
}

void StyleBox::set_expand_margin(Side p_side, float p_size) {
	ERR_FAIL_INDEX((int)p_side, 4);
	_mark_theme_override(EXPAND_MARGIN_LEFT + int(p_side));
	_expand_margin[p_side] = p_size;
	emit_changed();
}

float StyleBox::get_expand_margin(Side p_side) const {
	ERR_FAIL_INDEX_V((int)p_side, 4, 0.0);
	return _expand_margin[p_side];
}

void StyleBox::set_expand_margin_all(float p_expand_margin_size) {
	for (int i = 0; i < 4; i++) {
		_mark_theme_override(EXPAND_MARGIN_LEFT + i);
	}
	for (int i = 0; i < 4; i++) {
		_expand_margin[i] = p_expand_margin_size;
	}
	emit_changed();
}

void StyleBox::set_expand_margin_individual(float p_left, float p_top, float p_right, float p_bottom) {
	for (int i = 0; i < 4; i++) {
		_mark_theme_override(EXPAND_MARGIN_LEFT + i);
	}
	_expand_margin[SIDE_LEFT] = p_left;
	_expand_margin[SIDE_TOP] = p_top;
	_expand_margin[SIDE_RIGHT] = p_right;
	_expand_margin[SIDE_BOTTOM] = p_bottom;
	emit_changed();
}

void StyleBox::set_shadow_color(const Color &p_color) {
	_mark_theme_override(SHADOW_COLOR);
	_shadow_color = p_color;
	emit_changed();
}

Color StyleBox::get_shadow_color() const {
	return _shadow_color;
}

void StyleBox::set_shadow_size(int p_size) {
	_mark_theme_override(SHADOW_SIZE);
	_shadow_size = MAX(0, p_size);
	emit_changed();
}

int StyleBox::get_shadow_size() const {
	return _shadow_size;
}

void StyleBox::set_shadow_offset(const Point2 &p_offset) {
	_mark_theme_override(SHADOW_OFFSET);
	_shadow_offset = p_offset;
	emit_changed();
}

Point2 StyleBox::get_shadow_offset() const {
	return _shadow_offset;
}

void StyleBox::set_anti_aliased(bool p_anti_aliased) {
	_mark_theme_override(ANTI_ALIASING);
	_anti_aliased = p_anti_aliased;
	emit_changed();
	notify_property_list_changed();
}

bool StyleBox::is_anti_aliased() const {
	return _anti_aliased;
}

void StyleBox::set_aa_size(const real_t p_aa_size) {
	_mark_theme_override(ANTI_ALIASING_SIZE);
	_aa_size = CLAMP(p_aa_size, 0.01, 10);
	emit_changed();
}

real_t StyleBox::get_aa_size() const {
	return _aa_size;
}

void StyleBox::set_minimum_size(const Size2 &p_size) {
	_mark_theme_override(MINIMUM_SIZE);
	ERR_FAIL_COND(!p_size.is_finite());
	_minimum_size = p_size.maxf(0);
	emit_changed();
}

Size2 StyleBox::get_intrinsic_minimum_size() const {
	return _minimum_size;
}

Size2 StyleBox::get_minimum_size() const {
	Size2 min_size = Size2(get_content_inset(SIDE_LEFT) + get_content_inset(SIDE_RIGHT), get_content_inset(SIDE_TOP) + get_content_inset(SIDE_BOTTOM));
	min_size = min_size.max(_minimum_size);
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

float StyleBox::get_content_inset(Side p_side) const {
	ERR_FAIL_INDEX_V((int)p_side, 4, 0.0);

	return _border_width[p_side] + _padding[p_side];
}

Point2 StyleBox::get_content_offset() const {
	return Point2(get_content_inset(SIDE_LEFT), get_content_inset(SIDE_TOP));
}

void StyleBox::draw(RID p_canvas_item, const Rect2 &p_rect) const {
	if (GDVIRTUAL_CALL(_draw, p_canvas_item, p_rect)) {
		return;
	}

	const bool draw_border = (_border_width[0] > 0) || (_border_width[1] > 0) || (_border_width[2] > 0) || (_border_width[3] > 0);
	const bool draw_background = _background_color.a > 0.0f;
	const bool draw_shadow = _shadow_size > 0 && _shadow_color.a > 0.0f;
	const bool draw_texture = _texture.is_valid() && _texture_modulate.a > 0.0f;
	if (!draw_border && !draw_background && !draw_shadow && !draw_texture) {
		return;
	}

	Rect2 style_rect = p_rect.grow_individual(_expand_margin[SIDE_LEFT], _expand_margin[SIDE_TOP], _expand_margin[SIDE_RIGHT], _expand_margin[SIDE_BOTTOM]);
	if (style_rect.size.width <= 0 || style_rect.size.height <= 0) {
		return;
	}

	const bool rounded_corners = (_corner_radius[0] > 0) || (_corner_radius[1] > 0) || (_corner_radius[2] > 0) || (_corner_radius[3] > 0);
	// Only enable antialiasing if it is actually needed. This improves performance
	// and maximizes sharpness for non-skewed StyleBoxes with sharp corners.
	const bool aa_on = (rounded_corners || !_skew.is_zero_approx()) && _anti_aliased;

	const bool blend_on = _blend_border && draw_border;

	Color border_color_alpha = Color(_border_color.r, _border_color.g, _border_color.b, 0);
	Color border_color_blend = (draw_background ? _background_color : border_color_alpha);
	Color border_color_inner = blend_on ? border_color_blend : _border_color;

	real_t aa_size_scaled = 1.0f;
	if (aa_on) {
		real_t scale_factor = TextServer::get_current_drawn_item_oversampling();
		if (scale_factor == 0.0) {
			scale_factor = 1.0;
		}

		// Adjust AA feather size to account for the 2D scale factor, so that
		// antialiasing doesn't become blurry at viewport resolutions higher
		// than the default when using the `canvas_items` stretch mode
		// (or when using `oversampling` values different than `1.0`).
		aa_size_scaled = _aa_size / scale_factor;
	}

	// Adapt borders (prevent weird overlapping/glitchy drawings).
	real_t aa_shrink = aa_on ? aa_size_scaled * 2 : 0;
	real_t width = MAX(style_rect.size.width - aa_shrink, 0);
	real_t height = MAX(style_rect.size.height - aa_shrink, 0);
	real_t adapted_border[4] = { 1000000.0, 1000000.0, 1000000.0, 1000000.0 };
	_adapt_values(SIDE_TOP, SIDE_BOTTOM, adapted_border, _border_width, height, height, height);
	_adapt_values(SIDE_LEFT, SIDE_RIGHT, adapted_border, _border_width, width, width, width);

	// Adapt corners (prevent weird overlapping/glitchy drawings).
	real_t adapted_corner[4] = { 1000000.0, 1000000.0, 1000000.0, 1000000.0 };
	_adapt_values(CORNER_TOP_RIGHT, CORNER_BOTTOM_RIGHT, adapted_corner, _corner_radius, height, height - adapted_border[SIDE_BOTTOM], height - adapted_border[SIDE_TOP]);
	_adapt_values(CORNER_TOP_LEFT, CORNER_BOTTOM_LEFT, adapted_corner, _corner_radius, height, height - adapted_border[SIDE_BOTTOM], height - adapted_border[SIDE_TOP]);
	_adapt_values(CORNER_TOP_LEFT, CORNER_TOP_RIGHT, adapted_corner, _corner_radius, width, width - adapted_border[SIDE_RIGHT], width - adapted_border[SIDE_LEFT]);
	_adapt_values(CORNER_BOTTOM_LEFT, CORNER_BOTTOM_RIGHT, adapted_corner, _corner_radius, width, width - adapted_border[SIDE_RIGHT], width - adapted_border[SIDE_LEFT]);

	Rect2 infill_rect = style_rect.grow_individual(-adapted_border[SIDE_LEFT], -adapted_border[SIDE_TOP], -adapted_border[SIDE_RIGHT], -adapted_border[SIDE_BOTTOM]);

	Rect2 border_style_rect = style_rect;

	if (aa_on) {
		for (int i = 0; i < 4; i++) {
			if (_border_width[i] > 0) {
				border_style_rect = border_style_rect.grow_side((Side)i, -aa_size_scaled);
			}
		}
	}

	Vector<Point2> verts;
	Vector<int> indices;
	Vector<Color> colors;

	// Create shadow.
	if (draw_shadow) {
		Rect2 shadow_inner_rect = style_rect;
		shadow_inner_rect.position += _shadow_offset;

		Rect2 shadow_rect = style_rect.grow(_shadow_size);
		shadow_rect.position += _shadow_offset;

		Color shadow_color_transparent = Color(_shadow_color.r, _shadow_color.g, _shadow_color.b, 0);

		_draw_rounded_rectangle(verts, indices, colors, shadow_inner_rect, adapted_corner,
				shadow_rect, shadow_inner_rect, _shadow_color, shadow_color_transparent, _corner_detail, _skew);

		if (draw_background || draw_texture) {
			_draw_rounded_rectangle(verts, indices, colors, shadow_inner_rect, adapted_corner,
					shadow_inner_rect, shadow_inner_rect, _shadow_color, _shadow_color, _corner_detail, _skew, true);
		}
	}

	// Create infill (no AA).
	if (draw_background && (!aa_on || blend_on)) {
		_draw_rounded_rectangle(verts, indices, colors, border_style_rect, adapted_corner,
				infill_rect, infill_rect, _background_color, _background_color, _corner_detail, _skew, true);
	}

	real_t aa_border_width[4] = {};
	real_t aa_border_width_half[4] = {};
	real_t aa_fill_width[4] = {};
	real_t aa_fill_width_half[4] = {};
	if (aa_on) {
		if (draw_border) {
			for (int i = 0; i < 4; i++) {
				if (_border_width[i] > 0) {
					aa_border_width[i] = aa_size_scaled;
					aa_border_width_half[i] = aa_size_scaled * 0.5;
					aa_fill_width[i] = 0;
					aa_fill_width_half[i] = 0;
				} else {
					aa_border_width[i] = 0;
					aa_border_width_half[i] = 0;
					aa_fill_width[i] = aa_size_scaled;
					aa_fill_width_half[i] = aa_size_scaled * 0.5;
				}
			}
		} else {
			for (int i = 0; i < 4; i++) {
				aa_border_width[i] = 0;
				aa_border_width_half[i] = 0;
				aa_fill_width[i] = aa_size_scaled;
				aa_fill_width_half[i] = aa_size_scaled * 0.5;
			}
		}

		if (draw_background) {
			// Infill rect, transparent side of antialiasing gradient (base infill rect enlarged by AA size)
			Rect2 infill_rect_aa_transparent = infill_rect.grow_individual(aa_fill_width_half[SIDE_LEFT], aa_fill_width_half[SIDE_TOP],
					aa_fill_width_half[SIDE_RIGHT], aa_fill_width_half[SIDE_BOTTOM]);
			// Infill rect, colored side of antialiasing gradient (base infill rect shrunk by AA size)
			Rect2 infill_rect_aa_colored = infill_rect_aa_transparent.grow_individual(-aa_fill_width[SIDE_LEFT], -aa_fill_width[SIDE_TOP],
					-aa_fill_width[SIDE_RIGHT], -aa_fill_width[SIDE_BOTTOM]);
			if (!blend_on) {
				// Create center fill, not antialiased yet
				_draw_rounded_rectangle(verts, indices, colors, border_style_rect, adapted_corner,
						infill_rect_aa_colored, infill_rect_aa_colored, _background_color, _background_color, _corner_detail, _skew, true);
			}
			if (!blend_on || !draw_border) {
				Color alpha_bg = Color(_background_color.r, _background_color.g, _background_color.b, 0);
				// Add antialiasing on the center fill
				_draw_rounded_rectangle(verts, indices, colors, border_style_rect, adapted_corner,
						infill_rect_aa_transparent, infill_rect_aa_colored, _background_color, alpha_bg, _corner_detail, _skew);
			}
		}
	}

	if (draw_texture) {
		_submit_geometry(p_canvas_item, style_rect, aa_on ? aa_size_scaled : 0, verts, indices, colors);
		verts.clear();
		indices.clear();
		colors.clear();
		_draw_texture(p_canvas_item, style_rect, adapted_corner, aa_on ? aa_size_scaled : 0);
	}

	// Create border (no AA).
	if (draw_border && !aa_on) {
		_draw_rounded_rectangle(verts, indices, colors, border_style_rect, adapted_corner,
				border_style_rect, infill_rect, border_color_inner, _border_color, _corner_detail, _skew);
	}

	if (aa_on) {
		if (draw_border) {
			// Inner border rect, fully colored side of antialiasing gradient (base inner rect enlarged by AA size)
			Rect2 inner_rect_aa_colored = infill_rect.grow_individual(aa_border_width_half[SIDE_LEFT], aa_border_width_half[SIDE_TOP],
					aa_border_width_half[SIDE_RIGHT], aa_border_width_half[SIDE_BOTTOM]);
			// Inner border rect, transparent side of antialiasing gradient (base inner rect shrunk by AA size)
			Rect2 inner_rect_aa_transparent = inner_rect_aa_colored.grow_individual(-aa_border_width[SIDE_LEFT], -aa_border_width[SIDE_TOP],
					-aa_border_width[SIDE_RIGHT], -aa_border_width[SIDE_BOTTOM]);
			// Outer border rect, transparent side of antialiasing gradient (base outer rect enlarged by AA size)
			Rect2 outer_rect_aa_transparent = style_rect.grow_individual(aa_border_width_half[SIDE_LEFT], aa_border_width_half[SIDE_TOP],
					aa_border_width_half[SIDE_RIGHT], aa_border_width_half[SIDE_BOTTOM]);
			// Outer border rect, colored side of antialiasing gradient (base outer rect shrunk by AA size)
			Rect2 outer_rect_aa_colored = border_style_rect.grow_individual(aa_border_width_half[SIDE_LEFT], aa_border_width_half[SIDE_TOP],
					aa_border_width_half[SIDE_RIGHT], aa_border_width_half[SIDE_BOTTOM]);

			// Create border ring, not antialiased yet
			_draw_rounded_rectangle(verts, indices, colors, border_style_rect, adapted_corner,
					outer_rect_aa_colored, ((blend_on) ? infill_rect : inner_rect_aa_colored), border_color_inner, _border_color, _corner_detail, _skew);
			if (!blend_on) {
				// Add antialiasing on the ring inner border
				_draw_rounded_rectangle(verts, indices, colors, border_style_rect, adapted_corner,
						inner_rect_aa_colored, inner_rect_aa_transparent, border_color_blend, _border_color, _corner_detail, _skew);
			}
			// Add antialiasing on the ring outer border
			_draw_rounded_rectangle(verts, indices, colors, border_style_rect, adapted_corner,
					outer_rect_aa_transparent, outer_rect_aa_colored, _border_color, border_color_alpha, _corner_detail, _skew);
		}
	}

	_submit_geometry(p_canvas_item, style_rect, aa_on ? aa_size_scaled : 0, verts, indices, colors);
}

Rect2 StyleBox::get_draw_rect(const Rect2 &p_rect) const {
	Rect2 custom_rect;
	if (GDVIRTUAL_CALL(_get_draw_rect, p_rect, custom_rect)) {
		return custom_rect;
	}

	Rect2 draw_rect = p_rect.grow_individual(_expand_margin[SIDE_LEFT], _expand_margin[SIDE_TOP], _expand_margin[SIDE_RIGHT], _expand_margin[SIDE_BOTTOM]);

	if (_shadow_size > 0) {
		Rect2 shadow_rect = draw_rect.grow(_shadow_size);
		shadow_rect.position += _shadow_offset;
		draw_rect = draw_rect.merge(shadow_rect);
	}

	return draw_rect;
}

CanvasItem *StyleBox::get_current_item_drawn() const {
	return CanvasItem::get_current_item_drawn();
}

bool StyleBox::test_mask(const Point2 &p_point, const Rect2 &p_rect) const {
	bool ret = true;
	GDVIRTUAL_CALL(_test_mask, p_point, p_rect, ret);
	return ret;
}

Error StyleBox::set_theme_expression(const StringName &p_property, const String &p_expression) {
	const int index = _get_theme_property_index(p_property);
	ERR_FAIL_COND_V_MSG(index < 0, ERR_INVALID_PARAMETER, vformat("StyleBox property '%s' does not support theme expressions.", p_property));
	ERR_FAIL_COND_V_MSG(Theme::validate_theme_expression(p_expression) != OK, ERR_PARSE_ERROR, vformat("Invalid theme expression '%s' for '%s'.", p_expression, p_property));
	++_theme_override_revision;
	_theme_expressions[p_property] = p_expression.strip_edges();
	_theme_overrides |= uint64_t(1) << index;
	notify_property_list_changed();
	emit_changed();
	return OK;
}

String StyleBox::get_theme_expression(const StringName &p_property) const {
	const String *expression = _theme_expressions.getptr(p_property);
	return expression ? *expression : String();
}

bool StyleBox::is_theme_expression_property(const StringName &p_property) {
	return _get_theme_property_index(p_property) >= 0;
}

bool StyleBox::has_theme_override(const StringName &p_property) const {
	const int index = _get_theme_property_index(p_property);
	return index >= 0 && (_theme_overrides & (uint64_t(1) << index));
}

void StyleBox::clear_theme_override(const StringName &p_property) {
	const int index = _get_theme_property_index(p_property);
	ERR_FAIL_COND_MSG(index < 0, vformat("Unknown StyleBox theme property '%s'.", p_property));
	_set_theme_value(index, _get_theme_native_default(index));
	++_theme_override_revision;
	_theme_overrides &= ~(uint64_t(1) << index);
	_theme_expressions.erase(p_property);
	notify_property_list_changed();
	emit_changed();
}

Ref<StyleBox> StyleBox::resolve_theme(const Theme &p_theme) const {
	return _resolve_theme(p_theme, nullptr);
}

Ref<StyleBox> StyleBox::resolve_theme_in_context(ThemeResolver &r_context) const {
	ERR_FAIL_COND_V(r_context._themes.is_empty(), Ref<StyleBox>());
	return _resolve_theme(*r_context._themes[0].ptr(), &r_context);
}

void StyleBox::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_theme_expression", "property", "expression"), &StyleBox::set_theme_expression);
	ClassDB::bind_method(D_METHOD("get_theme_expression", "property"), &StyleBox::get_theme_expression);
	ClassDB::bind_method(D_METHOD("clear_theme_override", "property"), &StyleBox::clear_theme_override);
	ClassDB::bind_method(D_METHOD("_set_theme_value", "property", "value"), &StyleBox::_set_theme_value);
	ClassDB::bind_method(D_METHOD("_get_theme_value", "property"), &StyleBox::_get_theme_value);
	ClassDB::bind_method(D_METHOD("set_texture", "texture"), &StyleBox::set_texture);
	ClassDB::bind_method(D_METHOD("get_texture"), &StyleBox::get_texture);

	ClassDB::bind_method(D_METHOD("set_texture_modulate", "texture_modulate"), &StyleBox::set_texture_modulate);
	ClassDB::bind_method(D_METHOD("get_texture_modulate"), &StyleBox::get_texture_modulate);

	ClassDB::bind_method(D_METHOD("set_texture_slice", "side", "texture_slice"), &StyleBox::set_texture_slice);
	ClassDB::bind_method(D_METHOD("get_texture_slice", "side"), &StyleBox::get_texture_slice);
	ClassDB::bind_method(D_METHOD("set_texture_slice_all", "texture_slice"), &StyleBox::set_texture_slice_all);
	ClassDB::bind_method(D_METHOD("set_texture_slice_individual", "left", "top", "right", "bottom"), &StyleBox::set_texture_slice_individual);

	ClassDB::bind_method(D_METHOD("set_h_axis_stretch_mode", "axis_stretch_horizontal"), &StyleBox::set_h_axis_stretch_mode);
	ClassDB::bind_method(D_METHOD("get_h_axis_stretch_mode"), &StyleBox::get_h_axis_stretch_mode);
	ClassDB::bind_method(D_METHOD("set_v_axis_stretch_mode", "axis_stretch_vertical"), &StyleBox::set_v_axis_stretch_mode);
	ClassDB::bind_method(D_METHOD("get_v_axis_stretch_mode"), &StyleBox::get_v_axis_stretch_mode);

	ClassDB::bind_method(D_METHOD("set_region_rect", "region_rect"), &StyleBox::set_region_rect);
	ClassDB::bind_method(D_METHOD("get_region_rect"), &StyleBox::get_region_rect);

	ClassDB::bind_method(D_METHOD("set_background_color", "background_color"), &StyleBox::set_background_color);
	ClassDB::bind_method(D_METHOD("get_background_color"), &StyleBox::get_background_color);

	ClassDB::bind_method(D_METHOD("set_skew", "skew"), &StyleBox::set_skew);
	ClassDB::bind_method(D_METHOD("get_skew"), &StyleBox::get_skew);

	ClassDB::bind_method(D_METHOD("set_padding", "side", "padding"), &StyleBox::set_padding);
	ClassDB::bind_method(D_METHOD("get_padding", "side"), &StyleBox::get_padding);
	ClassDB::bind_method(D_METHOD("set_padding_all", "padding"), &StyleBox::set_padding_all);
	ClassDB::bind_method(D_METHOD("set_padding_individual", "left", "top", "right", "bottom"), &StyleBox::set_padding_individual);

	ClassDB::bind_method(D_METHOD("set_border_color", "border_color"), &StyleBox::set_border_color);
	ClassDB::bind_method(D_METHOD("get_border_color"), &StyleBox::get_border_color);
	ClassDB::bind_method(D_METHOD("set_border_blend", "border_blend"), &StyleBox::set_border_blend);
	ClassDB::bind_method(D_METHOD("get_border_blend"), &StyleBox::get_border_blend);
	ClassDB::bind_method(D_METHOD("set_border_width", "side", "border_width"), &StyleBox::set_border_width);
	ClassDB::bind_method(D_METHOD("get_border_width", "side"), &StyleBox::get_border_width);
	ClassDB::bind_method(D_METHOD("set_border_width_all", "border_width"), &StyleBox::set_border_width_all);
	ClassDB::bind_method(D_METHOD("get_border_width_min"), &StyleBox::get_border_width_min);

	ClassDB::bind_method(D_METHOD("set_corner_radius", "corner", "corner_radius"), &StyleBox::set_corner_radius);
	ClassDB::bind_method(D_METHOD("get_corner_radius", "corner"), &StyleBox::get_corner_radius);
	ClassDB::bind_method(D_METHOD("set_corner_radius_all", "corner_radius"), &StyleBox::set_corner_radius_all);
	ClassDB::bind_method(D_METHOD("set_corner_radius_individual", "top_left", "top_right", "bottom_right", "bottom_left"), &StyleBox::set_corner_radius_individual);
	ClassDB::bind_method(D_METHOD("set_corner_detail", "corner_detail"), &StyleBox::set_corner_detail);
	ClassDB::bind_method(D_METHOD("get_corner_detail"), &StyleBox::get_corner_detail);

	ClassDB::bind_method(D_METHOD("set_expand_margin", "side", "expand_margin"), &StyleBox::set_expand_margin);
	ClassDB::bind_method(D_METHOD("get_expand_margin", "side"), &StyleBox::get_expand_margin);
	ClassDB::bind_method(D_METHOD("set_expand_margin_all", "expand_margin"), &StyleBox::set_expand_margin_all);
	ClassDB::bind_method(D_METHOD("set_expand_margin_individual", "left", "top", "right", "bottom"), &StyleBox::set_expand_margin_individual);

	ClassDB::bind_method(D_METHOD("set_shadow_color", "shadow_color"), &StyleBox::set_shadow_color);
	ClassDB::bind_method(D_METHOD("get_shadow_color"), &StyleBox::get_shadow_color);
	ClassDB::bind_method(D_METHOD("set_shadow_size", "shadow_size"), &StyleBox::set_shadow_size);
	ClassDB::bind_method(D_METHOD("get_shadow_size"), &StyleBox::get_shadow_size);
	ClassDB::bind_method(D_METHOD("set_shadow_offset", "shadow_offset"), &StyleBox::set_shadow_offset);
	ClassDB::bind_method(D_METHOD("get_shadow_offset"), &StyleBox::get_shadow_offset);

	ClassDB::bind_method(D_METHOD("set_anti_aliased", "anti_aliasing"), &StyleBox::set_anti_aliased);
	ClassDB::bind_method(D_METHOD("is_anti_aliased"), &StyleBox::is_anti_aliased);
	ClassDB::bind_method(D_METHOD("set_aa_size", "anti_aliasing_size"), &StyleBox::set_aa_size);
	ClassDB::bind_method(D_METHOD("get_aa_size"), &StyleBox::get_aa_size);

	ClassDB::bind_method(D_METHOD("set_minimum_size", "minimum_size"), &StyleBox::set_minimum_size);
	ClassDB::bind_method(D_METHOD("get_intrinsic_minimum_size"), &StyleBox::get_intrinsic_minimum_size);
	ClassDB::bind_method(D_METHOD("get_minimum_size"), &StyleBox::get_minimum_size);

	ClassDB::bind_method(D_METHOD("get_content_inset", "side"), &StyleBox::get_content_inset);
	ClassDB::bind_method(D_METHOD("get_content_offset"), &StyleBox::get_content_offset);

	ClassDB::bind_method(D_METHOD("draw", "canvas_item", "rect"), &StyleBox::draw);
	ClassDB::bind_method(D_METHOD("get_current_item_drawn"), &StyleBox::get_current_item_drawn);
	ClassDB::bind_method(D_METHOD("test_mask", "point", "rect"), &StyleBox::test_mask);

	ADD_PROPERTYI(PropertyInfo(Variant::NIL, "texture", PROPERTY_HINT_EXPRESSION, "", PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_NIL_IS_VARIANT), "_set_theme_value", "_get_theme_value", TEXTURE);
	ADD_PROPERTYI(PropertyInfo(Variant::NIL, "texture_modulate", PROPERTY_HINT_EXPRESSION, "", PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_NIL_IS_VARIANT), "_set_theme_value", "_get_theme_value", TEXTURE_MODULATE);
	ADD_GROUP("Texture Slices", "texture_slice_");
	ADD_PROPERTYI(PropertyInfo(Variant::NIL, "texture_slice_left", PROPERTY_HINT_EXPRESSION, "", PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_NIL_IS_VARIANT), "_set_theme_value", "_get_theme_value", TEXTURE_SLICE_LEFT);
	ADD_PROPERTYI(PropertyInfo(Variant::NIL, "texture_slice_top", PROPERTY_HINT_EXPRESSION, "", PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_NIL_IS_VARIANT), "_set_theme_value", "_get_theme_value", TEXTURE_SLICE_TOP);
	ADD_PROPERTYI(PropertyInfo(Variant::NIL, "texture_slice_right", PROPERTY_HINT_EXPRESSION, "", PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_NIL_IS_VARIANT), "_set_theme_value", "_get_theme_value", TEXTURE_SLICE_RIGHT);
	ADD_PROPERTYI(PropertyInfo(Variant::NIL, "texture_slice_bottom", PROPERTY_HINT_EXPRESSION, "", PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_NIL_IS_VARIANT), "_set_theme_value", "_get_theme_value", TEXTURE_SLICE_BOTTOM);
	ADD_GROUP("Axis Stretch", "axis_stretch_");
	ADD_PROPERTYI(PropertyInfo(Variant::NIL, "axis_stretch_horizontal", PROPERTY_HINT_EXPRESSION, "", PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_NIL_IS_VARIANT), "_set_theme_value", "_get_theme_value", AXIS_STRETCH_HORIZONTAL);
	ADD_PROPERTYI(PropertyInfo(Variant::NIL, "axis_stretch_vertical", PROPERTY_HINT_EXPRESSION, "", PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_NIL_IS_VARIANT), "_set_theme_value", "_get_theme_value", AXIS_STRETCH_VERTICAL);
	ADD_GROUP("Sub-Region", "region_");
	ADD_PROPERTYI(PropertyInfo(Variant::NIL, "region_rect", PROPERTY_HINT_EXPRESSION, "", PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_NIL_IS_VARIANT), "_set_theme_value", "_get_theme_value", REGION_RECT);
	ADD_GROUP("", "");
	ADD_PROPERTYI(PropertyInfo(Variant::NIL, "background_color", PROPERTY_HINT_EXPRESSION, "", PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_NIL_IS_VARIANT), "_set_theme_value", "_get_theme_value", BACKGROUND_COLOR);
	ADD_PROPERTYI(PropertyInfo(Variant::NIL, "skew", PROPERTY_HINT_EXPRESSION, "", PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_NIL_IS_VARIANT), "_set_theme_value", "_get_theme_value", SKEW);
	ADD_GROUP("Padding", "padding_");
	ADD_PROPERTYI(PropertyInfo(Variant::NIL, "padding_left", PROPERTY_HINT_EXPRESSION, "", PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_NIL_IS_VARIANT), "_set_theme_value", "_get_theme_value", PADDING_LEFT);
	ADD_PROPERTYI(PropertyInfo(Variant::NIL, "padding_top", PROPERTY_HINT_EXPRESSION, "", PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_NIL_IS_VARIANT), "_set_theme_value", "_get_theme_value", PADDING_TOP);
	ADD_PROPERTYI(PropertyInfo(Variant::NIL, "padding_right", PROPERTY_HINT_EXPRESSION, "", PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_NIL_IS_VARIANT), "_set_theme_value", "_get_theme_value", PADDING_RIGHT);
	ADD_PROPERTYI(PropertyInfo(Variant::NIL, "padding_bottom", PROPERTY_HINT_EXPRESSION, "", PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_NIL_IS_VARIANT), "_set_theme_value", "_get_theme_value", PADDING_BOTTOM);
	ADD_GROUP("Border", "border_");
	ADD_PROPERTYI(PropertyInfo(Variant::NIL, "border_color", PROPERTY_HINT_EXPRESSION, "", PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_NIL_IS_VARIANT), "_set_theme_value", "_get_theme_value", BORDER_COLOR);
	ADD_PROPERTYI(PropertyInfo(Variant::NIL, "border_blend", PROPERTY_HINT_EXPRESSION, "", PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_NIL_IS_VARIANT), "_set_theme_value", "_get_theme_value", BORDER_BLEND);
	ADD_SUBGROUP("Width", "border_width_");
	ADD_PROPERTYI(PropertyInfo(Variant::NIL, "border_width_left", PROPERTY_HINT_EXPRESSION, "", PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_NIL_IS_VARIANT), "_set_theme_value", "_get_theme_value", BORDER_WIDTH_LEFT);
	ADD_PROPERTYI(PropertyInfo(Variant::NIL, "border_width_top", PROPERTY_HINT_EXPRESSION, "", PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_NIL_IS_VARIANT), "_set_theme_value", "_get_theme_value", BORDER_WIDTH_TOP);
	ADD_PROPERTYI(PropertyInfo(Variant::NIL, "border_width_right", PROPERTY_HINT_EXPRESSION, "", PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_NIL_IS_VARIANT), "_set_theme_value", "_get_theme_value", BORDER_WIDTH_RIGHT);
	ADD_PROPERTYI(PropertyInfo(Variant::NIL, "border_width_bottom", PROPERTY_HINT_EXPRESSION, "", PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_NIL_IS_VARIANT), "_set_theme_value", "_get_theme_value", BORDER_WIDTH_BOTTOM);
	ADD_GROUP("Corner", "corner_");
	ADD_SUBGROUP("Radius", "corner_radius_");
	ADD_PROPERTYI(PropertyInfo(Variant::NIL, "corner_radius_top_left", PROPERTY_HINT_EXPRESSION, "", PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_NIL_IS_VARIANT), "_set_theme_value", "_get_theme_value", CORNER_RADIUS_TOP_LEFT);
	ADD_PROPERTYI(PropertyInfo(Variant::NIL, "corner_radius_top_right", PROPERTY_HINT_EXPRESSION, "", PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_NIL_IS_VARIANT), "_set_theme_value", "_get_theme_value", CORNER_RADIUS_TOP_RIGHT);
	ADD_PROPERTYI(PropertyInfo(Variant::NIL, "corner_radius_bottom_right", PROPERTY_HINT_EXPRESSION, "", PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_NIL_IS_VARIANT), "_set_theme_value", "_get_theme_value", CORNER_RADIUS_BOTTOM_RIGHT);
	ADD_PROPERTYI(PropertyInfo(Variant::NIL, "corner_radius_bottom_left", PROPERTY_HINT_EXPRESSION, "", PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_NIL_IS_VARIANT), "_set_theme_value", "_get_theme_value", CORNER_RADIUS_BOTTOM_LEFT);
	ADD_PROPERTYI(PropertyInfo(Variant::NIL, "corner_detail", PROPERTY_HINT_EXPRESSION, "", PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_NIL_IS_VARIANT), "_set_theme_value", "_get_theme_value", CORNER_DETAIL);
	ADD_GROUP("Expand Margins", "expand_margin_");
	ADD_PROPERTYI(PropertyInfo(Variant::NIL, "expand_margin_left", PROPERTY_HINT_EXPRESSION, "", PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_NIL_IS_VARIANT), "_set_theme_value", "_get_theme_value", EXPAND_MARGIN_LEFT);
	ADD_PROPERTYI(PropertyInfo(Variant::NIL, "expand_margin_top", PROPERTY_HINT_EXPRESSION, "", PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_NIL_IS_VARIANT), "_set_theme_value", "_get_theme_value", EXPAND_MARGIN_TOP);
	ADD_PROPERTYI(PropertyInfo(Variant::NIL, "expand_margin_right", PROPERTY_HINT_EXPRESSION, "", PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_NIL_IS_VARIANT), "_set_theme_value", "_get_theme_value", EXPAND_MARGIN_RIGHT);
	ADD_PROPERTYI(PropertyInfo(Variant::NIL, "expand_margin_bottom", PROPERTY_HINT_EXPRESSION, "", PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_NIL_IS_VARIANT), "_set_theme_value", "_get_theme_value", EXPAND_MARGIN_BOTTOM);
	ADD_GROUP("Shadow", "shadow_");
	ADD_PROPERTYI(PropertyInfo(Variant::NIL, "shadow_color", PROPERTY_HINT_EXPRESSION, "", PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_NIL_IS_VARIANT), "_set_theme_value", "_get_theme_value", SHADOW_COLOR);
	ADD_PROPERTYI(PropertyInfo(Variant::NIL, "shadow_size", PROPERTY_HINT_EXPRESSION, "", PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_NIL_IS_VARIANT), "_set_theme_value", "_get_theme_value", SHADOW_SIZE);
	ADD_PROPERTYI(PropertyInfo(Variant::NIL, "shadow_offset", PROPERTY_HINT_EXPRESSION, "", PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_NIL_IS_VARIANT), "_set_theme_value", "_get_theme_value", SHADOW_OFFSET);
	ADD_GROUP("Anti Aliasing", "anti_aliasing_");
	ADD_PROPERTYI(PropertyInfo(Variant::NIL, "anti_aliasing", PROPERTY_HINT_EXPRESSION, "", PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_NIL_IS_VARIANT), "_set_theme_value", "_get_theme_value", ANTI_ALIASING);
	ADD_PROPERTYI(PropertyInfo(Variant::NIL, "anti_aliasing_size", PROPERTY_HINT_EXPRESSION, "", PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_NIL_IS_VARIANT), "_set_theme_value", "_get_theme_value", ANTI_ALIASING_SIZE);
	ADD_GROUP("", "");
	ADD_PROPERTYI(PropertyInfo(Variant::NIL, "minimum_size", PROPERTY_HINT_EXPRESSION, "", PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_NIL_IS_VARIANT), "_set_theme_value", "_get_theme_value", MINIMUM_SIZE);
	// A literal equal to the native default is still an explicit override of the theme.
	for (int i = 0; i < MAX_THEME_PROPERTIES; i++) {
		ClassDB::set_property_default_value(get_class_static(), _get_theme_property_name(i), Variant());
	}

	BIND_ENUM_CONSTANT(STRETCH);
	BIND_ENUM_CONSTANT(TILE);
	BIND_ENUM_CONSTANT(TILE_FIT);

	GDVIRTUAL_BIND(_get_minimum_size)
	GDVIRTUAL_BIND(_draw, "to_canvas_item", "rect")
	GDVIRTUAL_BIND(_get_draw_rect, "rect")
	GDVIRTUAL_BIND(_test_mask, "point", "rect")
}

void StyleBox::_validate_property(PropertyInfo &r_property) const {
	const int index = _get_theme_property_index(r_property.name);
	if (index >= 0) {
		if (!(_theme_overrides & (uint64_t(1) << index))) {
			r_property.usage &= ~PROPERTY_USAGE_STORAGE;
		} else {
			r_property.usage |= PROPERTY_USAGE_STORE_IF_DEFAULT;
			if (_get_theme_property_type(index) == Variant::OBJECT) {
				r_property.usage |= PROPERTY_USAGE_STORE_IF_NULL;
			}
		}
		if (_theme_expressions.has(r_property.name)) {
			r_property.type = Variant::STRING;
			r_property.hint = PROPERTY_HINT_EXPRESSION;
			r_property.hint_string = "";
		} else {
			r_property.type = _get_theme_property_type(index);
			r_property.hint = PROPERTY_HINT_NONE;
			r_property.hint_string = "";
			if (r_property.type == Variant::OBJECT) {
				r_property.hint = PROPERTY_HINT_RESOURCE_TYPE;
				r_property.hint_string = String(Texture2D::get_class_static());
			} else if (index == CORNER_DETAIL) {
				r_property.hint = PROPERTY_HINT_RANGE;
				r_property.hint_string = "1,20,1";
			} else if (index == ANTI_ALIASING_SIZE) {
				r_property.hint = PROPERTY_HINT_RANGE;
				r_property.hint_string = "0.01,10,0.01,suffix:px";
			} else if (r_property.type == Variant::INT || r_property.type == Variant::FLOAT) {
				const bool allows_negative = index >= EXPAND_MARGIN_LEFT && index <= EXPAND_MARGIN_BOTTOM;
				const String step = r_property.type == Variant::INT ? "1" : "0.01";
				r_property.hint = PROPERTY_HINT_RANGE;
				r_property.hint_string = (allows_negative ? "-100,100," : "0,100,") + step + (allows_negative ? ",or_less,or_greater,suffix:px" : ",or_greater,suffix:px");
			}
		}
	}
	if (!Engine::get_singleton()->is_editor_hint()) {
		return;
	}
	if (!_anti_aliased && r_property.name == "anti_aliasing_size") {
		r_property.usage &= ~PROPERTY_USAGE_EDITOR;
	}
	if (_texture.is_null() && (r_property.name.begins_with("texture_slice_") || r_property.name.begins_with("axis_stretch_") || r_property.name == "region_rect" || r_property.name == "texture_modulate")) {
		r_property.usage &= ~PROPERTY_USAGE_EDITOR;
	}
}

bool StyleBox::_property_can_revert(const StringName &p_property) const {
	return has_theme_override(p_property);
}

bool StyleBox::_property_get_revert(const StringName &p_property, Variant &r_value) const {
	if (!has_theme_override(p_property)) {
		return false;
	}
	r_value = String();
	return true;
}

void StyleBox::reset_state() {
	for (int i = 0; i < MAX_THEME_PROPERTIES; i++) {
		_set_theme_value(i, _get_theme_native_default(i));
	}
	++_theme_override_revision;
	_theme_overrides = 0;
	_theme_expressions.clear();
	Resource::reset_state();
	notify_property_list_changed();
	emit_changed();
}

void StyleBox::_set_inner_corner_radius(const Rect2 p_style_rect, const Rect2 p_inner_rect, const real_t p_corner_radius[4], real_t *r_inner_corner_radius) {
	real_t border_left = p_inner_rect.position.x - p_style_rect.position.x;
	real_t border_top = p_inner_rect.position.y - p_style_rect.position.y;
	real_t border_right = p_style_rect.size.width - p_inner_rect.size.width - border_left;
	real_t border_bottom = p_style_rect.size.height - p_inner_rect.size.height - border_top;

	r_inner_corner_radius[0] = MAX(p_corner_radius[0] - MIN(border_top, border_left), 0); // Top left.
	r_inner_corner_radius[1] = MAX(p_corner_radius[1] - MIN(border_top, border_right), 0); // Top right.
	r_inner_corner_radius[2] = MAX(p_corner_radius[2] - MIN(border_bottom, border_right), 0); // Bottom right.
	r_inner_corner_radius[3] = MAX(p_corner_radius[3] - MIN(border_bottom, border_left), 0); // Bottom left.
}

void StyleBox::_set_corner_scale(const Rect2 &p_style_rect, const Rect2 &p_inner_rect, const real_t p_corner_radius[4], Point2 *r_inner_scale) {
	real_t border_left = p_inner_rect.position.x - p_style_rect.position.x;
	real_t border_top = p_inner_rect.position.y - p_style_rect.position.y;
	real_t border_right = p_style_rect.size.width - p_inner_rect.size.width - border_left;
	real_t border_bottom = p_style_rect.size.height - p_inner_rect.size.height - border_top;

	// Amount of overflow along an edge.
	// Ex. SIDE_LEFT edge is the overflow between top_left and bottom_left corners.
	// MIN(0,) is to ignore underflow, and negating is to make values positive.
	real_t edge_overflow[4] = {
		-MIN(0, p_inner_rect.size.y - p_corner_radius[CORNER_TOP_LEFT] - p_corner_radius[CORNER_BOTTOM_LEFT]),
		-MIN(0, p_inner_rect.size.x - p_corner_radius[CORNER_TOP_LEFT] - p_corner_radius[CORNER_TOP_RIGHT]),
		-MIN(0, p_inner_rect.size.y - p_corner_radius[CORNER_TOP_RIGHT] - p_corner_radius[CORNER_BOTTOM_RIGHT]),
		-MIN(0, p_inner_rect.size.x - p_corner_radius[CORNER_BOTTOM_LEFT] - p_corner_radius[CORNER_BOTTOM_RIGHT])
	};

	// Sums of borders.
	real_t hb_sum = border_left + border_right;
	real_t vb_sum = border_top + border_bottom;

	// Ratio of each side to the sum of itself and opposite side.
	// Since overflow only happens with opposite borders, you only need to get the ratio of each border relative to the sum of involved borders.
	real_t ratios[4] = {
		// Prevent divide by 0 errors.
		hb_sum > 0 ? (border_left / hb_sum) : 0,
		vb_sum > 0 ? (border_top / vb_sum) : 0,
		hb_sum > 0 ? (border_right / hb_sum) : 0,
		vb_sum > 0 ? (border_bottom / vb_sum) : 0
	};

	// Raw amount each corner should shrink.
	Point2 corner_reduction[4] = {
		Point2(edge_overflow[SIDE_TOP] * ratios[SIDE_LEFT], edge_overflow[SIDE_LEFT] * ratios[SIDE_TOP]),
		Point2(edge_overflow[SIDE_TOP] * ratios[SIDE_RIGHT], edge_overflow[SIDE_RIGHT] * ratios[SIDE_TOP]),
		Point2(edge_overflow[SIDE_BOTTOM] * ratios[SIDE_RIGHT], edge_overflow[SIDE_RIGHT] * ratios[SIDE_BOTTOM]),
		Point2(edge_overflow[SIDE_BOTTOM] * ratios[SIDE_LEFT], edge_overflow[SIDE_LEFT] * ratios[SIDE_BOTTOM]),
	};

	// Corner Radii as Point2s.
	Point2 pcr[4] = {
		Point2(p_corner_radius[0], p_corner_radius[0]),
		Point2(p_corner_radius[1], p_corner_radius[1]),
		Point2(p_corner_radius[2], p_corner_radius[2]),
		Point2(p_corner_radius[3], p_corner_radius[3]),
	};

	// If corner radii are too small, they won't shrink the full amount.
	// Adjacent corners will have to shrink the leftovers if they can.
	// Minf(0) is to ignore non-leftovers, and negating is to make values positive.
	Point2 leftovers[4] = {
		-((pcr[0] - corner_reduction[0]).minf(0)),
		-((pcr[1] - corner_reduction[1]).minf(0)),
		-((pcr[2] - corner_reduction[2]).minf(0)),
		-((pcr[3] - corner_reduction[3]).minf(0)),
	};

	// New shrunken radii after distributing the leftovers.
	Point2 distributed[4] = {
		((pcr[0] - corner_reduction[0] - leftovers[3] - leftovers[1]).maxf(0)),
		((pcr[1] - corner_reduction[1] - leftovers[0] - leftovers[2]).maxf(0)),
		((pcr[2] - corner_reduction[2] - leftovers[1] - leftovers[3]).maxf(0)),
		((pcr[3] - corner_reduction[3] - leftovers[2] - leftovers[0]).maxf(0)),
	};

	// How much the curve should scale to achieve the shrunken radii.
	for (int i = 0; i < 4; i++) {
		// Unshrinkable is how much is still left over, even after distributing leftovers.
		// Exclude it from the final scale.
		Point2 unshrinkable = (leftovers[(i + 1) % 4] + leftovers[(i + 4 - 1) % 4] - distributed[i]).maxf(0);
		r_inner_scale[i] = distributed[i] / (pcr[i] - unshrinkable).maxf(FLT_EPSILON);
	}
}

void StyleBox::_draw_rounded_rectangle(Vector<Vector2> &r_verts, Vector<int> &r_indices, Vector<Color> &r_colors, const Rect2 &p_style_rect, const real_t p_corner_radius[4],
		const Rect2 &p_ring_rect, const Rect2 &p_inner_rect, const Color &p_inner_color, const Color &p_outer_color, const int p_corner_detail, const Vector2 &p_skew, bool p_is_filled) {
	int vert_offset = r_verts.size();
	int adapted_corner_detail = (p_corner_radius[0] > 0) || (p_corner_radius[1] > 0) || (p_corner_radius[2] > 0) || (p_corner_radius[3] > 0) ? p_corner_detail : 1;

	bool draw_border = !p_is_filled;

	real_t ring_corner_radius[4];
	_set_inner_corner_radius(p_style_rect, p_ring_rect, p_corner_radius, ring_corner_radius);

	Point2 ring_scale[4];
	_set_corner_scale(p_style_rect, p_ring_rect, ring_corner_radius, ring_scale);

	// Corner radius center points.
	Vector<Point2> outer_points = {
		p_ring_rect.position + Vector2(ring_corner_radius[0], ring_corner_radius[0]) * ring_scale[0], //tl
		Point2(p_ring_rect.position.x + p_ring_rect.size.x - ring_corner_radius[1] * ring_scale[1].x, p_ring_rect.position.y + ring_corner_radius[1] * ring_scale[1].y), //tr
		p_ring_rect.position + p_ring_rect.size - Vector2(ring_corner_radius[2], ring_corner_radius[2]) * ring_scale[2], //br
		Point2(p_ring_rect.position.x + ring_corner_radius[3] * ring_scale[3].x, p_ring_rect.position.y + p_ring_rect.size.y - ring_corner_radius[3] * ring_scale[3].y) //bl
	};

	real_t inner_corner_radius[4];
	_set_inner_corner_radius(p_style_rect, p_inner_rect, p_corner_radius, inner_corner_radius);

	Point2 inner_scale[4];
	_set_corner_scale(p_style_rect, p_inner_rect, inner_corner_radius, inner_scale);

	Vector<Point2> inner_points = {
		p_inner_rect.position + Vector2(inner_corner_radius[0], inner_corner_radius[0]) * inner_scale[0], //tl
		Point2(p_inner_rect.position.x + p_inner_rect.size.x - inner_corner_radius[1] * inner_scale[1].x, p_inner_rect.position.y + inner_corner_radius[1] * inner_scale[1].y), //tr
		p_inner_rect.position + p_inner_rect.size - Vector2(inner_corner_radius[2], inner_corner_radius[2]) * inner_scale[2], //br
		Point2(p_inner_rect.position.x + inner_corner_radius[3] * inner_scale[3].x, p_inner_rect.position.y + p_inner_rect.size.y - inner_corner_radius[3] * inner_scale[3].y) //bl
	};

	// Calculate the vertices.

	// If the center is filled, we do not draw the border and directly use the inner ring as reference. Because all calls to this
	// method either draw a ring or a filled rounded rectangle, but not both.
	const real_t quarter_arc_rad = Math::PI / 2.0;
	const Point2 style_rect_center = p_style_rect.get_center();

	const int colors_size = r_colors.size();
	const int verts_size = r_verts.size();
	const int new_verts_amount = (adapted_corner_detail + 1) * (draw_border ? 8 : 4);

	r_colors.resize(colors_size + new_verts_amount);
	r_verts.resize(verts_size + new_verts_amount);
	Color *colors_ptr = r_colors.ptrw();
	Vector2 *verts_ptr = r_verts.ptrw();

	for (int corner_idx = 0; corner_idx < 4; corner_idx++) {
		for (int detail = 0; detail <= adapted_corner_detail; detail++) {
			int idx_ofs = (adapted_corner_detail + 1) * corner_idx + detail;
			if (draw_border) {
				idx_ofs *= 2;
			}

			const real_t pt_angle = (corner_idx + detail / (double)adapted_corner_detail) * quarter_arc_rad + Math::PI;
			const real_t angle_cosine = Math::cos(pt_angle);
			const real_t angle_sine = Math::sin(pt_angle);

			{
				const real_t x = inner_corner_radius[corner_idx] * angle_cosine * inner_scale[corner_idx].x + inner_points[corner_idx].x;
				const real_t y = inner_corner_radius[corner_idx] * angle_sine * inner_scale[corner_idx].y + inner_points[corner_idx].y;
				const float x_skew = -p_skew.x * (y - style_rect_center.y);
				const float y_skew = -p_skew.y * (x - style_rect_center.x);
				verts_ptr[verts_size + idx_ofs] = Vector2(x + x_skew, y + y_skew);
				colors_ptr[colors_size + idx_ofs] = p_inner_color;
			}

			if (draw_border) {
				const real_t x = ring_corner_radius[corner_idx] * angle_cosine * ring_scale[corner_idx].x + outer_points[corner_idx].x;
				const real_t y = ring_corner_radius[corner_idx] * angle_sine * ring_scale[corner_idx].y + outer_points[corner_idx].y;
				const float x_skew = -p_skew.x * (y - style_rect_center.y);
				const float y_skew = -p_skew.y * (x - style_rect_center.x);
				verts_ptr[verts_size + idx_ofs + 1] = Vector2(x + x_skew, y + y_skew);
				colors_ptr[colors_size + idx_ofs + 1] = p_outer_color;
			}
		}
	}

	int ring_vert_count = r_verts.size() - vert_offset;

	// Fill the r_indices and the r_colors for the border.

	if (draw_border) {
		int indices_size = r_indices.size();
		r_indices.resize(indices_size + ring_vert_count * 3);
		int *indices_ptr = r_indices.ptrw();

		for (int i = 0; i < ring_vert_count; i++) {
			int idx_ofs = indices_size + i * 3;
			indices_ptr[idx_ofs] = vert_offset + i % ring_vert_count;
			indices_ptr[idx_ofs + 1] = vert_offset + (i + 2) % ring_vert_count;
			indices_ptr[idx_ofs + 2] = vert_offset + (i + 1) % ring_vert_count;
		}
	}

	if (p_is_filled) {
		// Compute the triangles pattern to draw the rounded rectangle.
		// Consists of vertical stripes of two triangles each.

		int stripes_count = ring_vert_count / 2 - 1;
		int last_vert_id = ring_vert_count - 1;

		int indices_size = r_indices.size();
		r_indices.resize(indices_size + stripes_count * 6);
		int *indices_ptr = r_indices.ptrw();

		for (int i = 0; i < stripes_count; i++) {
			int idx_ofs = indices_size + i * 6;
			// Polygon 1.
			indices_ptr[idx_ofs] = vert_offset + i;
			indices_ptr[idx_ofs + 1] = vert_offset + last_vert_id - i - 1;
			indices_ptr[idx_ofs + 2] = vert_offset + i + 1;
			// Polygon 2.
			indices_ptr[idx_ofs + 3] = vert_offset + i;
			indices_ptr[idx_ofs + 4] = vert_offset + last_vert_id - i;
			indices_ptr[idx_ofs + 5] = vert_offset + last_vert_id - i - 1;
		}
	}
}

void StyleBox::_adapt_values(int p_index_a, int p_index_b, real_t *r_adapted_values, const real_t *p_values, const real_t p_width, const real_t p_max_a, const real_t p_max_b) {
	real_t value_a = p_values[p_index_a];
	real_t value_b = p_values[p_index_b];
	real_t factor = (value_a + value_b == 0.0) ? 1.0 : MIN(1.0, p_width / (value_a + value_b));
	r_adapted_values[p_index_a] = MIN(MIN(value_a * factor, p_max_a), r_adapted_values[p_index_a]);
	r_adapted_values[p_index_b] = MIN(MIN(value_b * factor, p_max_b), r_adapted_values[p_index_b]);
}

void StyleBox::_submit_geometry(RID p_canvas_item, const Rect2 &p_rect, real_t p_aa_size, const Vector<Vector2> &p_vertices, const Vector<int> &p_indices, const Vector<Color> &p_colors) {
	if (p_vertices.is_empty() || p_indices.is_empty()) {
		return;
	}
	const Rect2 uv_rect = p_rect.grow(p_aa_size);
	Vector<Point2> uvs;
	uvs.resize(p_vertices.size());
	for (int i = 0; i < p_vertices.size(); i++) {
		uvs.write[i] = (p_vertices[i] - uv_rect.position) / uv_rect.size;
	}
	RenderingServer::get_singleton()->canvas_item_add_triangle_array(p_canvas_item, p_indices, p_vertices, p_colors, uvs);
}

Vector<StyleBox::TextureAxisSegment> StyleBox::_make_texture_axis(real_t p_draw_size, real_t p_source_size, real_t p_begin, real_t p_end, AxisStretchMode p_mode) {
	Vector<TextureAxisSegment> segments;
	const real_t source_scale = MIN(1.0, p_source_size / MAX(p_begin + p_end, (real_t)CMP_EPSILON));
	p_begin *= source_scale;
	p_end *= source_scale;
	const real_t draw_scale = MIN(1.0, p_draw_size / MAX(p_begin + p_end, (real_t)CMP_EPSILON));
	const real_t draw_begin = p_begin * draw_scale;
	const real_t draw_end = p_draw_size - p_end * draw_scale;
	if (draw_begin > 0) {
		segments.push_back({ 0, draw_begin, 0, p_begin });
	}
	const real_t source_center = MAX(p_source_size - p_begin - p_end, (real_t)0);
	const real_t center_size = draw_end - draw_begin;
	if (center_size > 0) {
		if (p_mode == STRETCH || source_center <= CMP_EPSILON) {
			segments.push_back({ draw_begin, draw_end, p_begin, p_source_size - p_end });
		} else {
			const real_t repeats = MAX(1.0, Math::floor(center_size / source_center + 0.5));
			const real_t tile_size = p_mode == TILE_FIT ? center_size / repeats : source_center;
			for (real_t begin = draw_begin; begin < draw_end;) {
				const real_t end = MIN(begin + tile_size, draw_end);
				if (end <= begin) {
					break;
				}
				segments.push_back({ begin, end, p_begin, p_begin + source_center * (end - begin) / tile_size });
				begin = end;
			}
		}
	}
	if (draw_end < p_draw_size) {
		segments.push_back({ draw_end, p_draw_size, p_source_size - p_end, p_source_size });
	}
	return segments;
}

Vector<StyleBox::TextureVertex> StyleBox::_clip_texture_polygon(const Vector<TextureVertex> &p_polygon, int p_axis, real_t p_edge, bool p_keep_greater) {
	Vector<TextureVertex> result;
	if (p_polygon.is_empty()) {
		return result;
	}
	TextureVertex previous = p_polygon[p_polygon.size() - 1];
	real_t previous_distance = previous.position[p_axis] - p_edge;
	bool previous_inside = p_keep_greater ? previous_distance >= 0 : previous_distance <= 0;
	for (const TextureVertex &current : p_polygon) {
		const real_t distance = current.position[p_axis] - p_edge;
		const bool inside = p_keep_greater ? distance >= 0 : distance <= 0;
		if (inside != previous_inside) {
			const real_t weight = previous_distance / (previous_distance - distance);
			result.push_back({ previous.position.lerp(current.position, weight), previous.color.lerp(current.color, weight) });
		}
		if (inside) {
			result.push_back(current);
		}
		previous = current;
		previous_distance = distance;
		previous_inside = inside;
	}
	return result;
}

void StyleBox::_draw_texture(RID p_canvas_item, const Rect2 &p_rect, const real_t *p_corner_radius, real_t p_aa_size) const {
	Rect2 rect = p_rect;
	Rect2 source_rect = _region_rect;
	Ref<Texture2D> source_texture = _texture;
	// Resolve atlas coordinates before normalizing UVs against the underlying image.
	while (true) {
		Ref<AtlasTexture> atlas = source_texture;
		if (atlas.is_null()) {
			break;
		}
		if (!atlas->get_rect_region(rect, source_rect, rect, source_rect)) {
			return;
		}
		source_texture = atlas->get_atlas();
		if (source_texture.is_null()) {
			return;
		}
	}
	const RID texture_rid = source_texture->get_scaled_rid();
	if (!texture_rid.is_valid()) {
		return;
	}
	const Size2 source_size = source_texture->get_size();
	if (source_rect.size == Size2()) {
		source_rect.size = source_size;
	}
	if (!source_texture->get_rect_region(rect, source_rect, rect, source_rect) || rect.size.x <= 0 || rect.size.y <= 0 || source_rect.size.x <= 0 || source_rect.size.y <= 0 || source_size.x <= 0 || source_size.y <= 0) {
		return;
	}
	const bool rounded = p_corner_radius[0] > 0 || p_corner_radius[1] > 0 || p_corner_radius[2] > 0 || p_corner_radius[3] > 0;
	const Vector2 start(_texture_slice[SIDE_LEFT], _texture_slice[SIDE_TOP]);
	const Vector2 end(_texture_slice[SIDE_RIGHT], _texture_slice[SIDE_BOTTOM]);
	const bool slices_fit = start.x + end.x <= MIN(rect.size.x, source_rect.size.x) && start.y + end.y <= MIN(rect.size.y, source_rect.size.y);
	// DPI textures use logical source coordinates with a potentially larger rasterized RID.
	if (!rounded && _skew.is_zero_approx() && slices_fit && !Object::cast_to<DPITexture>(source_texture.ptr())) {
		RenderingServer::get_singleton()->canvas_item_add_nine_patch(p_canvas_item, rect, source_rect, texture_rid, start, end, RSE::NinePatchAxisMode(_axis_h), RSE::NinePatchAxisMode(_axis_v), true, _texture_modulate);
		return;
	}

	Vector<Vector2> shape_vertices;
	Vector<int> shape_indices;
	Vector<Color> shape_colors;
	const real_t aa_size = MIN(p_aa_size, MIN(p_rect.size.x, p_rect.size.y));
	const Rect2 fill_rect = p_rect.grow(-aa_size * 0.5);
	_draw_rounded_rectangle(shape_vertices, shape_indices, shape_colors, p_rect, p_corner_radius,
			fill_rect, fill_rect, _texture_modulate, _texture_modulate, _corner_detail, Vector2(), true);
	if (aa_size > 0) {
		Color transparent = _texture_modulate;
		transparent.a = 0;
		_draw_rounded_rectangle(shape_vertices, shape_indices, shape_colors, p_rect, p_corner_radius,
				p_rect.grow(aa_size * 0.5), fill_rect, _texture_modulate, transparent, _corner_detail, Vector2());
	}

	const Vector<TextureAxisSegment> horizontal = _make_texture_axis(rect.size.x, source_rect.size.x, start.x, end.x, _axis_h);
	const Vector<TextureAxisSegment> vertical = _make_texture_axis(rect.size.y, source_rect.size.y, start.y, end.y, _axis_v);
	Vector<Vector2> vertices;
	Vector<int> indices;
	Vector<Color> colors;
	Vector<Point2> uvs;
	const Vector2 center = p_rect.get_center();
	for (int y = 0; y < vertical.size(); y++) {
		const TextureAxisSegment &v = vertical[y];
		for (int x = 0; x < horizontal.size(); x++) {
			const TextureAxisSegment &h = horizontal[x];
			const Vector2 clip_begin = rect.position + Vector2(h.begin - (x == 0 ? aa_size * 0.5 : 0), v.begin - (y == 0 ? aa_size * 0.5 : 0));
			const Vector2 clip_end = rect.position + Vector2(h.end + (x == horizontal.size() - 1 ? aa_size * 0.5 : 0), v.end + (y == vertical.size() - 1 ? aa_size * 0.5 : 0));
			for (int i = 0; i < shape_indices.size(); i += 3) {
				Vector<TextureVertex> polygon;
				for (int j = 0; j < 3; j++) {
					const int index = shape_indices[i + j];
					polygon.push_back({ shape_vertices[index], shape_colors[index] });
				}
				polygon = _clip_texture_polygon(polygon, 0, clip_begin.x, true);
				polygon = _clip_texture_polygon(polygon, 0, clip_end.x, false);
				polygon = _clip_texture_polygon(polygon, 1, clip_begin.y, true);
				polygon = _clip_texture_polygon(polygon, 1, clip_end.y, false);
				for (int j = 1; j + 1 < polygon.size(); j++) {
					const TextureVertex triangle[3] = { polygon[0], polygon[j], polygon[j + 1] };
					for (const TextureVertex &vertex : triangle) {
						const Vector2 ratio(CLAMP((vertex.position.x - rect.position.x - h.begin) / (h.end - h.begin), 0.0, 1.0), CLAMP((vertex.position.y - rect.position.y - v.begin) / (v.end - v.begin), 0.0, 1.0));
						const Vector2 source(source_rect.position.x + Math::lerp(h.source_begin, h.source_end, ratio.x), source_rect.position.y + Math::lerp(v.source_begin, v.source_end, ratio.y));
						const Vector2 offset = vertex.position - center;
						indices.push_back(vertices.size());
						vertices.push_back(vertex.position + Vector2(-_skew.x * offset.y, -_skew.y * offset.x));
						colors.push_back(vertex.color);
						uvs.push_back(source / source_size);
					}
				}
				// Keep temporary mesh memory bounded even for heavily tiled styles.
				if (vertices.size() >= 60000) {
					RenderingServer::get_singleton()->canvas_item_add_triangle_array(p_canvas_item, indices, vertices, colors, uvs, Vector<int>(), Vector<float>(), texture_rid);
					vertices.clear();
					indices.clear();
					colors.clear();
					uvs.clear();
				}
			}
		}
	}
	if (!vertices.is_empty()) {
		RenderingServer::get_singleton()->canvas_item_add_triangle_array(p_canvas_item, indices, vertices, colors, uvs, Vector<int>(), Vector<float>(), texture_rid);
	}
}

void StyleBox::_texture_changed() {
	emit_changed();
}

Ref<StyleBox> StyleBox::_resolve_theme(const Theme &p_theme, ThemeResolver *r_context) const {
	const auto has_style_parameter = [&p_theme, r_context](const StringName &p_parameter) {
		return r_context ? r_context->has_style_parameter(p_parameter) : p_theme.has_style_parameter(p_parameter);
	};
	bool dependent = !_theme_expressions.is_empty();
	for (int i = 0; !dependent && i < CORNER_DETAIL; i++) {
		dependent = !(_theme_overrides & (uint64_t(1) << i)) && has_style_parameter(_get_theme_default_name(i));
	}
	if (!dependent) {
		return Ref<StyleBox>(const_cast<StyleBox *>(this));
	}
	Ref<StyleBox> resolved = duplicate();
	ERR_FAIL_COND_V(resolved.is_null(), Ref<StyleBox>());
	const double scale = r_context ? r_context->get_base_scale() : p_theme.has_default_base_scale() ? p_theme.get_default_base_scale() : 1.0;
	for (int i = 0; i < MAX_THEME_PROPERTIES; i++) {
		const StringName property = _get_theme_property_name(i);
		const String *source = _theme_expressions.getptr(property);
		Variant value;
		bool pixels = false;
		if (source) {
			Error error = OK;
			value = r_context ? r_context->evaluate_expression(*source, &pixels, &error) : p_theme.evaluate_theme_expression(*source, &pixels, &error);
			ERR_FAIL_COND_V_MSG(error != OK, Ref<StyleBox>(), vformat("Could not resolve '%s' in stylebox '%s'.", property, get_path()));
		} else {
			if (_theme_overrides & (uint64_t(1) << i)) {
				continue;
			}
			const StringName parameter = _get_theme_default_name(i);
			if (parameter.is_empty() || !has_style_parameter(parameter)) {
				continue;
			}
			Error error = OK;
			value = r_context ? r_context->get_style_parameter(parameter, error) : p_theme.get_style_parameter(parameter);
			ERR_FAIL_COND_V_MSG(error != OK, Ref<StyleBox>(), vformat("Could not resolve '%s' in stylebox '%s'.", parameter, get_path()));
		}
		const Variant::Type type = _get_theme_property_type(i);
		if (type == Variant::INT || type == Variant::FLOAT) {
			ERR_FAIL_COND_V_MSG(!value.is_num(), Ref<StyleBox>(), vformat("Theme value for '%s' must be numeric.", property));
			const bool dimension = i < BACKGROUND_COLOR || (i >= EXPAND_MARGIN_LEFT && i <= TEXTURE_SLICE_BOTTOM);
			const double number = double(value) * (dimension && !pixels ? scale : 1.0);
			ERR_FAIL_COND_V_MSG(!Math::is_finite(number) || number < INT32_MIN || number > INT32_MAX, Ref<StyleBox>(), vformat("Theme value for '%s' is outside the supported range.", property));
			value = number;
		} else if (type == Variant::VECTOR2 && (i == MINIMUM_SIZE || i == SHADOW_OFFSET) && !pixels) {
			ERR_FAIL_COND_V_MSG(value.get_type() != Variant::VECTOR2, Ref<StyleBox>(), vformat("Theme value for '%s' must be a Vector2.", property));
			const Vector2 vector = value;
			value = vector * scale;
		}
		if (type == Variant::OBJECT) {
			ERR_FAIL_COND_V_MSG(value.get_type() != Variant::OBJECT && value.get_type() != Variant::NIL, Ref<StyleBox>(), vformat("Theme value for '%s' must be a resource.", property));
		} else if (type != Variant::FLOAT && type != Variant::INT) {
			ERR_FAIL_COND_V_MSG(value.get_type() != type, Ref<StyleBox>(), vformat("Invalid theme value type for '%s'.", property));
		}
		if (type == Variant::COLOR) {
			const Color color = value;
			ERR_FAIL_COND_V_MSG(!Math::is_finite(color.r) || !Math::is_finite(color.g) || !Math::is_finite(color.b) || !Math::is_finite(color.a), Ref<StyleBox>(), vformat("Non-finite theme color for '%s'.", property));
		} else if (type == Variant::VECTOR2) {
			const Vector2 vector = value;
			ERR_FAIL_COND_V_MSG(!Math::is_finite(vector.x) || !Math::is_finite(vector.y), Ref<StyleBox>(), vformat("Non-finite theme vector for '%s'.", property));
		}
		resolved->_set_theme_value(i, value);
	}
	return resolved;
}

StringName StyleBox::_get_theme_property_name(int p_property) {
	static const char *const NAMES[] = {
		"padding_left", "padding_top", "padding_right", "padding_bottom",
		"border_width_left", "border_width_top", "border_width_right", "border_width_bottom",
		"corner_radius_top_left", "corner_radius_top_right", "corner_radius_bottom_right", "corner_radius_bottom_left",
		"corner_detail",
		"background_color", "border_color", "shadow_color",
		"expand_margin_left",
		"expand_margin_top",
		"expand_margin_right",
		"expand_margin_bottom",
		"shadow_size",
		"anti_aliasing_size",
		"texture_slice_left",
		"texture_slice_top",
		"texture_slice_right",
		"texture_slice_bottom",
		"minimum_size",
		"shadow_offset",
		"texture",
		"texture_modulate",
		"skew",
		"anti_aliasing",
		"border_blend",
		"axis_stretch_horizontal",
		"axis_stretch_vertical",
		"region_rect",

	};
	ERR_FAIL_INDEX_V(p_property, MAX_THEME_PROPERTIES, StringName());
	return NAMES[p_property];
}

Variant::Type StyleBox::_get_theme_property_type(int p_property) {
	if (p_property < BORDER_WIDTH_LEFT || (p_property >= EXPAND_MARGIN_LEFT && p_property <= EXPAND_MARGIN_BOTTOM) || p_property == ANTI_ALIASING_SIZE || (p_property >= TEXTURE_SLICE_LEFT && p_property <= TEXTURE_SLICE_BOTTOM)) {
		return Variant::FLOAT;
	}
	if (p_property < BACKGROUND_COLOR || p_property == SHADOW_SIZE || p_property == AXIS_STRETCH_HORIZONTAL || p_property == AXIS_STRETCH_VERTICAL) {
		return Variant::INT;
	}
	if (p_property == BACKGROUND_COLOR || p_property == BORDER_COLOR || p_property == SHADOW_COLOR || p_property == TEXTURE_MODULATE) {
		return Variant::COLOR;
	}
	if (p_property == MINIMUM_SIZE || p_property == SHADOW_OFFSET || p_property == SKEW) {
		return Variant::VECTOR2;
	}
	if (p_property == ANTI_ALIASING || p_property == BORDER_BLEND) {
		return Variant::BOOL;
	}
	return p_property == REGION_RECT ? Variant::RECT2 : Variant::OBJECT;
}

Variant StyleBox::_get_theme_native_default(int p_property) {
	switch (p_property) {
		case BACKGROUND_COLOR:
			return Color(0, 0, 0, 0);
		case BORDER_COLOR:
			return Color(0.8, 0.8, 0.8);
		case SHADOW_COLOR:
			return Color(0, 0, 0, 0.6);
		case TEXTURE_MODULATE:
			return Color(1, 1, 1, 1);
		case CORNER_DETAIL:
			return 8;
		case ANTI_ALIASING_SIZE:
			return 1.0;
		case ANTI_ALIASING:
			return true;
		case BORDER_BLEND:
			return false;
		case MINIMUM_SIZE:
		case SHADOW_OFFSET:
		case SKEW:
			return Vector2();
		case REGION_RECT:
			return Rect2();
		case TEXTURE:
			return Ref<Texture2D>();
		default:
			return 0;
	}
}

int StyleBox::_get_theme_property_index(const StringName &p_property) {
	for (int i = 0; i < MAX_THEME_PROPERTIES; i++) {
		if (p_property == _get_theme_property_name(i)) {
			return i;
		}
	}
	return -1;
}

StringName StyleBox::_get_theme_default_name(int p_property) {
	if (p_property < BORDER_WIDTH_LEFT) {
		return "default_padding";
	}
	if (p_property < CORNER_RADIUS_TOP_LEFT) {
		return "default_border_width";
	}
	if (p_property < CORNER_DETAIL) {
		return "default_border_radius";
	}
	return StringName();
}


bool StyleBox::_mark_theme_override(int p_property) {
	++_theme_override_revision;
	const uint64_t bit = uint64_t(1) << p_property;
	const bool was_inherited = !(_theme_overrides & bit);
	_theme_overrides |= bit;
	if (_theme_expressions.erase(_get_theme_property_name(p_property))) {
		notify_property_list_changed();
		return true;
	}
	return was_inherited;
}

void StyleBox::_set_theme_value(int p_property, const Variant &p_value) {
	ERR_FAIL_INDEX(p_property, MAX_THEME_PROPERTIES);
	const StringName property = _get_theme_property_name(p_property);
	if (p_value.get_type() == Variant::STRING) {
		const String text = String(p_value).strip_edges();
		if (text.is_empty()) {
			clear_theme_override(property);
			return;
		}
		const Error error = set_theme_expression(property, text);
		ERR_FAIL_COND(error != OK);
		return;
	}
	Variant value = p_value;
	const Variant::Type type = _get_theme_property_type(p_property);
	if (type == Variant::INT || type == Variant::FLOAT) {
		ERR_FAIL_COND_MSG(!value.is_num(), vformat("Expected a number for '%s'.", property));
		const double number = value;
		ERR_FAIL_COND_MSG(!Math::is_finite(number) || number < INT32_MIN || number > INT32_MAX, vformat("Invalid numeric value for '%s'.", property));
		value = type == Variant::INT ? Variant(int(Math::round(number))) : Variant(number);
	} else if (type == Variant::OBJECT) {
		ERR_FAIL_COND_MSG(value.get_type() != Variant::NIL && value.get_type() != Variant::OBJECT, vformat("Expected a resource for '%s'.", property));
	} else {
		ERR_FAIL_COND_MSG(value.get_type() != type, vformat("Invalid value type for '%s'.", property));
	}
	if (type == Variant::COLOR) {
		const Color color = value;
		ERR_FAIL_COND_MSG(!Math::is_finite(color.r) || !Math::is_finite(color.g) || !Math::is_finite(color.b) || !Math::is_finite(color.a), vformat("Non-finite color for '%s'.", property));
	} else if (type == Variant::VECTOR2) {
		const Vector2 vector = value;
		ERR_FAIL_COND_MSG(!Math::is_finite(vector.x) || !Math::is_finite(vector.y), vformat("Non-finite vector for '%s'.", property));
	}
	if (p_property < BORDER_WIDTH_LEFT) {
		set_padding(static_cast<Side>(p_property), value);
	} else if (p_property < CORNER_RADIUS_TOP_LEFT) {
		set_border_width(static_cast<Side>(p_property - BORDER_WIDTH_LEFT), value);
	} else if (p_property < CORNER_DETAIL) {
		set_corner_radius(static_cast<Corner>(p_property - CORNER_RADIUS_TOP_LEFT), value);
	} else if (p_property >= EXPAND_MARGIN_LEFT && p_property <= EXPAND_MARGIN_BOTTOM) {
		set_expand_margin(static_cast<Side>(p_property - EXPAND_MARGIN_LEFT), value);
	} else if (p_property >= TEXTURE_SLICE_LEFT && p_property <= TEXTURE_SLICE_BOTTOM) {
		set_texture_slice(static_cast<Side>(p_property - TEXTURE_SLICE_LEFT), value);
	} else {
		switch (p_property) {
			case CORNER_DETAIL:
				set_corner_detail(value);
				break;
			case BACKGROUND_COLOR:
				set_background_color(value);
				break;
			case BORDER_COLOR:
				set_border_color(value);
				break;
			case SHADOW_COLOR:
				set_shadow_color(value);
				break;
			case SHADOW_SIZE:
				set_shadow_size(value);
				break;
			case ANTI_ALIASING_SIZE:
				set_aa_size(value);
				break;
			case MINIMUM_SIZE:
				set_minimum_size(value);
				break;
			case SHADOW_OFFSET:
				set_shadow_offset(value);
				break;
			case TEXTURE:
				set_texture(value);
				break;
			case TEXTURE_MODULATE:
				set_texture_modulate(value);
				break;
			case SKEW:
				set_skew(value);
				break;
			case ANTI_ALIASING:
				set_anti_aliased(value);
				break;
			case BORDER_BLEND:
				set_border_blend(value);
				break;
			case AXIS_STRETCH_HORIZONTAL:
				set_h_axis_stretch_mode(static_cast<AxisStretchMode>(int(value)));
				break;
			case AXIS_STRETCH_VERTICAL:
				set_v_axis_stretch_mode(static_cast<AxisStretchMode>(int(value)));
				break;
			case REGION_RECT:
				set_region_rect(value);
				break;
			default:
				ERR_FAIL();
		}
	}
}

Variant StyleBox::_get_theme_value(int p_property) const {
	ERR_FAIL_INDEX_V(p_property, MAX_THEME_PROPERTIES, Variant());
	const String *expression = _theme_expressions.getptr(_get_theme_property_name(p_property));
	if (expression) {
		return *expression;
	}
	if (p_property < BORDER_WIDTH_LEFT) {
		return get_padding(static_cast<Side>(p_property));
	}
	if (p_property < CORNER_RADIUS_TOP_LEFT) {
		return get_border_width(static_cast<Side>(p_property - BORDER_WIDTH_LEFT));
	}
	if (p_property < CORNER_DETAIL) {
		return get_corner_radius(static_cast<Corner>(p_property - CORNER_RADIUS_TOP_LEFT));
	}
	if (p_property >= EXPAND_MARGIN_LEFT && p_property <= EXPAND_MARGIN_BOTTOM) {
		return get_expand_margin(static_cast<Side>(p_property - EXPAND_MARGIN_LEFT));
	}
	if (p_property >= TEXTURE_SLICE_LEFT && p_property <= TEXTURE_SLICE_BOTTOM) {
		return get_texture_slice(static_cast<Side>(p_property - TEXTURE_SLICE_LEFT));
	}
	switch (p_property) {
		case CORNER_DETAIL:
			return get_corner_detail();
		case BACKGROUND_COLOR:
			return get_background_color();
		case BORDER_COLOR:
			return get_border_color();
		case SHADOW_COLOR:
			return get_shadow_color();
		case SHADOW_SIZE:
			return get_shadow_size();
		case ANTI_ALIASING_SIZE:
			return get_aa_size();
		case MINIMUM_SIZE:
			return get_intrinsic_minimum_size();
		case SHADOW_OFFSET:
			return get_shadow_offset();
		case TEXTURE:
			return get_texture();
		case TEXTURE_MODULATE:
			return get_texture_modulate();
		case SKEW:
			return get_skew();
		case ANTI_ALIASING:
			return is_anti_aliased();
		case BORDER_BLEND:
			return get_border_blend();
		case AXIS_STRETCH_HORIZONTAL:
			return get_h_axis_stretch_mode();
		case AXIS_STRETCH_VERTICAL:
			return get_v_axis_stretch_mode();
		case REGION_RECT:
			return get_region_rect();
		default:
			ERR_FAIL_V(Variant());
	}
}
