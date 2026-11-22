/**************************************************************************/
/*  theme.cpp                                                             */
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

#include "theme.h"

#include "core/math/expression.h"
#include "core/math/math_funcs.h"
#include "core/object/callable_mp.h"
#include "core/object/class_db.h"
#include "core/string/char_utils.h"
#include "core/variant/array.h"
#include "scene/theme/theme_db.h"

Theme::Theme() = default;

Theme::~Theme() = default;

bool Theme::is_valid_type_name(const String &p_name) {
	int len = p_name.length();
	const char32_t *str = p_name.ptr();
	for (int i = 0; i < len; i++) {
		if (!is_ascii_identifier_char(str[i])) {
			return false;
		}
	}
	return true;
}

bool Theme::is_valid_item_name(const String &p_name) {
	if (p_name.is_empty()) {
		return false;
	}
	int len = p_name.length();
	const char32_t *str = p_name.ptr();
	for (int i = 0; i < len; i++) {
		if (!is_ascii_identifier_char(str[i])) {
			return false;
		}
	}
	return true;
}

String Theme::validate_type_name(const String &p_name) {
	String type_name = p_name.strip_edges();
	int len = type_name.length();
	char32_t *buffer = type_name.ptrw();
	for (int i = 0; i < len; i++) {
		if (!is_ascii_identifier_char(buffer[i])) {
			buffer[i] = '_';
		}
	}
	return type_name;
}

void Theme::set_style_parameter(const StringName &p_name, const Variant &p_value) {
	const Variant::Type type = _get_style_parameter_type(p_name);
	ERR_FAIL_COND_MSG(type == Variant::NIL, vformat("Unknown theme style parameter '%s'.", p_name));
	if (p_value.get_type() == Variant::NIL) {
		clear_style_parameter(p_name);
		return;
	}
	if (p_value.get_type() == Variant::STRING) {
		ERR_FAIL_COND_MSG(validate_theme_expression(p_value) != OK, vformat("Invalid expression for theme parameter '%s'.", p_name));
		_style_parameters[p_name] = p_value;
		_emit_theme_changed(true);
		return;
	}
	Variant value = p_value;
	if (type == Variant::FLOAT) {
		ERR_FAIL_COND_MSG(p_value.get_type() != Variant::FLOAT && p_value.get_type() != Variant::INT, vformat("Theme parameter '%s' requires a number.", p_name));
		const double number = p_value;
		ERR_FAIL_COND_MSG(!Math::is_finite(number) || (p_name != "default_contrast" && number < 0), vformat("Invalid numeric value for theme parameter '%s'.", p_name));
		ERR_FAIL_COND_MSG(p_name == "default_contrast" && (number < -1 || number > 1), "Theme contrast must be between -1 and 1.");
		ERR_FAIL_COND_MSG(p_name == "default_icon_saturation" && number > 2, "Theme icon saturation must be between 0 and 2.");
		value = number;
	} else {
		ERR_FAIL_COND_MSG(p_value.get_type() != type, vformat("Theme parameter '%s' requires a color.", p_name));
		const Color color = p_value;
		ERR_FAIL_COND_MSG(!Math::is_finite(color.r) || !Math::is_finite(color.g) || !Math::is_finite(color.b) || !Math::is_finite(color.a), vformat("Theme parameter '%s' requires finite color components.", p_name));
	}
	_style_parameters[p_name] = value;
	_emit_theme_changed(true);
}

Variant Theme::get_style_parameter(const StringName &p_name) const {
	const Variant *value = _style_parameters.getptr(p_name);
	if (value && value->get_type() == Variant::STRING) {
		Error error = OK;
		return _resolve_expression_value(p_name, error);
	}
	return value ? *value : Variant();
}

bool Theme::has_style_parameter(const StringName &p_name) const {
	return _style_parameters.has(p_name);
}

void Theme::clear_style_parameter(const StringName &p_name) {
	if (_style_parameters.erase(p_name)) {
		_emit_theme_changed(true);
	}
}

void Theme::set_theme_expression(const StringName &p_property, const String &p_expression) {
	ERR_FAIL_COND_MSG(validate_theme_expression(p_expression) != OK, vformat("Invalid theme expression for '%s'.", p_property));
	if (_get_style_parameter_type(p_property) != Variant::NIL) {
		set_style_parameter(p_property, p_expression);
		return;
	}
	const String property = p_property;
	ERR_FAIL_COND_MSG(property.get_slice_count("/") != 3, vformat("Invalid theme item expression target '%s'.", property));
	const StringName type = property.get_slicec('/', 0);
	const StringName item_name = property.get_slicec('/', 2);
	const String category = property.get_slicec('/', 1);
	if (category == "colors") {
		set_color(item_name, type, Color());
	} else if (category == "constants") {
		set_constant(item_name, type, 0);
	} else if (category == "font_sizes") {
		set_font_size(item_name, type, 0);
	} else if (category == "icons") {
		set_icon(item_name, type, Ref<Texture2D>());
	} else if (category == "fonts") {
		set_font(item_name, type, Ref<Font>());
	} else if (category == "styles") {
		set_stylebox(item_name, type, Ref<StyleBox>());
	} else if (category == "sounds") {
		set_sound(item_name, type, Ref<AudioStream>());
	} else {
		ERR_FAIL_MSG(vformat("Unsupported theme expression target '%s'.", property));
	}
	_item_expressions[p_property] = p_expression.strip_edges();
	_emit_theme_changed(true);
}

String Theme::get_theme_expression(const StringName &p_property) const {
	const Variant *parameter = _style_parameters.getptr(p_property);
	if (parameter && parameter->get_type() == Variant::STRING) {
		return *parameter;
	}
	const String *expression = _item_expressions.getptr(p_property);
	return expression ? *expression : String();
}

bool Theme::is_theme_expression_property(const StringName &p_property) {
	const String property = p_property;
	if (_get_style_parameter_type(p_property) != Variant::NIL) {
		return true;
	}
	if (property.get_slice_count("/") != 3) {
		return false;
	}
	const String category = property.get_slicec('/', 1);
	return category == "colors" || category == "constants" || category == "fonts" || category == "font_sizes" || category == "icons" || category == "styles" || category == "sounds";
}

Error Theme::validate_theme_expression(const String &p_expression) {
	ParsedExpression parsed;
	return _get_parsed_theme_expression(p_expression, parsed);
}

Variant Theme::evaluate_theme_expression(const String &p_expression, bool *r_pixels, Error *r_error) const {
	return _evaluate_theme_expression(p_expression, r_pixels, r_error, nullptr);
}

void Theme::set_default_base_scale(float p_base_scale) {
	if (_default_base_scale == p_base_scale) {
		return;
	}

	_default_base_scale = p_base_scale;

	_emit_theme_changed();
}

float Theme::get_default_base_scale() const {
	return _default_base_scale;
}

bool Theme::has_default_base_scale() const {
	return _default_base_scale > 0.0;
}

void Theme::set_default_font_family(const Ref<Font> &p_default_font_family) {
	if (_default_font_family == p_default_font_family) {
		return;
	}

	if (_default_font_family.is_valid()) {
		_default_font_family->disconnect_changed(callable_mp(this, &Theme::_emit_theme_changed));
	}

	_default_font_family = p_default_font_family;

	if (_default_font_family.is_valid()) {
		_default_font_family->connect_changed(callable_mp(this, &Theme::_emit_theme_changed).bind(false), CONNECT_REFERENCE_COUNTED);
	}

	_emit_theme_changed();
}

Ref<Font> Theme::get_default_font_family() const {
	return _default_font_family;
}

bool Theme::has_default_font_family() const {
	return _default_font_family.is_valid();
}

void Theme::set_default_font_size(int p_font_size) {
	if (_default_font_size == p_font_size) {
		return;
	}

	_default_font_size = p_font_size;

	_emit_theme_changed();
}

int Theme::get_default_font_size() const {
	return _default_font_size;
}

bool Theme::has_default_font_size() const {
	return _default_font_size > 0;
}

void Theme::set_icon(const StringName &p_name, const StringName &p_theme_type, const Ref<Texture2D> &p_icon) {
	_item_expressions.erase(String(p_theme_type) + "/icons/" + p_name);
	ERR_FAIL_COND_MSG(!is_valid_item_name(p_name), vformat("Invalid item name: '%s'", p_name));
	ERR_FAIL_COND_MSG(!is_valid_type_name(p_theme_type), vformat("Invalid type name: '%s'", p_theme_type));

	bool existing = false;
	if (_icon_map[p_theme_type].has(p_name) && _icon_map[p_theme_type][p_name].is_valid()) {
		existing = true;
		_icon_map[p_theme_type][p_name]->disconnect_changed(callable_mp(this, &Theme::_emit_theme_changed));
	}

	_icon_map[p_theme_type][p_name] = p_icon;

	if (p_icon.is_valid()) {
		_icon_map[p_theme_type][p_name]->connect_changed(callable_mp(this, &Theme::_emit_theme_changed).bind(false), CONNECT_REFERENCE_COUNTED);
	}

	_emit_theme_changed(!existing);
}

Ref<Texture2D> Theme::get_icon(const StringName &p_name, const StringName &p_theme_type) const {
	const StringName expression_key = String(p_theme_type) + "/icons/" + p_name;
	if (_item_expressions.has(expression_key)) {
		Error error = OK;
		const Variant value = _resolve_expression_value(expression_key, error);
		ERR_FAIL_COND_V_MSG(error != OK || (value.get_type() != Variant::NIL && value.get_type() != Variant::OBJECT), ThemeDB::get_singleton()->get_fallback_icon(), vformat("Invalid theme expression result for '%s'.", expression_key));
		const Ref<Texture2D> icon = value;
		return icon.is_valid() ? icon : ThemeDB::get_singleton()->get_fallback_icon();
	}
	if (_icon_map.has(p_theme_type) && _icon_map[p_theme_type].has(p_name) && _icon_map[p_theme_type][p_name].is_valid()) {
		return _icon_map[p_theme_type][p_name];
	} else {
		return ThemeDB::get_singleton()->get_fallback_icon();
	}
}

bool Theme::has_icon(const StringName &p_name, const StringName &p_theme_type) const {
	if (_item_expressions.has(String(p_theme_type) + "/icons/" + p_name)) {
		return true;
	}
	return (_icon_map.has(p_theme_type) && _icon_map[p_theme_type].has(p_name) && _icon_map[p_theme_type][p_name].is_valid());
}

bool Theme::has_icon_nocheck(const StringName &p_name, const StringName &p_theme_type) const {
	return (_icon_map.has(p_theme_type) && _icon_map[p_theme_type].has(p_name));
}

void Theme::rename_icon(const StringName &p_old_name, const StringName &p_name, const StringName &p_theme_type) {
	ERR_FAIL_COND_MSG(!is_valid_item_name(p_name), vformat("Invalid item name: '%s'", p_name));
	ERR_FAIL_COND_MSG(!is_valid_type_name(p_theme_type), vformat("Invalid type name: '%s'", p_theme_type));
	ERR_FAIL_COND_MSG(!_icon_map.has(p_theme_type), "Cannot rename the icon '" + String(p_old_name) + "' because the node type '" + String(p_theme_type) + "' does not exist.");
	ERR_FAIL_COND_MSG(_icon_map[p_theme_type].has(p_name), "Cannot rename the icon '" + String(p_old_name) + "' because the new name '" + String(p_name) + "' already exists.");
	ERR_FAIL_COND_MSG(!_icon_map[p_theme_type].has(p_old_name), "Cannot rename the icon '" + String(p_old_name) + "' because it does not exist.");

	_icon_map[p_theme_type][p_name] = _icon_map[p_theme_type][p_old_name];
	_icon_map[p_theme_type].erase(p_old_name);

	const StringName old_key = String(p_theme_type) + "/icons/" + p_old_name;
	if (_item_expressions.has(old_key)) {
		const String expression = _item_expressions[old_key];
		_item_expressions[String(p_theme_type) + "/icons/" + p_name] = expression;
		_item_expressions.erase(old_key);
	}

	_emit_theme_changed(true);
}

void Theme::clear_icon(const StringName &p_name, const StringName &p_theme_type) {
	_item_expressions.erase(String(p_theme_type) + "/icons/" + p_name);
	ERR_FAIL_COND_MSG(!_icon_map.has(p_theme_type), "Cannot clear the icon '" + String(p_name) + "' because the node type '" + String(p_theme_type) + "' does not exist.");
	ERR_FAIL_COND_MSG(!_icon_map[p_theme_type].has(p_name), "Cannot clear the icon '" + String(p_name) + "' because it does not exist.");

	if (_icon_map[p_theme_type][p_name].is_valid()) {
		_icon_map[p_theme_type][p_name]->disconnect_changed(callable_mp(this, &Theme::_emit_theme_changed));
	}

	_icon_map[p_theme_type].erase(p_name);

	_emit_theme_changed(true);
}

void Theme::get_icon_list(const StringName &p_theme_type, List<StringName> *p_list) const {
	ERR_FAIL_NULL(p_list);

	if (!_icon_map.has(p_theme_type)) {
		return;
	}

	for (const KeyValue<StringName, Ref<Texture2D>> &E : _icon_map[p_theme_type]) {
		p_list->push_back(E.key);
	}
}

void Theme::add_icon_type(const StringName &p_theme_type) {
	ERR_FAIL_COND_MSG(!is_valid_type_name(p_theme_type), vformat("Invalid type name: '%s'", p_theme_type));

	if (_icon_map.has(p_theme_type)) {
		return;
	}
	_icon_map[p_theme_type] = ThemeIconMap();
}

void Theme::remove_icon_type(const StringName &p_theme_type) {
	if (!_icon_map.has(p_theme_type)) {
		return;
	}

	_freeze_change_propagation();

	for (const KeyValue<StringName, Ref<Texture2D>> &E : _icon_map[p_theme_type]) {
		Ref<Texture2D> icon = E.value;
		if (icon.is_valid()) {
			icon->disconnect_changed(callable_mp(this, &Theme::_emit_theme_changed));
		}
	}

	_icon_map.erase(p_theme_type);

	_move_expression_type(p_theme_type, StringName(), "icons");

	_unfreeze_and_propagate_changes();
}

void Theme::rename_icon_type(const StringName &p_old_theme_type, const StringName &p_theme_type) {
	ERR_FAIL_COND_MSG(!is_valid_type_name(p_theme_type), vformat("Invalid type name: '%s'", p_theme_type));

	if (!_icon_map.has(p_old_theme_type) || _icon_map.has(p_theme_type)) {
		return;
	}

	_icon_map[p_theme_type] = _icon_map[p_old_theme_type];
	_icon_map.erase(p_old_theme_type);

	_move_expression_type(p_old_theme_type, p_theme_type, "icons");
}

void Theme::get_icon_type_list(List<StringName> *p_list) const {
	ERR_FAIL_NULL(p_list);

	for (const KeyValue<StringName, ThemeIconMap> &E : _icon_map) {
		p_list->push_back(E.key);
	}
}

