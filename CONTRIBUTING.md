# Contributing to Rex

This repository contains the Rex language frontend, the C runtime used by
generated programs, and the example programs used as regression coverage.

Contributions should keep the compiler, runtime, examples, and docs aligned.

## 1. Scope

Typical contribution areas:
- parser, typechecker, and code generation changes under `rex/compiler`
- runtime changes under `rex/runtime_c`
- example coverage under `rex/examples`
- language and tooling documentation under `docs`
- installer and release workflow updates under `tools` and `.github/workflows`

## 2. Local Setup

Required tools:
- Lua 5.4+ (or compatible Lua runtime)
- a working C compiler (`cc`, `clang`, or `gcc`)

Common local commands from the repo root:

```bash
cd rex
lua compiler/cli/rex.lua run examples/hello.rex
lua compiler/cli/rex.lua check examples/hello.rex
lua compiler/cli/rex.lua test
```

## 3. Change Guidelines

When changing language behavior:
- update the parser, typechecker, and codegen together
- add or update example coverage in `rex/examples`
- update docs in `docs` when syntax or behavior changes
- prefer small, focused commits over unrelated mixed changes

When changing runtime behavior:
- keep public behavior consistent across platforms when possible
- avoid breaking existing example programs unless the change is intentional

When changing documentation:
- document current behavior, not aspirational behavior
- keep examples in docs consistent with the compiler as implemented today

## 4. Testing Before Push

At minimum, run:

```bash
cd rex
lua compiler/cli/rex.lua test
```

For syntax or semantic changes, also run the affected examples directly:

```bash
cd rex
lua compiler/cli/rex.lua run examples/test_compound_assign.rex
lua compiler/cli/rex.lua run examples/test_nested_assign.rex
lua compiler/cli/rex.lua run examples/test_struct_lit.rex
lua compiler/cli/rex.lua run examples/test_multi_match.rex
lua compiler/cli/rex.lua run examples/test_wildcard_match.rex
```

If a change affects another subsystem, run the closest relevant examples there
as well.

## 5. Examples and Regression Coverage

Examples in `rex/examples` are part of the stability story for the language.

For new syntax or changed semantics:
- add a focused sample that demonstrates the intended behavior
- keep the sample small and readable
- use example names that describe the feature being covered

## 6. Documentation Rules

When a change affects users:
- update `README.md` if the public entry points or release process changed
- update the relevant guides in `docs`
- avoid conversational or assistant-like wording in project docs
- keep documentation written in a neutral project-maintainer tone

## 7. Commit and Review Expectations

Before pushing:
- review `git diff` and make sure no unrelated files are included
- keep generated artifacts out of commits unless intentionally tracked
- separate docs-only changes from compiler/runtime changes when practical

Commit messages should describe the actual change clearly, for example:
- `Add struct literals and richer match arms`
- `Fix mixed index-member assignment targets`
- `Refresh docs for current language features`

## 8. Releases

Releases are handled through `.github/workflows/release.yml`.

Before moving a release tag:
- make sure the intended commit is already on `main`
- confirm docs and examples match the released language behavior
- confirm the release notes reflect the actual shipped changes

If a release description needs custom text, edit the GitHub release body
directly or update the workflow to provide an explicit release body.
