/**************************************************************************/
/*  class_db.cpp                                                          */
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

#include "class_db.h"

#include "core/config/engine.h"
#include "core/templates/sort_array.h"
#include "core/version.h"

#define ERR_FAIL_NO_CLASS(m_type, m_class) ERR_FAIL_NULL_MSG(m_type, vformat("Cannot get class \"%s\".", m_class))

#ifdef DEBUG_ENABLED

MethodDefinition D_METHODP(const char *p_name, const char *const **p_args, uint32_t p_argcount) {
	MethodDefinition md;
	md.name = StringName(p_name);
	md.args.resize(p_argcount);
	for (uint32_t i = 0; i < p_argcount; i++) {
		md.args.write[i] = StringName(*p_args[i]);
	}
	return md;
}

#endif // DEBUG_ENABLED

ClassDB::APIType ClassDB::current_api = API_CORE;
HashMap<ClassDB::APIType, uint32_t> ClassDB::api_hashes_cache;

void ClassDB::set_current_api(APIType p_api) {
	DEV_ASSERT(!api_hashes_cache.has(p_api)); // This API type may not be suitable for caching of hash if it can change later.
	current_api = p_api;
}

ClassDB::APIType ClassDB::get_current_api() {
	return current_api;
}

HashMap<StringName, ClassDB::ClassInfo> ClassDB::classes;
HashMap<StringName, StringName> ClassDB::resource_base_extensions;

bool ClassDB::_is_parent_class(const StringName &p_class, const StringName &p_inherits) {
	const ClassInfo *c = classes.getptr(p_class);
	return c ? c->gdtype->get_name_hierarchy().has(p_inherits) : false;
}

bool ClassDB::is_parent_class(const StringName &p_class, const StringName &p_inherits) {
	Locker::Lock lock(Locker::STATE_READ);

	return _is_parent_class(p_class, p_inherits);
}

// This function only sorts items added by this function.
// If `p_classes` is not empty before calling and a global sort is needed, caller must handle that separately.
void ClassDB::get_class_list(LocalVector<StringName> &p_classes) {
	Locker::Lock lock(Locker::STATE_READ);

	if (classes.is_empty()) {
		return;
	}

	p_classes.reserve(p_classes.size() + classes.size());
	for (const KeyValue<StringName, ClassInfo> &cls : classes) {
		p_classes.push_back(cls.key);
	}

	SortArray<StringName, StringName::AlphCompare> sorter;
	sorter.sort(&p_classes[p_classes.size() - classes.size()], classes.size());
}

#ifdef TOOLS_ENABLED
// This function only sorts items added by this function.
// If `p_classes` is not empty before calling and a global sort is needed, caller must handle that separately.

#endif

void ClassDB::get_inheriters_from_class(const StringName &p_class, LocalVector<StringName> &p_classes) {
	Locker::Lock lock(Locker::STATE_READ);

	for (const KeyValue<StringName, ClassInfo> &E : classes) {
		if (E.key != p_class && _is_parent_class(E.key, p_class)) {
			p_classes.push_back(E.key);
		}
	}
}

void ClassDB::get_direct_inheriters_from_class(const StringName &p_class, List<StringName> *p_classes) {
	Locker::Lock lock(Locker::STATE_READ);

	for (const KeyValue<StringName, ClassInfo> &E : classes) {
		if (E.value.gdtype->get_super_type_name() == p_class) {
			p_classes->push_back(E.key);
		}
	}
}

StringName ClassDB::get_parent_class_nocheck(const StringName &p_class) {
	Locker::Lock lock(Locker::STATE_READ);

	ClassInfo *ti = classes.getptr(p_class);
	return ti ? ti->gdtype->get_super_type_name() : StringName();
}

bool ClassDB::get_inheritance_chain_nocheck(const StringName &p_class, Vector<StringName> &r_result) {
	Locker::Lock lock(Locker::STATE_READ);

	ClassInfo *ti = classes.getptr(p_class);
	if (!ti) {
		return false;
	}

	r_result.append_array(ti->gdtype->get_name_hierarchy());

	return true;
}

StringName ClassDB::_get_parent_class(const StringName &p_class) {
	ClassInfo *ti = classes.getptr(p_class);
	ERR_FAIL_NULL_V_MSG(ti, StringName(), vformat("Cannot get class '%s'.", String(p_class)));
	return ti->gdtype->get_super_type_name();
}

StringName ClassDB::get_parent_class(const StringName &p_class) {
	Locker::Lock lock(Locker::STATE_READ);

	return _get_parent_class(p_class);
}

ClassDB::APIType ClassDB::get_api_type(const StringName &p_class) {
	Locker::Lock lock(Locker::STATE_READ);

	ClassInfo *ti = classes.getptr(p_class);

	ERR_FAIL_NULL_V_MSG(ti, API_NONE, vformat("Cannot get class '%s'.", String(p_class)));
	return ti->api;
}

