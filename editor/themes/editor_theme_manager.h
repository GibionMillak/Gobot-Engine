/**************************************************************************/
/*  editor_theme_manager.h                                                */
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

#include "core/templates/vector.h"
#include "editor/themes/editor_theme.h"

#include <atomic>

class EditorThemeManager {
public:
	static Ref<EditorTheme> generate_theme(const Ref<EditorTheme> &p_old_theme = nullptr);
	static bool is_generated_theme_outdated();
	static void set_theme_outdated();
	static Ref<Theme> get_default_theme();
	static Vector<Ref<Resource>> get_builtin_themes();
	static float get_icon_saturation();
	static bool is_dark_theme();
	static bool is_dark_icon_and_font();
	static void initialize();
	static void finalize();

private:
	enum ColorMode {
		AUTO_COLOR,
		DARK_COLOR,
		LIGHT_COLOR,
	};

	static String _get_benchmark_key();
	static Ref<EditorTheme> _create_base_theme(const Ref<EditorTheme> &p_old_theme);
	static void _apply_editor_preferences(const Ref<Theme> &p_theme);
	static String _get_text_editor_color_setting(const StringName &p_name);
	static void _apply_preference_defaults(const Ref<Theme> &p_theme);
	static void _load_text_editor_theme();
	static bool _is_complete_theme_definition(const Ref<Theme> &p_theme);
	static Ref<Theme> _get_theme_definition();
	static void _theme_definition_changed();
	static void _refresh_theme_definition();
	static void _reset_dirty_flag();

	static int _benchmark_run;
	static inline bool _outdated_cache = false;
	static inline bool _outdated_cache_dirty = true;
	static bool _definition_refresh_pending;
	static Ref<Theme> _theme_definition;
	static std::atomic<bool> _active_dark_theme;
	static std::atomic<float> _active_icon_saturation;
};
