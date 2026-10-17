/**************************************************************************/
/*  project_autoload_manager.cpp                                          */
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

#include "project_autoload_manager.h"

#include "core/config/project_settings.h"
#include "core/core_constants.h"
#include "core/error/error_macros.h"
#include "core/io/file_access.h"
#include "core/io/resource_loader.h"
#include "core/io/resource_uid.h"
#include "core/object/callable_mp.h"
#include "core/object/class_db.h"
#include "core/object/script_language.h"
#include "core/templates/hash_map.h"
#include "editor/editor_undo_redo_manager.h"
#include "scene/main/scene_tree.h"
#include "scene/main/window.h"
#include "scene/resources/packed_scene.h"

ProjectAutoloadManager::ProjectAutoloadManager() {
	ProjectSettings::get_singleton()->add_hidden_prefix("autoload/");

	// Make first cache
	List<PropertyInfo> props;
	ProjectSettings::get_singleton()->get_property_list(&props);
	for (const PropertyInfo &pi : props) {
		if (!pi.name.begins_with("autoload/")) {
			continue;
		}

		String name = pi.name.get_slicec('/', 1);
		String scr_path = GLOBAL_GET(pi.name);

		if (name.is_empty()) {
			continue;
		}

		AutoloadInfo info;
		info.is_singleton = scr_path.begins_with("*");

		if (info.is_singleton) {
			scr_path = scr_path.substr(1);
		}

		info.name = name;
		info.path = ResourceUID::get_singleton()->path_to_uid(scr_path);
		info.order = ProjectSettings::get_singleton()->get_order(pi.name);

		if (info.is_singleton) {
			// Make sure name references work before parsing scripts
			for (int i = 0; i < ScriptServer::get_language_count(); i++) {
				ScriptServer::get_language(i)->add_named_global_constant(info.name, Variant());
			}
		}

		_autoload_cache.push_back(info);
	}
}

ProjectAutoloadManager::~ProjectAutoloadManager() {
	for (const AutoloadInfo &info : _autoload_cache) {
		if (info.node && !info.in_editor) {
			memdelete(info.node);
		}
	}
}

void ProjectAutoloadManager::init_autoloads() {
	for (AutoloadInfo &info : _autoload_cache) {
		info.node = _create_autoload(ResourceUID::ensure_path(info.path));

		if (info.node) {
			Ref<Script> scr = info.node->get_script();
			info.in_editor = scr.is_valid() && scr->is_tool();
			info.node->set_name(info.name);
		}

		if (info.is_singleton) {
			for (int i = 0; i < ScriptServer::get_language_count(); i++) {
				ScriptServer::get_language(i)->add_named_global_constant(info.name, info.node);
			}
		}

		if (!info.is_singleton && !info.in_editor && info.node != nullptr) {
			memdelete(info.node);
			info.node = nullptr;
		}
	}

	for (const AutoloadInfo &info : _autoload_cache) {
		if (info.node && info.in_editor) {
			// It's important to add the node without deferring because code in plugins or tool scripts
			// could use the autoload node when they are enabled.
			get_tree()->get_root()->add_child(info.node);
		}
	}
}

