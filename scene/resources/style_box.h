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
#include "core/math/color.h"
#include "core/math/math_defs.h"
#include "core/math/rect2.h"
#include "core/math/vector2.h"
#include "core/object/gdvirtual.gen.h"
#include "core/templates/rid.h"
#include "core/templates/vector.h"
#include "scene/resources/texture.h"

class CanvasItem;

class StyleBox : public Resource {
	GDCLASS(StyleBox, Resource);
	RES_BASE_EXTENSION("stylebox");
	OBJ_SAVE_TYPE(StyleBox);

public:
	enum AxisStretchMode {
		STRETCH,
		TILE,
		TILE_FIT,
	};

	StyleBox() = default;

	void set_texture(Ref<Texture2D> p_texture);
	Ref<Texture2D> get_texture() const;

	void set_texture_modulate(const Color &p_modulate);
	Color get_texture_modulate() const;

	// Source-image slices affect nine-patch sampling, independently of layout padding.
	void set_texture_slice(Side p_side, float p_size);
	float get_texture_slice(Side p_side) const;
	void set_texture_slice_all(float p_size);
	void set_texture_slice_individual(float p_left, float p_top, float p_right, float p_bottom);

	void set_h_axis_stretch_mode(AxisStretchMode p_mode);
	AxisStretchMode get_h_axis_stretch_mode() const;
	void set_v_axis_stretch_mode(AxisStretchMode p_mode);
	AxisStretchMode get_v_axis_stretch_mode() const;

	void set_region_rect(const Rect2 &p_region_rect);
	Rect2 get_region_rect() const;

	void set_background_color(const Color &p_color);
	Color get_background_color() const;

	void set_skew(Vector2 p_skew);
	Vector2 get_skew() const;

	// Padding is nonnegative, measured in pixels, and added inside the border.
	void set_padding(Side p_side, float p_padding);
	float get_padding(Side p_side) const;
	void set_padding_all(float p_padding);
	void set_padding_individual(float p_left, float p_top, float p_right, float p_bottom);

	void set_border_color(const Color &p_color);
	Color get_border_color() const;
	void set_border_blend(bool p_blend);
	bool get_border_blend() const;
	void set_border_width(Side p_side, int p_width);
	int get_border_width(Side p_side) const;
	void set_border_width_all(int p_size);
	int get_border_width_min() const;

	void set_corner_radius(const Corner p_corner, int p_radius);
	int get_corner_radius(const Corner p_corner) const;
	void set_corner_radius_all(int p_radius);
	void set_corner_radius_individual(const int p_top_left, const int p_top_right, const int p_bottom_right, const int p_bottom_left);
	void set_corner_detail(int p_corner_detail);
	int get_corner_detail() const;

	void set_expand_margin(Side p_side, float p_size);
	float get_expand_margin(Side p_side) const;
	void set_expand_margin_all(float p_expand_margin_size);
	void set_expand_margin_individual(float p_left, float p_top, float p_right, float p_bottom);

	void set_shadow_color(const Color &p_color);
	Color get_shadow_color() const;
	void set_shadow_size(int p_size);
	int get_shadow_size() const;
	void set_shadow_offset(const Point2 &p_offset);
	Point2 get_shadow_offset() const;

	void set_anti_aliased(bool p_anti_aliased);
	bool is_anti_aliased() const;
	void set_aa_size(const real_t p_aa_size);
	real_t get_aa_size() const;

	void set_minimum_size(const Size2 &p_size);
	Size2 get_intrinsic_minimum_size() const;

	// Computed minimum: the maximum of the intrinsic size and border-plus-padding totals.
	virtual Size2 get_minimum_size() const;

	// Total layout inset: border width plus padding.
	float get_content_inset(Side p_side) const;
	Point2 get_content_offset() const;

	virtual void draw(RID p_canvas_item, const Rect2 &p_rect) const;
	virtual Rect2 get_draw_rect(const Rect2 &p_rect) const;
	CanvasItem *get_current_item_drawn() const;
	virtual bool test_mask(const Point2 &p_point, const Rect2 &p_rect) const;

protected:
	static void _bind_methods();
	void _validate_property(PropertyInfo &r_property) const;

	GDVIRTUAL0RC(Size2, _get_minimum_size)
	GDVIRTUAL2C(_draw, RID, Rect2)
	GDVIRTUAL1RC(Rect2, _get_draw_rect, Rect2)
	GDVIRTUAL2RC(bool, _test_mask, Point2, Rect2)

private:
	struct TextureAxisSegment {
		real_t begin = 0;
		real_t end = 0;
		real_t source_begin = 0;
		real_t source_end = 0;
	};

	struct TextureVertex {
		Vector2 position;
		Color color;
	};

	static void _set_inner_corner_radius(const Rect2 p_style_rect, const Rect2 p_inner_rect, const real_t p_corner_radius[4], real_t *r_inner_corner_radius);
	static void _set_corner_scale(const Rect2 &p_style_rect, const Rect2 &p_inner_rect, const real_t p_corner_radius[4], Point2 *r_inner_scale);
	static void _draw_rounded_rectangle(Vector<Vector2> &r_verts, Vector<int> &r_indices, Vector<Color> &r_colors, const Rect2 &p_style_rect, const real_t p_corner_radius[4],
			const Rect2 &p_ring_rect, const Rect2 &p_inner_rect, const Color &p_inner_color, const Color &p_outer_color, const int p_corner_detail, const Vector2 &p_skew, bool p_is_filled = false);
	static void _adapt_values(int p_index_a, int p_index_b, real_t *r_adapted_values, const real_t *p_values, const real_t p_width, const real_t p_max_a, const real_t p_max_b);

	static void _submit_geometry(RID p_canvas_item, const Rect2 &p_rect, real_t p_aa_size, const Vector<Vector2> &p_vertices, const Vector<int> &p_indices, const Vector<Color> &p_colors);
	static Vector<TextureAxisSegment> _make_texture_axis(real_t p_draw_size, real_t p_source_size, real_t p_begin, real_t p_end, AxisStretchMode p_mode);
	static Vector<TextureVertex> _clip_texture_polygon(const Vector<TextureVertex> &p_polygon, int p_axis, real_t p_edge, bool p_keep_greater);
	void _draw_texture(RID p_canvas_item, const Rect2 &p_rect, const real_t *p_corner_radius, real_t p_aa_size) const;
	void _texture_changed();

	Ref<Texture2D> _texture;
	Color _texture_modulate = Color(1, 1, 1, 1);
	float _texture_slice[4] = {};
	AxisStretchMode _axis_h = STRETCH;
	AxisStretchMode _axis_v = STRETCH;
	Rect2 _region_rect;
	Color _background_color = Color(0, 0, 0, 0);
	Vector2 _skew;
	float _padding[4] = {};
	Color _border_color = Color(0.8, 0.8, 0.8);
	bool _blend_border = false;
	real_t _border_width[4] = {};
	real_t _corner_radius[4] = {};
	int _corner_detail = 8;
	real_t _expand_margin[4] = {};
	Color _shadow_color = Color(0, 0, 0, 0.6);
	int _shadow_size = 0;
	Point2 _shadow_offset;
	bool _anti_aliased = true;
	real_t _aa_size = 1;
	Size2 _minimum_size;
};

VARIANT_ENUM_CAST(StyleBox::AxisStretchMode)