uint32_t ClassDB::get_api_hash(APIType p_api) {
#ifdef DEBUG_ENABLED
	Locker::Lock lock(Locker::STATE_WRITE);

	if (api_hashes_cache.has(p_api)) {
		return api_hashes_cache[p_api];
	}

	uint64_t hash = hash_murmur3_one_64(HashMapHasherDefault::hash(GODOT_VERSION_FULL_CONFIG));

	List<StringName> class_list;
	for (const KeyValue<StringName, ClassInfo> &E : classes) {
		class_list.push_back(E.key);
	}
	// Must be alphabetically sorted for hash to compute.
	class_list.sort_custom<StringName::AlphCompare>();

	for (const StringName &E : class_list) {
		ClassInfo *t = classes.getptr(E);
		ERR_FAIL_NULL_V_MSG(t, 0, vformat("Cannot get class '%s'.", String(E)));
		if (t->api != p_api || !t->exposed) {
			continue;
		}
		hash = hash_murmur3_one_64(t->gdtype->get_name().hash(), hash);
		hash = hash_murmur3_one_64(t->gdtype->get_super_type_name().hash(), hash);

		LocalVector<StringName> methods;
		LocalVector<StringName> constants;
		LocalVector<StringName> signals;
		LocalVector<StringName> setgets;

		for (const KeyValue<StringName, GDType::Member> &kv : t->gdtype->members(true)) {
			if (kv.value.type == GDType::Member::Type::METHOD) {
				String name = kv.key.string();

				ERR_CONTINUE(name.is_empty());

				if (name[0] == '_') {
					continue; // Ignore non-virtual methods that start with an underscore
				}

				methods.push_back(kv.key);
			} else if (kv.value.type == GDType::Member::Type::INTEGER_CONSTANT) {
				constants.push_back(kv.key);
			} else if (kv.value.type == GDType::Member::Type::SIGNAL) {
				signals.push_back(kv.key);
			} else if (kv.value.type == GDType::Member::Type::PROPERTY) {
				setgets.push_back(kv.key);
			}
		}

		{ //methods
			methods.sort_custom<StringName::AlphCompare>();

			for (const StringName &F : methods) {
				const MethodBind *mb = t->gdtype->members(true)[F].payload.method;
				hash = hash_murmur3_one_64(mb->get_name().hash(), hash);
				hash = hash_murmur3_one_64(mb->get_argument_count(), hash);
				hash = hash_murmur3_one_64(mb->get_argument_type(-1), hash); //return

				for (int i = 0; i < mb->get_argument_count(); i++) {
					const PropertyInfo info = mb->get_argument_info(i);
					hash = hash_murmur3_one_64(info.type, hash);
					hash = hash_murmur3_one_64(info.name.hash(), hash);
					hash = hash_murmur3_one_64(info.hint, hash);
					hash = hash_murmur3_one_64(info.hint_string.hash(), hash);
				}

				hash = hash_murmur3_one_64(mb->get_default_argument_count(), hash);

				for (int i = 0; i < mb->get_argument_count(); i++) {
					if (mb->has_default_argument(i)) {
						Variant da = mb->get_default_argument(i);
						hash = hash_murmur3_one_64(da.hash(), hash);
					}
				}

				hash = hash_murmur3_one_64(mb->get_hint_flags(), hash);
			}
		}

		{ //constants
			constants.sort_custom<StringName::AlphCompare>();

			for (const StringName &F : constants) {
				hash = hash_murmur3_one_64(F.hash(), hash);
				hash = hash_murmur3_one_64(uint64_t(t->gdtype->members(true)[F].payload.integer_constant.value), hash);
			}
		}

		{ //signals
			signals.sort_custom<StringName::AlphCompare>();

			for (const StringName &F : signals) {
				const MethodInfo &mi = *t->gdtype->members(true)[F].payload.signal;
				hash = hash_murmur3_one_64(F.hash(), hash);
				for (const PropertyInfo &pi : mi.arguments) {
					hash = hash_murmur3_one_64(pi.type, hash);
				}
			}
		}

		{
			setgets.sort_custom<StringName::AlphCompare>();

			for (const StringName &F : setgets) {
				const GDType::Member &member = t->gdtype->members(true)[F];
				const GDType::Member::Property &psg = member.payload.property;

				hash = hash_murmur3_one_64(F.hash(), hash);
				hash = hash_murmur3_one_64((psg.setter ? psg.setter->get_name() : StringName()).hash(), hash);
				hash = hash_murmur3_one_64((psg.getter ? psg.getter->get_name() : StringName()).hash(), hash);
			}
		}

		//property list
		for (const PropertyInfo *F : t->gdtype->get_ordered_self_properties()) {
			hash = hash_murmur3_one_64(F->name.hash(), hash);
			hash = hash_murmur3_one_64(F->type, hash);
			hash = hash_murmur3_one_64(F->hint, hash);
			hash = hash_murmur3_one_64(F->hint_string.hash(), hash);
			hash = hash_murmur3_one_64(F->usage, hash);
		}
	}

	hash = hash_fmix32(hash);

	api_hashes_cache[p_api] = hash;

	return hash;
#else
	return 0;
#endif // DEBUG_ENABLED
}

bool ClassDB::class_exists(const StringName &p_class) {
	Locker::Lock lock(Locker::STATE_READ);
	return classes.has(p_class);
}

Object *ClassDB::instantiate(const StringName &p_class) {
	ClassInfo *ti;
	{
		Locker::Lock lock(Locker::STATE_READ);
		ti = classes.getptr(p_class);
		ERR_FAIL_NULL_V_MSG(ti, nullptr, vformat("Cannot get class '%s'.", String(p_class)));
		ERR_FAIL_COND_V_MSG(ti->disabled, nullptr, vformat("Class '%s' is disabled.", String(p_class)));
		ERR_FAIL_COND_V_MSG(!ti->exposed, nullptr, vformat("Class '%s' isn't exposed.", String(p_class)));
		ERR_FAIL_NULL_V_MSG(ti->creation_func, nullptr, vformat("Class '%s' or its base class cannot be instantiated.", String(p_class)));
	}

#ifdef TOOLS_ENABLED
	if (ti->api == API_EDITOR && !Engine::get_singleton()->is_editor_hint()) {
		ERR_PRINT(vformat("Class '%s' can only be instantiated by editor.", String(p_class)));
		return nullptr;
	}
#endif

	return ti->creation_func(true);
}

bool ClassDB::_can_instantiate(const ClassInfo *p_class_info) {
	if (!p_class_info) {
		return false;
	}

	if (!p_class_info->exposed) {
		return false;
	}

	if (p_class_info->disabled || !p_class_info->creation_func) {
		return false;
	}

	return true;
}

const GDType *ClassDB::get_gdtype(const StringName &p_class) {
	Locker::Lock lock(Locker::STATE_READ);
	ClassInfo *type = classes.getptr(p_class);
	ERR_FAIL_NULL_V(type, nullptr);
	return type->gdtype;
}

bool ClassDB::can_instantiate(const StringName &p_class) {
	Locker::Lock lock(Locker::STATE_READ);

	ClassInfo *ti = classes.getptr(p_class);
	ERR_FAIL_NULL_V_MSG(ti, false, vformat("Cannot get class '%s'.", String(p_class)));
#ifdef TOOLS_ENABLED
	if (ti->api == API_EDITOR && !Engine::get_singleton()->is_editor_hint()) {
		return false;
	}
#endif
	return _can_instantiate(ti);
}

bool ClassDB::is_abstract(const StringName &p_class) {
	Locker::Lock lock(Locker::STATE_READ);

	ClassInfo *ti = classes.getptr(p_class);
	ERR_FAIL_NULL_V_MSG(ti, false, vformat("Cannot get class '%s'.", String(p_class)));

	if (ti->creation_func != nullptr) {
		return false;
	}
	return true;
}