void Theme::set_stylebox(const StringName &p_name, const StringName &p_theme_type, const Ref<StyleBox> &p_style) {
	_item_expressions.erase(String(p_theme_type) + "/styles/" + p_name);
	ERR_FAIL_COND_MSG(!is_valid_item_name(p_name), vformat("Invalid item name: '%s'", p_name));
	ERR_FAIL_COND_MSG(!is_valid_type_name(p_theme_type), vformat("Invalid type name: '%s'", p_theme_type));

	bool existing = false;
	if (_style_map[p_theme_type].has(p_name) && _style_map[p_theme_type][p_name].is_valid()) {
		existing = true;
		_style_map[p_theme_type][p_name]->disconnect_changed(callable_mp(this, &Theme::_emit_theme_changed));
	}

	_style_map[p_theme_type][p_name] = p_style;

	if (p_style.is_valid()) {
		_style_map[p_theme_type][p_name]->connect_changed(callable_mp(this, &Theme::_emit_theme_changed).bind(false), CONNECT_REFERENCE_COUNTED);
	}

	_emit_theme_changed(!existing);
}

Ref<StyleBox> Theme::get_stylebox(const StringName &p_name, const StringName &p_theme_type) const {
	const StringName expression_key = String(p_theme_type) + "/styles/" + p_name;
	if (_item_expressions.has(expression_key)) {
		Error error = OK;
		const Variant value = _resolve_expression_value(expression_key, error);
		ERR_FAIL_COND_V_MSG(error != OK || (value.get_type() != Variant::NIL && value.get_type() != Variant::OBJECT), ThemeDB::get_singleton()->get_fallback_stylebox(), vformat("Invalid theme expression result for '%s'.", expression_key));
		const Ref<StyleBox> style = value;
		return style.is_valid() ? _resolve_stylebox(style) : ThemeDB::get_singleton()->get_fallback_stylebox();
	}
	if (_style_map.has(p_theme_type) && _style_map[p_theme_type].has(p_name) && _style_map[p_theme_type][p_name].is_valid()) {
		return _resolve_stylebox(_style_map[p_theme_type][p_name]);
	} else {
		return ThemeDB::get_singleton()->get_fallback_stylebox();
	}
}

Ref<StyleBox> Theme::get_stylebox_definition(const StringName &p_name, const StringName &p_theme_type) const {
	if (_style_map.has(p_theme_type) && _style_map[p_theme_type].has(p_name)) {
		return _style_map[p_theme_type][p_name];
	}
	return Ref<StyleBox>();
}

bool Theme::has_stylebox(const StringName &p_name, const StringName &p_theme_type) const {
	if (_item_expressions.has(String(p_theme_type) + "/styles/" + p_name)) {
		return true;
	}
	return (_style_map.has(p_theme_type) && _style_map[p_theme_type].has(p_name) && _style_map[p_theme_type][p_name].is_valid());
}

bool Theme::has_stylebox_nocheck(const StringName &p_name, const StringName &p_theme_type) const {
	return (_style_map.has(p_theme_type) && _style_map[p_theme_type].has(p_name));
}

void Theme::rename_stylebox(const StringName &p_old_name, const StringName &p_name, const StringName &p_theme_type) {
	ERR_FAIL_COND_MSG(!is_valid_item_name(p_name), vformat("Invalid item name: '%s'", p_name));
	ERR_FAIL_COND_MSG(!is_valid_type_name(p_theme_type), vformat("Invalid type name: '%s'", p_theme_type));
	ERR_FAIL_COND_MSG(!_style_map.has(p_theme_type), "Cannot rename the stylebox '" + String(p_old_name) + "' because the node type '" + String(p_theme_type) + "' does not exist.");
	ERR_FAIL_COND_MSG(_style_map[p_theme_type].has(p_name), "Cannot rename the stylebox '" + String(p_old_name) + "' because the new name '" + String(p_name) + "' already exists.");
	ERR_FAIL_COND_MSG(!_style_map[p_theme_type].has(p_old_name), "Cannot rename the stylebox '" + String(p_old_name) + "' because it does not exist.");

	_style_map[p_theme_type][p_name] = _style_map[p_theme_type][p_old_name];
	_style_map[p_theme_type].erase(p_old_name);

	const StringName old_key = String(p_theme_type) + "/styles/" + p_old_name;
	if (_item_expressions.has(old_key)) {
		const String expression = _item_expressions[old_key];
		_item_expressions[String(p_theme_type) + "/styles/" + p_name] = expression;
		_item_expressions.erase(old_key);
	}

	_emit_theme_changed(true);
}

void Theme::clear_stylebox(const StringName &p_name, const StringName &p_theme_type) {
	_item_expressions.erase(String(p_theme_type) + "/styles/" + p_name);
	ERR_FAIL_COND_MSG(!_style_map.has(p_theme_type), "Cannot clear the stylebox '" + String(p_name) + "' because the node type '" + String(p_theme_type) + "' does not exist.");
	ERR_FAIL_COND_MSG(!_style_map[p_theme_type].has(p_name), "Cannot clear the stylebox '" + String(p_name) + "' because it does not exist.");

	if (_style_map[p_theme_type][p_name].is_valid()) {
		_style_map[p_theme_type][p_name]->disconnect_changed(callable_mp(this, &Theme::_emit_theme_changed));
	}

	_style_map[p_theme_type].erase(p_name);

	_emit_theme_changed(true);
}

void Theme::get_stylebox_list(const StringName &p_theme_type, List<StringName> *p_list) const {
	ERR_FAIL_NULL(p_list);

	if (!_style_map.has(p_theme_type)) {
		return;
	}

	for (const KeyValue<StringName, Ref<StyleBox>> &E : _style_map[p_theme_type]) {
		p_list->push_back(E.key);
	}
}

void Theme::add_stylebox_type(const StringName &p_theme_type) {
	ERR_FAIL_COND_MSG(!is_valid_type_name(p_theme_type), vformat("Invalid type name: '%s'", p_theme_type));

	if (_style_map.has(p_theme_type)) {
		return;
	}
	_style_map[p_theme_type] = ThemeStyleMap();
}

void Theme::remove_stylebox_type(const StringName &p_theme_type) {
	if (!_style_map.has(p_theme_type)) {
		return;
	}

	_freeze_change_propagation();

	for (const KeyValue<StringName, Ref<StyleBox>> &E : _style_map[p_theme_type]) {
		Ref<StyleBox> style = E.value;
		if (style.is_valid()) {
			style->disconnect_changed(callable_mp(this, &Theme::_emit_theme_changed));
		}
	}

	_style_map.erase(p_theme_type);

	_move_expression_type(p_theme_type, StringName(), "styles");

	_unfreeze_and_propagate_changes();
}

void Theme::rename_stylebox_type(const StringName &p_old_theme_type, const StringName &p_theme_type) {
	ERR_FAIL_COND_MSG(!is_valid_type_name(p_theme_type), vformat("Invalid type name: '%s'", p_theme_type));

	if (!_style_map.has(p_old_theme_type) || _style_map.has(p_theme_type)) {
		return;
	}

	_style_map[p_theme_type] = _style_map[p_old_theme_type];
	_style_map.erase(p_old_theme_type);

	_move_expression_type(p_old_theme_type, p_theme_type, "styles");
}

void Theme::get_stylebox_type_list(List<StringName> *p_list) const {
	ERR_FAIL_NULL(p_list);

	for (const KeyValue<StringName, ThemeStyleMap> &E : _style_map) {
		p_list->push_back(E.key);
	}
}

void Theme::set_font(const StringName &p_name, const StringName &p_theme_type, const Ref<Font> &p_font) {
	_item_expressions.erase(String(p_theme_type) + "/fonts/" + p_name);
	ERR_FAIL_COND_MSG(!is_valid_item_name(p_name), vformat("Invalid item name: '%s'", p_name));
	ERR_FAIL_COND_MSG(!is_valid_type_name(p_theme_type), vformat("Invalid type name: '%s'", p_theme_type));

	bool existing = false;
	if (_font_map[p_theme_type][p_name].is_valid()) {
		existing = true;
		_font_map[p_theme_type][p_name]->disconnect_changed(callable_mp(this, &Theme::_emit_theme_changed));
	}

	_font_map[p_theme_type][p_name] = p_font;

	if (p_font.is_valid()) {
		_font_map[p_theme_type][p_name]->connect_changed(callable_mp(this, &Theme::_emit_theme_changed).bind(false), CONNECT_REFERENCE_COUNTED);
	}

	_emit_theme_changed(!existing);
}

Ref<Font> Theme::get_font(const StringName &p_name, const StringName &p_theme_type) const {
	const StringName expression_key = String(p_theme_type) + "/fonts/" + p_name;
	if (_item_expressions.has(expression_key)) {
		Error error = OK;
		const Variant value = _resolve_expression_value(expression_key, error);
		ERR_FAIL_COND_V_MSG(error != OK || (value.get_type() != Variant::NIL && value.get_type() != Variant::OBJECT), (has_default_font_family() ? _default_font_family : ThemeDB::get_singleton()->get_fallback_font()), vformat("Invalid theme expression result for '%s'.", expression_key));
		const Ref<Font> font = value;
		return font.is_valid() ? font : has_default_font_family() ? _default_font_family : ThemeDB::get_singleton()->get_fallback_font();
	}
	if (_font_map.has(p_theme_type) && _font_map[p_theme_type].has(p_name) && _font_map[p_theme_type][p_name].is_valid()) {
		return _font_map[p_theme_type][p_name];
	} else if (has_default_font_family()) {
		return _default_font_family;
	} else {
		return ThemeDB::get_singleton()->get_fallback_font();
	}
}

bool Theme::has_font(const StringName &p_name, const StringName &p_theme_type) const {
	if (_item_expressions.has(String(p_theme_type) + "/fonts/" + p_name)) {
		return true;
	}
	return ((_font_map.has(p_theme_type) && _font_map[p_theme_type].has(p_name) && _font_map[p_theme_type][p_name].is_valid()) || has_default_font_family());
}

bool Theme::has_font_no_default(const StringName &p_name, const StringName &p_theme_type) const {
	if (_item_expressions.has(String(p_theme_type) + "/fonts/" + p_name)) {
		return true;
	}
	return (_font_map.has(p_theme_type) && _font_map[p_theme_type].has(p_name) && _font_map[p_theme_type][p_name].is_valid());
}

bool Theme::has_font_nocheck(const StringName &p_name, const StringName &p_theme_type) const {
	return (_font_map.has(p_theme_type) && _font_map[p_theme_type].has(p_name));
}

void Theme::rename_font(const StringName &p_old_name, const StringName &p_name, const StringName &p_theme_type) {
	ERR_FAIL_COND_MSG(!is_valid_item_name(p_name), vformat("Invalid item name: '%s'", p_name));
	ERR_FAIL_COND_MSG(!is_valid_type_name(p_theme_type), vformat("Invalid type name: '%s'", p_theme_type));
	ERR_FAIL_COND_MSG(!_font_map.has(p_theme_type), "Cannot rename the font '" + String(p_old_name) + "' because the node type '" + String(p_theme_type) + "' does not exist.");
	ERR_FAIL_COND_MSG(_font_map[p_theme_type].has(p_name), "Cannot rename the font '" + String(p_old_name) + "' because the new name '" + String(p_name) + "' already exists.");
	ERR_FAIL_COND_MSG(!_font_map[p_theme_type].has(p_old_name), "Cannot rename the font '" + String(p_old_name) + "' because it does not exist.");

	_font_map[p_theme_type][p_name] = _font_map[p_theme_type][p_old_name];
	_font_map[p_theme_type].erase(p_old_name);

	const StringName old_key = String(p_theme_type) + "/fonts/" + p_old_name;
	if (_item_expressions.has(old_key)) {
		const String expression = _item_expressions[old_key];
		_item_expressions[String(p_theme_type) + "/fonts/" + p_name] = expression;
		_item_expressions.erase(old_key);
	}

	_emit_theme_changed(true);
}

void Theme::clear_font(const StringName &p_name, const StringName &p_theme_type) {
	_item_expressions.erase(String(p_theme_type) + "/fonts/" + p_name);
	ERR_FAIL_COND_MSG(!_font_map.has(p_theme_type), "Cannot clear the font '" + String(p_name) + "' because the node type '" + String(p_theme_type) + "' does not exist.");
	ERR_FAIL_COND_MSG(!_font_map[p_theme_type].has(p_name), "Cannot clear the font '" + String(p_name) + "' because it does not exist.");

	if (_font_map[p_theme_type][p_name].is_valid()) {
		_font_map[p_theme_type][p_name]->disconnect_changed(callable_mp(this, &Theme::_emit_theme_changed));
	}

	_font_map[p_theme_type].erase(p_name);

	_emit_theme_changed(true);
}

void Theme::get_font_list(const StringName &p_theme_type, List<StringName> *p_list) const {
	ERR_FAIL_NULL(p_list);

	if (!_font_map.has(p_theme_type)) {
		return;
	}

	for (const KeyValue<StringName, Ref<Font>> &E : _font_map[p_theme_type]) {
		p_list->push_back(E.key);
	}
}

void Theme::add_font_type(const StringName &p_theme_type) {
	ERR_FAIL_COND_MSG(!is_valid_type_name(p_theme_type), vformat("Invalid type name: '%s'", p_theme_type));

	if (_font_map.has(p_theme_type)) {
		return;
	}
	_font_map[p_theme_type] = ThemeFontMap();
}

void Theme::remove_font_type(const StringName &p_theme_type) {
	if (!_font_map.has(p_theme_type)) {
		return;
	}

	_freeze_change_propagation();

	for (const KeyValue<StringName, Ref<Font>> &E : _font_map[p_theme_type]) {
		Ref<Font> font = E.value;
		if (font.is_valid()) {
			font->disconnect_changed(callable_mp(this, &Theme::_emit_theme_changed));
		}
	}

	_font_map.erase(p_theme_type);

	_move_expression_type(p_theme_type, StringName(), "fonts");

	_unfreeze_and_propagate_changes();
}

void Theme::rename_font_type(const StringName &p_old_theme_type, const StringName &p_theme_type) {
	ERR_FAIL_COND_MSG(!is_valid_type_name(p_theme_type), vformat("Invalid type name: '%s'", p_theme_type));

	if (!_font_map.has(p_old_theme_type) || _font_map.has(p_theme_type)) {
		return;
	}

	_font_map[p_theme_type] = _font_map[p_old_theme_type];
	_font_map.erase(p_old_theme_type);

	_move_expression_type(p_old_theme_type, p_theme_type, "fonts");
}

void Theme::get_font_type_list(List<StringName> *p_list) const {
	ERR_FAIL_NULL(p_list);

	for (const KeyValue<StringName, ThemeFontMap> &E : _font_map) {
		p_list->push_back(E.key);
	}
}

void Theme::set_font_size(const StringName &p_name, const StringName &p_theme_type, int p_font_size) {
	_item_expressions.erase(String(p_theme_type) + "/font_sizes/" + p_name);
	ERR_FAIL_COND_MSG(!is_valid_item_name(p_name), vformat("Invalid item name: '%s'", p_name));
	ERR_FAIL_COND_MSG(!is_valid_type_name(p_theme_type), vformat("Invalid type name: '%s'", p_theme_type));

	bool existing = has_font_size_nocheck(p_name, p_theme_type);
	_font_size_map[p_theme_type][p_name] = p_font_size;

	_emit_theme_changed(!existing);
}

