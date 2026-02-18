/**
 * Tests for libmonome.c public API functions that don't require I/O.
 * Uses stack-allocated struct monome with mock function tables.
 */

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <monome.h>
#include "internal.h"

static int tests_run = 0;
static int tests_passed = 0;

#define RUN_TEST(fn) do { \
	tests_run++; \
	printf("  %-50s", #fn); \
	fn(); \
	tests_passed++; \
	printf("PASS\n"); \
} while(0)

/* --- mock LED stubs --- */

static int mock_led_called = 0;

static int mock_led_set(monome_t *m, uint_t x, uint_t y, uint_t on) {
	(void)m; (void)x; (void)y; (void)on;
	mock_led_called = 1;
	return MONOME_OK;
}

static int mock_led_all(monome_t *m, uint_t status) {
	(void)m; (void)status;
	mock_led_called = 1;
	return MONOME_OK;
}

static int mock_led_map(monome_t *m, uint_t x, uint_t y, const uint8_t *d) {
	(void)m; (void)x; (void)y; (void)d;
	mock_led_called = 1;
	return MONOME_OK;
}

static int mock_led_row(monome_t *m, uint_t x, uint_t y,
                        size_t c, const uint8_t *d) {
	(void)m; (void)x; (void)y; (void)c; (void)d;
	mock_led_called = 1;
	return MONOME_OK;
}

static int mock_led_col(monome_t *m, uint_t x, uint_t y,
                        size_t c, const uint8_t *d) {
	(void)m; (void)x; (void)y; (void)c; (void)d;
	mock_led_called = 1;
	return MONOME_OK;
}

static int mock_led_intensity(monome_t *m, uint_t b) {
	(void)m; (void)b;
	mock_led_called = 1;
	return MONOME_OK;
}

static monome_led_functions_t mock_led_fns = {
	.set       = mock_led_set,
	.all       = mock_led_all,
	.map       = mock_led_map,
	.row       = mock_led_row,
	.col       = mock_led_col,
	.intensity = mock_led_intensity
};

/* helper: build a zeroed monome with given dimensions */
static monome_t make_monome(int rows, int cols) {
	monome_t m;
	memset(&m, 0, sizeof(m));
	m.rows = rows;
	m.cols = cols;
	m.rotation = MONOME_ROTATE_0;
	return m;
}

/* --- rotation get/set tests --- */

static void test_rotation_get_set(void) {
	monome_t m = make_monome(8, 8);

	monome_set_rotation(&m, MONOME_ROTATE_90);
	assert(monome_get_rotation(&m) == MONOME_ROTATE_90);

	monome_set_rotation(&m, MONOME_ROTATE_270);
	assert(monome_get_rotation(&m) == MONOME_ROTATE_270);
}

static void test_rotation_masks_to_2_bits(void) {
	monome_t m = make_monome(8, 8);

	/* 5 & 3 = 1, so should store MONOME_ROTATE_90 */
	monome_set_rotation(&m, (monome_rotate_t)5);
	assert(monome_get_rotation(&m) == MONOME_ROTATE_90);

	/* 4 & 3 = 0 */
	monome_set_rotation(&m, (monome_rotate_t)4);
	assert(monome_get_rotation(&m) == MONOME_ROTATE_0);
}

/* --- string getter tests --- */

static void test_string_getters(void) {
	monome_t m = make_monome(8, 8);
	m.serial = "m1000123";
	m.device = "/dev/ttyUSB0";
	m.friendly = "monome 128";
	m.proto = "series";

	assert(monome_get_serial(&m) == m.serial);
	assert(monome_get_devpath(&m) == m.device);
	assert(monome_get_friendly_name(&m) == m.friendly);
	assert(monome_get_proto(&m) == m.proto);
}

static void test_string_getters_null(void) {
	monome_t m = make_monome(8, 8);
	/* all string fields are NULL from memset */
	assert(monome_get_serial(&m) == NULL);
	assert(monome_get_devpath(&m) == NULL);
	assert(monome_get_friendly_name(&m) == NULL);
	assert(monome_get_proto(&m) == NULL);
}

/* --- get_fd test --- */

static void test_get_fd(void) {
	monome_t m = make_monome(8, 8);
	m.fd = 42;
	assert(monome_get_fd(&m) == 42);
}

/* --- handler registration tests --- */

static void dummy_handler(const monome_event_t *e, void *data) {
	(void)e; (void)data;
}