bool ClassDB::is_virtual(const StringName &p_class) {
	Locker::Lock lock(Locker::STATE_READ);

	ClassInfo *ti = classes.getptr(p_class);
	ERR_FAIL_NULL_V_MSG(ti, false, vformat("Cannot get class '%s'.", String(p_class)));

#ifdef TOOLS_ENABLED
	if (ti->api == API_EDITOR && !Engine::get_singleton()->is_editor_hint()) {
		return false;
	}
#endif
	return (_can_instantiate(ti) && ti->is_virtual);
}

void ClassDB::_add_class(GDType &p_class, const GDType *p_inherits) {
	Locker::Lock lock(Locker::STATE_WRITE);

	const StringName &name = p_class.get_name();

	ERR_FAIL_COND_MSG(classes.has(name), vformat("Class '%s' already exists.", name));

	classes[name] = ClassInfo();
	ClassInfo &ti = classes[name];
	ti.gdtype = &p_class;
	ti.api = current_api;

	if (p_inherits) {
		ERR_FAIL_COND(!classes.has(ti.gdtype->get_super_type_name())); //it MUST be registered.
		ti.inherits_ptr = &classes[ti.gdtype->get_super_type_name()];
	} else {
		ti.inherits_ptr = nullptr;
	}
}

static MethodInfo info_from_bind(const MethodBind *p_method) {
	MethodInfo minfo;
	minfo.name = p_method->get_name();
	minfo.id = p_method->get_method_id();

	for (int i = 0; i < p_method->get_argument_count(); i++) {
		minfo.arguments.push_back(p_method->get_argument_info(i));
	}

	minfo.return_val = p_method->get_return_info();
	minfo.flags = p_method->get_hint_flags();

	for (int i = 0; i < p_method->get_argument_count(); i++) {
		if (p_method->has_default_argument(i)) {
			minfo.default_arguments.push_back(p_method->get_default_argument(i));
		}
	}

	return minfo;
}

void ClassDB::get_method_list(const StringName &p_class, List<MethodInfo> *p_methods, bool p_no_inheritance) {
	Locker::Lock lock(Locker::STATE_READ);

	ClassInfo *type = classes.getptr(p_class);
	ERR_FAIL_NULL(type);

	do {
		if (type->disabled) {
			type = type->inherits_ptr;
			continue;
		}

		for (const KeyValue<StringName, GDType::Member> &kv : type->gdtype->members(true)) {
			if (kv.value.type == GDType::Member::Type::METHOD) {
				p_methods->push_back(info_from_bind(kv.value.payload.method));
			}
		}
		type = type->inherits_ptr;
	} while (type && !p_no_inheritance);

#ifdef DEBUG_ENABLED
	ClassDB::get_virtual_methods(p_class, p_methods, p_no_inheritance);
#endif
}

void ClassDB::get_method_list_with_hashes(const StringName &p_class, List<Pair<MethodInfo, uint32_t>> *p_methods, bool p_no_inheritance) {
	Locker::Lock lock(Locker::STATE_READ);

	ClassInfo *type = classes.getptr(p_class);
	ERR_FAIL_NULL(type);

	while (type) {
		if (type->disabled) {
			if (p_no_inheritance) {
				break;
			}

			type = type->inherits_ptr;
			continue;
		}
		for (const KeyValue<StringName, GDType::Member> &kv : type->gdtype->members(true)) {
			if (kv.value.type == GDType::Member::Type::METHOD) {
				p_methods->push_back(Pair(info_from_bind(kv.value.payload.method), kv.value.payload.method->get_hash()));
			}
		}
#ifdef DEBUG_ENABLED
		for (const MethodInfo &E : type->virtual_methods) {
			Pair<MethodInfo, uint32_t> pair(E, E.get_compatibility_hash());
			p_methods->push_back(pair);
		}
#endif

		if (p_no_inheritance) {
			break;
		}

		type = type->inherits_ptr;
	}
}

bool ClassDB::get_method_info(const StringName &p_class, const StringName &p_method, MethodInfo *r_info, bool p_no_inheritance) {
	Locker::Lock lock(Locker::STATE_READ);

	ClassInfo *type = classes.getptr(p_class);

	while (type) {
		if (type->disabled) {
			if (p_no_inheritance) {
				break;
			}

			type = type->inherits_ptr;
			continue;
		}

#ifdef DEBUG_ENABLED
		const GDType::Member *member = type->gdtype->members(true).getptr(p_method);
		if (member && member->type == GDType::Member::Type::METHOD) {
			if (r_info != nullptr) {
				*r_info = info_from_bind(member->payload.method);
			}
			return true;
		} else if (type->virtual_methods_map.has(p_method)) {
			if (r_info) {
				*r_info = type->virtual_methods_map[p_method];
			}
			return true;
		}
#else
		const GDType::Member *member = type->gdtype->members(true).getptr(p_method);
		if (member && member->type == GDType::Member::Type::METHOD) {
			if (r_info) {
				*r_info = info_from_bind(member->payload.method);
			}
			return true;
		}
#endif // DEBUG_ENABLED

		if (p_no_inheritance) {
			break;
		}

		type = type->inherits_ptr;
	}

	return false;
}

const MethodBind *ClassDB::get_method(const StringName &p_class, const StringName &p_name) {
	Locker::Lock lock(Locker::STATE_READ);

	ClassInfo *type = classes.getptr(p_class);
	if (!type) {
		return nullptr;
	}

	const GDType::Member *member = type->gdtype->members().getptr(p_name);
	return member && member->type == GDType::Member::Type::METHOD ? member->payload.method : nullptr;
}

const MethodBind *ClassDB::get_method_by_hash(const StringName &p_class, const StringName &p_name, uint64_t p_hash, bool *r_method_exists) {
	Locker::Lock lock(Locker::STATE_READ);

	ClassInfo *type = classes.getptr(p_class);

	while (type) {
		const GDType::Member *member = type->gdtype->members(true).getptr(p_name);
		if (member && member->type == GDType::Member::Type::METHOD) {
			if (r_method_exists) {
				*r_method_exists = true;
			}
			if (member->payload.method->get_hash() == p_hash) {
				return member->payload.method;
			}
		}

		type = type->inherits_ptr;
	}
	return nullptr;
}

