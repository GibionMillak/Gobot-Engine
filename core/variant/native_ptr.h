/**************************************************************************/
/*  native_ptr.h                                                          */
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

#include "core/variant/method_ptrcall.h"
#include "core/variant/type_info.h"
#include "core/variant/variant_caster.h"
#include "core/variant/variant_internal.h"

struct AudioFrame;

// Metafunction for pointee name. Specialized in GDVIRTUAL_NATIVE_PTR; takes into account const; falls back to "void".
template <typename PointeeType>
struct NativePtrName {
	static const char *get() { return "void"; }
};
template <typename PointeeType>
struct NativePtrName<const PointeeType> {
	static const char *get() {
		static const CharString name = (String("const ") + NativePtrName<PointeeType>::get()).utf8();
		return name.get_data();
	}
};
template <typename PointeeType>
struct NativePtrName<PointeeType *> {
	static const char *get() {
		static const CharString name = (String(NativePtrName<PointeeType>::get()) + " *").utf8();
		return name.get_data();
	}
};

#define GDVIRTUAL_NATIVE_PTR(m_type) \
	template <> \
	struct NativePtrName<m_type> { \
		static const char *get() { return #m_type; } \
	};

// Raw pointer transported through Variant for native backends. PointeeType can be const-qualified to map to `const MyClass*`.
template <typename PointeeType>
struct NativePtr {
	NativePtr() = default;
	NativePtr(PointeeType *p_assign) { data = p_assign; }
	static const char *get_name() { return NativePtrName<PointeeType>::get(); }
	operator PointeeType *() const { return data; }
	operator Variant() const { return uint64_t(data); }

	PointeeType *data = nullptr;
};

template <typename PointeeType>
struct GetTypeInfo<NativePtr<PointeeType>> {
	static constexpr Variant::Type VARIANT_TYPE = Variant::INT;
	static constexpr GodotTypeInfo::Metadata METADATA = GodotTypeInfo::METADATA_NONE;
	static inline PropertyInfo get_class_info() {
		return PropertyInfo(Variant::INT, String(), PROPERTY_HINT_INT_IS_POINTER, NativePtr<PointeeType>::get_name());
	}
};

template <typename PointeeType>
struct VariantCaster<NativePtr<PointeeType>> {
	static _FORCE_INLINE_ NativePtr<PointeeType> cast(const Variant &p_variant) {
		return NativePtr<PointeeType>(reinterpret_cast<PointeeType *>(p_variant.operator uint64_t()));
	}
};

template <typename PointeeType>
struct VariantInternalAccessor<NativePtr<PointeeType>> {
	static _FORCE_INLINE_ const NativePtr<PointeeType> &get(const Variant *p_variant) {
		return *reinterpret_cast<const NativePtr<PointeeType> *>(VariantInternal::get_int(p_variant));
	}
	static _FORCE_INLINE_ void set(Variant *r_variant, const NativePtr<PointeeType> &p_value) {
		*VariantInternal::get_int(r_variant) = uint64_t(p_value.data);
	}
};

template <typename PointeeType>
struct PtrToArg<NativePtr<PointeeType>> {
	_FORCE_INLINE_ static NativePtr<PointeeType> convert(const void *p_ptr) {
		return NativePtr<PointeeType>(static_cast<PointeeType *>(const_cast<void *>(p_ptr)));
	}
	using EncodeT = PointeeType *;
	_FORCE_INLINE_ static void encode(NativePtr<PointeeType> p_val, void *r_ptr) {
		*static_cast<PointeeType **>(r_ptr) = p_val.data;
	}
};

GDVIRTUAL_NATIVE_PTR(AudioFrame)
GDVIRTUAL_NATIVE_PTR(bool)
GDVIRTUAL_NATIVE_PTR(char)
GDVIRTUAL_NATIVE_PTR(char16_t)
GDVIRTUAL_NATIVE_PTR(char32_t)
GDVIRTUAL_NATIVE_PTR(wchar_t)
GDVIRTUAL_NATIVE_PTR(uint8_t)
GDVIRTUAL_NATIVE_PTR(int8_t)
GDVIRTUAL_NATIVE_PTR(uint16_t)
GDVIRTUAL_NATIVE_PTR(int16_t)
GDVIRTUAL_NATIVE_PTR(uint32_t)
GDVIRTUAL_NATIVE_PTR(int32_t)
GDVIRTUAL_NATIVE_PTR(int64_t)
GDVIRTUAL_NATIVE_PTR(uint64_t)
GDVIRTUAL_NATIVE_PTR(float)
GDVIRTUAL_NATIVE_PTR(double)
