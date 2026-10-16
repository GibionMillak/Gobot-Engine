---
name: bindings
description: Use when adding or editing C++ classes that register with Godot/Gobot ClassDB or are exposed to GDScript, including GDCLASS, _bind_methods, ClassDB::bind_method, ADD_PROPERTY, BIND_ENUM_CONSTANT, signals, and _notification handlers.
---

# Method Binding and Notifications

## `_bind_methods()` ordering

Group calls in this order:

1. `ClassDB::bind_method`
2. `ADD_PROPERTY`
3. `BIND_ENUM_CONSTANT`

## Setter/getter binding

Bind the setter first and the getter second. Use the property name as the setter argument name, without a `p_` prefix in `D_METHOD`:

```cpp
ClassDB::bind_method(D_METHOD("set_<thing>", "<thing>"), &Dock::set_<thing>);
ClassDB::bind_method(D_METHOD("get_<thing>"), &Dock::get_<thing>);
```

## `_notification`

Implement `_notification(int p_what)` with `switch (p_what)`. Use a `case` for each handled notification and include the appropriate `break`. Do not replace the switch with an `if` check when only one notification is handled.

## Registration requirements

For new methods or properties exposed to GDScript, ensure `GDCLASS`, `ClassDB::bind_method`, `ADD_PROPERTY`, and signal-registration macros and their includes are present and correctly referenced. Do not assume registration is automatic. Also verify the class is registered in the module's register types file.

Do not fabricate API members: confirm any signal, method, or property in the actual class or its ancestors before binding or using it.
