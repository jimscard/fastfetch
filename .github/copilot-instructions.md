# GitHub Copilot instructions for fastfetch

This file gives focused, non-generic guidance for Copilot sessions operating on the fastfetch repository. It pulls important, actionable details from README.md and CONTRIBUTING.md so Copilot agents can build, test, and change the code reliably.

---

## Build, test and lint commands (concrete)

Quick script (recommended for local iterative development):

- ./run.sh                # builds and runs (convenience wrapper used by contributors)

Manual build (recommended):

- Configure + build fastfetch
  - cmake -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
  - cmake --build build --target fastfetch -j$(nproc)
  - ./build/fastfetch

- Configure with tests enabled (CI uses similar flags):
  - cmake -B build -DBUILD_TESTS=On -DCMAKE_BUILD_TYPE=RelWithDebInfo [other -D flags...]
  - cmake --build build --target package -j$(nproc)

- Run tests
  - Run all tests: cd build && ctest --output-on-failure
  - Run a single test (by name/regex): cd build && ctest -R '<regex>' --output-on-failure
    (ctest -R uses a regular expression; examples in CI use plain `ctest --output-on-failure`)

- Packaging
  - cmake --build build --target package    # produces fastfetch-* artifacts (CPack)

- Spellcheck (used by CI):
  - pip3 install codespell && codespell

- Formatting helpers (style configuration present as .clang-format):
  - clang-format -i <files>   # the repo provides .clang-format; apply as needed

Notes: CI config uses many explicit -DENABLE_* and MODULE_DISABLE_<NAME> flags to produce minimal-dependency builds. See .github/workflows/* for concrete examples.

---

## High-level architecture (big picture)

- Language & focus: C (C23), portable CLI tool for system information with a design goal of minimal startup time and optional dependencies.

- Layers
  - src/fastfetch.c — entry point, CLI parsing and module dispatch
  - src/modules/ — module layer: formatting, JSON output, and per-module options/formatters
  - src/detection/ — detection layer: platform-specific data collection (one platform file per OS), returns minimal result structs
  - src/common/ — utilities: FFstrbuf, FFlist, dlopen helpers, formatting helpers
  - src/logo/ and src/3rdparty/ — assets and bundled libraries (yyjson, sixel, etc.)

- Flow: fastfetch parses options -> modules request data via detection interfaces -> detection implementations (one per platform) populate result structs -> modules format/print/generate JSON.

- Packaging: CPack/package target used in CI to create distributable binaries; `flashfetch` is a small stripped-down helper binary also built by default.

---

## Key repository-specific conventions

- Strict separation: `modules/` (format/print) vs `detection/` (data retrieval). Avoid cross-layer responsibilities: detection code must not print; modules must not perform raw platform syscalls. Follow that boundary in changes.

- Detection return convention: platform detection functions return `const char*` error strings (nullptr => success). Keep that pattern when adding implementations.

- Module registration: each module exposes a single `FFModuleBaseInfo` descriptor (hand-written vtable). The repository relies on this C polymorphism; do not attempt large refactors that remove or rewrite the pattern without full cross-file changes.

- Optional deps: many features are `dlopen`-ed at runtime (`BINARY_LINK_TYPE=dlopen` default). When adding optional dependencies, use the repo's `common/library.h` dlopen helpers rather than linking directly.

- Build flags and names to know (used across CI and docs):
  - BUILD_TESTS, BUILD_FLASHFETCH, ENABLE_LTO, ENABLE_ASAN
  - ENABLE_<FEATURE>=ON/OFF (VULKAN, WAYLAND, X11, DBUS, IMAGEMAGICK, etc.)
  - MODULE_DISABLE_<NAME>=ON/OFF (disable specific modules)

- LTO behavior: link-time optimization (ENABLE_LTO=ON) is used to strip out disabled modules; building with LTO affects symbol visibility and dead-code elimination—be careful when incrementally testing changes (use Debug to avoid LTO if needed).

- Presets and configuration: configs are JSONC in `presets/` and runtime config uses JSONC (`--gen-config`) — prefer the preset files for CI/testing.

- Tests: `tests/` contains unit tests; CI runs `ctest` from the build directory. Use `ctest -R <regex>` to run a single test.

---

## Where to look first when making changes

- Feature (module) change: src/modules/<name>/ + src/detection/<name>/
- Add platform: implement src/detection/<feature>/<feature>_<platform>.c and update CMakeLists where platform blocks are enumerated
- Build/debug: run ./run.sh for quick iteration, or use cmake -B build; run tests with ctest

---

This file was assembled from README.md and CONTRIBUTING.md to provide Copilot sessions with the concrete commands, high-level structure, and repo-specific conventions they need. If you'd like, Copilot can also:
- Add short shell snippets for common tasks (create build dir, run a single module locally)
- Extend the instructions with examples of `ctest -R` regex patterns or a catalogue of common CMake flags used in CI

Would you like any adjustments or extra coverage (examples, more CI details, or module-specific patterns)?
