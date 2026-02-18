/**
 * Tests for coordinate rotation transforms, level map transforms,
 * and bit map transforms. Exercises rotspec[] without hardware.
 */

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include <monome.h>
#include "internal.h"
#include "rotation.h"

static int tests_run = 0;
static int tests_passed = 0;

#define RUN_TEST(fn) do { \
	tests_run++; \
	printf("  %-50s", #fn); \
	fn(); \
	tests_passed++; \
	printf("PASS\n"); \
} while(0)

/* helper: build a fake monome with given dimensions and rotation */
static monome_t make_monome(int rows, int cols, monome_rotate_t rot) {
	monome_t m;
	memset(&m, 0, sizeof(m));
	m.rows = rows;
	m.cols = cols;
	m.rotation = rot;
	return m;
}

/* --- coordinate transform tests --- */

static void test_r0_identity(void) {
	monome_t m = make_monome(8, 8, MONOME_ROTATE_0);
	uint_t x, y;

	x = 3; y = 5;
	rotspec[0].output_cb(&m, &x, &y);
	assert(x == 3 && y == 5);

	x = 0; y = 0;
	rotspec[0].input_cb(&m, &x, &y);
	assert(x == 0 && y == 0);

	x = 7; y = 7;
	rotspec[0].output_cb(&m, &x, &y);
	assert(x == 7 && y == 7);
}

static void test_r0_identity_rect(void) {
	monome_t m = make_monome(8, 16, MONOME_ROTATE_0);
	uint_t x, y;

	x = 15; y = 7;
	rotspec[0].output_cb(&m, &x, &y);
	assert(x == 15 && y == 7);

	rotspec[0].input_cb(&m, &x, &y);
	assert(x == 15 && y == 7);
}

static void test_coord_roundtrip_all_rotations(void) {
	/* use 8x8 square grid so dimensions are invariant under rotation */
	monome_t m = make_monome(8, 8, MONOME_ROTATE_0);
	uint_t corners[][2] = {{0,0}, {7,7}, {3,5}, {0,7}, {7,0}};
	int r, c;

	for (r = 0; r < 4; r++) {
		m.rotation = r;
		for (c = 0; c < 5; c++) {
			uint_t x = corners[c][0], y = corners[c][1];
			uint_t ox = x, oy = y;

			rotspec[r].output_cb(&m, &x, &y);
			rotspec[r].input_cb(&m, &x, &y);

			assert(x == ox && y == oy);
		}
	}
}

static void test_r90_specific_8x8(void) {
	/* on 8x8, r90 output: (x,y) -> (y, cols-1-x) = (y, 7-x) */
	monome_t m = make_monome(8, 8, MONOME_ROTATE_90);
	uint_t x = 3, y = 5;

	rotspec[1].output_cb(&m, &x, &y);
	assert(x == 5 && y == 4);
}

static void test_r180_specific_8x8(void) {
	/* on 8x8, r180 output: (x,y) -> (7-x, 7-y) */
	monome_t m = make_monome(8, 8, MONOME_ROTATE_180);
	uint_t x = 2, y = 3;

	rotspec[2].output_cb(&m, &x, &y);
	assert(x == 5 && y == 4);
}

static void test_r270_specific_8x8(void) {
	/* on 8x8, r270 output: (x,y) -> (rows-1-y, x) = (7-y, x) */
	monome_t m = make_monome(8, 8, MONOME_ROTATE_270);
	uint_t x = 3, y = 5;

	rotspec[3].output_cb(&m, &x, &y);
	assert(x == 2 && y == 3);
}

/* --- level map tests --- */

static void test_level_map_r0_identity(void) {
	monome_t m = make_monome(8, 8, MONOME_ROTATE_0);
	uint8_t src[64], dst[64];
	int i;

	for (i = 0; i < 64; i++)
		src[i] = (uint8_t)i;

	rotspec[0].level_map_cb(&m, dst, src);
	assert(memcmp(dst, src, 64) == 0);
}

static void test_level_map_r180_reversal(void) {
	monome_t m = make_monome(8, 8, MONOME_ROTATE_180);
	uint8_t src[64], dst[64];
	int i;

	for (i = 0; i < 64; i++)
		src[i] = (uint8_t)i;

	rotspec[2].level_map_cb(&m, dst, src);

	for (i = 0; i < 64; i++)
		assert(dst[63 - i] == src[i]);
}

