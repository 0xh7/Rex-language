# Rex CLI Reference

All commands below are run from the `rex` directory:

```bash
cd rex
lua compiler/cli/rex.lua <command> ...
```

## 1. `init`

Create a new project skeleton.

```bash
lua compiler/cli/rex.lua init <dir>
```

Creates:
- `<dir>/rex.toml`
- `<dir>/src/main.rex`

## 2. `build`

Parse + typecheck + generate C, and (by default) compile native binary.

```bash
lua compiler/cli/rex.lua build [input] [options]
```

Default input:
- `src/main.rex`

Options:
- `--out <path>`: native output path
- `--c-out <path>`: generated C output path
- `--no-entry`: generate code without `main` entry wrapper
- `--no-native`: skip native compilation (generate C only)
- `--cc <compiler>`: C compiler command
- `--mode release|debug`: build mode

Examples:

```bash
lua compiler/cli/rex.lua build examples/hello.rex
lua compiler/cli/rex.lua build examples/hello.rex --no-native --c-out build/hello.c
lua compiler/cli/rex.lua build src/main.rex --mode debug --cc clang
```

## 3. `run`

Build then execute a Rex file.

```bash
lua compiler/cli/rex.lua run [input] [options]
```

Default input:
- `src/main.rex`

Options:
- `--out <path>`
- `--c-out <path>`
- `--cc <compiler>`
- `--mode release|debug`

Example:

```bash
lua compiler/cli/rex.lua run examples/hello.rex
```

## 4. `bench`

Run benchmark file multiple times and report elapsed stats.

```bash
lua compiler/cli/rex.lua bench [input] [options]
```

Default input:
- `examples/benchmark.rex`

Options:
- `--runs <n>` (must be >= 1)
- `--cc <compiler>`
- `--mode release|debug`

Example:

```bash
lua compiler/cli/rex.lua bench examples/benchmark.rex --runs 10
```

## 5. `test`

Build all example files to C.

```bash
lua compiler/cli/rex.lua test
```

This validates parsing/typechecking/codegen across the example set.

## 6. `fmt`

Format a source file (currently trims trailing spaces and normalizes ending newline).

```bash
lua compiler/cli/rex.lua fmt [input]
```

Default input:
- `src/main.rex`

## 7. `lint`

Run parser + typechecker validation.

```bash
lua compiler/cli/rex.lua lint [input]
```

Default input:
- `src/main.rex`

## 8. `check`

Same validation pipeline as lint, intended as quick correctness check.

```bash
lua compiler/cli/rex.lua check [input]
```

Default input:
- `src/main.rex`

## 9. Build Modes

- `release`: optimized build (default)
- `debug`: debug-friendly build flags

You can also set mode through environment:
- `REX_BUILD_MODE`
- `REX_MODE`

## 10. Environment Variables

- `CC`: default C compiler
- `REX_BUILD_MODE` / `REX_MODE`: default build mode
- `REX_CFLAGS`: extra C compiler flags
- `REX_OPT_FLAG`: override optimization behavior

## 11. Include Preprocessing

Before lexing/parsing, Rex can expand includes using comments:

```rex
// @include "relative/path.rex"
```

Include cycles are detected and reported as errors.