void ClassDB::bind_integer_constant(const StringName &p_class, const StringName &p_enum, const StringName &p_name, int64_t p_constant, bool p_is_bitfield) {
	Locker::Lock lock(Locker::STATE_WRITE);

	ClassInfo *type = classes.getptr(p_class);
	ERR_FAIL_NO_CLASS(type, p_class);

	type->gdtype->bind_integer_constant(p_enum, p_name, p_constant, p_is_bitfield);
}

void ClassDB::get_integer_constant_list(const StringName &p_class, List<String> *p_constants, bool p_no_inheritance) {
	Locker::Lock lock(Locker::STATE_READ);

	ClassInfo *type = classes.getptr(p_class);
	ERR_FAIL_NO_CLASS(type, p_class);

	for (const KeyValue<StringName, GDType::Member> &kv : type->gdtype->members(p_no_inheritance)) {
		if (kv.value.type == GDType::Member::Type::INTEGER_CONSTANT) {
			p_constants->push_back(kv.key);
		}
	}
}

int64_t ClassDB::get_integer_constant(const StringName &p_class, const StringName &p_name, bool *p_success) {
	Locker::Lock lock(Locker::STATE_READ);

	ClassInfo *type = classes.getptr(p_class);
	if (type) {
		const GDType::Member *member = type->gdtype->members().getptr(p_name);
		if (member && member->type == GDType::Member::Type::INTEGER_CONSTANT) {
			if (p_success) {
				*p_success = true;
			}
			return member->payload.integer_constant.value;
		}
	}

	if (p_success) {
		*p_success = false;
	}
	return 0;
}

bool ClassDB::has_integer_constant(const StringName &p_class, const StringName &p_name, bool p_no_inheritance) {
	Locker::Lock lock(Locker::STATE_READ);

	ClassInfo *type = classes.getptr(p_class);
	ERR_FAIL_NULL_V(type, false);

	const GDType::Member *member = type->gdtype->members(p_no_inheritance).getptr(p_name);
	return member && member->type == GDType::Member::Type::INTEGER_CONSTANT;
}

StringName ClassDB::get_integer_constant_enum(const StringName &p_class, const StringName &p_name, bool p_no_inheritance) {
	Locker::Lock lock(Locker::STATE_READ);

	ClassInfo *type = classes.getptr(p_class);
	ERR_FAIL_NULL_V(type, StringName());

	const GDType::EnumInfo *enum_info = type->gdtype->get_integer_constant_enum(p_name, p_no_inheritance);
	return enum_info ? enum_info->name : StringName();
}

void ClassDB::get_enum_list(const StringName &p_class, List<StringName> *p_enums, bool p_no_inheritance) {
	Locker::Lock lock(Locker::STATE_READ);

	ClassInfo *type = classes.getptr(p_class);
	if (!type) {
		// TODO This should probably error, but didn't previously.
		// Making this error leads to a few prints during unit tests.
		return;
	}

	for (const KeyValue<StringName, GDType::Member> &kv : type->gdtype->members(p_no_inheritance)) {
		if (kv.value.type == GDType::Member::Type::ENUM) {
			p_enums->push_back(kv.key);
		}
	}
}

void ClassDB::get_enum_constants(const StringName &p_class, const StringName &p_enum, List<StringName> *p_constants, bool p_no_inheritance) {
	Locker::Lock lock(Locker::STATE_READ);

	ClassInfo *type = classes.getptr(p_class);
	ERR_FAIL_NO_CLASS(type, p_class);

	const GDType::Member *member = type->gdtype->members(p_no_inheritance).getptr(p_enum);
	ERR_FAIL_NULL(member);
	ERR_FAIL_COND(member->type != GDType::Member::Type::ENUM);

	for (const KeyValue<StringName, int64_t> &kv : member->payload.enum_info->values) {
		p_constants->push_back(kv.key);
	}
}

void ClassDB::set_method_error_return_values(const StringName &p_class, const StringName &p_method, const Vector<Error> &p_values) {
#ifdef DEBUG_ENABLED
	Locker::Lock lock(Locker::STATE_WRITE);
	ClassInfo *type = classes.getptr(p_class);

	ERR_FAIL_NO_CLASS(type, p_class);

	type->method_error_values[p_method] = p_values;
#endif // DEBUG_ENABLED
}

Vector<Error> ClassDB::get_method_error_return_values(const StringName &p_class, const StringName &p_method) {
#ifdef DEBUG_ENABLED
	Locker::Lock lock(Locker::STATE_READ);
	ClassInfo *type = classes.getptr(p_class);

	ERR_FAIL_NULL_V(type, Vector<Error>());

	if (!type->method_error_values.has(p_method)) {
		return Vector<Error>();
	}
	return type->method_error_values[p_method];
#else
	return Vector<Error>();
#endif // DEBUG_ENABLED
}

bool ClassDB::has_enum(const StringName &p_class, const StringName &p_name, bool p_no_inheritance) {
	Locker::Lock lock(Locker::STATE_READ);

	ClassInfo *type = classes.getptr(p_class);
	ERR_FAIL_NULL_V(type, false);

	const GDType::Member *member = type->gdtype->members(p_no_inheritance).getptr(p_name);
	return member && member->type == GDType::Member::Type::ENUM;
}

bool ClassDB::is_enum_bitfield(const StringName &p_class, const StringName &p_name, bool p_no_inheritance) {
	Locker::Lock lock(Locker::STATE_READ);

	ClassInfo *type = classes.getptr(p_class);
	ERR_FAIL_NULL_V(type, false);

	const GDType::Member *member = type->gdtype->members(p_no_inheritance).getptr(p_name);
	// FIXME This fails unexpectedly often. Keeping legacy behavior to silently fail for now.
	if (!member || member->type != GDType::Member::Type::ENUM) {
		return false;
	}

	return member->payload.enum_info->is_bitfield;
}

void ClassDB::add_signal(const StringName &p_class, const MethodInfo &p_signal) {
	Locker::Lock lock(Locker::STATE_WRITE);

	ClassInfo *type = classes.getptr(p_class);
	ERR_FAIL_NO_CLASS(type, p_class);

	type->gdtype->add_signal(p_signal);
}

