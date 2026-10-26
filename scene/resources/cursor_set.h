/**************************************************************************/
/*  cursor_set.h                                                          */
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

#include "core/input/input.h"
#include "core/io/resource.h"
#include "core/math/vector2.h"

class Texture2D;

class CursorSet : public Resource {
	GDCLASS(CursorSet, Resource);

public:
	CursorSet();
	~CursorSet() override;

	void set_texture(Input::CursorShape p_shape, const Ref<Texture2D> &p_texture);
	Ref<Texture2D> get_texture(Input::CursorShape p_shape) const;
	void set_hotspot(Input::CursorShape p_shape, const Vector2 &p_hotspot);
	Vector2 get_hotspot(Input::CursorShape p_shape) const;

	// Applies every shape; missing textures restore the corresponding system cursor.
	void apply() const;

protected:
	static void _bind_methods();

private:
	Ref<Texture2D> _textures[Input::CursorShape::CURSOR_MAX];
	Vector2 _hotspots[Input::CursorShape::CURSOR_MAX];
};
