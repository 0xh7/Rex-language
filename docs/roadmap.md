# Rex Roadmap

This roadmap defines what Rex must stabilize before `1.0`, what limitations are
acceptable in `1.0`, and what work is explicitly post-`1.0`.

## 1. Release Position

- Rex already ships real releases, package workflows, and installers.
- `1.0`  it is a stability contract for syntax,
  CLI behavior, package rules, and core runtime expectations.
- Before `1.0`, Rex should prefer bug fixes, diagnostics, tests, and doc sync over
  large new syntax families.

## 2. Must Fix Before `1.0`

### Language and Compiler
- Tighten diagnostics for common parser/typechecker failures.
- Expand parser/typechecker coverage beyond example-only regression sweeps.
- Freeze the current syntax direction: keyword-first, explicit, low-punctuation, and inspectable.
- Decide and document whether the current numeric model is the intended `1.0` rule.

### Project and Package Workflow
- Make version semantics explicit everywhere:
  - `rex.toml` `version` is the project/package version
  - Rex CLI/language release version is separate
- Keep manifest-aware workflows stable for `init`, `build`, `run`, `check`, `install`, and `deps`.
- Freeze package import rules and package limitations as documented behavior.

### Standard Library and Runtime
- Continue hardening `io`, `fs`, `json`, `http`, and `collections`.
- Improve consistency across module naming, signatures, and error behavior.
- Add more runtime-level tests for platform-specific functionality.

### Tooling and Releases
- Provide a visible CLI version surface.
- Keep release artifacts, installer defaults, and docs aligned with the published release line.
- Improve VS Code basics enough for daily editing and diagnostics.

### Documentation
- Keep docs aligned with implemented behavior after each significant change.
- Make `README`, `spec`, `package-manager-v1`, and CLI docs agree on what Rex `1.0` means.
- Replace vague “short/mid/long term” planning with explicit `1.0` gates.

## 3. Accepted `1.0` Limitations

- No public registry
- No feature flags
- No workspaces
- No semantic-version solver
- No remote publishing flow
- Package imports remain single-segment
- Imported package struct literals still require constructors
- Dependency entry-file exports remain the default public package surface
- Dependency type names may need to stay unique across the resolved graph
- `rex fmt` remains intentionally minimal
- `rex test` remains example/regression oriented rather than a full test runner

## 4. Explicitly Post-`1.0`

- Package hosting and distribution services
- Richer language tooling hooks such as deep navigation/integration
- Additional backends and compile pipeline experiments
- Performance and memory profiling workflows
- Large type-system redesigns or new syntax families

## 5. Change Control Before `1.0`

- Prefer diagnostics, tests, runtime hardening, and documentation over syntax growth.
- Any pre-`1.0` syntax addition must preserve Rex's current style instead of importing a foreign syntax family.
- If a limitation is acceptable for `1.0`, document it explicitly instead of leaving it ambiguous.
