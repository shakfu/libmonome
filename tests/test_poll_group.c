/**
 * Tests for poll group data structure operations.
 * These exercise the pure data structure logic (new/add/remove/free)
 * without requiring any hardware.
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
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

/* use a fake monome_t -- we only need distinct pointer values */
static monome_t fake_monomes[8];

static void test_new_free(void) {
	monome_poll_group_t *g = monome_poll_group_new();
	assert(g != NULL);
	assert(g->count == 0);
	assert(g->capacity == MONOME_POLL_GROUP_INITIAL_CAP);
	monome_poll_group_free(g);
}

static void test_free_null(void) {
	/* should not crash */
	monome_poll_group_free(NULL);
}

static void test_add_one(void) {
	monome_poll_group_t *g = monome_poll_group_new();
	int ret = monome_poll_group_add(g, &fake_monomes[0]);
	assert(ret == MONOME_OK);
	assert(g->count == 1);
	assert(g->monomes[0] == &fake_monomes[0]);
	monome_poll_group_free(g);
}

static void test_add_multiple(void) {
	monome_poll_group_t *g = monome_poll_group_new();
	int i;

	for (i = 0; i < 4; i++) {
		int ret = monome_poll_group_add(g, &fake_monomes[i]);
		assert(ret == MONOME_OK);
	}

	assert(g->count == 4);

	for (i = 0; i < 4; i++)
		assert(g->monomes[i] == &fake_monomes[i]);

	monome_poll_group_free(g);
}

static void test_add_triggers_growth(void) {
	monome_poll_group_t *g = monome_poll_group_new();
	unsigned int initial_cap = g->capacity;
	int i;

	/* fill to capacity */
	for (i = 0; (unsigned int)i < initial_cap; i++)
		assert(monome_poll_group_add(g, &fake_monomes[i]) == MONOME_OK);

	assert(g->count == initial_cap);

	/* next add should trigger a realloc */
	assert(monome_poll_group_add(g, &fake_monomes[initial_cap]) == MONOME_OK);
	assert(g->count == initial_cap + 1);
	assert(g->capacity > initial_cap);

	monome_poll_group_free(g);
}

static void test_add_duplicate_rejected(void) {
	monome_poll_group_t *g = monome_poll_group_new();

	assert(monome_poll_group_add(g, &fake_monomes[0]) == MONOME_OK);
	assert(monome_poll_group_add(g, &fake_monomes[0]) == MONOME_ERROR_INVALID_ARG);
	assert(g->count == 1);

	monome_poll_group_free(g);
}

static void test_add_null_args(void) {
	monome_poll_group_t *g = monome_poll_group_new();

	assert(monome_poll_group_add(NULL, &fake_monomes[0]) == MONOME_ERROR_INVALID_ARG);
	assert(monome_poll_group_add(g, NULL) == MONOME_ERROR_INVALID_ARG);

	monome_poll_group_free(g);
}

static void test_remove_one(void) {
	monome_poll_group_t *g = monome_poll_group_new();

	monome_poll_group_add(g, &fake_monomes[0]);
	monome_poll_group_add(g, &fake_monomes[1]);

	assert(monome_poll_group_remove(g, &fake_monomes[0]) == MONOME_OK);
	assert(g->count == 1);
	/* remove swaps with last element */
	assert(g->monomes[0] == &fake_monomes[1]);

	monome_poll_group_free(g);
}

static void test_remove_last(void) {
	monome_poll_group_t *g = monome_poll_group_new();

	monome_poll_group_add(g, &fake_monomes[0]);
	assert(monome_poll_group_remove(g, &fake_monomes[0]) == MONOME_OK);
	assert(g->count == 0);

	monome_poll_group_free(g);
}

static void test_remove_not_found(void) {
	monome_poll_group_t *g = monome_poll_group_new();

	monome_poll_group_add(g, &fake_monomes[0]);
	assert(monome_poll_group_remove(g, &fake_monomes[1]) == MONOME_ERROR_INVALID_ARG);
	assert(g->count == 1);

	monome_poll_group_free(g);
}

static void test_remove_null_args(void) {
	monome_poll_group_t *g = monome_poll_group_new();

	assert(monome_poll_group_remove(NULL, &fake_monomes[0]) == MONOME_ERROR_INVALID_ARG);
	assert(monome_poll_group_remove(g, NULL) == MONOME_ERROR_INVALID_ARG);

	monome_poll_group_free(g);
}

static void test_add_remove_add(void) {
	monome_poll_group_t *g = monome_poll_group_new();

	monome_poll_group_add(g, &fake_monomes[0]);
	monome_poll_group_remove(g, &fake_monomes[0]);
	assert(g->count == 0);

	/* re-adding after removal should succeed */
	assert(monome_poll_group_add(g, &fake_monomes[0]) == MONOME_OK);
	assert(g->count == 1);

	monome_poll_group_free(g);
}

int main(void) {
	printf("test_poll_group:\n");

	RUN_TEST(test_new_free);
	RUN_TEST(test_free_null);
	RUN_TEST(test_add_one);
	RUN_TEST(test_add_multiple);
	RUN_TEST(test_add_triggers_growth);
	RUN_TEST(test_add_duplicate_rejected);
	RUN_TEST(test_add_null_args);
	RUN_TEST(test_remove_one);
	RUN_TEST(test_remove_last);
	RUN_TEST(test_remove_not_found);
	RUN_TEST(test_remove_null_args);
	RUN_TEST(test_add_remove_add);

	printf("\n%d/%d tests passed\n", tests_passed, tests_run);
	return (tests_passed == tests_run) ? 0 : 1;
}