static void test_register_handler(void) {
	monome_t m = make_monome(8, 8);
	int userdata = 99;
	int ret;

	ret = monome_register_handler(&m, MONOME_BUTTON_DOWN, dummy_handler,
	                              &userdata);
	assert(ret == 0);
	assert(m.handlers[MONOME_BUTTON_DOWN].cb == dummy_handler);
	assert(m.handlers[MONOME_BUTTON_DOWN].data == &userdata);
}

static void test_unregister_handler(void) {
	monome_t m = make_monome(8, 8);

	monome_register_handler(&m, MONOME_BUTTON_DOWN, dummy_handler, NULL);
	monome_unregister_handler(&m, MONOME_BUTTON_DOWN);

	assert(m.handlers[MONOME_BUTTON_DOWN].cb == NULL);
	assert(m.handlers[MONOME_BUTTON_DOWN].data == NULL);
}

static void test_register_out_of_range(void) {
	monome_t m = make_monome(8, 8);
	int ret;

	ret = monome_register_handler(&m, MONOME_EVENT_MAX, dummy_handler, NULL);
	assert(ret == EINVAL);

	ret = monome_register_handler(&m, (monome_event_type_t)255,
	                              dummy_handler, NULL);
	assert(ret == EINVAL);
}

/* --- event_get_grid test --- */

static void test_event_get_grid(void) {
	monome_t m = make_monome(8, 8);
	monome_event_t e;
	unsigned int x, y;
	monome_t *mp;

	e.monome = &m;
	e.grid.x = 3;
	e.grid.y = 5;

	monome_event_get_grid(&e, &x, &y, &mp);
	assert(x == 3);
	assert(y == 5);
	assert(mp == &m);
}

/* --- LED REQUIRE / bounds tests --- */

static void test_led_set_null_led(void) {
	monome_t m = make_monome(8, 8);
	/* m.led is NULL */
	assert(monome_led_set(&m, 0, 0, 1) == MONOME_ERROR_UNSUPPORTED);
}

static void test_led_all_null_led(void) {
	monome_t m = make_monome(8, 8);
	assert(monome_led_all(&m, 1) == MONOME_ERROR_UNSUPPORTED);
}

static void test_led_set_out_of_range(void) {
	monome_t m = make_monome(8, 8);
	m.led = &mock_led_fns;

	assert(monome_led_set(&m, 8, 0, 1) == MONOME_ERROR_OUT_OF_RANGE);
	assert(monome_led_set(&m, 0, 8, 1) == MONOME_ERROR_OUT_OF_RANGE);
	assert(monome_led_set(&m, 8, 8, 1) == MONOME_ERROR_OUT_OF_RANGE);
}

static void test_led_set_valid_dispatches(void) {
	monome_t m = make_monome(8, 8);
	m.led = &mock_led_fns;

	mock_led_called = 0;
	assert(monome_led_set(&m, 0, 0, 1) == MONOME_OK);
	assert(mock_led_called == 1);

	mock_led_called = 0;
	assert(monome_led_set(&m, 7, 7, 0) == MONOME_OK);
	assert(mock_led_called == 1);
}

static void test_led_all_dispatches(void) {
	monome_t m = make_monome(8, 8);
	m.led = &mock_led_fns;

	mock_led_called = 0;
	assert(monome_led_all(&m, 1) == MONOME_OK);
	assert(mock_led_called == 1);
}

static void test_led_map_bounds(void) {
	monome_t m = make_monome(8, 16);
	m.led = &mock_led_fns;
	uint8_t data[8] = {0};

	/* valid offset */
	mock_led_called = 0;
	assert(monome_led_map(&m, 0, 0, data) == MONOME_OK);
	assert(mock_led_called == 1);

	/* x_off out of range */
	assert(monome_led_map(&m, 16, 0, data) == MONOME_ERROR_OUT_OF_RANGE);
	/* y_off out of range */
	assert(monome_led_map(&m, 0, 8, data) == MONOME_ERROR_OUT_OF_RANGE);
}

static void test_led_row_bounds(void) {
	monome_t m = make_monome(8, 8);
	m.led = &mock_led_fns;
	uint8_t data[1] = {0xFF};

	mock_led_called = 0;
	assert(monome_led_row(&m, 0, 0, 1, data) == MONOME_OK);
	assert(mock_led_called == 1);

	assert(monome_led_row(&m, 0, 8, 1, data) == MONOME_ERROR_OUT_OF_RANGE);
}

