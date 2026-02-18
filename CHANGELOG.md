# Changelog

## Unreleased

### Added
- Comprehensive CTest suite covering pure-logic code paths without hardware.
  Four test executables registered with CTest:
  - `test_poll_group` -- poll group data structure operations (new/add/remove/free,
    duplicate rejection, capacity growth, NULL argument handling)
  - `test_rotation` -- coordinate transforms (identity, specific values, round-trip),
    level map transforms, bit map transforms, rotspec flags, dimension swapping
    under ROW_COL_SWAP
  - `test_monobright` -- `reduce_levels_to_bitmask()` threshold behavior (boundary
    values at 7/8, single-bit patterns, ascending/descending sequences)
  - `test_core` -- rotation get/set with 2-bit masking, string getters, fd getter,
    handler registration/unregistration, event grid extraction, REQUIRE macro
    (NULL capability -> UNSUPPORTED), bounds checking (OUT_OF_RANGE), LED/ring/tilt
    dispatch through mock function tables
- Poll group API for multi-device event handling (`monome_poll_group_*`).
  The existing event API (`monome_event_loop`, `monome_event_handle_next`)
  operates on a single `monome_t`, which means applications using multiple
  devices (e.g. a grid and an arc) had to manage their own fd multiplexing
  or resort to one thread per device. The poll group provides a unified
  mechanism to wait on multiple devices in a single call, dispatching
  registered event handlers as input arrives.
  - `monome_poll_group_new` / `monome_poll_group_free` -- lifecycle
  - `monome_poll_group_add` / `monome_poll_group_remove` -- dynamic
    membership; the group uses a growable array (initial capacity 4,
    doubles on overflow) and guards against duplicate entries
  - `monome_poll_group_wait` -- blocks until input is available on any
    member device (or until `timeout_ms` elapses; pass -1 for no timeout).
    Returns the number of events dispatched, 0 on timeout, or -1 on error.
    Platform-specific: uses `poll()` on Linux, `select()` on Darwin
  - `monome_poll_group_loop` -- convenience wrapper that calls
    `monome_poll_group_wait` in an infinite loop (shared POSIX
    implementation in `posix.c`)
- README.md (replaces plain-text README)

### Removed
- Plain-text README (replaced by README.md)

## 1.4.8 (2025-12-19)

### Added
- Arc intensity support
- Windows event loop
- CMake support for building C examples (#84)
- Makefile (standalone, replacing waf)

### Fixed
- Limit brightness to 4 bits for arc intensity
- `getopt_long` usage when `char` is unsigned (#86)
- AppleClang CFLAGS
- Python bindings compilation for Cython < 3.1
- Clang build fix
- CMake build fix (#81)
- Read timeout on Windows

### Changed
- Updated python bindings
- Removed waf build system
