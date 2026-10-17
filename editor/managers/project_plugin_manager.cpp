/**************************************************************************/
/*  project_plugin_manager.cpp                                            */
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

#include "project_plugin_manager.h"

#include "core/config/project_settings.h"
#include "core/error/error_macros.h"
#include "core/io/config_file.h"
#include "core/io/dir_access.h"
#include "core/io/file_access.h"
#include "core/io/resource_loader.h"
#include "core/object/callable_mp.h"
#include "core/object/class_db.h"
#include "core/object/script_language.h"
#include "core/string/print_string.h"
#include "editor/editor_node.h"
#include "editor/file_system/editor_file_system.h"
#include "editor/plugins/editor_plugin.h"

ProjectPluginManager::ProjectPluginManager() {
	ProjectSettings::get_singleton()->add_hidden_prefix("editor_plugins/");
}

void ProjectPluginManager::init_plugins() {
	_initializing = true;
	Vector<String> addons;
	if (ProjectSettings::get_singleton()->has_setting("editor_plugins/enabled")) {
		addons = GLOBAL_GET("editor_plugins/enabled");
	}

	for (const String &addon : addons) {
		set_plugin_enabled(addon, true);
	}
	_initializing = false;

	if (!_pending_plugins.is_empty()) {
		EditorFileSystem::get_singleton()->connect("script_classes_updated", callable_mp(this, &ProjectPluginManager::_enable_pending_plugins), CONNECT_ONE_SHOT);
	}
}

Vector<ProjectPluginManager::PluginEntry> ProjectPluginManager::get_plugins() const {
	Vector<String> paths = _find_plugin_configs("res://addons");
	paths.sort();
	Vector<PluginEntry> entries;
	for (const String &path : paths) {
		Ref<ConfigFile> config;
		config.instantiate();
		if (config->load(path) != OK) {
			WARN_PRINT("Can't load plugin config at: " + path);
			continue;
		}
		Vector<String> missing_keys;
		for (const String required_key : { "name", "author", "version", "description", "script" }) {
			if (!config->has_section_key("plugin", required_key)) {
				missing_keys.push_back("\"plugin/" + required_key + "\"");
			}
		}
		if (!missing_keys.is_empty()) {
			WARN_PRINT(vformat("Plugin config at \"%s\" is missing the following keys: %s", path, String(",").join(missing_keys)));
			continue;
		}
		PluginEntry entry;
		entry.config_path = path;
		entry.name = config->get_value("plugin", "name");
		entry.author = config->get_value("plugin", "author");
		entry.version = config->get_value("plugin", "version");
		entry.description = config->get_value("plugin", "description");
		entry.script = config->get_value("plugin", "script");
		entry.enabled = is_plugin_enabled(path);
		entries.push_back(entry);
	}
	return entries;
}

void ProjectPluginManager::set_plugin_enabled(const String &p_plugin, bool p_enabled, bool p_config_changed) {
	String plugin_path = p_plugin;

	if (!plugin_path.begins_with("res://")) {
		plugin_path = "res://addons/" + plugin_path + "/plugin.cfg";
	}

	ERR_FAIL_COND(p_enabled && _plugins.has(plugin_path));
	ERR_FAIL_COND(!p_enabled && !_plugins.has(plugin_path));

	if (!p_enabled) {
		EditorPlugin *addon = _plugins[plugin_path];
		EditorNode::remove_editor_plugin(addon, p_config_changed);
		memdelete(addon);
		_plugins.erase(plugin_path);
		_update_config();
		emit_signal("plugins_changed");
		return;
	}

	Ref<ConfigFile> cf;
	cf.instantiate();
	if (!DirAccess::exists(plugin_path.get_base_dir())) {
		_remove_from_enabled(plugin_path);
		WARN_PRINT("Addon '" + plugin_path + "' failed to load. No directory found. Removing from enabled plugins.");
		return;
	}
	Error err = cf->load(plugin_path);
	if (err != OK) {
		emit_signal("plugin_load_failed", vformat(TTR("Unable to enable addon plugin at: '%s' parsing of config failed."), plugin_path));
		return;
	}

	String plugin_version;
	if (cf->has_section_key("plugin", "version")) {
		plugin_version = cf->get_value("plugin", "version");
	}

	if (!cf->has_section_key("plugin", "script")) {
		emit_signal("plugin_load_failed", vformat(TTR("Unable to find script field for addon plugin at: '%s'."), plugin_path));
		return;
	}

	String script_path = cf->get_value("plugin", "script");
	Ref<Script> scr; // We need to save it for creating "ep" below.

	// Only try to load the script if it has a name. Else, the plugin has no init script.
	EditorPlugin *ep = nullptr;
	if (script_path.length() > 0) {
		script_path = plugin_path.get_base_dir().path_join(script_path);
		// We should not use the cached version on startup to prevent a script reload
		// if it is already loaded and potentially running from autoloads. See GH-100750.
		scr = ResourceLoader::load(script_path, "Script", EditorFileSystem::get_singleton()->doing_first_scan() ? ResourceFormatLoader::CACHE_MODE_REUSE : ResourceFormatLoader::CACHE_MODE_IGNORE);

		if (scr.is_null()) {
			emit_signal("plugin_load_failed", vformat(TTR("Unable to load addon script from path: '%s'."), script_path));
			return;
		}

		// Errors in the script cause the base_type to be an empty StringName.
		if (scr->get_instance_base_type() == StringName()) {
			if (_initializing) {
				// However, if it happens during initialization, waiting for file scan might help.
				_pending_plugins.push_back(p_plugin);
				return;
			}

			emit_signal("plugin_load_failed", vformat(TTR("Unable to load addon script from path: '%s'. This might be due to a code error in that script.\nDisabling the addon at '%s' to prevent further errors."), script_path, plugin_path));
			_remove_from_enabled(plugin_path);
			return;
		}

		// Plugin init scripts must inherit from EditorPlugin and be tools.
		if (!ClassDB::is_parent_class(scr->get_instance_base_type(), "EditorPlugin")) {
			emit_signal("plugin_load_failed", vformat(TTR("Unable to load addon script from path: '%s'. Base type is not 'EditorPlugin'."), script_path));
			return;
		}

		if (!scr->is_tool()) {
			emit_signal("plugin_load_failed", vformat(TTR("Unable to load addon script from path: '%s'. Script is not in tool mode."), script_path));
			return;
		}

		Object *obj = ClassDB::instantiate(scr->get_instance_base_type());
		ep = Object::cast_to<EditorPlugin>(obj);
		ERR_FAIL_NULL(ep);
		ep->set_script(scr);
	} else {
		ep = memnew(EditorPlugin);
	}

	ep->set_plugin_version(plugin_version);
	_plugins[plugin_path] = ep;
	EditorNode::add_editor_plugin(ep, p_config_changed);

	_update_config();
	emit_signal("plugins_changed");
}

