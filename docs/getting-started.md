# Getting Started with Rex

This guide gets you from zero to running your first Rex program in a few minutes.

## 1. Prerequisites

Install:
- Lua 5.4+ (or compatible runtime)
- A C compiler (`cc`, `clang`, or `gcc`)

Then open a terminal in the repository root.

## 2. Run an Existing Example

From the project root:

```bash
cd rex
lua compiler/cli/rex.lua run examples/hello.rex
```

You should see output similar to:
- `Hello from Rex`
- elapsed time in milliseconds

## 3. Check a File (Types + Ownership)

Before running a program, use `check`:

```bash
cd rex
lua compiler/cli/rex.lua check examples/hello.rex
```

If everything is valid, the command prints `OK ...`.

## 4. Create a New Project

Initialize a project folder:

```bash
cd rex
lua compiler/cli/rex.lua init my-app
```

This creates:
- `my-app/rex.toml`
- `my-app/src/main.rex`

## 5. Build and Run Your Own File

Build to C and native binary:

```bash
cd rex
lua compiler/cli/rex.lua build my-app/src/main.rex
```

Run directly:

```bash
cd rex
lua compiler/cli/rex.lua run my-app/src/main.rex
```

## 6. Useful Daily Commands

- Format source:
  - `lua compiler/cli/rex.lua fmt path/to/file.rex`
- Lint (same validation pipeline used in check):
  - `lua compiler/cli/rex.lua lint path/to/file.rex`
- Build all examples to generated C:
  - `lua compiler/cli/rex.lua test`

## 7. Where to Go Next

- Syntax: `docs/syntax.md`
- Ownership: `docs/ownership.md`
- Standard library: `docs/stdlib.md`
- Full command list: `docs/cli-reference.md`
- Common errors and fixes: `docs/troubleshooting.md`