static void test_led_col_bounds(void) {
	monome_t m = make_monome(8, 8);
	m.led = &mock_led_fns;
	uint8_t data[1] = {0xFF};

	mock_led_called = 0;
	assert(monome_led_col(&m, 0, 0, 1, data) == MONOME_OK);
	assert(mock_led_called == 1);

	assert(monome_led_col(&m, 8, 0, 1, data) == MONOME_ERROR_OUT_OF_RANGE);
}

/* --- ring / tilt REQUIRE tests --- */

static void test_led_ring_null(void) {
	monome_t m = make_monome(8, 8);
	uint8_t levels[64] = {0};

	assert(monome_led_ring_set(&m, 0, 0, 0) == MONOME_ERROR_UNSUPPORTED);
	assert(monome_led_ring_all(&m, 0, 0) == MONOME_ERROR_UNSUPPORTED);
	assert(monome_led_ring_map(&m, 0, levels) == MONOME_ERROR_UNSUPPORTED);
	assert(monome_led_ring_range(&m, 0, 0, 10, 5) == MONOME_ERROR_UNSUPPORTED);
	assert(monome_led_ring_intensity(&m, 10) == MONOME_ERROR_UNSUPPORTED);
}

static void test_tilt_null(void) {
	monome_t m = make_monome(8, 8);

	assert(monome_tilt_enable(&m, 0) == MONOME_ERROR_UNSUPPORTED);
	assert(monome_tilt_disable(&m, 0) == MONOME_ERROR_UNSUPPORTED);
}

/* --- led_level REQUIRE tests --- */

static void test_led_level_null(void) {
	monome_t m = make_monome(8, 8);
	uint8_t data[64] = {0};

	assert(monome_led_level_set(&m, 0, 0, 0) == MONOME_ERROR_UNSUPPORTED);
	assert(monome_led_level_all(&m, 0) == MONOME_ERROR_UNSUPPORTED);
	assert(monome_led_level_map(&m, 0, 0, data) == MONOME_ERROR_UNSUPPORTED);
	assert(monome_led_level_row(&m, 0, 0, 1, data) == MONOME_ERROR_UNSUPPORTED);
	assert(monome_led_level_col(&m, 0, 0, 1, data) == MONOME_ERROR_UNSUPPORTED);
}

/* --- on/off convenience wrappers --- */

static void test_led_on_off(void) {
	monome_t m = make_monome(8, 8);
	m.led = &mock_led_fns;

	mock_led_called = 0;
	assert(monome_led_on(&m, 3, 4) == MONOME_OK);
	assert(mock_led_called == 1);

	mock_led_called = 0;
	assert(monome_led_off(&m, 3, 4) == MONOME_OK);
	assert(mock_led_called == 1);

	/* out of range */
	assert(monome_led_on(&m, 8, 0) == MONOME_ERROR_OUT_OF_RANGE);
	assert(monome_led_off(&m, 0, 8) == MONOME_ERROR_OUT_OF_RANGE);
}

/* --- intensity dispatch --- */

static void test_led_intensity_dispatch(void) {
	monome_t m = make_monome(8, 8);
	m.led = &mock_led_fns;

	mock_led_called = 0;
	assert(monome_led_intensity(&m, 10) == MONOME_OK);
	assert(mock_led_called == 1);
}

int main(void) {
	printf("test_core:\n");

	RUN_TEST(test_rotation_get_set);
	RUN_TEST(test_rotation_masks_to_2_bits);
	RUN_TEST(test_string_getters);
	RUN_TEST(test_string_getters_null);
	RUN_TEST(test_get_fd);
	RUN_TEST(test_register_handler);
	RUN_TEST(test_unregister_handler);
	RUN_TEST(test_register_out_of_range);
	RUN_TEST(test_event_get_grid);
	RUN_TEST(test_led_set_null_led);
	RUN_TEST(test_led_all_null_led);
	RUN_TEST(test_led_set_out_of_range);
	RUN_TEST(test_led_set_valid_dispatches);
	RUN_TEST(test_led_all_dispatches);
	RUN_TEST(test_led_map_bounds);
	RUN_TEST(test_led_row_bounds);
	RUN_TEST(test_led_col_bounds);
	RUN_TEST(test_led_ring_null);
	RUN_TEST(test_tilt_null);
	RUN_TEST(test_led_level_null);
	RUN_TEST(test_led_on_off);
	RUN_TEST(test_led_intensity_dispatch);

	printf("\n%d/%d tests passed\n", tests_passed, tests_run);
	return (tests_passed == tests_run) ? 0 : 1;
}