void ProjectAutoloadManager::refresh_autoloads() {
	if (_updating_autoload) {
		return;
	}

	_updating_autoload = true;

	HashMap<String, AutoloadInfo> to_remove;
	List<AutoloadInfo *> to_add;

	for (const AutoloadInfo &info : _autoload_cache) {
		to_remove.insert(info.name, info);
	}

	_autoload_cache.clear();

	List<PropertyInfo> props;
	ProjectSettings::get_singleton()->get_property_list(&props);

	for (const PropertyInfo &pi : props) {
		if (!pi.name.begins_with("autoload/")) {
			continue;
		}

		String name = pi.name.get_slicec('/', 1);
		String scr_path = GLOBAL_GET(pi.name);

		if (name.is_empty()) {
			continue;
		}

		AutoloadInfo info;
		info.is_singleton = scr_path.begins_with("*");

		if (info.is_singleton) {
			scr_path = scr_path.substr(1);
		}

		info.name = name;
		info.path = ResourceUID::get_singleton()->path_to_uid(scr_path);
		info.order = ProjectSettings::get_singleton()->get_order(pi.name);

		bool need_to_add = true;
		if (to_remove.has(name)) {
			AutoloadInfo &old_info = to_remove[name];
			if (old_info.path == info.path) {
				// Still the same resource, check status
				info.node = old_info.node;
				if (info.node) {
					Ref<Script> scr = info.node->get_script();
					info.in_editor = scr.is_valid() && scr->is_tool();
					if (info.is_singleton == old_info.is_singleton && info.in_editor == old_info.in_editor) {
						to_remove.erase(name);
						need_to_add = false;
					} else {
						info.node = nullptr;
					}
				}
			}
		}

		_autoload_cache.push_back(info);

		if (need_to_add) {
			to_add.push_back(&(_autoload_cache.back()->get()));
		}
	}

	// Remove deleted/changed autoloads
	for (KeyValue<String, AutoloadInfo> &E : to_remove) {
		AutoloadInfo &info = E.value;
		if (info.is_singleton) {
			for (int i = 0; i < ScriptServer::get_language_count(); i++) {
				ScriptServer::get_language(i)->remove_named_global_constant(info.name);
			}
		}
		if (info.in_editor) {
			ERR_CONTINUE(!info.node);
			callable_mp((Node *)get_tree()->get_root(), &Node::remove_child).call_deferred(info.node);
		}

		if (info.node) {
			info.node->queue_free();
			info.node = nullptr;
		}
	}

	// Load new/changed autoloads
	List<Node *> nodes_to_add;
	for (AutoloadInfo *info : to_add) {
		info->node = _create_autoload(info->path);

		ERR_CONTINUE(!info->node);
		info->node->set_name(info->name);

		Ref<Script> scr = info->node->get_script();
		info->in_editor = scr.is_valid() && scr->is_tool();

		if (info->in_editor) {
			//defer so references are all valid on _ready()
			nodes_to_add.push_back(info->node);
		}

		if (info->is_singleton) {
			for (int i = 0; i < ScriptServer::get_language_count(); i++) {
				ScriptServer::get_language(i)->add_named_global_constant(info->name, info->node);
			}
		}

		if (!info->in_editor && !info->is_singleton) {
			// No reason to keep this node
			memdelete(info->node);
			info->node = nullptr;
		}
	}

	for (Node *E : nodes_to_add) {
		get_tree()->get_root()->add_child(E);
	}

	_updating_autoload = false;
	emit_signal("autoloads_changed");
}

Vector<ProjectAutoloadManager::AutoloadEntry> ProjectAutoloadManager::get_autoloads() const {
	Vector<AutoloadEntry> entries;
	for (const AutoloadInfo &info : _autoload_cache) {
		AutoloadEntry entry;
		entry.name = info.name;
		entry.path = ResourceUID::ensure_path(info.path);
		entry.is_singleton = info.is_singleton;
		entries.push_back(entry);
	}
	return entries;
}

bool ProjectAutoloadManager::is_name_valid(const String &p_name, String *r_error) const {
	if (!p_name.is_valid_unicode_identifier()) {
		if (r_error) {
			*r_error = TTR("Must be a valid Unicode identifier.");
		}

		return false;
	}

	if (ClassDB::class_exists(p_name)) {
		if (r_error) {
			*r_error = TTR("Must not collide with an existing engine class name.");
		}

		return false;
	}

	if (ScriptServer::is_global_class(p_name)) {
		if (r_error) {
			*r_error = TTR("Must not collide with an existing global script class name.");
		}

		return false;
	}

	if (Variant::get_type_by_name(p_name) < Variant::VARIANT_MAX) {
		if (r_error) {
			*r_error = TTR("Must not collide with an existing built-in type name.");
		}

		return false;
	}

	for (int i = 0; i < CoreConstants::get_global_constant_count(); i++) {
		if (CoreConstants::get_global_constant_name(i) == p_name) {
			if (r_error) {
				*r_error = TTR("Must not collide with an existing global constant name.");
			}

			return false;
		}
	}

	for (int i = 0; i < ScriptServer::get_language_count(); i++) {
		for (const String &keyword : ScriptServer::get_language(i)->get_reserved_words()) {
			if (keyword == p_name) {
				if (r_error) {
					*r_error = TTR("Keyword cannot be used as an Autoload name.");
				}

				return false;
			}
		}
	}

	return true;
}

bool ProjectAutoloadManager::add_autoload(const String &p_name, const String &p_path, bool p_use_undo, String *r_error) {
	if (!is_name_valid(p_name, r_error)) {
		return false;
	}
	if (!p_path.begins_with("res://") || !FileAccess::exists(p_path)) {
		if (r_error) {
			*r_error = vformat(TTR("Autoload path '%s' must point to an existing file inside the project."), p_path);
		}
		return false;
	}

	ProjectSettings *settings = ProjectSettings::get_singleton();
	const String setting = "autoload/" + p_name;
	const String value = "*" + ResourceUID::get_singleton()->path_to_uid(p_path);
	if (!p_use_undo) {
		settings->set(setting, value);
		_commit_changes();
		return true;
	}

	EditorUndoRedoManager *undo_redo = EditorUndoRedoManager::get_singleton();
	undo_redo->create_action(TTR("Add Autoload"));
	undo_redo->add_do_property(settings, setting, value);
	undo_redo->add_undo_property(settings, setting, settings->has_setting(setting) ? settings->get(setting) : Variant());
	undo_redo->add_do_method(this, "_commit_changes");
	undo_redo->add_undo_method(this, "_commit_changes");
	undo_redo->commit_action();
	return true;
}