int Theme::get_font_size(const StringName &p_name, const StringName &p_theme_type) const {
	const StringName expression_key = String(p_theme_type) + "/font_sizes/" + p_name;
	if (_item_expressions.has(expression_key)) {
		Error error = OK;
		const Variant value = _resolve_expression_value(expression_key, error);
		ERR_FAIL_COND_V_MSG(error != OK || (value.get_type() != Variant::INT && value.get_type() != Variant::FLOAT && value.get_type() != Variant::BOOL), (has_default_font_size() ? _default_font_size : ThemeDB::get_singleton()->get_fallback_font_size()), vformat("Invalid theme expression result for '%s'.", expression_key));
		const int size = value;
		return size > 0 ? size : has_default_font_size() ? _default_font_size : ThemeDB::get_singleton()->get_fallback_font_size();
	}
	if (_font_size_map.has(p_theme_type) && _font_size_map[p_theme_type].has(p_name) && (_font_size_map[p_theme_type][p_name] > 0)) {
		return _font_size_map[p_theme_type][p_name];
	} else if (has_default_font_size()) {
		return _default_font_size;
	} else {
		return ThemeDB::get_singleton()->get_fallback_font_size();
	}
}

bool Theme::has_font_size(const StringName &p_name, const StringName &p_theme_type) const {
	if (_item_expressions.has(String(p_theme_type) + "/font_sizes/" + p_name)) {
		return true;
	}
	return ((_font_size_map.has(p_theme_type) && _font_size_map[p_theme_type].has(p_name) && (_font_size_map[p_theme_type][p_name] > 0)) || has_default_font_size());
}

bool Theme::has_font_size_no_default(const StringName &p_name, const StringName &p_theme_type) const {
	if (_item_expressions.has(String(p_theme_type) + "/font_sizes/" + p_name)) {
		return true;
	}
	return (_font_size_map.has(p_theme_type) && _font_size_map[p_theme_type].has(p_name) && (_font_size_map[p_theme_type][p_name] > 0));
}

bool Theme::has_font_size_nocheck(const StringName &p_name, const StringName &p_theme_type) const {
	return (_font_size_map.has(p_theme_type) && _font_size_map[p_theme_type].has(p_name));
}

void Theme::rename_font_size(const StringName &p_old_name, const StringName &p_name, const StringName &p_theme_type) {
	ERR_FAIL_COND_MSG(!is_valid_item_name(p_name), vformat("Invalid item name: '%s'", p_name));
	ERR_FAIL_COND_MSG(!is_valid_type_name(p_theme_type), vformat("Invalid type name: '%s'", p_theme_type));
	ERR_FAIL_COND_MSG(!_font_size_map.has(p_theme_type), "Cannot rename the font size '" + String(p_old_name) + "' because the node type '" + String(p_theme_type) + "' does not exist.");
	ERR_FAIL_COND_MSG(_font_size_map[p_theme_type].has(p_name), "Cannot rename the font size '" + String(p_old_name) + "' because the new name '" + String(p_name) + "' already exists.");
	ERR_FAIL_COND_MSG(!_font_size_map[p_theme_type].has(p_old_name), "Cannot rename the font size '" + String(p_old_name) + "' because it does not exist.");

	_font_size_map[p_theme_type][p_name] = _font_size_map[p_theme_type][p_old_name];
	_font_size_map[p_theme_type].erase(p_old_name);

	const StringName old_key = String(p_theme_type) + "/font_sizes/" + p_old_name;
	if (_item_expressions.has(old_key)) {
		const String expression = _item_expressions[old_key];
		_item_expressions[String(p_theme_type) + "/font_sizes/" + p_name] = expression;
		_item_expressions.erase(old_key);
	}

	_emit_theme_changed(true);
}

void Theme::clear_font_size(const StringName &p_name, const StringName &p_theme_type) {
	_item_expressions.erase(String(p_theme_type) + "/font_sizes/" + p_name);
	ERR_FAIL_COND_MSG(!_font_size_map.has(p_theme_type), "Cannot clear the font size '" + String(p_name) + "' because the node type '" + String(p_theme_type) + "' does not exist.");
	ERR_FAIL_COND_MSG(!_font_size_map[p_theme_type].has(p_name), "Cannot clear the font size '" + String(p_name) + "' because it does not exist.");

	_font_size_map[p_theme_type].erase(p_name);

	_emit_theme_changed(true);
}

void Theme::get_font_size_list(const StringName &p_theme_type, List<StringName> *p_list) const {
	ERR_FAIL_NULL(p_list);

	if (!_font_size_map.has(p_theme_type)) {
		return;
	}

	for (const KeyValue<StringName, int> &E : _font_size_map[p_theme_type]) {
		p_list->push_back(E.key);
	}
}

void Theme::add_font_size_type(const StringName &p_theme_type) {
	ERR_FAIL_COND_MSG(!is_valid_type_name(p_theme_type), vformat("Invalid type name: '%s'", p_theme_type));

	if (_font_size_map.has(p_theme_type)) {
		return;
	}
	_font_size_map[p_theme_type] = ThemeFontSizeMap();
}

void Theme::remove_font_size_type(const StringName &p_theme_type) {
	if (!_font_size_map.has(p_theme_type)) {
		return;
	}

	_font_size_map.erase(p_theme_type);

	_move_expression_type(p_theme_type, StringName(), "font_sizes");
}

void Theme::rename_font_size_type(const StringName &p_old_theme_type, const StringName &p_theme_type) {
	ERR_FAIL_COND_MSG(!is_valid_type_name(p_theme_type), vformat("Invalid type name: '%s'", p_theme_type));

	if (!_font_size_map.has(p_old_theme_type) || _font_size_map.has(p_theme_type)) {
		return;
	}

	_font_size_map[p_theme_type] = _font_size_map[p_old_theme_type];
	_font_size_map.erase(p_old_theme_type);

	_move_expression_type(p_old_theme_type, p_theme_type, "font_sizes");
}

void Theme::get_font_size_type_list(List<StringName> *p_list) const {
	ERR_FAIL_NULL(p_list);

	for (const KeyValue<StringName, ThemeFontSizeMap> &E : _font_size_map) {
		p_list->push_back(E.key);
	}
}

void Theme::set_color(const StringName &p_name, const StringName &p_theme_type, const Color &p_color) {
	_item_expressions.erase(String(p_theme_type) + "/colors/" + p_name);
	ERR_FAIL_COND_MSG(!is_valid_item_name(p_name), vformat("Invalid item name: '%s'", p_name));
	ERR_FAIL_COND_MSG(!is_valid_type_name(p_theme_type), vformat("Invalid type name: '%s'", p_theme_type));

	bool existing = has_color_nocheck(p_name, p_theme_type);
	_color_map[p_theme_type][p_name] = p_color;

	_emit_theme_changed(!existing);
}

Color Theme::get_color(const StringName &p_name, const StringName &p_theme_type) const {
	const StringName expression_key = String(p_theme_type) + "/colors/" + p_name;
	if (_item_expressions.has(expression_key)) {
		Error error = OK;
		const Variant value = _resolve_expression_value(expression_key, error);
		ERR_FAIL_COND_V_MSG(error != OK || (value.get_type() != Variant::COLOR), Color(), vformat("Invalid theme expression result for '%s'.", expression_key));
		return value;
	}
	if (_color_map.has(p_theme_type) && _color_map[p_theme_type].has(p_name)) {
		return _color_map[p_theme_type][p_name];
	} else {
		return Color();
	}
}

bool Theme::has_color(const StringName &p_name, const StringName &p_theme_type) const {
	return (_color_map.has(p_theme_type) && _color_map[p_theme_type].has(p_name));
}

bool Theme::has_color_nocheck(const StringName &p_name, const StringName &p_theme_type) const {
	return (_color_map.has(p_theme_type) && _color_map[p_theme_type].has(p_name));
}

void Theme::rename_color(const StringName &p_old_name, const StringName &p_name, const StringName &p_theme_type) {
	ERR_FAIL_COND_MSG(!is_valid_item_name(p_name), vformat("Invalid item name: '%s'", p_name));
	ERR_FAIL_COND_MSG(!is_valid_type_name(p_theme_type), vformat("Invalid type name: '%s'", p_theme_type));
	ERR_FAIL_COND_MSG(!_color_map.has(p_theme_type), "Cannot rename the color '" + String(p_old_name) + "' because the node type '" + String(p_theme_type) + "' does not exist.");
	ERR_FAIL_COND_MSG(_color_map[p_theme_type].has(p_name), "Cannot rename the color '" + String(p_old_name) + "' because the new name '" + String(p_name) + "' already exists.");
	ERR_FAIL_COND_MSG(!_color_map[p_theme_type].has(p_old_name), "Cannot rename the color '" + String(p_old_name) + "' because it does not exist.");

	_color_map[p_theme_type][p_name] = _color_map[p_theme_type][p_old_name];
	_color_map[p_theme_type].erase(p_old_name);

	const StringName old_key = String(p_theme_type) + "/colors/" + p_old_name;
	if (_item_expressions.has(old_key)) {
		const String expression = _item_expressions[old_key];
		_item_expressions[String(p_theme_type) + "/colors/" + p_name] = expression;
		_item_expressions.erase(old_key);
	}

	_emit_theme_changed(true);
}

void Theme::clear_color(const StringName &p_name, const StringName &p_theme_type) {
	_item_expressions.erase(String(p_theme_type) + "/colors/" + p_name);
	ERR_FAIL_COND_MSG(!_color_map.has(p_theme_type), "Cannot clear the color '" + String(p_name) + "' because the node type '" + String(p_theme_type) + "' does not exist.");
	ERR_FAIL_COND_MSG(!_color_map[p_theme_type].has(p_name), "Cannot clear the color '" + String(p_name) + "' because it does not exist.");

	_color_map[p_theme_type].erase(p_name);

	_emit_theme_changed(true);
}

void Theme::get_color_list(const StringName &p_theme_type, List<StringName> *p_list) const {
	ERR_FAIL_NULL(p_list);

	if (!_color_map.has(p_theme_type)) {
		return;
	}

	for (const KeyValue<StringName, Color> &E : _color_map[p_theme_type]) {
		p_list->push_back(E.key);
	}
}

void Theme::add_color_type(const StringName &p_theme_type) {
	ERR_FAIL_COND_MSG(!is_valid_type_name(p_theme_type), vformat("Invalid type name: '%s'", p_theme_type));

	if (_color_map.has(p_theme_type)) {
		return;
	}
	_color_map[p_theme_type] = ThemeColorMap();
}

void Theme::remove_color_type(const StringName &p_theme_type) {
	if (!_color_map.has(p_theme_type)) {
		return;
	}

	_color_map.erase(p_theme_type);

	_move_expression_type(p_theme_type, StringName(), "colors");
}

void Theme::rename_color_type(const StringName &p_old_theme_type, const StringName &p_theme_type) {
	ERR_FAIL_COND_MSG(!is_valid_type_name(p_theme_type), vformat("Invalid type name: '%s'", p_theme_type));

	if (!_color_map.has(p_old_theme_type) || _color_map.has(p_theme_type)) {
		return;
	}

	_color_map[p_theme_type] = _color_map[p_old_theme_type];
	_color_map.erase(p_old_theme_type);

	_move_expression_type(p_old_theme_type, p_theme_type, "colors");
}

void Theme::get_color_type_list(List<StringName> *p_list) const {
	ERR_FAIL_NULL(p_list);

	for (const KeyValue<StringName, ThemeColorMap> &E : _color_map) {
		p_list->push_back(E.key);
	}
}

void Theme::set_constant(const StringName &p_name, const StringName &p_theme_type, int p_constant) {
	_item_expressions.erase(String(p_theme_type) + "/constants/" + p_name);
	ERR_FAIL_COND_MSG(!is_valid_item_name(p_name), vformat("Invalid item name: '%s'", p_name));
	ERR_FAIL_COND_MSG(!is_valid_type_name(p_theme_type), vformat("Invalid type name: '%s'", p_theme_type));

	bool existing = has_constant_nocheck(p_name, p_theme_type);
	_constant_map[p_theme_type][p_name] = p_constant;

	_emit_theme_changed(!existing);
}

int Theme::get_constant(const StringName &p_name, const StringName &p_theme_type) const {
	const StringName expression_key = String(p_theme_type) + "/constants/" + p_name;
	if (_item_expressions.has(expression_key)) {
		Error error = OK;
		const Variant value = _resolve_expression_value(expression_key, error);
		ERR_FAIL_COND_V_MSG(error != OK || (value.get_type() != Variant::INT && value.get_type() != Variant::FLOAT && value.get_type() != Variant::BOOL), 0, vformat("Invalid theme expression result for '%s'.", expression_key));
		return value;
	}
	if (_constant_map.has(p_theme_type) && _constant_map[p_theme_type].has(p_name)) {
		return _constant_map[p_theme_type][p_name];
	} else {
		return 0;
	}
}

bool Theme::has_constant(const StringName &p_name, const StringName &p_theme_type) const {
	return (_constant_map.has(p_theme_type) && _constant_map[p_theme_type].has(p_name));
}

bool Theme::has_constant_nocheck(const StringName &p_name, const StringName &p_theme_type) const {
	return (_constant_map.has(p_theme_type) && _constant_map[p_theme_type].has(p_name));
}

void Theme::rename_constant(const StringName &p_old_name, const StringName &p_name, const StringName &p_theme_type) {
	ERR_FAIL_COND_MSG(!is_valid_item_name(p_name), vformat("Invalid item name: '%s'", p_name));
	ERR_FAIL_COND_MSG(!is_valid_type_name(p_theme_type), vformat("Invalid type name: '%s'", p_theme_type));
	ERR_FAIL_COND_MSG(!_constant_map.has(p_theme_type), "Cannot rename the constant '" + String(p_old_name) + "' because the node type '" + String(p_theme_type) + "' does not exist.");
	ERR_FAIL_COND_MSG(_constant_map[p_theme_type].has(p_name), "Cannot rename the constant '" + String(p_old_name) + "' because the new name '" + String(p_name) + "' already exists.");
	ERR_FAIL_COND_MSG(!_constant_map[p_theme_type].has(p_old_name), "Cannot rename the constant '" + String(p_old_name) + "' because it does not exist.");

	_constant_map[p_theme_type][p_name] = _constant_map[p_theme_type][p_old_name];
	_constant_map[p_theme_type].erase(p_old_name);

	const StringName old_key = String(p_theme_type) + "/constants/" + p_old_name;
	if (_item_expressions.has(old_key)) {
		const String expression = _item_expressions[old_key];
		_item_expressions[String(p_theme_type) + "/constants/" + p_name] = expression;
		_item_expressions.erase(old_key);
	}

	_emit_theme_changed(true);
}

