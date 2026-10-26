/**************************************************************************/
/*  cursor_set.cpp                                                        */
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

#include "cursor_set.h"

#include "core/object/class_db.h"
#include "scene/resources/texture.h"

CursorSet::CursorSet() = default;

CursorSet::~CursorSet() = default;

void CursorSet::set_texture(Input::CursorShape p_shape, const Ref<Texture2D> &p_texture) {
	ERR_FAIL_INDEX(p_shape, Input::CursorShape::CURSOR_MAX);
	if (_textures[p_shape] == p_texture) {
		return;
	}
	_textures[p_shape] = p_texture;
	emit_changed();
}

Ref<Texture2D> CursorSet::get_texture(Input::CursorShape p_shape) const {
	ERR_FAIL_INDEX_V(p_shape, Input::CursorShape::CURSOR_MAX, Ref<Texture2D>());
	return _textures[p_shape];
}

void CursorSet::set_hotspot(Input::CursorShape p_shape, const Vector2 &p_hotspot) {
	ERR_FAIL_INDEX(p_shape, Input::CursorShape::CURSOR_MAX);
	ERR_FAIL_COND_MSG(!p_hotspot.is_finite() || p_hotspot.x < 0 || p_hotspot.y < 0, "Cursor hotspots must be finite and non-negative.");
	if (_hotspots[p_shape] == p_hotspot) {
		return;
	}
	_hotspots[p_shape] = p_hotspot;
	emit_changed();
}

Vector2 CursorSet::get_hotspot(Input::CursorShape p_shape) const {
	ERR_FAIL_INDEX_V(p_shape, Input::CursorShape::CURSOR_MAX, Vector2());
	return _hotspots[p_shape];
}

void CursorSet::apply() const {
	for (int i = 0; i < Input::CursorShape::CURSOR_MAX; i++) {
		const Ref<Texture2D> &texture = _textures[i];
		if (texture.is_valid()) {
			const Vector2 size = texture->get_size();
			ERR_CONTINUE_MSG(size.x <= 0 || size.y <= 0 || size.x > 256 || size.y > 256, "Cursor textures must have dimensions between 1 and 256 pixels.");
			ERR_CONTINUE_MSG(_hotspots[i].x >= size.x || _hotspots[i].y >= size.y, "Cursor hotspots must be within their texture's dimensions.");
		}
		Input::get_singleton()->set_custom_mouse_cursor(texture, static_cast<Input::CursorShape>(i), _hotspots[i]);
	}
}

