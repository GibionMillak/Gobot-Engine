/**************************************************************************/
/*  test_style_box.cpp                                                    */
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

#include "tests/test_macros.h"

TEST_FORCE_LINK(test_style_box)

#include "scene/resources/style_box.h"

namespace TestStyleBox {

TEST_CASE("[StyleBox] Constructor") {
	Ref<StyleBox> style_box = memnew(StyleBox);

	CHECK(style_box->get_content_offset() == Point2(0, 0));
	CHECK(style_box->get_current_item_drawn() == nullptr);
}

TEST_CASE("[StyleBox] set_padding, set_padding_all, set_padding_individual, get_padding, get_content_inset") {
	Ref<StyleBox> style_box = memnew(StyleBox);

	SUBCASE("set_padding, get_padding") {
		style_box->set_padding(SIDE_LEFT, 2);
		style_box->set_padding(SIDE_TOP, 3);
		style_box->set_padding(SIDE_RIGHT, 4);
		style_box->set_padding(SIDE_BOTTOM, 5);

		CHECK(style_box->get_padding(SIDE_LEFT) == 2);
		CHECK(style_box->get_padding(SIDE_TOP) == 3);
		CHECK(style_box->get_padding(SIDE_RIGHT) == 4);
		CHECK(style_box->get_padding(SIDE_BOTTOM) == 5);
	}

	SUBCASE("set_padding_all, get_padding") {
		style_box->set_padding_all(10);

		CHECK(style_box->get_padding(SIDE_LEFT) == 10);
		CHECK(style_box->get_padding(SIDE_TOP) == 10);
		CHECK(style_box->get_padding(SIDE_RIGHT) == 10);
		CHECK(style_box->get_padding(SIDE_BOTTOM) == 10);
	}

	SUBCASE("set_padding, get_content_inset") {
		style_box->set_padding(SIDE_LEFT, -1);
		style_box->set_padding(SIDE_RIGHT, -2);
		style_box->set_padding(SIDE_TOP, 3);
		style_box->set_padding(SIDE_BOTTOM, 4);

		CHECK_MESSAGE(style_box->get_content_inset(SIDE_LEFT) == 0,
				"Value is lesser than zero, so it returns 0");
		CHECK_MESSAGE(style_box->get_content_inset(SIDE_RIGHT) == 0,
				"Value is lesser than zero, so it returns 0");
		CHECK_MESSAGE(style_box->get_content_inset(SIDE_TOP) == 3,
				"Value is higher than zero, so it returns value");
		CHECK_MESSAGE(style_box->get_content_inset(SIDE_BOTTOM) == 4,
				"Value is higher than zero, so it returns value");
	}

	SUBCASE("set_padding_individual, get_minimum_size") {
		style_box->set_padding_individual(-1, 2, 5, 15);

		CHECK(style_box->get_minimum_size() == Size2(5, 17));
	}

	SUBCASE("set_padding_individual, get_content_offset") {
		style_box->set_padding_individual(-3, 5, 1, 2);

		CHECK_MESSAGE(style_box->get_content_offset() == Point2(0, 5),
				"Returns Point2 with get_content_inset of SIDE_LEFT and SIDE_TOP");
	}
}

TEST_CASE("[StyleBox] Texture defaults") {
	Ref<StyleBox> style_box = memnew(StyleBox);

	CHECK(style_box->get_h_axis_stretch_mode() == style_box->STRETCH);
	CHECK(style_box->get_v_axis_stretch_mode() == style_box->STRETCH);

	CHECK(style_box->get_expand_margin(SIDE_LEFT) == 0);
	CHECK(style_box->get_expand_margin(SIDE_TOP) == 0);
	CHECK(style_box->get_expand_margin(SIDE_RIGHT) == 0);
	CHECK(style_box->get_expand_margin(SIDE_BOTTOM) == 0);

	CHECK(style_box->get_texture_modulate() == Color(1, 1, 1, 1));
	CHECK(style_box->get_region_rect() == Rect2(0, 0, 0, 0));
	CHECK(style_box->get_texture() == Ref<Texture2D>());

	CHECK(style_box->get_texture_slice(SIDE_LEFT) == 0);
	CHECK(style_box->get_texture_slice(SIDE_TOP) == 0);
	CHECK(style_box->get_texture_slice(SIDE_RIGHT) == 0);
	CHECK(style_box->get_texture_slice(SIDE_BOTTOM) == 0);
}

TEST_CASE("[StyleBox] set_texture, get_texture") {
	Ref<StyleBox> style_box = memnew(StyleBox);
	Ref<Texture2D> texture = memnew(Texture2D);

	style_box->set_texture(texture);
	CHECK(style_box->get_texture() == texture);
}

TEST_CASE("[StyleBox] set_texture_slice, set_texture_slice_all, set_texture_slice_individual, get_texture_slice") {
	Ref<StyleBox> style_box = memnew(StyleBox);

	SUBCASE("set_texture_slice, get_texture_slice") {
		style_box->set_texture_slice(SIDE_LEFT, 1);
		style_box->set_texture_slice(SIDE_TOP, 1);
		style_box->set_texture_slice(SIDE_RIGHT, 1);
		style_box->set_texture_slice(SIDE_BOTTOM, 1);

		CHECK(style_box->get_texture_slice(SIDE_LEFT) == 1);
		CHECK(style_box->get_texture_slice(SIDE_TOP) == 1);
		CHECK(style_box->get_texture_slice(SIDE_RIGHT) == 1);
		CHECK(style_box->get_texture_slice(SIDE_BOTTOM) == 1);
	}

	SUBCASE("set_texture_slice_all") {
		style_box->set_texture_slice_all(2);

		CHECK(style_box->get_texture_slice(SIDE_LEFT) == 2);
		CHECK(style_box->get_texture_slice(SIDE_TOP) == 2);
		CHECK(style_box->get_texture_slice(SIDE_RIGHT) == 2);
		CHECK(style_box->get_texture_slice(SIDE_BOTTOM) == 2);
	}

	SUBCASE("set_texture_slice_individual") {
		style_box->set_texture_slice_individual(3, 4, 5, 6);

		CHECK(style_box->get_texture_slice(SIDE_LEFT) == 3);
		CHECK(style_box->get_texture_slice(SIDE_TOP) == 4);
		CHECK(style_box->get_texture_slice(SIDE_RIGHT) == 5);
		CHECK(style_box->get_texture_slice(SIDE_BOTTOM) == 6);
	}
}

TEST_CASE("[StyleBox] set_expand_margin, set_expand_margin_all, set_expand_margin_individual") {
	Ref<StyleBox> style_box = memnew(StyleBox);

	SUBCASE("set_expand_margin, get_expand_margin") {
		style_box->set_expand_margin(SIDE_LEFT, 1);
		style_box->set_expand_margin(SIDE_TOP, 1);
		style_box->set_expand_margin(SIDE_RIGHT, 1);
		style_box->set_expand_margin(SIDE_BOTTOM, 1);

		CHECK(style_box->get_expand_margin(SIDE_LEFT) == 1);
		CHECK(style_box->get_expand_margin(SIDE_TOP) == 1);
		CHECK(style_box->get_expand_margin(SIDE_RIGHT) == 1);
		CHECK(style_box->get_expand_margin(SIDE_BOTTOM) == 1);
	}

	SUBCASE("set_expand_margin_all") {
		style_box->set_expand_margin_all(2);

		CHECK(style_box->get_expand_margin(SIDE_LEFT) == 2);
		CHECK(style_box->get_expand_margin(SIDE_TOP) == 2);
		CHECK(style_box->get_expand_margin(SIDE_RIGHT) == 2);
		CHECK(style_box->get_expand_margin(SIDE_BOTTOM) == 2);
	}

	SUBCASE("set_expand_margin_individual") {
		style_box->set_expand_margin_individual(3, 4, 5, 6);

		CHECK(style_box->get_expand_margin(SIDE_LEFT) == 3);
		CHECK(style_box->get_expand_margin(SIDE_TOP) == 4);
		CHECK(style_box->get_expand_margin(SIDE_RIGHT) == 5);
		CHECK(style_box->get_expand_margin(SIDE_BOTTOM) == 6);
	}
}

TEST_CASE("[StyleBox] set_region_rect, get_region_rect") {
	Ref<StyleBox> style_box = memnew(StyleBox);

	style_box->set_region_rect(Rect2(1, 1, 1, 1));
	CHECK(style_box->get_region_rect() == Rect2(1, 1, 1, 1));
}

TEST_CASE("[StyleBox] set_h_axis_stretch_mode, set_v_axis_stretch_mode, get_h_axis_stretch_mode, get_v_axis_stretch_mode") {
	Ref<StyleBox> style_box = memnew(StyleBox);

	SUBCASE("set_h_axis_stretch_mode, get_h_axis_stretch_mode") {
		style_box->set_h_axis_stretch_mode(style_box->TILE);
		CHECK(style_box->get_h_axis_stretch_mode() == style_box->TILE);

		style_box->set_h_axis_stretch_mode(style_box->TILE_FIT);
		CHECK(style_box->get_h_axis_stretch_mode() == style_box->TILE_FIT);

		style_box->set_h_axis_stretch_mode(style_box->STRETCH);
		CHECK(style_box->get_h_axis_stretch_mode() == style_box->STRETCH);
	}

	SUBCASE("set_v_axis_stretch_mode, get_v_axis_stretch_mode") {
		style_box->set_v_axis_stretch_mode(style_box->TILE);
		CHECK(style_box->get_v_axis_stretch_mode() == style_box->TILE);

		style_box->set_v_axis_stretch_mode(style_box->TILE_FIT);
		CHECK(style_box->get_v_axis_stretch_mode() == style_box->TILE_FIT);

		style_box->set_v_axis_stretch_mode(style_box->STRETCH);
		CHECK(style_box->get_v_axis_stretch_mode() == style_box->STRETCH);
	}
}

TEST_CASE("[StyleBox] set_texture_modulate, get_texture_modulate") {
	Ref<StyleBox> style_box = memnew(StyleBox);

	style_box->set_texture_modulate(Color(0, 0, 0, 0));
	CHECK(style_box->get_texture_modulate() == Color(0, 0, 0, 0));
}

TEST_CASE("[StyleBox] get_draw_rect") {
	Ref<StyleBox> style_box = memnew(StyleBox);

	style_box->set_expand_margin_all(5);
	CHECK(style_box->get_draw_rect(Rect2(0, 0, 1, 1)) == Rect2(-5, -5, 11, 11));
}

TEST_CASE("[StyleBox] Drawing minimums and image slices are independent of content insets") {
	Ref<StyleBox> style = memnew(StyleBox);
	CHECK(style->get_background_color() == Color(0, 0, 0, 0));
	style->set_minimum_size(Size2(0, 2));
	CHECK(style->get_minimum_size() == Size2(0, 2));
	CHECK(style->get_content_offset() == Vector2());

	style->set_texture_slice_all(10);
	CHECK(style->get_content_inset(SIDE_LEFT) == 0);
	style->set_padding_all(4);
	style->set_border_width_all(1);
	CHECK(style->get_content_inset(SIDE_LEFT) == 5);
	CHECK(style->get_content_offset() == Vector2(5, 5));
	CHECK(style->get_minimum_size() == Size2(10, 10));
	style->set_minimum_size(Size2(24, 2));
	CHECK(style->get_minimum_size() == Size2(24, 10));
	CHECK(style->get_content_offset() == Vector2(5, 5));
}

} // namespace TestStyleBox