void Theme::clear_constant(const StringName &p_name, const StringName &p_theme_type) {
	_item_expressions.erase(String(p_theme_type) + "/constants/" + p_name);
	ERR_FAIL_COND_MSG(!_constant_map.has(p_theme_type), "Cannot clear the constant '" + String(p_name) + "' because the node type '" + String(p_theme_type) + "' does not exist.");
	ERR_FAIL_COND_MSG(!_constant_map[p_theme_type].has(p_name), "Cannot clear the constant '" + String(p_name) + "' because it does not exist.");

	_constant_map[p_theme_type].erase(p_name);

	_emit_theme_changed(true);
}

void Theme::get_constant_list(const StringName &p_theme_type, List<StringName> *p_list) const {
	ERR_FAIL_NULL(p_list);

	if (!_constant_map.has(p_theme_type)) {
		return;
	}

	for (const KeyValue<StringName, int> &E : _constant_map[p_theme_type]) {
		p_list->push_back(E.key);
	}
}

void Theme::add_constant_type(const StringName &p_theme_type) {
	ERR_FAIL_COND_MSG(!is_valid_type_name(p_theme_type), vformat("Invalid type name: '%s'", p_theme_type));

	if (_constant_map.has(p_theme_type)) {
		return;
	}
	_constant_map[p_theme_type] = ThemeConstantMap();
}

void Theme::remove_constant_type(const StringName &p_theme_type) {
	if (!_constant_map.has(p_theme_type)) {
		return;
	}

	_constant_map.erase(p_theme_type);

	_move_expression_type(p_theme_type, StringName(), "constants");
}

void Theme::rename_constant_type(const StringName &p_old_theme_type, const StringName &p_theme_type) {
	ERR_FAIL_COND_MSG(!is_valid_type_name(p_theme_type), vformat("Invalid type name: '%s'", p_theme_type));

	if (!_constant_map.has(p_old_theme_type) || _constant_map.has(p_theme_type)) {
		return;
	}

	_constant_map[p_theme_type] = _constant_map[p_old_theme_type];
	_constant_map.erase(p_old_theme_type);

	_move_expression_type(p_old_theme_type, p_theme_type, "constants");
}

void Theme::get_constant_type_list(List<StringName> *p_list) const {
	ERR_FAIL_NULL(p_list);

	for (const KeyValue<StringName, ThemeConstantMap> &E : _constant_map) {
		p_list->push_back(E.key);
	}
}

void Theme::set_sound(const StringName &p_name, const StringName &p_theme_type, const Ref<AudioStream> &p_sound) {
	_item_expressions.erase(String(p_theme_type) + "/sounds/" + p_name);
	ERR_FAIL_COND_MSG(!is_valid_item_name(p_name), vformat("Invalid item name: '%s'", p_name));
	ERR_FAIL_COND_MSG(!is_valid_type_name(p_theme_type), vformat("Invalid type name: '%s'", p_theme_type));

	bool existing = false;
	if (_sound_map[p_theme_type][p_name].is_valid()) {
		existing = true;
		_sound_map[p_theme_type][p_name]->disconnect_changed(callable_mp(this, &Theme::_emit_theme_changed));
	}

	_sound_map[p_theme_type][p_name] = p_sound;

	if (p_sound.is_valid()) {
		_sound_map[p_theme_type][p_name]->connect_changed(callable_mp(this, &Theme::_emit_theme_changed).bind(false), CONNECT_REFERENCE_COUNTED);
	}

	_emit_theme_changed(!existing);
}

Ref<AudioStream> Theme::get_sound(const StringName &p_name, const StringName &p_theme_type) const {
	const StringName expression_key = String(p_theme_type) + "/sounds/" + p_name;
	if (_item_expressions.has(expression_key)) {
		Error error = OK;
		const Variant value = _resolve_expression_value(expression_key, error);
		ERR_FAIL_COND_V_MSG(error != OK || (value.get_type() != Variant::NIL && value.get_type() != Variant::OBJECT), Ref<AudioStream>(), vformat("Invalid theme expression result for '%s'.", expression_key));
		return value;
	}
	if (_sound_map.has(p_theme_type) && _sound_map[p_theme_type].has(p_name) && _sound_map[p_theme_type][p_name].is_valid()) {
		return _sound_map[p_theme_type][p_name];
	} else {
		return ThemeDB::get_singleton()->get_fallback_sound();
	}
}

bool Theme::has_sound(const StringName &p_name, const StringName &p_theme_type) const {
	if (_item_expressions.has(String(p_theme_type) + "/sounds/" + p_name)) {
		return true;
	}
	return (_sound_map.has(p_theme_type) && _sound_map[p_theme_type].has(p_name) && _sound_map[p_theme_type][p_name].is_valid());
}

bool Theme::has_sound_nocheck(const StringName &p_name, const StringName &p_theme_type) const {
	return (_sound_map.has(p_theme_type) && _sound_map[p_theme_type].has(p_name));
}

void Theme::rename_sound(const StringName &p_old_name, const StringName &p_name, const StringName &p_theme_type) {
	ERR_FAIL_COND_MSG(!is_valid_item_name(p_name), vformat("Invalid item name: '%s'", p_name));
	ERR_FAIL_COND_MSG(!is_valid_type_name(p_theme_type), vformat("Invalid type name: '%s'", p_theme_type));
	ERR_FAIL_COND_MSG(!_sound_map.has(p_theme_type), "Cannot rename the sound '" + String(p_old_name) + "' because the node type '" + String(p_theme_type) + "' does not exist.");
	ERR_FAIL_COND_MSG(_sound_map[p_theme_type].has(p_name), "Cannot rename the sound '" + String(p_old_name) + "' because the new name '" + String(p_name) + "' already exists.");
	ERR_FAIL_COND_MSG(!_sound_map[p_theme_type].has(p_old_name), "Cannot rename the sound '" + String(p_old_name) + "' because it does not exist.");

	_sound_map[p_theme_type][p_name] = _sound_map[p_theme_type][p_old_name];
	_sound_map[p_theme_type].erase(p_old_name);

	const StringName old_key = String(p_theme_type) + "/sounds/" + p_old_name;
	if (_item_expressions.has(old_key)) {
		const String expression = _item_expressions[old_key];
		_item_expressions[String(p_theme_type) + "/sounds/" + p_name] = expression;
		_item_expressions.erase(old_key);
	}

	_emit_theme_changed(true);
}

void Theme::clear_sound(const StringName &p_name, const StringName &p_theme_type) {
	_item_expressions.erase(String(p_theme_type) + "/sounds/" + p_name);
	ERR_FAIL_COND_MSG(!_sound_map.has(p_theme_type), "Cannot clear the sound '" + String(p_name) + "' because the node type '" + String(p_theme_type) + "' does not exist.");
	ERR_FAIL_COND_MSG(!_sound_map[p_theme_type].has(p_name), "Cannot clear the sound '" + String(p_name) + "' because it does not exist.");

	if (_sound_map[p_theme_type][p_name].is_valid()) {
		_sound_map[p_theme_type][p_name]->disconnect_changed(callable_mp(this, &Theme::_emit_theme_changed));
	}

	_sound_map[p_theme_type].erase(p_name);

	_emit_theme_changed(true);
}

void Theme::get_sound_list(StringName p_theme_type, List<StringName> *p_list) const {
	ERR_FAIL_NULL(p_list);

	if (!_sound_map.has(p_theme_type)) {
		return;
	}

	for (const KeyValue<StringName, Ref<AudioStream>> &E : _sound_map[p_theme_type]) {
		p_list->push_back(E.key);
	}
}

void Theme::add_sound_type(const StringName &p_theme_type) {
	ERR_FAIL_COND_MSG(!is_valid_type_name(p_theme_type), vformat("Invalid type name: '%s'", p_theme_type));

	if (_sound_map.has(p_theme_type)) {
		return;
	}
	_sound_map[p_theme_type] = ThemeSoundMap();
}

void Theme::remove_sound_type(const StringName &p_theme_type) {
	if (!_sound_map.has(p_theme_type)) {
		return;
	}

	_freeze_change_propagation();

	for (const KeyValue<StringName, Ref<AudioStream>> &E : _sound_map[p_theme_type]) {
		Ref<AudioStream> sound = E.value;
		if (sound.is_valid()) {
			sound->disconnect_changed(callable_mp(this, &Theme::_emit_theme_changed));
		}
	}

	_sound_map.erase(p_theme_type);

	_move_expression_type(p_theme_type, StringName(), "sounds");

	_unfreeze_and_propagate_changes();
}

void Theme::rename_sound_type(const StringName &p_old_theme_type, const StringName &p_theme_type) {
	ERR_FAIL_COND_MSG(!is_valid_type_name(p_theme_type), vformat("Invalid type name: '%s'", p_theme_type));

	if (!_sound_map.has(p_old_theme_type) || _sound_map.has(p_theme_type)) {
		return;
	}

	_sound_map[p_theme_type] = _sound_map[p_old_theme_type];
	_sound_map.erase(p_old_theme_type);

	_move_expression_type(p_old_theme_type, p_theme_type, "sounds");
}

void Theme::get_sound_type_list(List<StringName> *p_list) const {
	ERR_FAIL_NULL(p_list);

	for (const KeyValue<StringName, ThemeSoundMap> &E : _sound_map) {
		p_list->push_back(E.key);
	}
}

void Theme::set_theme_item(DataType p_data_type, const StringName &p_name, const StringName &p_theme_type, const Variant &p_value) {
	ERR_FAIL_INDEX(p_data_type, DATA_TYPE_MAX);
	if (p_value.get_type() == Variant::STRING) {
		set_theme_expression(String(p_theme_type) + "/" + ITEM_CATEGORIES[p_data_type] + "/" + p_name, p_value);
		return;
	}
	switch (p_data_type) {
		case DATA_TYPE_COLOR:
			{
			ERR_FAIL_COND_MSG(p_value.get_type() != Variant::COLOR, "Theme item's data type (Color) does not match Variant's type (" + Variant::get_type_name(p_value.get_type()) + ").");

			Color color_value = p_value;
			set_color(p_name, p_theme_type, color_value);
		} break;
		case DATA_TYPE_CONSTANT:
			{
			ERR_FAIL_COND_MSG(p_value.get_type() != Variant::INT, "Theme item's data type (int) does not match Variant's type (" + Variant::get_type_name(p_value.get_type()) + ").");

			int constant_value = p_value;
			set_constant(p_name, p_theme_type, constant_value);
		} break;
		case DATA_TYPE_FONT:
			{
			ERR_FAIL_COND_MSG(p_value.get_type() != Variant::OBJECT, "Theme item's data type (Object) does not match Variant's type (" + Variant::get_type_name(p_value.get_type()) + ").");

			Ref<Font> font_value = Object::cast_to<Font>(p_value.get_validated_object());
			set_font(p_name, p_theme_type, font_value);
		} break;
		case DATA_TYPE_FONT_SIZE:
			{
			ERR_FAIL_COND_MSG(p_value.get_type() != Variant::INT, "Theme item's data type (int) does not match Variant's type (" + Variant::get_type_name(p_value.get_type()) + ").");

			int font_size_value = p_value;
			set_font_size(p_name, p_theme_type, font_size_value);
		} break;
		case DATA_TYPE_ICON:
			{
			ERR_FAIL_COND_MSG(p_value.get_type() != Variant::OBJECT, "Theme item's data type (Object) does not match Variant's type (" + Variant::get_type_name(p_value.get_type()) + ").");

			Ref<Texture2D> icon_value = Object::cast_to<Texture2D>(p_value.get_validated_object());
			set_icon(p_name, p_theme_type, icon_value);
		} break;
		case DATA_TYPE_STYLEBOX:
			{
			ERR_FAIL_COND_MSG(p_value.get_type() != Variant::OBJECT, "Theme item's data type (Object) does not match Variant's type (" + Variant::get_type_name(p_value.get_type()) + ").");

			Ref<StyleBox> stylebox_value = Object::cast_to<StyleBox>(p_value.get_validated_object());
			set_stylebox(p_name, p_theme_type, stylebox_value);
		} break;
		case DATA_TYPE_SOUND:
			{
			ERR_FAIL_COND_MSG(p_value.get_type() != Variant::OBJECT, "Theme item's data type (Object) does not match Variant's type (" + Variant::get_type_name(p_value.get_type()) + ").");

			Ref<AudioStream> sound_value = Object::cast_to<AudioStream>(p_value.get_validated_object());
			set_sound(p_name, p_theme_type, sound_value);
		} break;
		case DATA_TYPE_MAX:
			break; // Can't happen, but silences warning.
	}
}

Variant Theme::get_theme_item(DataType p_data_type, const StringName &p_name, const StringName &p_theme_type) const {
	switch (p_data_type) {
		case DATA_TYPE_COLOR:
			return get_color(p_name, p_theme_type);
		case DATA_TYPE_CONSTANT:
			return get_constant(p_name, p_theme_type);
		case DATA_TYPE_FONT:
			return get_font(p_name, p_theme_type);
		case DATA_TYPE_FONT_SIZE:
			return get_font_size(p_name, p_theme_type);
		case DATA_TYPE_ICON:
			return get_icon(p_name, p_theme_type);
		case DATA_TYPE_STYLEBOX:
			return get_stylebox(p_name, p_theme_type);
		case DATA_TYPE_SOUND:
			return get_sound(p_name, p_theme_type);
		case DATA_TYPE_MAX:
			break; // Can't happen, but silences warning.
	}

	return Variant();
}

Variant Theme::get_theme_item_definition(DataType p_data_type, const StringName &p_name, const StringName &p_theme_type) const {
	ERR_FAIL_INDEX_V(p_data_type, DATA_TYPE_MAX, Variant());
	const StringName property = String(p_theme_type) + "/" + ITEM_CATEGORIES[p_data_type] + "/" + p_name;
	const String *expression = _item_expressions.getptr(property);
	if (expression) {
		return *expression;
	}
	switch (p_data_type) {
		case DATA_TYPE_COLOR:
			if (_color_map.has(p_theme_type) && _color_map[p_theme_type].has(p_name)) {
				return _color_map[p_theme_type][p_name];
			}
			break;
		case DATA_TYPE_CONSTANT:
			if (_constant_map.has(p_theme_type) && _constant_map[p_theme_type].has(p_name)) {
				return _constant_map[p_theme_type][p_name];
			}
			break;
		case DATA_TYPE_FONT:
			if (_font_map.has(p_theme_type) && _font_map[p_theme_type].has(p_name)) {
				return _font_map[p_theme_type][p_name];
			}
			break;
		case DATA_TYPE_FONT_SIZE:
			if (_font_size_map.has(p_theme_type) && _font_size_map[p_theme_type].has(p_name)) {
				return _font_size_map[p_theme_type][p_name];
			}
			break;
		case DATA_TYPE_ICON:
			if (_icon_map.has(p_theme_type) && _icon_map[p_theme_type].has(p_name)) {
				return _icon_map[p_theme_type][p_name];
			}
			break;
		case DATA_TYPE_STYLEBOX:
			if (_style_map.has(p_theme_type) && _style_map[p_theme_type].has(p_name)) {
				return _style_map[p_theme_type][p_name];
			}
			break;
		case DATA_TYPE_SOUND:
			if (_sound_map.has(p_theme_type) && _sound_map[p_theme_type].has(p_name)) {
				return _sound_map[p_theme_type][p_name];
			}
			break;
		case DATA_TYPE_MAX:
			break;
	}
	return Variant();
}

