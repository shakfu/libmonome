# libmonome

libmonome is a library for easy interaction with monome devices. It currently runs on Linux (on which it is primarily developed), Darwin/macOS, Windows, and OpenBSD.

It was developed to make monome devices easy to use with programming languages like C and Python, and adding wrappers for use in your favorite language with a suitable FFI is straightforward.

libmonome has support for 40h and series devices through a unified API, and by default includes a third backend which wraps around the OSC protocol and presents the same API as a physical device. This means that a program written using libmonome can, at runtime, decide whether to communicate with a running monomeserial instance over OSC or whether to access the physical device directly.

## Platform support

| Platform | Device detection | Poll group `wait` |
|----------|-----------------|-------------------|
| Linux    | libudev         | `poll()`          |
| macOS    | IOKit           | `select()`        |
| Windows  | SetupAPI        | `WaitForMultipleObjects` |
| OpenBSD  | --              | --                |

## Dependencies

- **CMake** >= 3.27
- **C17** compiler
- **Linux only**: `libudev` (via pkg-config)
- **Windows only**: `setupapi` (part of the Windows SDK)

No external dependencies are required on macOS.

## Build

```sh
mkdir build; cd build
cmake ..
make
sudo make install
```

## Single-device usage

Open a device, register an event handler, and enter the event loop. The callback fires whenever the registered event occurs.

### Direct serial access

```c
#include <stdlib.h>
#include <monome.h>

unsigned int grid[16][16] = {0};

void handle_press(const monome_event_t *e, void *data) {
    unsigned int x = e->grid.x;
    unsigned int y = e->grid.y;

    grid[x][y] = !grid[x][y];
    monome_led_set(e->monome, x, y, grid[x][y]);
}

int main(int argc, char *argv[]) {
    monome_t *monome;

    if( !(monome = monome_open("/dev/ttyUSB0")) )
        return -1;

    monome_led_all(monome, 0);
    monome_register_handler(monome, MONOME_BUTTON_DOWN, handle_press, NULL);
    monome_event_loop(monome);

    monome_close(monome);
    return 0;
}
```

### Over OSC

To communicate with a device through monomeserial over OSC, pass an `osc.udp://` URI and a local port for receiving messages:

```c
int main(int argc, char *argv[]) {
    monome_t *monome;

    if( !(monome = monome_open("osc.udp://127.0.0.1:8080/monome", "8000")) )
        return -1;

    monome_led_all(monome, 0);
    monome_register_handler(monome, MONOME_BUTTON_DOWN, handle_press, NULL);
    monome_event_loop(monome);

    monome_close(monome);
    return 0;
}
```

## Multi-device usage (poll groups)

The single-device event API (`monome_event_loop`, `monome_event_handle_next`) operates on one device at a time. For example, with a single grid:

```c
monome_t *grid;

/* direct serial */
grid = monome_open("/dev/ttyUSB0");

/* or over OSC */
grid = monome_open("osc.udp://127.0.0.1:8080/monome", "8000");

monome_register_handler(grid, MONOME_BUTTON_DOWN, handle_press, NULL);
monome_event_loop(grid);
```

If your application uses multiple devices -- for example a grid and an arc -- you can use the poll group API to wait on all of them in a single call.

### Direct serial access

```c
#include <stdlib.h>
#include <monome.h>

void handle_grid_press(const monome_event_t *e, void *data) {
    unsigned int x = e->grid.x;
    unsigned int y = e->grid.y;
    monome_led_set(e->monome, x, y, 1);
}

void handle_enc_delta(const monome_event_t *e, void *data) {
    /* respond to encoder turn */
}

int main(int argc, char *argv[]) {
    monome_t *grid, *arc;
    monome_poll_group_t *group;

    if( !(grid = monome_open("/dev/ttyUSB0")) )
        return -1;
    if( !(arc = monome_open("/dev/ttyUSB1")) ) {
        monome_close(grid);
        return -1;
    }

    monome_register_handler(grid, MONOME_BUTTON_DOWN, handle_grid_press, NULL);
    monome_register_handler(arc, MONOME_ENCODER_DELTA, handle_enc_delta, NULL);

    group = monome_poll_group_new();
    monome_poll_group_add(group, grid);
    monome_poll_group_add(group, arc);

    /* blocks until input arrives on either device, dispatches handlers */
    monome_poll_group_loop(group);

    monome_poll_group_free(group);
    monome_close(arc);
    monome_close(grid);
    return 0;
}
```

### Over OSC

When communicating with devices through monomeserial over OSC, each device gets its own UDP endpoint. The second argument to `monome_open` sets the local port for receiving messages:

```c
int main(int argc, char *argv[]) {
    monome_t *grid, *arc;
    monome_poll_group_t *group;

    if( !(grid = monome_open("osc.udp://127.0.0.1:8080/monome", "8000")) )
        return -1;
    if( !(arc = monome_open("osc.udp://127.0.0.1:8081/monome", "8001")) ) {
        monome_close(grid);
        return -1;
    }

    monome_register_handler(grid, MONOME_BUTTON_DOWN, handle_grid_press, NULL);
    monome_register_handler(arc, MONOME_ENCODER_DELTA, handle_enc_delta, NULL);

    group = monome_poll_group_new();
    monome_poll_group_add(group, grid);
    monome_poll_group_add(group, arc);

    monome_poll_group_loop(group);

    monome_poll_group_free(group);
    monome_close(arc);
    monome_close(grid);
    return 0;
}
```

### Using `monome_poll_group_wait` directly

For finer control, use `monome_poll_group_wait` instead of the loop convenience wrapper. It returns the number of events dispatched, 0 on timeout, or -1 on error:

```c
int ret;
while( (ret = monome_poll_group_wait(group, 1000)) >= 0 ) {
    if( ret == 0 ) {
        /* timeout -- do periodic work here */
    }
}
```

Devices can be added and removed from a group dynamically with `monome_poll_group_add` and `monome_poll_group_remove`.

## Language bindings

- **Python** -- `bindings/python/`
- **Rust** -- `bindings/rust/`

Adding bindings for other languages with a suitable FFI is straightforward since the public API is a flat C interface with an opaque `monome_t` handle.

## License

ISC. See [COPYRIGHT](COPYRIGHT) for the full text.

## Author

libmonome was written by william light (visinin on the monome forums). You can contact him at <wrl@illest.net>.
