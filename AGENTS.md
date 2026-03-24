# AGENTS.md

## Tech Stack
- Use C++17.
- Use Qt 5.12.
- Prefer standard C++ library over Qt when functionality overlaps.
- Minimize Qt dependencies unless required by UI, signals/slots, or framework integration.

---

## Code Style
- Use Allman brace style.
- Keep code simple, explicit, and readable.
- Avoid unnecessary abstractions.
- Prefer small, localized changes over large refactors.

---

## Naming Conventions
- Class names: PascalCase
- Public member functions: snake_case
- Private member functions: _snake_case
- Private member variables: m_snake_case
- Static variables: s_snake_case

- Maintain strict consistency across headers and source files.
- Do not introduce new naming styles.

---

## Architecture & Structure
- All code must reside inside namespaces.
- Avoid global symbols.

- Use a `_details` namespace for:
  - Helper classes
  - Internal utilities
  - Implementation details

- `_details` namespace:
  - Must NOT be exposed as part of public API
  - Must remain internal-only unless explicitly required

---

## C++ Guidelines
- Prefer modern C++ (C++17 features).
- Use:
  - `nullptr` instead of `NULL`
  - `override` for virtual overrides
  - `const` correctness wherever applicable
  - `[[nodiscard]]` when return values must not be ignored

- Prefer:
  - `std::string` over `QString` (unless Qt API requires)
  - `std::vector` over `QList` / `QVector`
  - `std::unique_ptr` / `std::shared_ptr` over raw pointers when ownership is non-trivial

---

## Qt Usage Rules
- Use Qt only where it is clearly required.
- Respect Qt parent-child ownership model.
- Avoid unnecessary coupling to Qt types.
- Do not introduce new Qt-heavy abstractions unless explicitly requested.

---

## Header / Source Organization
- Keep headers clean and minimal.
- Do not place large implementations in headers (except templates / inline cases).
- Prefer forward declarations to reduce unnecessary includes.
- Separate interface and implementation clearly.

---

## Comments & Documentation
- Do not write redundant comments.
- Prefer clear naming over excessive comments.
- Use structured comments for public APIs when necessary.

---

## Workflow Rules
- ALWAYS analyze before making changes.
- BEFORE modifying code:
  - List files to be changed
  - Explain the reason for changes

- DURING changes:
  - Prefer minimal, localized modifications
  - Do not refactor unrelated code

- AFTER changes:
  - Summarize what was changed
  - Provide verification steps

---

## Constraints
- Do NOT change public APIs unless necessary.
- Do NOT rename major classes, files, or symbols without clear justification.
- Do NOT introduce new dependencies unless explicitly requested.
- Do NOT reformat unrelated files.
- Preserve existing architecture and design.

---

## Priority Order (Very Important)
When conflicts occur, follow this priority:

1. Explicit user instructions
2. This AGENTS.md
3. Existing project style
4. General best practices