bool ProjectPluginManager::is_plugin_enabled(const String &p_plugin) const {
	if (p_plugin.begins_with("res://")) {
		return _plugins.has(p_plugin);
	}

	return _plugins.has("res://addons/" + p_plugin + "/plugin.cfg");
}

void ProjectPluginManager::unload_plugins() {
	for (const KeyValue<String, EditorPlugin *> &E : _plugins) {
		print_verbose(vformat("Unloading addon: %s", E.key));
		EditorNode::remove_editor_plugin(E.value, false);
		memdelete(E.value);
	}

	_plugins.clear();
	_pending_plugins.clear();
	EditorFileSystem *filesystem = EditorFileSystem::get_singleton();
	const Callable pending_callback = callable_mp(this, &ProjectPluginManager::_enable_pending_plugins);
	if (filesystem && filesystem->is_connected("script_classes_updated", pending_callback)) {
		filesystem->disconnect("script_classes_updated", pending_callback);
	}
}

void ProjectPluginManager::_bind_methods() {
	ADD_SIGNAL(MethodInfo("plugins_changed"));
	ADD_SIGNAL(MethodInfo("plugin_load_failed", PropertyInfo(Variant::STRING, "message")));
}

Vector<String> ProjectPluginManager::_find_plugin_configs(const String &p_dir) {
	Ref<DirAccess> da = DirAccess::create(DirAccess::ACCESS_RESOURCES);
	ERR_FAIL_COND_V(da.is_null(), Vector<String>());
	const Error err = da->change_dir(p_dir);
	if (err != OK) {
		return Vector<String>();
	}

	Vector<String> plugins;
	const Error listing_error = da->list_dir_begin();
	ERR_FAIL_COND_V_MSG(listing_error != OK, Vector<String>(), vformat("Cannot list plugin directory: %s.", p_dir));
	for (String path = da->get_next(); !path.is_empty(); path = da->get_next()) {
		if (path[0] == '.' || !da->current_is_dir()) {
			continue;
		}

		const String full_path = p_dir.path_join(path);
		const String plugin_config = full_path.path_join("plugin.cfg");
		if (FileAccess::exists(plugin_config)) {
			plugins.push_back(plugin_config);
		} else {
			plugins.append_array(_find_plugin_configs(full_path));
		}
	}

	da->list_dir_end();
	return plugins;
}

void ProjectPluginManager::_enable_pending_plugins() {
	for (uint32_t i = 0; i < _pending_plugins.size(); i++) {
		set_plugin_enabled(_pending_plugins[i], true);
	}
	_pending_plugins.clear();
}

void ProjectPluginManager::_remove_from_enabled(const String &p_name) {
	ProjectSettings *ps = ProjectSettings::get_singleton();
	PackedStringArray enabled_plugins = ps->get("editor_plugins/enabled");
	for (int i = 0; i < enabled_plugins.size(); ++i) {
		if (enabled_plugins.get(i) == p_name) {
			enabled_plugins.remove_at(i);
			break;
		}
	}
	ps->set("editor_plugins/enabled", enabled_plugins);
	_save_config();
	emit_signal("plugins_changed");
}

void ProjectPluginManager::_update_config() {
	if (_initializing) {
		return;
	}

	Vector<String> enabled_addons;

	for (const KeyValue<String, EditorPlugin *> &E : _plugins) {
		enabled_addons.push_back(E.key);
	}

	if (enabled_addons.is_empty()) {
		ProjectSettings::get_singleton()->set("editor_plugins/enabled", Variant());
	} else {
		enabled_addons.sort();
		ProjectSettings::get_singleton()->set("editor_plugins/enabled", enabled_addons);
	}

	_save_config();
}

void ProjectPluginManager::_save_config() {
	const Error error = ProjectSettings::get_singleton()->save();
	ERR_FAIL_COND_MSG(error != OK, "Failed to save project plugin settings.");
}