void ClassDB::get_signal_list(const StringName &p_class, List<MethodInfo> *p_signals, bool p_no_inheritance) {
	Locker::Lock lock(Locker::STATE_READ);

	ClassInfo *type = classes.getptr(p_class);
	ERR_FAIL_NO_CLASS(type, p_class);

	for (const KeyValue<StringName, GDType::Member> &kv : type->gdtype->members(p_no_inheritance)) {
		if (kv.value.type == GDType::Member::Type::SIGNAL) {
			p_signals->push_back(*kv.value.payload.signal);
		}
	}
}

bool ClassDB::has_signal(const StringName &p_class, const StringName &p_signal, bool p_no_inheritance) {
	Locker::Lock lock(Locker::STATE_READ);
	ClassInfo *type = classes.getptr(p_class);
	if (!type) {
		return false;
	}
	const GDType::Member *member = type->gdtype->members(p_no_inheritance).getptr(p_signal);
	return member && member->type == GDType::Member::Type::SIGNAL;
}

bool ClassDB::get_signal(const StringName &p_class, const StringName &p_signal, MethodInfo *r_signal) {
	Locker::Lock lock(Locker::STATE_READ);
	ClassInfo *type = classes.getptr(p_class);
	if (!type) {
		return false;
	}
	const GDType::Member *member = type->gdtype->members().getptr(p_signal);
	if (!member || member->type != GDType::Member::Type::SIGNAL) {
		return false;
	}
	*r_signal = *member->payload.signal;
	return true;
}

void ClassDB::add_property_group(const StringName &p_class, const String &p_name, const String &p_prefix, int p_indent_depth) {
	Locker::Lock lock(Locker::STATE_READ); // Doesn't modify ClassDB stuff.
	ClassInfo *type = classes.getptr(p_class);
	ERR_FAIL_NO_CLASS(type, p_class);

	String prefix = p_prefix;
	if (p_indent_depth > 0) {
		prefix = vformat("%s,%d", p_prefix, p_indent_depth);
	}

	type->gdtype->add_to_ordered_properties(PropertyInfo(Variant::NIL, p_name, PROPERTY_HINT_NONE, prefix, PROPERTY_USAGE_GROUP));
}

void ClassDB::add_property_subgroup(const StringName &p_class, const String &p_name, const String &p_prefix, int p_indent_depth) {
	Locker::Lock lock(Locker::STATE_READ); // Doesn't modify ClassDB stuff.
	ClassInfo *type = classes.getptr(p_class);
	ERR_FAIL_NO_CLASS(type, p_class);

	String prefix = p_prefix;
	if (p_indent_depth > 0) {
		prefix = vformat("%s,%d", p_prefix, p_indent_depth);
	}

	type->gdtype->add_to_ordered_properties(PropertyInfo(Variant::NIL, p_name, PROPERTY_HINT_NONE, prefix, PROPERTY_USAGE_SUBGROUP));
}

void ClassDB::add_property_array_count(const StringName &p_class, const String &p_label, const StringName &p_count_property, const StringName &p_count_setter, const StringName &p_count_getter, const String &p_array_element_prefix, uint32_t p_count_usage) {
	add_property(p_class, PropertyInfo(Variant::INT, p_count_property, PROPERTY_HINT_NONE, "", p_count_usage | PROPERTY_USAGE_ARRAY, vformat("%s,%s", p_label, p_array_element_prefix)), p_count_setter, p_count_getter);
}

void ClassDB::add_property_array(const StringName &p_class, const StringName &p_path, const String &p_array_element_prefix) {
	Locker::Lock lock(Locker::STATE_READ); // Doesn't modify ClassDB stuff.
	ClassInfo *type = classes.getptr(p_class);
	ERR_FAIL_NO_CLASS(type, p_class);

	type->gdtype->add_to_ordered_properties(PropertyInfo(Variant::NIL, p_path, PROPERTY_HINT_NONE, "", PROPERTY_USAGE_EDITOR | PROPERTY_USAGE_ARRAY, p_array_element_prefix));
}

// NOTE: For implementation simplicity reasons, this method doesn't allow setters to have optional arguments at the end.
void ClassDB::add_property(const StringName &p_class, const PropertyInfo &p_pinfo, const StringName &p_setter, const StringName &p_getter, int p_index) {
	Locker::Lock lock(Locker::STATE_READ); // Doesn't modify ClassDB stuff.

	ClassInfo *type = classes.getptr(p_class);
	ERR_FAIL_NO_CLASS(type, p_class);
	type->gdtype->add_property(p_pinfo, p_setter, p_getter, p_index);
}

void ClassDB::set_property_default_value(const StringName &p_class, const StringName &p_name, const Variant &p_default) {
	if (!default_values.has(p_class)) {
		default_values[p_class] = HashMap<StringName, Variant>();
	}
	default_values[p_class][p_name] = p_default;
}

void ClassDB::add_linked_property(const StringName &p_class, const String &p_property, const String &p_linked_property) {
#ifdef TOOLS_ENABLED
	Locker::Lock lock(Locker::STATE_READ); // Doesn't modify ClassDB stuff.
	ClassInfo *type = classes.getptr(p_class);
	ERR_FAIL_NO_CLASS(type, p_class);

	const GDType::Member *member = type->gdtype->members(true).getptr(p_property);
	ERR_FAIL_NULL(member);
	ERR_FAIL_COND(member->type != GDType::Member::Type::PROPERTY);

	const GDType::Member *linked_property = type->gdtype->members(true).getptr(p_linked_property);
	ERR_FAIL_NULL(linked_property);
	ERR_FAIL_COND(linked_property->type != GDType::Member::Type::PROPERTY);

	if (!type->linked_properties.has(p_property)) {
		type->linked_properties.insert(p_property, List<StringName>());
	}
	type->linked_properties[p_property].push_back(p_linked_property);

#endif
}

void ClassDB::get_property_list(const StringName &p_class, List<PropertyInfo> *p_list, bool p_no_inheritance, const Object *p_validator) {
	Locker::Lock lock(Locker::STATE_READ); // Doesn't modify ClassDB stuff.

	ClassInfo *type = classes.getptr(p_class);
	ClassInfo *check = type;
	while (check) {
		for (const PropertyInfo *pi : check->gdtype->get_ordered_self_properties()) {
			p_list->push_back(*pi);
			if (p_validator) {
				p_validator->validate_property(p_list->back()->get());
			}
		}

		if (p_no_inheritance) {
			return;
		}
		check = check->inherits_ptr;
	}
}