bool Theme::has_theme_item(DataType p_data_type, const StringName &p_name, const StringName &p_theme_type) const {
	switch (p_data_type) {
		case DATA_TYPE_COLOR:
			return has_color(p_name, p_theme_type);
		case DATA_TYPE_CONSTANT:
			return has_constant(p_name, p_theme_type);
		case DATA_TYPE_FONT:
			if (!_variation_map.has(p_theme_type)) {
				return has_font(p_name, p_theme_type);
			} else {
				return has_font_no_default(p_name, p_theme_type);
			}
		case DATA_TYPE_FONT_SIZE:
			if (!_variation_map.has(p_theme_type)) {
				return has_font_size(p_name, p_theme_type);
			} else {
				return has_font_size_no_default(p_name, p_theme_type);
			}
		case DATA_TYPE_ICON:
			return has_icon(p_name, p_theme_type);
		case DATA_TYPE_STYLEBOX:
			return has_stylebox(p_name, p_theme_type);
		case DATA_TYPE_SOUND:
			return has_sound(p_name, p_theme_type);
		case DATA_TYPE_MAX:
			break; // Can't happen, but silences warning.
	}

	return false;
}

bool Theme::has_theme_item_nocheck(DataType p_data_type, const StringName &p_name, const StringName &p_theme_type) const {
	switch (p_data_type) {
		case DATA_TYPE_COLOR:
			return has_color_nocheck(p_name, p_theme_type);
		case DATA_TYPE_CONSTANT:
			return has_constant_nocheck(p_name, p_theme_type);
		case DATA_TYPE_FONT:
			return has_font_nocheck(p_name, p_theme_type);
		case DATA_TYPE_FONT_SIZE:
			return has_font_size_nocheck(p_name, p_theme_type);
		case DATA_TYPE_ICON:
			return has_icon_nocheck(p_name, p_theme_type);
		case DATA_TYPE_STYLEBOX:
			return has_stylebox_nocheck(p_name, p_theme_type);
		case DATA_TYPE_SOUND:
			return has_sound_nocheck(p_name, p_theme_type);
		case DATA_TYPE_MAX:
			break; // Can't happen, but silences warning.
	}

	return false;
}

void Theme::rename_theme_item(DataType p_data_type, const StringName &p_old_name, const StringName &p_name, const StringName &p_theme_type) {
	switch (p_data_type) {
		case DATA_TYPE_COLOR:
			rename_color(p_old_name, p_name, p_theme_type);
			break;
		case DATA_TYPE_CONSTANT:
			rename_constant(p_old_name, p_name, p_theme_type);
			break;
		case DATA_TYPE_FONT:
			rename_font(p_old_name, p_name, p_theme_type);
			break;
		case DATA_TYPE_FONT_SIZE:
			rename_font_size(p_old_name, p_name, p_theme_type);
			break;
		case DATA_TYPE_ICON:
			rename_icon(p_old_name, p_name, p_theme_type);
			break;
		case DATA_TYPE_STYLEBOX:
			rename_stylebox(p_old_name, p_name, p_theme_type);
			break;
		case DATA_TYPE_SOUND:
			rename_sound(p_old_name, p_name, p_theme_type);
			break;
		case DATA_TYPE_MAX:
			break; // Can't happen, but silences warning.
	}
}

void Theme::clear_theme_item(DataType p_data_type, const StringName &p_name, const StringName &p_theme_type) {
	switch (p_data_type) {
		case DATA_TYPE_COLOR:
			clear_color(p_name, p_theme_type);
			break;
		case DATA_TYPE_CONSTANT:
			clear_constant(p_name, p_theme_type);
			break;
		case DATA_TYPE_FONT:
			clear_font(p_name, p_theme_type);
			break;
		case DATA_TYPE_FONT_SIZE:
			clear_font_size(p_name, p_theme_type);
			break;
		case DATA_TYPE_ICON:
			clear_icon(p_name, p_theme_type);
			break;
		case DATA_TYPE_STYLEBOX:
			clear_stylebox(p_name, p_theme_type);
			break;
		case DATA_TYPE_SOUND:
			clear_sound(p_name, p_theme_type);
			break;
		case DATA_TYPE_MAX:
			break; // Can't happen, but silences warning.
	}
}

void Theme::get_theme_item_list(DataType p_data_type, const StringName &p_theme_type, List<StringName> *p_list) const {
	switch (p_data_type) {
		case DATA_TYPE_COLOR:
			get_color_list(p_theme_type, p_list);
			break;
		case DATA_TYPE_CONSTANT:
			get_constant_list(p_theme_type, p_list);
			break;
		case DATA_TYPE_FONT:
			get_font_list(p_theme_type, p_list);
			break;
		case DATA_TYPE_FONT_SIZE:
			get_font_size_list(p_theme_type, p_list);
			break;
		case DATA_TYPE_ICON:
			get_icon_list(p_theme_type, p_list);
			break;
		case DATA_TYPE_STYLEBOX:
			get_stylebox_list(p_theme_type, p_list);
			break;
		case DATA_TYPE_SOUND:
			get_sound_list(p_theme_type, p_list);
			break;
		case DATA_TYPE_MAX:
			break; // Can't happen, but silences warning.
	}
}

void Theme::add_theme_item_type(DataType p_data_type, const StringName &p_theme_type) {
	switch (p_data_type) {
		case DATA_TYPE_COLOR:
			add_color_type(p_theme_type);
			break;
		case DATA_TYPE_CONSTANT:
			add_constant_type(p_theme_type);
			break;
		case DATA_TYPE_FONT:
			add_font_type(p_theme_type);
			break;
		case DATA_TYPE_FONT_SIZE:
			add_font_size_type(p_theme_type);
			break;
		case DATA_TYPE_ICON:
			add_icon_type(p_theme_type);
			break;
		case DATA_TYPE_STYLEBOX:
			add_stylebox_type(p_theme_type);
			break;
		case DATA_TYPE_SOUND:
			add_sound_type(p_theme_type);
			break;
		case DATA_TYPE_MAX:
			break; // Can't happen, but silences warning.
	}
}

void Theme::remove_theme_item_type(DataType p_data_type, const StringName &p_theme_type) {
	switch (p_data_type) {
		case DATA_TYPE_COLOR:
			remove_color_type(p_theme_type);
			break;
		case DATA_TYPE_CONSTANT:
			remove_constant_type(p_theme_type);
			break;
		case DATA_TYPE_FONT:
			remove_font_type(p_theme_type);
			break;
		case DATA_TYPE_FONT_SIZE:
			remove_font_size_type(p_theme_type);
			break;
		case DATA_TYPE_ICON:
			remove_icon_type(p_theme_type);
			break;
		case DATA_TYPE_STYLEBOX:
			remove_stylebox_type(p_theme_type);
			break;
		case DATA_TYPE_SOUND:
			remove_sound_type(p_theme_type);
			break;
		case DATA_TYPE_MAX:
			break; // Can't happen, but silences warning.
	}
}

void Theme::rename_theme_item_type(DataType p_data_type, const StringName &p_old_theme_type, const StringName &p_theme_type) {
	switch (p_data_type) {
		case DATA_TYPE_COLOR:
			rename_color_type(p_old_theme_type, p_theme_type);
			break;
		case DATA_TYPE_CONSTANT:
			rename_constant_type(p_old_theme_type, p_theme_type);
			break;
		case DATA_TYPE_FONT:
			rename_font_type(p_old_theme_type, p_theme_type);
			break;
		case DATA_TYPE_FONT_SIZE:
			rename_font_size_type(p_old_theme_type, p_theme_type);
			break;
		case DATA_TYPE_ICON:
			rename_icon_type(p_old_theme_type, p_theme_type);
			break;
		case DATA_TYPE_STYLEBOX:
			rename_stylebox_type(p_old_theme_type, p_theme_type);
			break;
		case DATA_TYPE_SOUND:
			rename_sound_type(p_old_theme_type, p_theme_type);
			break;
		case DATA_TYPE_MAX:
			break; // Can't happen, but silences warning.
	}
}

void Theme::get_theme_item_type_list(DataType p_data_type, List<StringName> *p_list) const {
	switch (p_data_type) {
		case DATA_TYPE_COLOR:
			get_color_type_list(p_list);
			break;
		case DATA_TYPE_CONSTANT:
			get_constant_type_list(p_list);
			break;
		case DATA_TYPE_FONT:
			get_font_type_list(p_list);
			break;
		case DATA_TYPE_FONT_SIZE:
			get_font_size_type_list(p_list);
			break;
		case DATA_TYPE_ICON:
			get_icon_type_list(p_list);
			break;
		case DATA_TYPE_STYLEBOX:
			get_stylebox_type_list(p_list);
			break;
		case DATA_TYPE_SOUND:
			get_sound_type_list(p_list);
			break;
		case DATA_TYPE_MAX:
			break; // Can't happen, but silences warning.
	}
}

void Theme::set_type_variation(const StringName &p_theme_type, const StringName &p_base_type) {
	ERR_FAIL_COND_MSG(!is_valid_type_name(p_theme_type), vformat("Invalid type name: '%s'", p_theme_type));
	ERR_FAIL_COND_MSG(!is_valid_type_name(p_base_type), vformat("Invalid type name: '%s'", p_base_type));
	ERR_FAIL_COND_MSG(p_theme_type == StringName(), vformat("An empty theme type cannot be marked as a variation of another type (\"%s\").", p_base_type));
	ERR_FAIL_COND_MSG(ClassDB::class_exists(p_theme_type), vformat("A type associated with a built-in class cannot be marked as a variation of another type (variation: \"%s\", base: \"%s\").", p_theme_type, p_base_type));
	ERR_FAIL_COND_MSG(p_base_type == StringName(), "An empty theme type cannot be the base type of a variation. Use clear_type_variation() instead if you want to unmark '" + String(p_theme_type) + "' as a variation.");

	if (_variation_map.has(p_theme_type)) {
		StringName old_base = _variation_map[p_theme_type];
		_variation_base_map[old_base].erase(p_theme_type);
	}

	_variation_map[p_theme_type] = p_base_type;
	_variation_base_map[p_base_type].push_back(p_theme_type);

	_emit_theme_changed(true);
}

bool Theme::is_type_variation(const StringName &p_theme_type, const StringName &p_base_type) const {
	return (_variation_map.has(p_theme_type) && _variation_map[p_theme_type] == p_base_type);
}

void Theme::clear_type_variation(const StringName &p_theme_type) {
	ERR_FAIL_COND_MSG(!_variation_map.has(p_theme_type), "Cannot clear the type variation '" + String(p_theme_type) + "' because it does not exist.");

	StringName base_type = _variation_map[p_theme_type];
	_variation_base_map[base_type].erase(p_theme_type);
	_variation_map.erase(p_theme_type);

	_emit_theme_changed(true);
}

StringName Theme::get_type_variation_base(const StringName &p_theme_type) const {
	if (!_variation_map.has(p_theme_type)) {
		return StringName();
	}

	return _variation_map[p_theme_type];
}

void Theme::get_type_variation_list(const StringName &p_base_type, List<StringName> *p_list) const {
	ERR_FAIL_NULL(p_list);

	if (!_variation_base_map.has(p_base_type)) {
		return;
	}

	for (const StringName &E : _variation_base_map[p_base_type]) {
		// Prevent infinite loops if variants were set to be cross-dependent (that's still invalid usage, but handling for stability sake).
		if (p_list->find(E)) {
			continue;
		}

		p_list->push_back(E);
		// Continue looking for sub-variations.
		get_type_variation_list(E, p_list);
	}
}

void Theme::add_type(const StringName &p_theme_type) {
	// Add a record to every data type map.
	for (int i = 0; i < Theme::DATA_TYPE_MAX; i++) {
		Theme::DataType dt = (Theme::DataType)i;
		add_theme_item_type(dt, p_theme_type);
	}

	_emit_theme_changed(true);
}

void Theme::remove_type(const StringName &p_theme_type) {
	// Gracefully remove the record from every data type map.
	for (int i = 0; i < Theme::DATA_TYPE_MAX; i++) {
		Theme::DataType dt = (Theme::DataType)i;
		remove_theme_item_type(dt, p_theme_type);
	}

	// If type is a variation, remove that connection.
	if (get_type_variation_base(p_theme_type) != StringName()) {
		clear_type_variation(p_theme_type);
	}

	// If type is a variation base, remove all those connections.
	List<StringName> names;
	get_type_variation_list(p_theme_type, &names);
	for (const StringName &E : names) {
		clear_type_variation(E);
	}

	_emit_theme_changed(true);
}

void Theme::rename_type(const StringName &p_old_theme_type, const StringName &p_theme_type) {
	// Gracefully rename the record in every data type map.
	for (int i = 0; i < Theme::DATA_TYPE_MAX; i++) {
		Theme::DataType dt = (Theme::DataType)i;
		rename_theme_item_type(dt, p_old_theme_type, p_theme_type);
	}

	// If type is a variation, replace that connection.
	const StringName base_type = get_type_variation_base(p_old_theme_type);
	if (base_type != StringName()) {
		clear_type_variation(p_old_theme_type);
		if (p_theme_type != StringName() && !ClassDB::class_exists(p_theme_type)) {
			set_type_variation(p_theme_type, base_type);
		}
	}

	// If type is a variation base, replace all those connections.
	List<StringName> names;
	get_type_variation_list(p_old_theme_type, &names);
	for (const StringName &E : names) {
		clear_type_variation(E);
		if (p_theme_type != StringName()) {
			set_type_variation(E, p_theme_type);
		}
	}

	_emit_theme_changed(true);
}

void Theme::get_type_list(List<StringName> *p_list) const {
	ERR_FAIL_NULL(p_list);

	// This Set guarantees uniqueness.
	// Because each map can have the same type defined, but for this method
	// we only want one occurrence of each type.
	HashSet<StringName> types;

	// Icons.
	for (const KeyValue<StringName, ThemeIconMap> &E : _icon_map) {
		types.insert(E.key);
	}

	// Styles.
	for (const KeyValue<StringName, ThemeStyleMap> &E : _style_map) {
		types.insert(E.key);
	}

	// Fonts.
	for (const KeyValue<StringName, ThemeFontMap> &E : _font_map) {
		types.insert(E.key);
	}

	// Font sizes.
	for (const KeyValue<StringName, ThemeFontSizeMap> &E : _font_size_map) {
		types.insert(E.key);
	}

	// Colors.
	for (const KeyValue<StringName, ThemeColorMap> &E : _color_map) {
		types.insert(E.key);
	}

	// Constants.
	for (const KeyValue<StringName, ThemeConstantMap> &E : _constant_map) {
		types.insert(E.key);
	}

	// Sounds.
	for (const KeyValue<StringName, ThemeSoundMap> &E : _sound_map) {
		types.insert(E.key);
	}

	// Variations.
	for (const KeyValue<StringName, StringName> &E : _variation_map) {
		types.insert(E.key);
	}

	// If it's the default theme, ask also ThemeDB, in case some types don't have assigned items.
	if (ThemeDB::get_singleton()->get_default_theme() == this) {
		ThemeDB::get_singleton()->get_class_list(types);
	}

	for (const StringName &E : types) {
		p_list->push_back(E);
	}
}

