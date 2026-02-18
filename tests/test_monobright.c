/**
 * Tests for reduce_levels_to_bitmask() in monobright.c.
 * Verifies threshold behavior (level > 7 maps to bit set).
 */

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include <monome.h>
#include "internal.h"
#include "monobright.h"

static int tests_run = 0;
static int tests_passed = 0;

#define RUN_TEST(fn) do { \
	tests_run++; \
	printf("  %-50s", #fn); \
	fn(); \
	tests_passed++; \
	printf("PASS\n"); \
} while(0)

static void test_all_zeros(void) {
	uint8_t levels[8] = {0, 0, 0, 0, 0, 0, 0, 0};
	assert(reduce_levels_to_bitmask(levels) == 0x00);
}

static void test_all_max(void) {
	uint8_t levels[8] = {255, 255, 255, 255, 255, 255, 255, 255};
	assert(reduce_levels_to_bitmask(levels) == 0xFF);
}

static void test_all_sevens(void) {
	uint8_t levels[8] = {7, 7, 7, 7, 7, 7, 7, 7};
	assert(reduce_levels_to_bitmask(levels) == 0x00);
}

static void test_all_eights(void) {
	uint8_t levels[8] = {8, 8, 8, 8, 8, 8, 8, 8};
	assert(reduce_levels_to_bitmask(levels) == 0xFF);
}

static void test_single_bit_patterns(void) {
	int i;
	for (i = 0; i < 8; i++) {
		uint8_t levels[8] = {0, 0, 0, 0, 0, 0, 0, 0};
		levels[i] = 8;
		assert(reduce_levels_to_bitmask(levels) == (1 << i));
	}
}

static void test_ascending_pattern(void) {
	/* {0,1,4,7,8,9,15,255} -> bits 4-7 set -> 0xF0 */
	uint8_t levels[8] = {0, 1, 4, 7, 8, 9, 15, 255};
	assert(reduce_levels_to_bitmask(levels) == 0xF0);
}

static void test_descending_boundary(void) {
	/* {255,15,9,8,7,4,1,0} -> bits 0-3 set -> 0x0F */
	uint8_t levels[8] = {255, 15, 9, 8, 7, 4, 1, 0};
	assert(reduce_levels_to_bitmask(levels) == 0x0F);
}

static void test_alternating(void) {
	/* even indices at threshold+1, odd at threshold */
	uint8_t levels[8] = {8, 7, 8, 7, 8, 7, 8, 7};
	assert(reduce_levels_to_bitmask(levels) == 0x55);
}

static void test_macro_consistency(void) {
	/* verify the macro agrees with the function */
	assert(reduce_level_to_bit(0) == 0);
	assert(reduce_level_to_bit(7) == 0);
	assert(reduce_level_to_bit(8) == 1);
	assert(reduce_level_to_bit(255) == 1);
}

int main(void) {
	printf("test_monobright:\n");

	RUN_TEST(test_all_zeros);
	RUN_TEST(test_all_max);
	RUN_TEST(test_all_sevens);
	RUN_TEST(test_all_eights);
	RUN_TEST(test_single_bit_patterns);
	RUN_TEST(test_ascending_pattern);
	RUN_TEST(test_descending_boundary);
	RUN_TEST(test_alternating);
	RUN_TEST(test_macro_consistency);

	printf("\n%d/%d tests passed\n", tests_passed, tests_run);
	return (tests_passed == tests_run) ? 0 : 1;
}