void CursorSet::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_texture", "shape", "texture"), &CursorSet::set_texture);
	ClassDB::bind_method(D_METHOD("get_texture", "shape"), &CursorSet::get_texture);
	ClassDB::bind_method(D_METHOD("set_hotspot", "shape", "hotspot"), &CursorSet::set_hotspot);
	ClassDB::bind_method(D_METHOD("get_hotspot", "shape"), &CursorSet::get_hotspot);
	ClassDB::bind_method(D_METHOD("apply"), &CursorSet::apply);

	ADD_GROUP("Arrow", "arrow/");
	ADD_PROPERTYI(PropertyInfo(Variant::OBJECT, "arrow/texture", PROPERTY_HINT_RESOURCE_TYPE, "Texture2D"), "set_texture", "get_texture", Input::CursorShape::CURSOR_ARROW);
	ADD_PROPERTYI(PropertyInfo(Variant::VECTOR2, "arrow/hotspot"), "set_hotspot", "get_hotspot", Input::CursorShape::CURSOR_ARROW);
	ADD_GROUP("Ibeam", "ibeam/");
	ADD_PROPERTYI(PropertyInfo(Variant::OBJECT, "ibeam/texture", PROPERTY_HINT_RESOURCE_TYPE, "Texture2D"), "set_texture", "get_texture", Input::CursorShape::CURSOR_IBEAM);
	ADD_PROPERTYI(PropertyInfo(Variant::VECTOR2, "ibeam/hotspot"), "set_hotspot", "get_hotspot", Input::CursorShape::CURSOR_IBEAM);
	ADD_GROUP("Pointing Hand", "pointing_hand/");
	ADD_PROPERTYI(PropertyInfo(Variant::OBJECT, "pointing_hand/texture", PROPERTY_HINT_RESOURCE_TYPE, "Texture2D"), "set_texture", "get_texture", Input::CursorShape::CURSOR_POINTING_HAND);
	ADD_PROPERTYI(PropertyInfo(Variant::VECTOR2, "pointing_hand/hotspot"), "set_hotspot", "get_hotspot", Input::CursorShape::CURSOR_POINTING_HAND);
	ADD_GROUP("Cross", "cross/");
	ADD_PROPERTYI(PropertyInfo(Variant::OBJECT, "cross/texture", PROPERTY_HINT_RESOURCE_TYPE, "Texture2D"), "set_texture", "get_texture", Input::CursorShape::CURSOR_CROSS);
	ADD_PROPERTYI(PropertyInfo(Variant::VECTOR2, "cross/hotspot"), "set_hotspot", "get_hotspot", Input::CursorShape::CURSOR_CROSS);
	ADD_GROUP("Wait", "wait/");
	ADD_PROPERTYI(PropertyInfo(Variant::OBJECT, "wait/texture", PROPERTY_HINT_RESOURCE_TYPE, "Texture2D"), "set_texture", "get_texture", Input::CursorShape::CURSOR_WAIT);
	ADD_PROPERTYI(PropertyInfo(Variant::VECTOR2, "wait/hotspot"), "set_hotspot", "get_hotspot", Input::CursorShape::CURSOR_WAIT);
	ADD_GROUP("Busy", "busy/");
	ADD_PROPERTYI(PropertyInfo(Variant::OBJECT, "busy/texture", PROPERTY_HINT_RESOURCE_TYPE, "Texture2D"), "set_texture", "get_texture", Input::CursorShape::CURSOR_BUSY);
	ADD_PROPERTYI(PropertyInfo(Variant::VECTOR2, "busy/hotspot"), "set_hotspot", "get_hotspot", Input::CursorShape::CURSOR_BUSY);
	ADD_GROUP("Drag", "drag/");
	ADD_PROPERTYI(PropertyInfo(Variant::OBJECT, "drag/texture", PROPERTY_HINT_RESOURCE_TYPE, "Texture2D"), "set_texture", "get_texture", Input::CursorShape::CURSOR_DRAG);
	ADD_PROPERTYI(PropertyInfo(Variant::VECTOR2, "drag/hotspot"), "set_hotspot", "get_hotspot", Input::CursorShape::CURSOR_DRAG);
	ADD_GROUP("Can Drop", "can_drop/");
	ADD_PROPERTYI(PropertyInfo(Variant::OBJECT, "can_drop/texture", PROPERTY_HINT_RESOURCE_TYPE, "Texture2D"), "set_texture", "get_texture", Input::CursorShape::CURSOR_CAN_DROP);
	ADD_PROPERTYI(PropertyInfo(Variant::VECTOR2, "can_drop/hotspot"), "set_hotspot", "get_hotspot", Input::CursorShape::CURSOR_CAN_DROP);
	ADD_GROUP("Forbidden", "forbidden/");
	ADD_PROPERTYI(PropertyInfo(Variant::OBJECT, "forbidden/texture", PROPERTY_HINT_RESOURCE_TYPE, "Texture2D"), "set_texture", "get_texture", Input::CursorShape::CURSOR_FORBIDDEN);
	ADD_PROPERTYI(PropertyInfo(Variant::VECTOR2, "forbidden/hotspot"), "set_hotspot", "get_hotspot", Input::CursorShape::CURSOR_FORBIDDEN);
	ADD_GROUP("Vsize", "vsize/");
	ADD_PROPERTYI(PropertyInfo(Variant::OBJECT, "vsize/texture", PROPERTY_HINT_RESOURCE_TYPE, "Texture2D"), "set_texture", "get_texture", Input::CursorShape::CURSOR_VSIZE);
	ADD_PROPERTYI(PropertyInfo(Variant::VECTOR2, "vsize/hotspot"), "set_hotspot", "get_hotspot", Input::CursorShape::CURSOR_VSIZE);
	ADD_GROUP("Hsize", "hsize/");
	ADD_PROPERTYI(PropertyInfo(Variant::OBJECT, "hsize/texture", PROPERTY_HINT_RESOURCE_TYPE, "Texture2D"), "set_texture", "get_texture", Input::CursorShape::CURSOR_HSIZE);
	ADD_PROPERTYI(PropertyInfo(Variant::VECTOR2, "hsize/hotspot"), "set_hotspot", "get_hotspot", Input::CursorShape::CURSOR_HSIZE);
	ADD_GROUP("Bdiagsize", "bdiagsize/");
	ADD_PROPERTYI(PropertyInfo(Variant::OBJECT, "bdiagsize/texture", PROPERTY_HINT_RESOURCE_TYPE, "Texture2D"), "set_texture", "get_texture", Input::CursorShape::CURSOR_BDIAGSIZE);
	ADD_PROPERTYI(PropertyInfo(Variant::VECTOR2, "bdiagsize/hotspot"), "set_hotspot", "get_hotspot", Input::CursorShape::CURSOR_BDIAGSIZE);
	ADD_GROUP("Fdiagsize", "fdiagsize/");
	ADD_PROPERTYI(PropertyInfo(Variant::OBJECT, "fdiagsize/texture", PROPERTY_HINT_RESOURCE_TYPE, "Texture2D"), "set_texture", "get_texture", Input::CursorShape::CURSOR_FDIAGSIZE);
	ADD_PROPERTYI(PropertyInfo(Variant::VECTOR2, "fdiagsize/hotspot"), "set_hotspot", "get_hotspot", Input::CursorShape::CURSOR_FDIAGSIZE);
	ADD_GROUP("Move", "move/");
	ADD_PROPERTYI(PropertyInfo(Variant::OBJECT, "move/texture", PROPERTY_HINT_RESOURCE_TYPE, "Texture2D"), "set_texture", "get_texture", Input::CursorShape::CURSOR_MOVE);
	ADD_PROPERTYI(PropertyInfo(Variant::VECTOR2, "move/hotspot"), "set_hotspot", "get_hotspot", Input::CursorShape::CURSOR_MOVE);
	ADD_GROUP("Vsplit", "vsplit/");
	ADD_PROPERTYI(PropertyInfo(Variant::OBJECT, "vsplit/texture", PROPERTY_HINT_RESOURCE_TYPE, "Texture2D"), "set_texture", "get_texture", Input::CursorShape::CURSOR_VSPLIT);
	ADD_PROPERTYI(PropertyInfo(Variant::VECTOR2, "vsplit/hotspot"), "set_hotspot", "get_hotspot", Input::CursorShape::CURSOR_VSPLIT);
	ADD_GROUP("Hsplit", "hsplit/");
	ADD_PROPERTYI(PropertyInfo(Variant::OBJECT, "hsplit/texture", PROPERTY_HINT_RESOURCE_TYPE, "Texture2D"), "set_texture", "get_texture", Input::CursorShape::CURSOR_HSPLIT);
	ADD_PROPERTYI(PropertyInfo(Variant::VECTOR2, "hsplit/hotspot"), "set_hotspot", "get_hotspot", Input::CursorShape::CURSOR_HSPLIT);
	ADD_GROUP("Help", "help/");
	ADD_PROPERTYI(PropertyInfo(Variant::OBJECT, "help/texture", PROPERTY_HINT_RESOURCE_TYPE, "Texture2D"), "set_texture", "get_texture", Input::CursorShape::CURSOR_HELP);
	ADD_PROPERTYI(PropertyInfo(Variant::VECTOR2, "help/hotspot"), "set_hotspot", "get_hotspot", Input::CursorShape::CURSOR_HELP);
}