void Theme::get_type_dependencies(const StringName &p_base_type, const StringName &p_type_variation, Vector<StringName> &r_result) {
	// Build the dependency chain for type variations.
	if (p_type_variation != StringName()) {
		StringName variation_name = p_type_variation;
		while (variation_name != StringName()) {
			r_result.push_back(variation_name);
			variation_name = get_type_variation_base(variation_name);

			// If we have reached the base type dependency, it's safe to stop (assuming no funny business was done to the Theme).
			if (variation_name == p_base_type) {
				break;
			}
		}
	}

	// Continue building the chain using native class hierarchy.
	ThemeDB::get_singleton()->get_native_type_dependencies(p_base_type, r_result);
}

void Theme::merge_with(const Ref<Theme> &p_other) {
	if (p_other.is_null()) {
		return;
	}

	_freeze_change_propagation();

	// Colors.
	{
		for (const KeyValue<StringName, ThemeColorMap> &E : p_other->_color_map) {
			for (const KeyValue<StringName, Color> &F : E.value) {
				set_color(F.key, E.key, F.value);
			}
		}
	}

	// Constants.
	{
		for (const KeyValue<StringName, ThemeConstantMap> &E : p_other->_constant_map) {
			for (const KeyValue<StringName, int> &F : E.value) {
				set_constant(F.key, E.key, F.value);
			}
		}
	}

	// Fonts.
	{
		for (const KeyValue<StringName, ThemeFontMap> &E : p_other->_font_map) {
			for (const KeyValue<StringName, Ref<Font>> &F : E.value) {
				set_font(F.key, E.key, F.value);
			}
		}
	}

	// Font sizes.
	{
		for (const KeyValue<StringName, ThemeFontSizeMap> &E : p_other->_font_size_map) {
			for (const KeyValue<StringName, int> &F : E.value) {
				set_font_size(F.key, E.key, F.value);
			}
		}
	}

	// Icons.
	{
		for (const KeyValue<StringName, ThemeIconMap> &E : p_other->_icon_map) {
			for (const KeyValue<StringName, Ref<Texture2D>> &F : E.value) {
				set_icon(F.key, E.key, F.value);
			}
		}
	}

	// Styleboxes.
	{
		for (const KeyValue<StringName, ThemeStyleMap> &E : p_other->_style_map) {
			for (const KeyValue<StringName, Ref<StyleBox>> &F : E.value) {
				set_stylebox(F.key, E.key, F.value);
			}
		}
	}

	// Sounds.
	{
		for (const KeyValue<StringName, ThemeSoundMap> &E : p_other->_sound_map) {
			for (const KeyValue<StringName, Ref<AudioStream>> &F : E.value) {
				set_sound(F.key, E.key, F.value);
			}
		}
	}

	// Type variations.
	{
		for (const KeyValue<StringName, StringName> &E : p_other->_variation_map) {
			set_type_variation(E.key, E.value);
		}
	}

	// Defaults.
	for (const KeyValue<StringName, Variant> &parameter : p_other->_style_parameters) {
		set_style_parameter(parameter.key, parameter.value);
	}
	if (p_other->has_default_font_family()) {
		set_default_font_family(p_other->_default_font_family);
	}
	if (p_other->has_default_font_size()) {
		set_default_font_size(p_other->_default_font_size);
	}
	if (p_other->has_default_base_scale()) {
		set_default_base_scale(p_other->_default_base_scale);
	}

	for (const KeyValue<StringName, String> &entry : p_other->_item_expressions) {
		_item_expressions[entry.key] = entry.value;
	}
	_unfreeze_and_propagate_changes();
}

void Theme::clear() {
	_item_expressions.clear();
	_expression_values.clear();
	_resolving_expressions.clear();
	_style_parameters.clear();
	// These items need disconnecting.
	{
		for (const KeyValue<StringName, ThemeIconMap> &E : _icon_map) {
			for (const KeyValue<StringName, Ref<Texture2D>> &F : E.value) {
				if (F.value.is_valid()) {
					Ref<Texture2D> icon = F.value;
					icon->disconnect_changed(callable_mp(this, &Theme::_emit_theme_changed));
				}
			}
		}
	}

	{
		for (const KeyValue<StringName, ThemeStyleMap> &E : _style_map) {
			for (const KeyValue<StringName, Ref<StyleBox>> &F : E.value) {
				if (F.value.is_valid()) {
					Ref<StyleBox> style = F.value;
					style->disconnect_changed(callable_mp(this, &Theme::_emit_theme_changed));
				}
			}
		}
	}

	{
		for (const KeyValue<StringName, ThemeFontMap> &E : _font_map) {
			for (const KeyValue<StringName, Ref<Font>> &F : E.value) {
				if (F.value.is_valid()) {
					Ref<Font> font = F.value;
					font->disconnect_changed(callable_mp(this, &Theme::_emit_theme_changed));
				}
			}
		}
	}

	{
		for (const KeyValue<StringName, ThemeSoundMap> &E : _sound_map) {
			for (const KeyValue<StringName, Ref<AudioStream>> &F : E.value) {
				if (F.value.is_valid()) {
					Ref<AudioStream> sound = F.value;
					sound->disconnect_changed(callable_mp(this, &Theme::_emit_theme_changed));
				}
			}
		}
	}

	_icon_map.clear();
	_style_map.clear();
	_font_map.clear();
	_font_size_map.clear();
	_color_map.clear();
	_constant_map.clear();
	_sound_map.clear();

	_variation_map.clear();
	_variation_base_map.clear();

	_emit_theme_changed(true);
}

void Theme::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_theme_item_definition", "data_type", "name", "theme_type"), &Theme::get_theme_item_definition);
	ClassDB::bind_method(D_METHOD("set_theme_expression", "property", "expression"), &Theme::set_theme_expression);
	ClassDB::bind_method(D_METHOD("get_theme_expression", "property"), &Theme::get_theme_expression);
	ClassDB::bind_method(D_METHOD("_set_style_parameter_value", "parameter", "value"), &Theme::_set_style_parameter_value);
	ClassDB::bind_method(D_METHOD("_get_style_parameter_value", "parameter"), &Theme::_get_style_parameter_value);
	ClassDB::bind_method(D_METHOD("get_stylebox_definition", "name", "theme_type"), &Theme::get_stylebox_definition);
	ClassDB::bind_method(D_METHOD("set_style_parameter", "name", "value"), &Theme::set_style_parameter);
	ClassDB::bind_method(D_METHOD("get_style_parameter", "name"), &Theme::get_style_parameter);
	ClassDB::bind_method(D_METHOD("has_style_parameter", "name"), &Theme::has_style_parameter);
	ClassDB::bind_method(D_METHOD("clear_style_parameter", "name"), &Theme::clear_style_parameter);
	ClassDB::bind_method(D_METHOD("set_icon", "name", "theme_type", "texture"), &Theme::set_icon);
	ClassDB::bind_method(D_METHOD("get_icon", "name", "theme_type"), &Theme::get_icon);
	ClassDB::bind_method(D_METHOD("has_icon", "name", "theme_type"), &Theme::has_icon);
	ClassDB::bind_method(D_METHOD("rename_icon", "old_name", "name", "theme_type"), &Theme::rename_icon);
	ClassDB::bind_method(D_METHOD("clear_icon", "name", "theme_type"), &Theme::clear_icon);
	ClassDB::bind_method(D_METHOD("get_icon_list", "theme_type"), &Theme::_get_icon_list);
	ClassDB::bind_method(D_METHOD("get_icon_type_list"), &Theme::_get_icon_type_list);

	ClassDB::bind_method(D_METHOD("set_stylebox", "name", "theme_type", "stylebox"), &Theme::set_stylebox);
	ClassDB::bind_method(D_METHOD("get_stylebox", "name", "theme_type"), &Theme::get_stylebox);
	ClassDB::bind_method(D_METHOD("has_stylebox", "name", "theme_type"), &Theme::has_stylebox);
	ClassDB::bind_method(D_METHOD("rename_stylebox", "old_name", "name", "theme_type"), &Theme::rename_stylebox);
	ClassDB::bind_method(D_METHOD("clear_stylebox", "name", "theme_type"), &Theme::clear_stylebox);
	ClassDB::bind_method(D_METHOD("get_stylebox_list", "theme_type"), &Theme::_get_stylebox_list);
	ClassDB::bind_method(D_METHOD("get_stylebox_type_list"), &Theme::_get_stylebox_type_list);

	ClassDB::bind_method(D_METHOD("set_font", "name", "theme_type", "font"), &Theme::set_font);
	ClassDB::bind_method(D_METHOD("get_font", "name", "theme_type"), &Theme::get_font);
	ClassDB::bind_method(D_METHOD("has_font", "name", "theme_type"), &Theme::has_font);
	ClassDB::bind_method(D_METHOD("rename_font", "old_name", "name", "theme_type"), &Theme::rename_font);
	ClassDB::bind_method(D_METHOD("clear_font", "name", "theme_type"), &Theme::clear_font);
	ClassDB::bind_method(D_METHOD("get_font_list", "theme_type"), &Theme::_get_font_list);
	ClassDB::bind_method(D_METHOD("get_font_type_list"), &Theme::_get_font_type_list);

	ClassDB::bind_method(D_METHOD("set_font_size", "name", "theme_type", "font_size"), &Theme::set_font_size);
	ClassDB::bind_method(D_METHOD("get_font_size", "name", "theme_type"), &Theme::get_font_size);
	ClassDB::bind_method(D_METHOD("has_font_size", "name", "theme_type"), &Theme::has_font_size);
	ClassDB::bind_method(D_METHOD("rename_font_size", "old_name", "name", "theme_type"), &Theme::rename_font_size);
	ClassDB::bind_method(D_METHOD("clear_font_size", "name", "theme_type"), &Theme::clear_font_size);
	ClassDB::bind_method(D_METHOD("get_font_size_list", "theme_type"), &Theme::_get_font_size_list);
	ClassDB::bind_method(D_METHOD("get_font_size_type_list"), &Theme::_get_font_size_type_list);

	ClassDB::bind_method(D_METHOD("set_color", "name", "theme_type", "color"), &Theme::set_color);
	ClassDB::bind_method(D_METHOD("get_color", "name", "theme_type"), &Theme::get_color);
	ClassDB::bind_method(D_METHOD("has_color", "name", "theme_type"), &Theme::has_color);
	ClassDB::bind_method(D_METHOD("rename_color", "old_name", "name", "theme_type"), &Theme::rename_color);
	ClassDB::bind_method(D_METHOD("clear_color", "name", "theme_type"), &Theme::clear_color);
	ClassDB::bind_method(D_METHOD("get_color_list", "theme_type"), &Theme::_get_color_list);
	ClassDB::bind_method(D_METHOD("get_color_type_list"), &Theme::_get_color_type_list);

	ClassDB::bind_method(D_METHOD("set_constant", "name", "theme_type", "constant"), &Theme::set_constant);
	ClassDB::bind_method(D_METHOD("get_constant", "name", "theme_type"), &Theme::get_constant);
	ClassDB::bind_method(D_METHOD("has_constant", "name", "theme_type"), &Theme::has_constant);
	ClassDB::bind_method(D_METHOD("rename_constant", "old_name", "name", "theme_type"), &Theme::rename_constant);
	ClassDB::bind_method(D_METHOD("clear_constant", "name", "theme_type"), &Theme::clear_constant);
	ClassDB::bind_method(D_METHOD("get_constant_list", "theme_type"), &Theme::_get_constant_list);
	ClassDB::bind_method(D_METHOD("get_constant_type_list"), &Theme::_get_constant_type_list);

	ClassDB::bind_method(D_METHOD("set_sound", "name", "theme_type", "sound"), &Theme::set_sound);
	ClassDB::bind_method(D_METHOD("get_sound", "name", "theme_type"), &Theme::get_sound);
	ClassDB::bind_method(D_METHOD("has_sound", "name", "theme_type"), &Theme::has_sound);
	ClassDB::bind_method(D_METHOD("rename_sound", "old_name", "name", "theme_type"), &Theme::rename_sound);
	ClassDB::bind_method(D_METHOD("clear_sound", "name", "theme_type"), &Theme::clear_sound);
	ClassDB::bind_method(D_METHOD("get_sound_list", "theme_type"), &Theme::_get_sound_list);
	ClassDB::bind_method(D_METHOD("get_sound_type_list"), &Theme::_get_sound_type_list);

	ClassDB::bind_method(D_METHOD("set_default_base_scale", "base_scale"), &Theme::set_default_base_scale);
	ClassDB::bind_method(D_METHOD("get_default_base_scale"), &Theme::get_default_base_scale);
	ClassDB::bind_method(D_METHOD("has_default_base_scale"), &Theme::has_default_base_scale);

	ClassDB::bind_method(D_METHOD("set_default_font_family", "default_font_family"), &Theme::set_default_font_family);
	ClassDB::bind_method(D_METHOD("get_default_font_family"), &Theme::get_default_font_family);
	ClassDB::bind_method(D_METHOD("has_default_font_family"), &Theme::has_default_font_family);

	ClassDB::bind_method(D_METHOD("set_default_font_size", "font_size"), &Theme::set_default_font_size);
	ClassDB::bind_method(D_METHOD("get_default_font_size"), &Theme::get_default_font_size);
	ClassDB::bind_method(D_METHOD("has_default_font_size"), &Theme::has_default_font_size);

	ClassDB::bind_method(D_METHOD("set_theme_item", "data_type", "name", "theme_type", "value"), &Theme::set_theme_item);
	ClassDB::bind_method(D_METHOD("get_theme_item", "data_type", "name", "theme_type"), &Theme::get_theme_item);
	ClassDB::bind_method(D_METHOD("has_theme_item", "data_type", "name", "theme_type"), &Theme::has_theme_item);
	ClassDB::bind_method(D_METHOD("rename_theme_item", "data_type", "old_name", "name", "theme_type"), &Theme::rename_theme_item);
	ClassDB::bind_method(D_METHOD("clear_theme_item", "data_type", "name", "theme_type"), &Theme::clear_theme_item);
	ClassDB::bind_method(D_METHOD("get_theme_item_list", "data_type", "theme_type"), &Theme::_get_theme_item_list);
	ClassDB::bind_method(D_METHOD("get_theme_item_type_list", "data_type"), &Theme::_get_theme_item_type_list);

	ClassDB::bind_method(D_METHOD("set_type_variation", "theme_type", "base_type"), &Theme::set_type_variation);
	ClassDB::bind_method(D_METHOD("is_type_variation", "theme_type", "base_type"), &Theme::is_type_variation);
	ClassDB::bind_method(D_METHOD("clear_type_variation", "theme_type"), &Theme::clear_type_variation);
	ClassDB::bind_method(D_METHOD("get_type_variation_base", "theme_type"), &Theme::get_type_variation_base);
	ClassDB::bind_method(D_METHOD("get_type_variation_list", "base_type"), &Theme::_get_type_variation_list);

	ClassDB::bind_method(D_METHOD("add_type", "theme_type"), &Theme::add_type);
	ClassDB::bind_method(D_METHOD("remove_type", "theme_type"), &Theme::remove_type);
	ClassDB::bind_method(D_METHOD("rename_type", "old_theme_type", "theme_type"), &Theme::rename_type);
	ClassDB::bind_method(D_METHOD("get_type_list"), &Theme::_get_type_list);

	ClassDB::bind_method(D_METHOD("merge_with", "other"), &Theme::merge_with);
	ClassDB::bind_method(D_METHOD("clear"), &Theme::clear);

	ADD_GROUP("Default", "default_");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "default_base_scale", PROPERTY_HINT_RANGE, "0.0,2.0,0.01,or_greater"), "set_default_base_scale", "get_default_base_scale");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "default_font_family", PROPERTY_HINT_RESOURCE_TYPE, Font::get_class_static()), "set_default_font_family", "get_default_font_family");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "default_font_size", PROPERTY_HINT_RANGE, "0,256,1,or_greater,suffix:px"), "set_default_font_size", "get_default_font_size");
	for (int i = 0; i < MAX_STYLE_PARAMETERS; i++) {
		const StringName name = _get_style_parameter_name(i);
		const Variant::Type type = _get_style_parameter_type(name);
		String range = type == Variant::FLOAT ? "0,2048,0.1,or_greater,suffix:px" : "";
		if (name == "default_contrast") {
			range = "-1,1,0.01";
		} else if (name == "default_icon_saturation") {
			range = "0,2,0.01";
		}
		ClassDB::add_property(get_class_static(), PropertyInfo(Variant::NIL, name, type == Variant::FLOAT ? PROPERTY_HINT_RANGE : PROPERTY_HINT_NONE, range, PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_NIL_IS_VARIANT), "_set_style_parameter_value", "_get_style_parameter_value", i);
		ClassDB::set_property_default_value(get_class_static(), name, Variant());
	}
	ADD_GROUP("", "");

	BIND_ENUM_CONSTANT(DATA_TYPE_COLOR);
	BIND_ENUM_CONSTANT(DATA_TYPE_CONSTANT);
	BIND_ENUM_CONSTANT(DATA_TYPE_FONT);
	BIND_ENUM_CONSTANT(DATA_TYPE_FONT_SIZE);
	BIND_ENUM_CONSTANT(DATA_TYPE_ICON);
	BIND_ENUM_CONSTANT(DATA_TYPE_STYLEBOX);
	BIND_ENUM_CONSTANT(DATA_TYPE_SOUND);
	BIND_ENUM_CONSTANT(DATA_TYPE_MAX);
}

