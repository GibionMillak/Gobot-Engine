# Gobot Engine Development Guidance

## Scope and Applicability

These instructions apply repository-wide to all first-party files and code, including code inherited from upstream projects. Follow them when creating, modifying, refactoring, or reviewing code.

Third-party and vendored code are exempt. Preserve their original conventions and avoid modifying them unless the task explicitly requires it. When ownership is unclear, inspect the file location, ownership markers, and build configuration before changing it.

Do not manually edit generated files. Modify the generator or its source data instead. First-party generated code should follow these instructions whenever practical.

## Skills (use when the task matches)

- UI construction (`memnew` on controls, building or refactoring GUI hierarchies): use `ui-construction` at `.agents/skills/ui-construction/SKILL.md`.
- Exposing classes, methods, properties, or signals to ClassDB/GDScript, `_bind_methods`, `_notification`: use `bindings` at `.agents/skills/bindings/SKILL.md`.
- Reviewing code: use `code-review` at `.agents/skills/code-review/SKILL.md`.

## Scope Discipline

Make every change necessary to implement the requested work correctly. This may include changing entire functions, classes, or files when justified by the task or when it is the best implementation approach.

New and directly modified code must follow these instructions. Small surrounding adjustments, such as reordering declarations, are allowed when they help integrate the requested change cleanly and consistently.

After completing the necessary changes, do not perform additional guideline-driven refactoring of unrelated existing code unless explicitly requested.

Proceed with additional changes that are clearly necessary to keep the codebase internally consistent, complete a migration, and avoid broken or dead code. Report these additional changes in the final summary. Pause for approval only when the work would substantially expand the task, cross into unrelated architecture, or require a meaningful design choice.

## Naming Conventions

- Class, struct, and template type names use `CamelCase`.
- Public methods and member functions use `snake_case`.
- Every private or protected member variable and member function uses exactly one leading underscore, such as `_feature_cache`, `_update_cache()`, and `_bind_methods()`.
- Public members do not use a leading underscore unless an unchangeable external API requires it.
- Local variables and public data members use `snake_case`.
- Constants and macros use `UPPER_SNAKE_CASE`.
- Avoid global variables when possible. Required global variables use `snake_case`.
- Template type parameters use descriptive `CamelCase` names, such as `ElementType` or `AllocatorType`.
- File and directory names use lowercase `snake_case`, such as `resource_loader.cpp`, `resource_loader.h`, and `scene_import/`.
- Always prefer `#pragma once` for header include protection.
- Avoid macros when a typed constant, inline function, template, or enum is suitable.

## Enums

- Enum type names use `CamelCase`.
- Enum values use `UPPER_SNAKE_CASE`.
- Do not prefix enum values with the enum type name or an abbreviated form of it.

```cpp
enum class DirectionType {
	RIGHT,
	LEFT,
	UP,
	DOWN,
};
```

Use `LEFT`, not `DIRECTION_LEFT`.

## Function Parameters

- Every input-only parameter in a declaration or definition starts with `p_`.
- Every output or in/out parameter starts with `r_`.
- These rules apply to values, references, and pointers.
- A `const` reference or pointer that is only read is an input parameter and uses `p_`.
- A non-const argument that may return modified state to the caller uses `r_`.

```cpp
void load_file(const String &p_path, Resource &r_resource);
void update_bounds(Bounds &r_bounds);
bool parse_data(const char *p_data, Error *r_error);
```

## Header File Structure

Organize class members by visibility in the strict order `public`, `protected`, then `private`.

Use the following order within a class where applicable:

1. Public nested types, enums, aliases, and constants.
2. Constructors and destructor.
3. Copy and move operations.
4. Static factories and singleton accessors.
5. Other public methods.
6. Public data members, which should generally be avoided.
7. Protected nested types and constants.
8. `_bind_methods()`.
9. `_notification()`.
10. Other protected methods.
11. Protected data members.
12. Private nested types and constants.
13. Private methods.
14. Private data members.

Keep construction mechanisms together. Static factory methods such as `create()` belong directly after constructors, the destructor, and copy/move operations.

## Source File Structure

Within each visibility section, implement methods in the `.cpp` file in the exact order in which they are declared in the matching header. Preserve the same public, protected, and private grouping.

