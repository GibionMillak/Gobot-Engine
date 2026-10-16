---
name: ui-construction
description: Use when creating, modifying, or refactoring C++ GUI code that instantiates controls or containers with memnew (HBoxContainer, VBoxContainer, HFlowContainer, VFlowContainer, GridContainer, Button, Label, PanelContainer), including building new UI classes or restructuring node hierarchies.
---

# UI Construction

## Variable-name suffixes

When creating or adding code that instantiates UI controls with `memnew`:

- `memnew(HBoxContainer)` uses `*_hbox`.
- `memnew(VBoxContainer)` uses `*_vbox`.
- `memnew(HFlowContainer)` uses `*_hflow`.
- `memnew(VFlowContainer)` uses `*_vflow`.
- `memnew(GridContainer)` uses `*_grid`.
- `memnew(Button)` uses `*_btn`.
- `memnew(Label)` uses `*_lbl`.
- `memnew(PanelContainer)` uses `*_pc`.

## Configuration order

For a UI control created with `memnew`, keep all configuration statements between its allocation and its `add_child()` call in alphabetical order by method name.

```cpp
some_hbox = memnew(HBoxContainer);
some_hbox->set_alignment(BoxContainer::ALIGNMENT_BEGIN);
some_hbox->set_h_size_flags(Control::SIZE_EXPAND_FILL);
add_child(some_hbox);
```

## Hierarchy scoping

When refactoring construction code or creating a new class, group each level of a UI node hierarchy inside a nested brace-delimited scope. Place a parent's children in a scope immediately after constructing and adding the parent. Sibling controls belong to the same scope, and descendants use further nested scopes that mirror the hierarchy.

```cpp
some_hbox = memnew(HBoxContainer);
some_hbox->set_alignment(BoxContainer::ALIGNMENT_BEGIN);
some_hbox->set_h_size_flags(Control::SIZE_EXPAND_FILL);
add_child(some_hbox);

{
	some_child_vbox = memnew(VBoxContainer);
	some_child_vbox->set_alignment(BoxContainer::ALIGNMENT_BEGIN);
	some_child_vbox->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	some_hbox->add_child(some_child_vbox);

	{
		some_button_1_btn = memnew(Button);
		some_button_1_btn->set_text("Button 1");
		some_child_vbox->add_child(some_button_1_btn);

		some_button_2_btn = memnew(Button);
		some_button_2_btn->set_text("Button 2");
		some_child_vbox->add_child(some_button_2_btn);
	}
}
```

Before calling any method on a control, verify it exists in the class or its ancestors (see API and Inheritance Verification in AGENTS.md), and include the headers for every control type used.