void ClassDB::get_linked_properties_info(const StringName &p_class, const StringName &p_property, List<StringName> *r_properties, bool p_no_inheritance) {
#ifdef TOOLS_ENABLED
	ClassInfo *check = classes.getptr(p_class);
	while (check) {
		if (!check->linked_properties.has(p_property)) {
			return;
		}
		for (const StringName &E : check->linked_properties[p_property]) {
			r_properties->push_back(E);
		}

		if (p_no_inheritance) {
			break;
		}
		check = check->inherits_ptr;
	}
#endif
}

bool ClassDB::get_property_info(const StringName &p_class, const StringName &p_property, PropertyInfo *r_info, bool p_no_inheritance, const Object *p_validator) {
	Locker::Lock lock(Locker::STATE_READ);

	ClassInfo *class_info = classes.getptr(p_class);
	if (class_info) {
		const GDType::Member *member = class_info->gdtype->members(p_no_inheritance).getptr(p_property);
		if (member && member->type == GDType::Member::Type::PROPERTY) {
			if (r_info) {
				*r_info = *member->payload.property.property_info;
			}
			if (p_validator) {
				p_validator->validate_property(*r_info);
			}
			return true;
		}
	}

	return false;
}

bool ClassDB::set_property(Object *p_object, const StringName &p_property, const Variant &p_value, bool *r_valid) {
	ERR_FAIL_NULL_V(p_object, false);

	return p_object->set_native(p_property, p_value, r_valid);
}

bool ClassDB::get_property(Object *p_object, const StringName &p_property, Variant &r_value) {
	ERR_FAIL_NULL_V(p_object, false);

	return p_object->get_native(p_property, r_value);
}

int ClassDB::get_property_index(const StringName &p_class, const StringName &p_property, bool *r_is_valid) {
	ClassInfo *class_info = classes.getptr(p_class);
	if (class_info) {
		const GDType::Member *member = class_info->gdtype->members().getptr(p_property);
		if (member && member->type == GDType::Member::Type::PROPERTY) {
			const GDType::Member::Property &psg = member->payload.property;
			if (r_is_valid) {
				*r_is_valid = true;
			}

			return psg.index;
		}
	}

	if (r_is_valid) {
		*r_is_valid = false;
	}

	return -1;
}

Variant::Type ClassDB::get_property_type(const StringName &p_class, const StringName &p_property, bool *r_is_valid) {
	ClassInfo *class_info = classes.getptr(p_class);
	if (class_info) {
		const GDType::Member *member = class_info->gdtype->members().getptr(p_property);
		if (member && member->type == GDType::Member::Type::PROPERTY) {
			const GDType::Member::Property &psg = member->payload.property;
			if (r_is_valid) {
				*r_is_valid = true;
			}

			return psg.property_info->type;
		}
	}

	if (r_is_valid) {
		*r_is_valid = false;
	}

	return Variant::NIL;
}

StringName ClassDB::get_property_setter(const StringName &p_class, const StringName &p_property) {
	ClassInfo *class_info = classes.getptr(p_class);
	if (class_info) {
		const GDType::Member *member = class_info->gdtype->members().getptr(p_property);
		if (member && member->type == GDType::Member::Type::PROPERTY) {
			return member->payload.property.setter ? member->payload.property.setter->get_name() : StringName();
		}
	}

	return StringName();
}

StringName ClassDB::get_property_getter(const StringName &p_class, const StringName &p_property) {
	ClassInfo *class_info = classes.getptr(p_class);
	if (class_info) {
		const GDType::Member *member = class_info->gdtype->members().getptr(p_property);
		if (member && member->type == GDType::Member::Type::PROPERTY) {
			return member->payload.property.getter ? member->payload.property.getter->get_name() : StringName();
		}
	}

	return StringName();
}

bool ClassDB::has_property(const StringName &p_class, const StringName &p_property, bool p_no_inheritance) {
	ClassInfo *class_info = classes.getptr(p_class);
	if (class_info) {
		const GDType::Member *member = class_info->gdtype->members(p_no_inheritance).getptr(p_property);
		return member ? member->type == GDType::Member::Type::PROPERTY : false;
	}

	return false;
}

void ClassDB::set_method_flags(const StringName &p_class, const StringName &p_method, int p_flags) {
	Locker::Lock lock(Locker::STATE_WRITE);
	ClassInfo *type = classes.getptr(p_class);
	ERR_FAIL_NULL(type);
	type->gdtype->set_method_flags(p_method, p_flags);
}

bool ClassDB::has_method(const StringName &p_class, const StringName &p_method, bool p_no_inheritance) {
	ClassInfo *type = classes.getptr(p_class);
	if (!type) {
		return false;
	}
	const GDType::Member *member = type->gdtype->members(p_no_inheritance).getptr(p_method);
	return member && member->type == GDType::Member::Type::METHOD;
}

int ClassDB::get_method_argument_count(const StringName &p_class, const StringName &p_method, bool *r_is_valid, bool p_no_inheritance) {
	Locker::Lock lock(Locker::STATE_READ);

	ClassInfo *type = classes.getptr(p_class);
	if (type) {
		const GDType::Member *member = type->gdtype->members(p_no_inheritance).getptr(p_method);
		if (member && member->type == GDType::Member::Type::METHOD) {
			if (r_is_valid) {
				*r_is_valid = true;
			}
			return member->payload.method->get_argument_count();
		}
	}

	if (r_is_valid) {
		*r_is_valid = false;
	}

	return 0;
}

void ClassDB::bind_method_custom(const StringName &p_class, MethodBind *p_method, bool p_take_ownership) {
	_bind_method_custom(p_class, p_method, p_take_ownership);
}

void ClassDB::_bind_method_custom(const StringName &p_class, MethodBind *p_method, bool p_take_ownership) {
	Locker::Lock lock(Locker::STATE_WRITE);

	StringName method_name = p_method->get_name();

	ClassInfo *type = classes.getptr(p_class);
	if (!type) {
		memdelete(p_method);
		ERR_FAIL_MSG(vformat("Couldn't bind custom method '%s' for instance '%s'.", method_name, p_class));
	}

	type->gdtype->bind_method(p_method, p_take_ownership);
}