## Includes and Dependencies

- Prefer forward declarations when a complete type is not required.
- Keep implementation-only dependencies in `.cpp` files.
- Each `.cpp` file includes its matching header first.
- Include what a file directly uses; do not rely on accidental transitive includes.
- Treat `.clang-format` as the authority for include grouping and ordering.
- Avoid introducing unnecessary dependencies into widely included headers.
- A forward declaration is insufficient when the file uses a complete definition (calling methods, accessing members, instantiating the type).

## API and Inheritance Verification

- Never assume a method or property exists through inheritance. Before using a member not defined in the class being edited, trace the `class X : public Y` chain and check the actual headers or source.
- Do not infer availability from names or similar Godot classes.
- Do not fabricate Godot API members. If a method, signal, or property cannot be confirmed, search the codebase or Godot source/documentation before using it.

## Formatting and Static Analysis

- Treat the repository's `.clang-format` and `.clang-tidy` files as the formatting and static-analysis authority.
- Ensure new and modified code conforms to those configurations.
- Do not automatically run `clang-format` over entire existing files.
- Format only changed regions when needed and practical.
- Full-file formatting is appropriate for newly created files or when explicitly requested.
- Avoid formatting changes outside the requested work.
- Do not modify formatter or linter configuration merely to make new code pass unless explicitly requested.
- Where these instructions define a stricter semantic or structural rule that the tools cannot express, these instructions take precedence.

## Modern C++ Practices

- Use `nullptr` instead of `NULL` or integer zero for null pointers.
- Mark overriding virtual functions with `override`.
- Apply `const` correctness to methods, parameters, pointers, references, and local values.
- Mark single-argument constructors `explicit` unless implicit conversion is intentional.
- Use `[[nodiscard]]` when accidentally ignoring a result would create a correctness or safety risk.
- Prefer default member initializers where appropriate.
- Use `= default` and `= delete` to express special-member behavior clearly.
- Follow repository-specific macros and compatibility requirements where they supersede standard syntax.

## Memory and Ownership

- Follow Gobot/Godot's established memory-management and ownership model.
- Use the appropriate engine facilities, including `Ref`, `ObjectID`, engine containers, and project allocation helpers.
- Do not introduce standard-library smart pointers or raw owning pointers into a subsystem that uses engine ownership facilities.
- Raw pointers must have clear non-owning or engine-managed lifetime semantics.
- Match the surrounding subsystem when ownership requirements differ.
- Make object lifetime and ownership clear through types and API design.

## Error Handling

- Follow the error-handling conventions of the surrounding subsystem.
- Prefer the repository's established error macros, error codes, and return-value patterns.
- Do not introduce C++ exceptions into code that does not already use them.
- Validate fallible operations and propagate failures appropriately.
- Error messages should provide useful context without duplicating or obscuring the underlying error.
- Do not silently ignore errors unless that behavior is intentional and documented.

## Compatibility and Legacy Code

- This is a private repository with a single user. Never preserve backward compatibility unless the user explicitly requests it for the current task.
- When creating, replacing, renaming, moving, or refactoring code, never introduce or retain dead code in the name of backward compatibility.
- Prefer a clean API or architecture change over compatibility wrappers, deprecated aliases, fallback paths, or duplicate old/new implementations.
- Do not leave dead, obsolete, or legacy code after replacing behavior.
- When a requested change requires updates elsewhere, identify and update clearly affected callers, bindings, resources, serialization code, and subsystems.
- Do not knowingly leave the repository in a partially migrated state.
- If the required migration materially exceeds the requested scope, explain the impact and request approval before proceeding.

Removing code made obsolete by a requested change is part of that change, not unrelated cleanup.

## Comments and Documentation

- Comments explain intent, constraints, invariants, or non-obvious reasoning.
- Do not add comments that merely restate straightforward code.
- Keep comments accurate when modifying related behavior.
- Document public APIs when ownership, lifetime, side effects, units, valid ranges, thread-safety, failure behavior, or other important semantics are not clear from the declaration.
- Prefer clear names and simple code over explanatory comments.

## Verification

- Do not compile the project or run tests after making changes unless explicitly requested.
- The user will compile and verify the project after each change.
- Perform basic code inspection and review the resulting diff for unintended changes.