void ProjectAutoloadManager::remove_autoload(const String &p_name, bool p_use_undo) {
	ProjectSettings *settings = ProjectSettings::get_singleton();
	const String setting = "autoload/" + p_name;
	ERR_FAIL_COND(!settings->has_setting(setting));
	if (!p_use_undo) {
		settings->set(setting, Variant());
		_commit_changes();
		return;
	}

	EditorUndoRedoManager *undo_redo = EditorUndoRedoManager::get_singleton();
	undo_redo->create_action(TTR("Remove Autoload"));
	undo_redo->add_do_property(settings, setting, Variant());
	undo_redo->add_undo_property(settings, setting, settings->get(setting));
	undo_redo->add_undo_method(settings, "set_order", setting, settings->get_order(setting));
	undo_redo->add_do_method(this, "_commit_changes");
	undo_redo->add_undo_method(this, "_commit_changes");
	undo_redo->commit_action();
}

bool ProjectAutoloadManager::rename_autoload(const String &p_name, const String &p_new_name, String *r_error) {
	if (p_name == p_new_name) {
		return true;
	}
	if (!is_name_valid(p_new_name, r_error)) {
		return false;
	}
	ProjectSettings *settings = ProjectSettings::get_singleton();
	const String old_setting = "autoload/" + p_name;
	const String new_setting = "autoload/" + p_new_name;
	if (!settings->has_setting(old_setting)) {
		if (r_error) {
			*r_error = vformat(TTR("Autoload '%s' does not exist."), p_name);
		}
		return false;
	}
	if (settings->has_setting(new_setting)) {
		if (r_error) {
			*r_error = vformat(TTR("Autoload '%s' already exists!"), p_new_name);
		}
		return false;
	}
	const int order = settings->get_order(old_setting);
	const Variant value = settings->get(old_setting);

	EditorUndoRedoManager *undo_redo = EditorUndoRedoManager::get_singleton();
	undo_redo->create_action(TTR("Rename Autoload"));
	undo_redo->add_do_property(settings, new_setting, value);
	undo_redo->add_do_method(settings, "set_order", new_setting, order);
	undo_redo->add_do_property(settings, old_setting, Variant());
	undo_redo->add_undo_property(settings, old_setting, value);
	undo_redo->add_undo_method(settings, "set_order", old_setting, order);
	undo_redo->add_undo_property(settings, new_setting, Variant());
	undo_redo->add_do_method(this, "_commit_changes");
	undo_redo->add_undo_method(this, "_commit_changes");
	undo_redo->commit_action();
	return true;
}

void ProjectAutoloadManager::set_autoload_singleton(const String &p_name, bool p_enabled) {
	ProjectSettings *settings = ProjectSettings::get_singleton();
	const String setting = "autoload/" + p_name;
	ERR_FAIL_COND(!settings->has_setting(setting));
	const String old_value = settings->get(setting);
	if (old_value.begins_with("*") == p_enabled) {
		return;
	}
	const String value = p_enabled ? "*" + old_value : old_value.substr(1);
	const int order = settings->get_order(setting);

	EditorUndoRedoManager *undo_redo = EditorUndoRedoManager::get_singleton();
	undo_redo->create_action(TTR("Toggle Autoload Globals"));
	undo_redo->add_do_property(settings, setting, value);
	undo_redo->add_undo_property(settings, setting, old_value);
	undo_redo->add_do_method(settings, "set_order", setting, order);
	undo_redo->add_undo_method(settings, "set_order", setting, order);
	undo_redo->add_do_method(this, "_commit_changes");
	undo_redo->add_undo_method(this, "_commit_changes");
	undo_redo->commit_action();
}