MethodBind *ClassDB::_bind_vararg_method(MethodBind *p_bind, const StringName &p_name, const Vector<Variant> &p_default_args) {
	MethodBind *bind = p_bind;
	bind->set_name(p_name);
	bind->set_default_arguments(p_default_args);

	String instance_type = bind->get_instance_class();

	ClassInfo *type = classes.getptr(instance_type);
	if (!type) {
		memdelete(bind);
		ERR_FAIL_NULL_V(type, nullptr);
	}

	return type->gdtype->bind_method(bind) ? bind : nullptr;
}

#ifdef DEBUG_ENABLED
MethodBind *ClassDB::bind_methodfi(uint32_t p_flags, MethodBind *p_bind, const MethodDefinition &p_method_name, const Variant **p_defs, int p_defcount) {
	StringName mdname = p_method_name.name;
#else
MethodBind *ClassDB::bind_methodfi(uint32_t p_flags, MethodBind *p_bind, const char *p_method_name, const Variant **p_defs, int p_defcount) {
	StringName mdname = StringName(p_method_name);
#endif // DEBUG_ENABLED

	Locker::Lock lock(Locker::STATE_WRITE);
	ERR_FAIL_NULL_V(p_bind, nullptr);
	p_bind->set_name(mdname);

	String instance_type = p_bind->get_instance_class();

#ifdef DEBUG_ENABLED

	ERR_FAIL_COND_V_MSG(has_method(instance_type, mdname), nullptr, vformat("Class '%s' already has a method '%s'.", String(instance_type), String(mdname)));
#endif // DEBUG_ENABLED

	ClassInfo *type = classes.getptr(instance_type);
	if (!type) {
		memdelete(p_bind);
		ERR_FAIL_V_MSG(nullptr, vformat("Couldn't bind method '%s' for instance '%s'.", mdname, instance_type));
	}

#ifdef DEBUG_ENABLED

	if (p_method_name.args.size() > p_bind->get_argument_count()) {
		memdelete(p_bind);
		ERR_FAIL_V_MSG(nullptr, vformat("Method definition provides more arguments than the method actually has '%s::%s'.", instance_type, mdname));
	}

	if (p_defcount > p_bind->get_argument_count()) {
		memdelete(p_bind);
		ERR_FAIL_V_MSG(nullptr, vformat("Method definition for '%s::%s' provides more default arguments than the method has arguments.", instance_type, mdname));
	}

	p_bind->set_argument_names(p_method_name.args);
#endif // DEBUG_ENABLED

	if (!type->gdtype->bind_method(p_bind)) {
		return nullptr;
	}

	Vector<Variant> defvals;

	defvals.resize(p_defcount);
	for (int i = 0; i < p_defcount; i++) {
		defvals.write[i] = *p_defs[i];
	}

	p_bind->set_default_arguments(defvals);
	p_bind->set_hint_flags(p_flags);
	return p_bind;
}

void ClassDB::add_virtual_method(const StringName &p_class, const MethodInfo &p_method, bool p_virtual, const Vector<String> &p_arg_names, bool p_object_core) {
	ERR_FAIL_COND_MSG(!classes.has(p_class), vformat("Request for nonexistent class '%s'.", p_class));

	Locker::Lock lock(Locker::STATE_WRITE);

#ifdef DEBUG_ENABLED
	MethodInfo mi = p_method;
	if (p_virtual) {
		mi.flags |= METHOD_FLAG_VIRTUAL;
	}
	if (p_object_core) {
		mi.flags |= METHOD_FLAG_OBJECT_CORE;
	}

	if (!p_object_core) {
		if (p_arg_names.size() != mi.arguments.size()) {
			WARN_PRINT(vformat("Mismatch argument name count for virtual method: '%s::%s'.", String(p_class), p_method.name));
		} else {
			for (int64_t i = 0; i < p_arg_names.size(); ++i) {
				mi.arguments.write[i].name = p_arg_names[i];
			}
		}
	}

	if (classes[p_class].virtual_methods_map.has(p_method.name)) {
		// overloading not supported
		ERR_FAIL_MSG(vformat("Virtual method already bound '%s::%s'.", String(p_class), p_method.name));
	}
	classes[p_class].virtual_methods.push_back(mi);
	classes[p_class].virtual_methods_map[p_method.name] = mi;

#endif // DEBUG_ENABLED
}

void ClassDB::get_virtual_methods(const StringName &p_class, List<MethodInfo> *p_methods, bool p_no_inheritance) {
	ERR_FAIL_COND_MSG(!classes.has(p_class), vformat("Request for nonexistent class '%s'.", p_class));

#ifdef DEBUG_ENABLED

	ClassInfo *type = classes.getptr(p_class);
	ClassInfo *check = type;
	while (check) {
		for (const MethodInfo &E : check->virtual_methods) {
			p_methods->push_back(E);
		}

		if (p_no_inheritance) {
			return;
		}
		check = check->inherits_ptr;
	}

#endif // DEBUG_ENABLED
}

void ClassDB::set_class_enabled(const StringName &p_class, bool p_enable) {
	Locker::Lock lock(Locker::STATE_WRITE);

	ERR_FAIL_COND_MSG(!classes.has(p_class), vformat("Request for nonexistent class '%s'.", p_class));
	classes[p_class].disabled = !p_enable;
}

bool ClassDB::is_class_enabled(const StringName &p_class) {
	Locker::Lock lock(Locker::STATE_READ);

	ClassInfo *ti = classes.getptr(p_class);

	ERR_FAIL_NULL_V_MSG(ti, false, vformat("Cannot get class '%s'.", String(p_class)));
	return !ti->disabled;
}

bool ClassDB::is_class_exposed(const StringName &p_class) {
	Locker::Lock lock(Locker::STATE_READ);

	ClassInfo *ti = classes.getptr(p_class);
	ERR_FAIL_NULL_V_MSG(ti, false, vformat("Cannot get class '%s'.", String(p_class)));
	return ti->exposed;
}

#ifdef TOOLS_ENABLED
void ClassDB::add_class_dependency(const StringName &p_class, const StringName &p_dependency) {
	Locker::Lock lock(Locker::STATE_WRITE);

	ERR_FAIL_COND_MSG(!classes.has(p_class), vformat("Request for nonexistent class '%s'.", p_class));
	if (classes[p_class].dependency_list.find(p_dependency)) {
		ERR_FAIL();
	}

	classes[p_class].dependency_list.push_back(p_dependency);
}

void ClassDB::get_class_dependencies(const StringName &p_class, List<StringName> *r_rependencies) {
	Locker::Lock lock(Locker::STATE_READ);

	ClassInfo *ti = classes.getptr(p_class);
	ERR_FAIL_NULL_MSG(ti, vformat("Cannot get class '%s'.", String(p_class)));

	for (const StringName &dep : ti->dependency_list) {
		r_rependencies->push_back(dep);
	}
}
#endif // TOOLS_ENABLED

void ClassDB::add_resource_base_extension(const StringName &p_extension, const StringName &p_class) {
	if (resource_base_extensions.has(p_extension)) {
		return;
	}

	resource_base_extensions[p_extension] = p_class;
}

void ClassDB::get_resource_base_extensions(List<String> *p_extensions) {
	for (const KeyValue<StringName, StringName> &E : resource_base_extensions) {
		p_extensions->push_back(E.key);
	}
}

bool ClassDB::is_resource_extension(const StringName &p_extension) {
	return resource_base_extensions.has(p_extension);
}

void ClassDB::get_extensions_for_type(const StringName &p_class, List<String> *p_extensions) {
	for (const KeyValue<StringName, StringName> &E : resource_base_extensions) {
		if (is_parent_class(p_class, E.value) || is_parent_class(E.value, p_class)) {
			p_extensions->push_back(E.key);
		}
	}
}

HashMap<StringName, HashMap<StringName, Variant>> ClassDB::default_values;
HashSet<StringName> ClassDB::default_values_cached;

Variant ClassDB::class_get_default_property_value(const StringName &p_class, const StringName &p_property, bool *r_valid) {
	Locker::Lock lock(Locker::STATE_WRITE);

	if (!default_values_cached.has(p_class)) {
		if (!default_values.has(p_class)) {
			default_values[p_class] = HashMap<StringName, Variant>();
		}

		Object *c = nullptr;
		bool cleanup_c = false;

		if (Engine::get_singleton()->has_singleton(p_class)) {
			c = Engine::get_singleton()->get_singleton_object(p_class);
			cleanup_c = false;
		} else if (ClassDB::can_instantiate(p_class) && !ClassDB::is_virtual(p_class)) { // Keep this condition in sync with doc_tools.cpp get_documentation_default_value.
			c = ClassDB::instantiate(p_class);
			cleanup_c = true;
		}

		if (c) {
			List<PropertyInfo> plist;
			c->get_property_list(&plist);
			for (const PropertyInfo &E : plist) {
				if (E.usage & (PROPERTY_USAGE_STORAGE | PROPERTY_USAGE_EDITOR)) {
					if (!default_values[p_class].has(E.name)) {
						Variant v = c->get(E.name);
						default_values[p_class][E.name] = v;
					}
				}
			}

			if (cleanup_c) {
				memdelete(c);
			}
		}

		default_values_cached.insert(p_class);
	}

	if (!default_values.has(p_class)) {
		if (r_valid != nullptr) {
			*r_valid = false;
		}
		return Variant();
	}

	if (!default_values[p_class].has(p_property)) {
		if (r_valid != nullptr) {
			*r_valid = false;
		}
		return Variant();
	}

	if (r_valid != nullptr) {
		*r_valid = true;
	}

	Variant var = default_values[p_class][p_property];

#ifdef DEBUG_ENABLED
	// Some properties may have an instantiated Object as default value,
	// (like Path2D's `curve` used to have), but that's not a good practice.
	// Instead, those properties should use PROPERTY_USAGE_EDITOR_INSTANTIATE_OBJECT
	// to be auto-instantiated when created in the editor with the following method:
	// EditorNode::get_editor_data().instantiate_object_properties(obj);
	if (var.get_type() == Variant::OBJECT) {
		Object *obj = var.get_validated_object();
		if (obj) {
			WARN_PRINT(vformat("Instantiated %s used as default value for %s's \"%s\" property.", obj->get_class(), p_class, p_property));
		}
	}
#endif // DEBUG_ENABLED

	return var;
}

void ClassDB::cleanup_defaults() {
	default_values.clear();
	default_values_cached.clear();
}

LocalVector<GDType **> ClassDB::gdtype_autorelease_pool;
void ClassDB::cleanup() {
	//OBJTYPE_LOCK; hah not here

	classes.clear();
	resource_base_extensions.clear();

	for (GDType **type : gdtype_autorelease_pool) {
		if (!*type) {
			WARN_PRINT("GDType in autorelease pool was cleaned up before being auto-released. Ignoring.");
			continue;
		}
		memdelete(*type);
		*type = nullptr;
	}
	gdtype_autorelease_pool.clear();
}

// Array to use in optional parameters on methods and the DEFVAL_ARRAY macro.
Array ClassDB::default_array_arg = Array::create_read_only();

bool ClassDB::is_default_array_arg(const Array &p_array) {
	return p_array.is_same_instance(default_array_arg);
}

//

ClassDB::Locker::Lock::Lock(Locker::State p_state) {
	DEV_ASSERT(p_state != STATE_UNLOCKED);
	if (p_state == STATE_READ) {
		if (Locker::thread_state == STATE_UNLOCKED) {
			state = STATE_READ;
			Locker::thread_state = STATE_READ;
			Locker::lock.read_lock();
		}
	} else if (p_state == STATE_WRITE) {
		if (Locker::thread_state == STATE_UNLOCKED) {
			state = STATE_WRITE;
			Locker::thread_state = STATE_WRITE;
			Locker::lock.write_lock();
		} else if (Locker::thread_state == STATE_READ) {
			CRASH_NOW_MSG("Lock can't be upgraded from read to write.");
		}
	}
}

ClassDB::Locker::Lock::~Lock() {
	if (state == STATE_READ) {
		Locker::lock.read_unlock();
		Locker::thread_state = STATE_UNLOCKED;
	} else if (state == STATE_WRITE) {
		Locker::lock.write_unlock();
		Locker::thread_state = STATE_UNLOCKED;
	}
}

#undef ERR_FAIL_NO_CLASS