bool Theme::_set(const StringName &p_name, const Variant &p_value) {
	String sname = p_name;
	if (p_value.get_type() == Variant::STRING && sname.contains_char('/') && sname.get_slicec('/', 1) != "base_type") {
		set_theme_expression(p_name, p_value);
		return true;
	}

	if (sname.contains_char('/')) {
		String type = sname.get_slicec('/', 1);
		String theme_type = sname.get_slicec('/', 0);
		String prop_name = sname.get_slicec('/', 2);

		if (type == "icons") {
			set_icon(prop_name, theme_type, p_value);
		} else if (type == "styles") {
			set_stylebox(prop_name, theme_type, p_value);
		} else if (type == "fonts") {
			set_font(prop_name, theme_type, p_value);
		} else if (type == "font_sizes") {
			set_font_size(prop_name, theme_type, p_value);
		} else if (type == "colors") {
			set_color(prop_name, theme_type, p_value);
		} else if (type == "constants") {
			set_constant(prop_name, theme_type, p_value);
		} else if (type == "sounds") {
			set_sound(prop_name, theme_type, p_value);
		} else if (type == "base_type") {
			set_type_variation(theme_type, p_value);
		} else {
			return false;
		}

		return true;
	}

	return false;
}

bool Theme::_get(const StringName &p_name, Variant &r_ret) const {
	const String property_name = p_name;
	if (!property_name.contains_char('/')) {
		return false;
	}
	const String category = property_name.get_slicec('/', 1);
	if (category == "base_type") {
		r_ret = get_type_variation_base(property_name.get_slicec('/', 0));
		return true;
	}
	for (int i = 0; i < DATA_TYPE_MAX; i++) {
		if (category == ITEM_CATEGORIES[i]) {
			r_ret = get_theme_item_definition(static_cast<DataType>(i), property_name.get_slicec('/', 2), property_name.get_slicec('/', 0));
			return true;
		}
	}
	return false;
}

void Theme::_get_property_list(List<PropertyInfo> *p_list) const {
	List<PropertyInfo> list;

	// Type variations.
	for (const KeyValue<StringName, StringName> &E : _variation_map) {
		list.push_back(PropertyInfo(Variant::STRING_NAME, String() + E.key + "/base_type"));
	}

	// Icons.
	for (const KeyValue<StringName, ThemeIconMap> &E : _icon_map) {
		for (const KeyValue<StringName, Ref<Texture2D>> &F : E.value) {
			list.push_back(PropertyInfo(Variant::OBJECT, String() + E.key + "/icons/" + F.key, PROPERTY_HINT_RESOURCE_TYPE, Texture2D::get_class_static(), PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_STORE_IF_NULL));
		}
	}

	// Styles.
	for (const KeyValue<StringName, ThemeStyleMap> &E : _style_map) {
		for (const KeyValue<StringName, Ref<StyleBox>> &F : E.value) {
			list.push_back(PropertyInfo(Variant::OBJECT, String() + E.key + "/styles/" + F.key, PROPERTY_HINT_RESOURCE_TYPE, StyleBox::get_class_static(), PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_STORE_IF_NULL));
		}
	}

	// Fonts.
	for (const KeyValue<StringName, ThemeFontMap> &E : _font_map) {
		for (const KeyValue<StringName, Ref<Font>> &F : E.value) {
			list.push_back(PropertyInfo(Variant::OBJECT, String() + E.key + "/fonts/" + F.key, PROPERTY_HINT_RESOURCE_TYPE, Font::get_class_static(), PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_STORE_IF_NULL));
		}
	}

	// Font sizes.
	for (const KeyValue<StringName, ThemeFontSizeMap> &E : _font_size_map) {
		for (const KeyValue<StringName, int> &F : E.value) {
			list.push_back(PropertyInfo(Variant::INT, String() + E.key + "/font_sizes/" + F.key, PROPERTY_HINT_RANGE, "0,256,1,or_greater,suffix:px"));
		}
	}

	// Colors.
	for (const KeyValue<StringName, ThemeColorMap> &E : _color_map) {
		for (const KeyValue<StringName, Color> &F : E.value) {
			list.push_back(PropertyInfo(Variant::COLOR, String() + E.key + "/colors/" + F.key));
		}
	}

	// Constants.
	for (const KeyValue<StringName, ThemeConstantMap> &E : _constant_map) {
		for (const KeyValue<StringName, int> &F : E.value) {
			list.push_back(PropertyInfo(Variant::INT, String() + E.key + "/constants/" + F.key));
		}
	}

	// Sounds.
	for (const KeyValue<StringName, ThemeSoundMap> &E : _sound_map) {
		for (const KeyValue<StringName, Ref<AudioStream>> &F : E.value) {
			list.push_back(PropertyInfo(Variant::OBJECT, String() + E.key + "/sounds/" + F.key, PROPERTY_HINT_RESOURCE_TYPE, AudioStream::get_class_static(), PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_STORE_IF_NULL));
		}
	}

	// Sort and store properties.
	list.sort();
	String previous_type;
	for (PropertyInfo &property : list) {
		const String current_type = property.name.get_slicec('/', 0);
		if (previous_type != current_type) {
			p_list->push_back(PropertyInfo(Variant::NIL, current_type, PROPERTY_HINT_NONE, current_type + "/", PROPERTY_USAGE_GROUP));
			previous_type = current_type;
		}
		_validate_property(property);
		p_list->push_back(property);
	}
}

void Theme::_validate_property(PropertyInfo &r_property) const {
	if (!get_theme_expression(r_property.name).is_empty()) {
		r_property.type = Variant::STRING;
		r_property.hint = PROPERTY_HINT_EXPRESSION;
		r_property.hint_string = "";
		return;
	}
	const Variant::Type type = _get_style_parameter_type(r_property.name);
	if (type != Variant::NIL) {
		r_property.type = type;
		if (!has_style_parameter(r_property.name)) {
			r_property.usage &= ~PROPERTY_USAGE_STORAGE;
		}
	}
}

void Theme::_freeze_change_propagation() {
	_no_change_propagation = true;
}

void Theme::_unfreeze_and_propagate_changes() {
	_no_change_propagation = false;
	_emit_theme_changed(true);
}

Ref<StyleBox> Theme::_resolve_stylebox(const Ref<StyleBox> &p_style) const {
	const Ref<StyleBox> *cached = _resolved_styleboxes.getptr(p_style);
	if (cached) {
		return *cached;
	}
	const Ref<StyleBox> resolved = p_style->resolve_theme(*this);
	ERR_FAIL_COND_V_MSG(resolved.is_null(), ThemeDB::get_singleton()->get_fallback_stylebox(), vformat("Could not resolve stylebox '%s' for theme '%s'.", p_style->get_path(), get_path()));
	_resolved_styleboxes.insert(p_style, resolved);
	return resolved;
}

void Theme::reset_state() {
	clear();
}

void Theme::_emit_theme_changed(bool p_notify_list_changed) {
	++_stylebox_revision;
	_expression_values.clear();
	_resolved_styleboxes.clear();
	if (_no_change_propagation) {
		return;
	}

	if (p_notify_list_changed) {
		notify_property_list_changed();
	}
	emit_changed();
}

Vector<String> Theme::_get_icon_list(const String &p_theme_type) const {
	Vector<String> ilret;
	List<StringName> il;

	get_icon_list(p_theme_type, &il);
	ilret.resize(il.size());

	int i = 0;
	String *w = ilret.ptrw();
	for (List<StringName>::Element *E = il.front(); E; E = E->next(), i++) {
		w[i] = E->get();
	}
	return ilret;
}

Vector<String> Theme::_get_icon_type_list() const {
	Vector<String> ilret;
	List<StringName> il;

	get_icon_type_list(&il);
	ilret.resize(il.size());

	int i = 0;
	String *w = ilret.ptrw();
	for (List<StringName>::Element *E = il.front(); E; E = E->next(), i++) {
		w[i] = E->get();
	}
	return ilret;
}

Vector<String> Theme::_get_stylebox_list(const String &p_theme_type) const {
	Vector<String> ilret;
	List<StringName> il;

	get_stylebox_list(p_theme_type, &il);
	ilret.resize(il.size());

	int i = 0;
	String *w = ilret.ptrw();
	for (List<StringName>::Element *E = il.front(); E; E = E->next(), i++) {
		w[i] = E->get();
	}
	return ilret;
}

Vector<String> Theme::_get_stylebox_type_list() const {
	Vector<String> ilret;
	List<StringName> il;

	get_stylebox_type_list(&il);
	ilret.resize(il.size());

	int i = 0;
	String *w = ilret.ptrw();
	for (List<StringName>::Element *E = il.front(); E; E = E->next(), i++) {
		w[i] = E->get();
	}
	return ilret;
}

Vector<String> Theme::_get_font_list(const String &p_theme_type) const {
	Vector<String> ilret;
	List<StringName> il;

	get_font_list(p_theme_type, &il);
	ilret.resize(il.size());

	int i = 0;
	String *w = ilret.ptrw();
	for (List<StringName>::Element *E = il.front(); E; E = E->next(), i++) {
		w[i] = E->get();
	}
	return ilret;
}

Vector<String> Theme::_get_font_type_list() const {
	Vector<String> ilret;
	List<StringName> il;

	get_font_type_list(&il);
	ilret.resize(il.size());

	int i = 0;
	String *w = ilret.ptrw();
	for (List<StringName>::Element *E = il.front(); E; E = E->next(), i++) {
		w[i] = E->get();
	}
	return ilret;
}

Vector<String> Theme::_get_font_size_list(const String &p_theme_type) const {
	Vector<String> ilret;
	List<StringName> il;

	get_font_size_list(p_theme_type, &il);
	ilret.resize(il.size());

	int i = 0;
	String *w = ilret.ptrw();
	for (List<StringName>::Element *E = il.front(); E; E = E->next(), i++) {
		w[i] = E->get();
	}
	return ilret;
}

Vector<String> Theme::_get_font_size_type_list() const {
	Vector<String> ilret;
	List<StringName> il;

	get_font_size_type_list(&il);
	ilret.resize(il.size());

	int i = 0;
	String *w = ilret.ptrw();
	for (List<StringName>::Element *E = il.front(); E; E = E->next(), i++) {
		w[i] = E->get();
	}
	return ilret;
}

Vector<String> Theme::_get_color_list(const String &p_theme_type) const {
	Vector<String> ilret;
	List<StringName> il;

	get_color_list(p_theme_type, &il);
	ilret.resize(il.size());

	int i = 0;
	String *w = ilret.ptrw();
	for (List<StringName>::Element *E = il.front(); E; E = E->next(), i++) {
		w[i] = E->get();
	}
	return ilret;
}

Vector<String> Theme::_get_color_type_list() const {
	Vector<String> ilret;
	List<StringName> il;

	get_color_type_list(&il);
	ilret.resize(il.size());

	int i = 0;
	String *w = ilret.ptrw();
	for (List<StringName>::Element *E = il.front(); E; E = E->next(), i++) {
		w[i] = E->get();
	}
	return ilret;
}

Vector<String> Theme::_get_constant_list(const String &p_theme_type) const {
	Vector<String> ilret;
	List<StringName> il;

	get_constant_list(p_theme_type, &il);
	ilret.resize(il.size());

	int i = 0;
	String *w = ilret.ptrw();
	for (List<StringName>::Element *E = il.front(); E; E = E->next(), i++) {
		w[i] = E->get();
	}
	return ilret;
}

Vector<String> Theme::_get_constant_type_list() const {
	Vector<String> ilret;
	List<StringName> il;

	get_constant_type_list(&il);
	ilret.resize(il.size());

	int i = 0;
	String *w = ilret.ptrw();
	for (List<StringName>::Element *E = il.front(); E; E = E->next(), i++) {
		w[i] = E->get();
	}
	return ilret;
}

Vector<String> Theme::_get_sound_list(const String &p_theme_type) const {
	Vector<String> ilret;
	List<StringName> il;

	get_sound_list(p_theme_type, &il);
	ilret.resize(il.size());

	int i = 0;
	String *w = ilret.ptrw();
	for (List<StringName>::Element *E = il.front(); E; E = E->next(), i++) {
		w[i] = E->get();
	}
	return ilret;
}