static void test_level_map_r90_r270_roundtrip(void) {
	monome_t m = make_monome(8, 8, MONOME_ROTATE_0);
	uint8_t src[64], tmp[64], dst[64];
	int i;

	for (i = 0; i < 64; i++)
		src[i] = (uint8_t)(i * 3 + 7);

	rotspec[1].level_map_cb(&m, tmp, src);
	rotspec[3].level_map_cb(&m, dst, tmp);

	assert(memcmp(dst, src, 64) == 0);
}

static void test_level_map_r180_double_identity(void) {
	monome_t m = make_monome(8, 8, MONOME_ROTATE_180);
	uint8_t src[64], tmp[64], dst[64];
	int i;

	for (i = 0; i < 64; i++)
		src[i] = (uint8_t)(i ^ 0xAA);

	rotspec[2].level_map_cb(&m, tmp, src);
	rotspec[2].level_map_cb(&m, dst, tmp);

	assert(memcmp(dst, src, 64) == 0);
}

/* --- bit map tests --- */

static void test_map_r0_identity(void) {
	monome_t m = make_monome(8, 8, MONOME_ROTATE_0);
	uint8_t data[8] = {0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80};
	uint8_t orig[8];

	memcpy(orig, data, 8);
	rotspec[0].map_cb(&m, data);
	assert(memcmp(data, orig, 8) == 0);
}

static void test_map_r90_r270_roundtrip(void) {
	monome_t m = make_monome(8, 8, MONOME_ROTATE_0);
	uint8_t data[8] = {0xFF, 0x00, 0xAA, 0x55, 0x0F, 0xF0, 0x33, 0xCC};
	uint8_t orig[8];

	memcpy(orig, data, 8);

	rotspec[1].map_cb(&m, data);
	rotspec[3].map_cb(&m, data);

	assert(memcmp(data, orig, 8) == 0);
}

static void test_map_r180_double_identity(void) {
	monome_t m = make_monome(8, 8, MONOME_ROTATE_180);
	uint8_t data[8] = {0xDE, 0xAD, 0xBE, 0xEF, 0xCA, 0xFE, 0xBA, 0xBE};
	uint8_t orig[8];

	memcpy(orig, data, 8);

	rotspec[2].map_cb(&m, data);
	rotspec[2].map_cb(&m, data);

	assert(memcmp(data, orig, 8) == 0);
}

/* --- flag tests --- */

static void test_rotspec_flags(void) {
	assert(rotspec[0].flags == 0);
	assert(rotspec[1].flags & ROW_COL_SWAP);
	assert(!(rotspec[2].flags & ROW_COL_SWAP));
	assert(rotspec[3].flags & ROW_COL_SWAP);
	assert(rotspec[1].flags & ROW_REVBITS);
	assert(rotspec[2].flags & ROW_REVBITS);
	assert(rotspec[2].flags & COL_REVBITS);
	assert(rotspec[3].flags & COL_REVBITS);
}

/* --- dimension swap tests --- */

static void test_dimension_swap(void) {
	monome_t m = make_monome(8, 16, MONOME_ROTATE_0);

	assert(monome_get_rows(&m) == 8);
	assert(monome_get_cols(&m) == 16);

	m.rotation = MONOME_ROTATE_90;
	assert(monome_get_rows(&m) == 16);
	assert(monome_get_cols(&m) == 8);

	m.rotation = MONOME_ROTATE_180;
	assert(monome_get_rows(&m) == 8);
	assert(monome_get_cols(&m) == 16);

	m.rotation = MONOME_ROTATE_270;
	assert(monome_get_rows(&m) == 16);
	assert(monome_get_cols(&m) == 8);
}

int main(void) {
	printf("test_rotation:\n");

	RUN_TEST(test_r0_identity);
	RUN_TEST(test_r0_identity_rect);
	RUN_TEST(test_coord_roundtrip_all_rotations);
	RUN_TEST(test_r90_specific_8x8);
	RUN_TEST(test_r180_specific_8x8);
	RUN_TEST(test_r270_specific_8x8);
	RUN_TEST(test_level_map_r0_identity);
	RUN_TEST(test_level_map_r180_reversal);
	RUN_TEST(test_level_map_r90_r270_roundtrip);
	RUN_TEST(test_level_map_r180_double_identity);
	RUN_TEST(test_map_r0_identity);
	RUN_TEST(test_map_r90_r270_roundtrip);
	RUN_TEST(test_map_r180_double_identity);
	RUN_TEST(test_rotspec_flags);
	RUN_TEST(test_dimension_swap);

	printf("\n%d/%d tests passed\n", tests_passed, tests_run);
	return (tests_passed == tests_run) ? 0 : 1;
}
