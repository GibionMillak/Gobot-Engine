/**************************************************************************/
/*  editor_theme.cpp                                                      */
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

#include "editor_theme.h"

#include "editor/editor_string_names.h"
#include "scene/theme/theme_db.h"

Color EditorTheme::get_color(const StringName &p_name, const StringName &p_theme_type) const {
	if (!has_color(p_name, p_theme_type) && p_theme_type == EditorStringName(Editor)) {
		WARN_PRINT(vformat("Trying to access a non-existing editor theme color '%s' in '%s'.", p_name, p_theme_type));
	}
	return Theme::get_color(p_name, p_theme_type);
}

int EditorTheme::get_constant(const StringName &p_name, const StringName &p_theme_type) const {
	if (!has_constant(p_name, p_theme_type) && p_theme_type == EditorStringName(Editor)) {
		WARN_PRINT(vformat("Trying to access a non-existing editor theme constant '%s' in '%s'.", p_name, p_theme_type));
	}
	return Theme::get_constant(p_name, p_theme_type);
}

Ref<Font> EditorTheme::get_font(const StringName &p_name, const StringName &p_theme_type) const {
	if (!has_font_no_default(p_name, p_theme_type) && p_theme_type == EditorStringName(Editor)) {
		WARN_PRINT(vformat("Trying to access a non-existing editor theme font '%s' in '%s'.", p_name, p_theme_type));
	}
	return Theme::get_font(p_name, p_theme_type);
}

int EditorTheme::get_font_size(const StringName &p_name, const StringName &p_theme_type) const {
	if (!has_font_size_no_default(p_name, p_theme_type) && p_theme_type == EditorStringName(Editor)) {
		WARN_PRINT(vformat("Trying to access a non-existing editor theme font size '%s' in '%s'.", p_name, p_theme_type));
	}
	return Theme::get_font_size(p_name, p_theme_type);
}

Ref<Texture2D> EditorTheme::get_icon(const StringName &p_name, const StringName &p_theme_type) const {
	if (!has_icon(p_name, p_theme_type) && p_theme_type == EditorStringName(Editor)) {
		WARN_PRINT(vformat("Trying to access a non-existing editor theme icon '%s' in '%s'.", p_name, p_theme_type));
	}
	return Theme::get_icon(p_name, p_theme_type);
}

Ref<StyleBox> EditorTheme::get_stylebox(const StringName &p_name, const StringName &p_theme_type) const {
	if (!has_stylebox(p_name, p_theme_type) && p_theme_type == EditorStringName(Editor)) {
		WARN_PRINT(vformat("Trying to access a non-existing editor theme stylebox '%s' in '%s'.", p_name, p_theme_type));
	}
	return Theme::get_stylebox(p_name, p_theme_type);
}