Vector<String> Theme::_get_sound_type_list() const {
	Vector<String> ilret;
	List<StringName> il;

	get_sound_type_list(&il);
	ilret.resize(il.size());

	int i = 0;
	String *w = ilret.ptrw();
	for (List<StringName>::Element *E = il.front(); E; E = E->next(), i++) {
		w[i] = E->get();
	}
	return ilret;
}

Vector<String> Theme::_get_theme_item_list(DataType p_data_type, const String &p_theme_type) const {
	switch (p_data_type) {
		case DATA_TYPE_COLOR:
			return _get_color_list(p_theme_type);
		case DATA_TYPE_CONSTANT:
			return _get_constant_list(p_theme_type);
		case DATA_TYPE_FONT:
			return _get_font_list(p_theme_type);
		case DATA_TYPE_FONT_SIZE:
			return _get_font_size_list(p_theme_type);
		case DATA_TYPE_ICON:
			return _get_icon_list(p_theme_type);
		case DATA_TYPE_STYLEBOX:
			return _get_stylebox_list(p_theme_type);
		case DATA_TYPE_SOUND:
			return _get_sound_list(p_theme_type);
		case DATA_TYPE_MAX:
			break; // Can't happen, but silences warning.
	}

	return Vector<String>();
}

Vector<String> Theme::_get_theme_item_type_list(DataType p_data_type) const {
	switch (p_data_type) {
		case DATA_TYPE_COLOR:
			return _get_color_type_list();
		case DATA_TYPE_CONSTANT:
			return _get_constant_type_list();
		case DATA_TYPE_FONT:
			return _get_font_type_list();
		case DATA_TYPE_FONT_SIZE:
			return _get_font_size_type_list();
		case DATA_TYPE_ICON:
			return _get_icon_type_list();
		case DATA_TYPE_STYLEBOX:
			return _get_stylebox_type_list();
		case DATA_TYPE_SOUND:
			return _get_sound_type_list();
		case DATA_TYPE_MAX:
			break; // Can't happen, but silences warning.
	}

	return Vector<String>();
}

Vector<String> Theme::_get_type_variation_list(const StringName &p_theme_type) const {
	Vector<String> ilret;
	List<StringName> il;

	get_type_variation_list(p_theme_type, &il);
	ilret.resize(il.size());

	int i = 0;
	String *w = ilret.ptrw();
	for (List<StringName>::Element *E = il.front(); E; E = E->next(), i++) {
		w[i] = E->get();
	}
	return ilret;
}

Vector<String> Theme::_get_type_list() const {
	Vector<String> ilret;
	List<StringName> il;

	get_type_list(&il);
	ilret.resize(il.size());

	int i = 0;
	String *w = ilret.ptrw();
	for (List<StringName>::Element *E = il.front(); E; E = E->next(), i++) {
		w[i] = E->get();
	}
	return ilret;
}

Variant::Type Theme::_get_style_parameter_type(const StringName &p_name) {
	if (p_name == "default_padding" || p_name == "default_border_width" || p_name == "default_border_radius" || p_name == "default_contrast" || p_name == "default_icon_saturation") {
		return Variant::FLOAT;
	}
	if (p_name == "default_accent_color" || p_name == "default_base_color") {
		return Variant::COLOR;
	}
	return Variant::NIL;
}

StringName Theme::_get_style_parameter_name(int p_parameter) {
	static const char *const NAMES[] = { "default_padding", "default_border_width", "default_border_radius", "default_accent_color", "default_base_color", "default_contrast", "default_icon_saturation" };
	ERR_FAIL_INDEX_V(p_parameter, MAX_STYLE_PARAMETERS, StringName());
	return NAMES[p_parameter];
}

void Theme::_set_style_parameter_value(int p_parameter, const Variant &p_value) {
	ERR_FAIL_INDEX(p_parameter, MAX_STYLE_PARAMETERS);
	set_style_parameter(_get_style_parameter_name(p_parameter), p_value);
}

Variant Theme::_get_style_parameter_value(int p_parameter) const {
	ERR_FAIL_INDEX_V(p_parameter, MAX_STYLE_PARAMETERS, Variant());
	const Variant *value = _style_parameters.getptr(_get_style_parameter_name(p_parameter));
	return value ? *value : Variant();
}

Error Theme::_get_parsed_theme_expression(const String &p_source, ParsedExpression &r_parsed) {
	const String source = p_source.strip_edges();
	ThemeDB *theme_db = ThemeDB::get_singleton();
	if (!theme_db) {
		return _parse_theme_expression(source, r_parsed);
	}

	MutexLock lock(theme_db->_parsed_expression_mutex);
	if (const ParsedExpression *cached = theme_db->_parsed_theme_expressions.getptr(source)) {
		r_parsed = *cached;
		return OK;
	}

	const Error error = _parse_theme_expression(source, r_parsed);
	if (error == OK) {
		if (theme_db->_parsed_theme_expressions.size() >= ThemeDB::MAX_PARSED_THEME_EXPRESSIONS) {
			theme_db->_parsed_theme_expressions.remove(theme_db->_parsed_theme_expressions.begin());
		}
		theme_db->_parsed_theme_expressions.insert(source, r_parsed);
	}
	return error;
}

Error Theme::_parse_theme_expression(const String &p_source, ParsedExpression &r_parsed) {
	String source = p_source;
	r_parsed.pixels = source.begins_with("pixels(") && source.ends_with(")");
	if (r_parsed.pixels) {
		source = source.substr(7, source.length() - 8);
	}
	String rewritten;
	Vector<String> inputs;
	for (int position = 0; position < source.length();) {
		const char32_t character = source[position];
		if (character == '"' || character == '\'') {
			const char32_t quote = character;
			rewritten += source[position++];
			while (position < source.length()) {
				const char32_t next = source[position++];
				rewritten += next;
				if (next == '\\' && position < source.length()) {
					rewritten += source[position++];
				} else if (next == quote) {
					break;
				}
			}
			continue;
		}
		if (source.substr(position, 6) != "theme." || (position > 0 && is_ascii_identifier_char(source[position - 1]))) {
			rewritten += source[position++];
			continue;
		}
		position += 6;
		auto identifier = [&]() -> String {
			const int start = position;
			if (position >= source.length() || (!is_ascii_alphabet_char(source[position]) && source[position] != '_')) {
				return String();
			}
			while (position < source.length() && is_ascii_identifier_char(source[position])) {
				position++;
			}
			return source.substr(start, position - start);
		};
		String reference = identifier();
		ERR_FAIL_COND_V(reference.is_empty(), ERR_PARSE_ERROR);
		const int after_root = position;
		if (position < source.length() && source[position] == '.') {
			position++;
			const String category = identifier();
			if (category == "colors" || category == "constants" || category == "fonts" || category == "font_sizes" || category == "icons" || category == "styles" || category == "sounds") {
				ERR_FAIL_COND_V(position >= source.length() || source[position++] != '.', ERR_PARSE_ERROR);
				const String name = identifier();
				ERR_FAIL_COND_V(name.is_empty(), ERR_PARSE_ERROR);
				reference += "/" + category + "/" + name;
			} else {
				position = after_root;
			}
		}
		int index = r_parsed.references.find(reference);
		if (index < 0) {
			index = r_parsed.references.size();
			r_parsed.references.push_back(reference);
			inputs.push_back("_theme_value_" + itos(index));
		}
		rewritten += inputs[index];
	}
	r_parsed.expression.instantiate();
	return r_parsed.expression->parse(rewritten, inputs);
}

Variant Theme::_evaluate_theme_expression(const String &p_expression, bool *r_pixels, Error *r_error, ThemeResolver *r_context) const {
	ParsedExpression parsed;
	const Error error = _get_parsed_theme_expression(p_expression, parsed);
	if (r_pixels) {
		*r_pixels = parsed.pixels;
	}
	if (r_error) {
		*r_error = error;
	}
	ERR_FAIL_COND_V_MSG(error != OK, Variant(), vformat("Invalid theme expression '%s'.", p_expression));

	struct EvaluationInputs {
		const Theme &theme;
		const Vector<StringName> &references;
		ThemeResolver *context;
		Error error = OK;
	};
	EvaluationInputs inputs{ *this, parsed.references, r_context };
	const auto resolve_input = [&inputs](int p_index, Variant &r_value) -> bool {
		if (p_index < 0 || p_index >= inputs.references.size()) {
			return false;
		}
		r_value = inputs.theme._resolve_expression_value(inputs.references[p_index], inputs.error, inputs.context);
		return inputs.error == OK;
	};
	Variant value;
	String error_text;
	const Error execution_error = parsed.expression->execute_with_resolver(resolve_input, value, error_text, true);
	if (execution_error != OK) {
		if (r_error) {
			*r_error = inputs.error == OK ? execution_error : inputs.error;
		}
		ERR_FAIL_V_MSG(Variant(), vformat("Could not evaluate theme expression '%s': %s", p_expression, error_text));
	}
	return value;
}

Variant Theme::_resolve_expression_value(const StringName &p_property, Error &r_error, ThemeResolver *r_context) const {
	if (p_property == SNAME("default_base_scale")) {
		return r_context ? r_context->get_base_scale() : has_default_base_scale() ? get_default_base_scale() : 1.0f;
	}

	HashMap<StringName, Variant> &values = r_context ? r_context->_expression_values : _expression_values;
	HashSet<StringName> &resolving = r_context ? r_context->_resolving_expressions : _resolving_expressions;
	if (const Variant *cached = values.getptr(p_property)) {
		return *cached;
	}
	if (resolving.has(p_property)) {
		r_error = ERR_CYCLIC_LINK;
		ERR_FAIL_V_MSG(Variant(), vformat("Cyclic theme expression at '%s' in theme '%s'.", p_property, get_path()));
	}

	const bool style_parameter = _get_style_parameter_type(p_property) != Variant::NIL;
	const String property = p_property;
	const String category = property.get_slicec('/', 1);
	Theme::DataType data_type = DATA_TYPE_MAX;
	for (int i = 0; i < DATA_TYPE_MAX; i++) {
		if (category == ITEM_CATEGORIES[i]) {
			data_type = static_cast<DataType>(i);
			break;
		}
	}
	const StringName type = property.get_slicec('/', 0);
	const StringName item_name = property.get_slicec('/', 2);

	const Theme *owner = this;
	if (style_parameter) {
		owner = r_context ? r_context->_get_style_parameter_owner(p_property) : has_style_parameter(p_property) ? this : nullptr;
	} else if (data_type != DATA_TYPE_MAX) {
		if (r_context) {
			owner = nullptr;
			for (const Ref<Theme> &theme : r_context->_themes) {
				if (theme->has_theme_item(data_type, item_name, type)) {
					owner = theme.ptr();
					break;
				}
			}
		} else if (!has_theme_item(data_type, item_name, type)) {
			owner = nullptr;
		}
	} else if (r_context && (p_property == SNAME("default_font_family") || p_property == SNAME("default_font_size"))) {
		owner = nullptr;
		for (const Ref<Theme> &theme : r_context->_themes) {
			if (p_property == SNAME("default_font_family") ? theme->has_default_font_family() : theme->has_default_font_size()) {
				owner = theme.ptr();
				break;
			}
		}
	}
	if (!owner) {
		r_error = ERR_DOES_NOT_EXIST;
		ERR_FAIL_V_MSG(Variant(), vformat("Missing value for theme reference '%s'.", property));
	}

	const String expression = owner->get_theme_expression(p_property);
	if (!expression.is_empty()) {
		resolving.insert(p_property);
		const Variant value = owner->_evaluate_theme_expression(expression, nullptr, &r_error, r_context);
		resolving.erase(p_property);
		if (r_error == OK) {
			values[p_property] = value;
		}
		return value;
	}
	if (style_parameter) {
		return *owner->_style_parameters.getptr(p_property);
	}
	if (data_type == DATA_TYPE_STYLEBOX) {
		resolving.insert(p_property);
		Ref<StyleBox> style = owner->get_stylebox_definition(item_name, type);
		style = r_context ? style->resolve_theme_in_context(*r_context) : owner->_resolve_stylebox(style);
		resolving.erase(p_property);
		if (style.is_null()) {
			r_error = ERR_INVALID_DATA;
		}
		return style;
	}
	if (data_type != DATA_TYPE_MAX) {
		return owner->get_theme_item(data_type, item_name, type);
	}

	bool valid = false;
	const Variant value = owner->get(p_property, &valid);
	if (!valid || value.get_type() == Variant::NIL) {
		r_error = ERR_DOES_NOT_EXIST;
		ERR_FAIL_V_MSG(Variant(), vformat("Missing theme property '%s' in theme '%s'.", property, owner->get_path()));
	}
	return value;
}

void Theme::_move_expression_type(const StringName &p_old_type, const StringName &p_type, const StringName &p_category) {
	const String prefix = String(p_old_type) + "/" + p_category + "/";
	Vector<StringName> keys;
	for (const KeyValue<StringName, String> &entry : _item_expressions) {
		if (String(entry.key).begins_with(prefix)) {
			keys.push_back(entry.key);
		}
	}
	for (const StringName &key : keys) {
		const String expression = _item_expressions[key];
		_item_expressions.erase(key);
		if (p_type != StringName()) {
			_item_expressions[String(p_type) + "/" + p_category + "/" + String(key).get_slicec('/', 2)] = expression;
		}
	}
	_expression_values.clear();
	_resolved_styleboxes.clear();
}

ThemeResolver::ThemeResolver(const Vector<Ref<Theme>> &p_themes) :
		_themes(p_themes) {
}

float ThemeResolver::get_base_scale() const {
	for (const Ref<Theme> &theme : _themes) {
		if (theme->has_default_base_scale()) {
			return theme->get_default_base_scale();
		}
	}
	return 1.0f;
}

bool ThemeResolver::has_style_parameter(const StringName &p_name) const {
	return _get_style_parameter_owner(p_name) != nullptr;
}

Variant ThemeResolver::get_style_parameter(const StringName &p_name, Error &r_error) {
	return _themes[0]->_resolve_expression_value(p_name, r_error, this);
}

Variant ThemeResolver::evaluate_expression(const String &p_expression, bool *r_pixels, Error *r_error) {
	return _themes[0]->_evaluate_theme_expression(p_expression, r_pixels, r_error, this);
}

Ref<StyleBox> ThemeResolver::resolve_stylebox_item(const StringName &p_name, const StringName &p_type) {
	ERR_FAIL_COND_V(_themes.is_empty(), Ref<StyleBox>());
	const StringName property = String(p_type) + "/styles/" + p_name;
	Error error = OK;
	const Variant value = _themes[0]->_resolve_expression_value(property, error, this);
	const Ref<StyleBox> style = value;
	ERR_FAIL_COND_V_MSG(error != OK || style.is_null(), Ref<StyleBox>(), vformat("Could not resolve theme stylebox '%s'.", property));
	// An expression may return a StyleBox that did not come from a named item.
	return style->resolve_theme_in_context(*this);
}

const Theme *ThemeResolver::_get_style_parameter_owner(const StringName &p_name) const {
	for (const Ref<Theme> &theme : _themes) {
		if (theme->has_style_parameter(p_name)) {
			return theme.ptr();
		}
	}
	return nullptr;
}