void ProjectAutoloadManager::swap_autoloads(const String &p_name, const String &p_other_name) {
	ProjectSettings *settings = ProjectSettings::get_singleton();
	const String setting = "autoload/" + p_name;
	const String other_setting = "autoload/" + p_other_name;
	ERR_FAIL_COND(!settings->has_setting(setting) || !settings->has_setting(other_setting));
	const int order = settings->get_order(setting);
	const int other_order = settings->get_order(other_setting);

	EditorUndoRedoManager *undo_redo = EditorUndoRedoManager::get_singleton();
	undo_redo->create_action(TTR("Move Autoload"));
	undo_redo->add_do_method(settings, "set_order", setting, other_order);
	undo_redo->add_undo_method(settings, "set_order", setting, order);
	undo_redo->add_do_method(settings, "set_order", other_setting, order);
	undo_redo->add_undo_method(settings, "set_order", other_setting, other_order);
	undo_redo->add_do_method(this, "_commit_changes");
	undo_redo->add_undo_method(this, "_commit_changes");
	undo_redo->commit_action();
}

void ProjectAutoloadManager::reorder_autoloads(const PackedStringArray &p_names, const String &p_before_name) {
	if (p_names.is_empty()) {
		return;
	}
	Vector<String> original;
	Vector<int> orders;
	for (const AutoloadInfo &info : _autoload_cache) {
		original.push_back(info.name);
		orders.push_back(info.order);
	}
	ERR_FAIL_COND(!p_before_name.is_empty() && !original.has(p_before_name));
	for (int i = 0; i < p_names.size(); i++) {
		ERR_FAIL_COND(!original.has(p_names[i]));
		for (int j = 0; j < i; j++) {
			ERR_FAIL_COND(p_names[i] == p_names[j]);
		}
	}

	Vector<String> reordered;
	int insertion_index = 0;
	bool before_destination = true;
	for (const String &name : original) {
		if (name == p_before_name) {
			before_destination = false;
		}
		if (!p_names.has(name)) {
			reordered.push_back(name);
			if (before_destination) {
				insertion_index++;
			}
		}
	}
	for (int i = 0; i < p_names.size(); i++) {
		reordered.insert(insertion_index + i, p_names[i]);
	}
	if (reordered == original) {
		return;
	}

	orders.sort();
	ProjectSettings *settings = ProjectSettings::get_singleton();
	EditorUndoRedoManager *undo_redo = EditorUndoRedoManager::get_singleton();
	undo_redo->create_action(TTR("Rearrange Autoloads"));
	for (int i = 0; i < reordered.size(); i++) {
		const String setting = "autoload/" + reordered[i];
		undo_redo->add_do_method(settings, "set_order", setting, orders[i]);
		undo_redo->add_undo_method(settings, "set_order", setting, settings->get_order(setting));
	}
	undo_redo->add_do_method(this, "_commit_changes");
	undo_redo->add_undo_method(this, "_commit_changes");
	undo_redo->commit_action();
}

void ProjectAutoloadManager::_bind_methods() {
	ClassDB::bind_method("_commit_changes", &ProjectAutoloadManager::_commit_changes);

	ADD_SIGNAL(MethodInfo("autoloads_changed"));
}

Node *ProjectAutoloadManager::_create_autoload(const String &p_path) {
	Node *n = nullptr;
	if (ResourceLoader::get_resource_type(p_path) == "PackedScene") {
		Ref<PackedScene> scn = ResourceLoader::load(p_path);
		ERR_FAIL_COND_V_MSG(scn.is_null(), nullptr, vformat("Failed to create an autoload, can't load from UID or path: %s.", p_path));

		n = scn->instantiate();
	} else {
		Ref<Resource> res = ResourceLoader::load(p_path);
		ERR_FAIL_COND_V_MSG(res.is_null(), nullptr, vformat("Failed to create an autoload, can't load from UID or path: %s.", p_path));

		Ref<Script> scr = res;
		if (scr.is_valid()) {
			ERR_FAIL_COND_V_MSG(!scr->is_script_valid(), nullptr, vformat("Failed to create an autoload, script '%s' is not compiling.", p_path));

			StringName ibt = scr->get_instance_base_type();
			bool valid_type = ClassDB::is_parent_class(ibt, "Node");
			ERR_FAIL_COND_V_MSG(!valid_type, nullptr, vformat("Failed to create an autoload, script '%s' does not inherit from 'Node'.", p_path));

			Object *obj = ClassDB::instantiate(ibt);
			ERR_FAIL_NULL_V_MSG(obj, nullptr, vformat("Failed to create an autoload, cannot instantiate '%s'.", ibt));

			n = Object::cast_to<Node>(obj);
			n->set_script(scr);
		}
	}

	ERR_FAIL_NULL_V_MSG(n, nullptr, vformat("Failed to create an autoload, path is not pointing to a scene or a script: %s.", p_path));

	return n;
}

void ProjectAutoloadManager::_commit_changes() {
	refresh_autoloads();
	const Error error = ProjectSettings::get_singleton()->save();
	ERR_FAIL_COND_MSG(error != OK, "Failed to save project autoloads.");
}
