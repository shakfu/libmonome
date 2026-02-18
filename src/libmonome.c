/**
 * Copyright (c) 2010 William Light <wrl@illest.net>
 * Copyright (c) 2013 Nedko Arnaudov <nedko@arnaudov.name>
 * 
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#define _GNU_SOURCE

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <monome.h>
#include "internal.h"
#include "platform.h"
#include "rotation.h"
#include "devices.h"

#ifndef LIBSUFFIX
#define LIBSUFFIX ".so"
#endif

#ifndef LIBDIR
#define LIBDIR "/usr/lib"
#endif

/**
 * private
 */

static monome_devmap_t *map_serial_to_device(const char *serial) {
	monome_devmap_t *m;
	int serialnum;

	for( m = mapping; m->sermatch; m++ )
		if( sscanf(serial, m->sermatch, &serialnum) )
			return m;

	return NULL;
}

/**
 * public
 */

monome_t *monome_open(const char *dev, ...) {
	monome_t *monome;
	monome_devmap_t *m;

	va_list arguments;
	char *serial, *proto;
	int error;

	if( !dev )
		return NULL;

	serial = NULL;
	m = NULL;

	/* first let's figure out which protocol to use */
	if( !strstr(dev, "://") ) {
		/* assume that the device is a tty...let's probe and see what device
		   we're dealing with */

		if( !(serial = monome_platform_get_dev_serial(dev)) )
			return NULL;

		if( (m = map_serial_to_device(serial)) )
			proto = m->proto;
		else
			return NULL;
	} else
		/* otherwise, we'll assume that what we have is an OSC URL.

		   in the future, maybe we'll have more protocol modules...something
		   to think about. */
		proto = "osc";

	if( !(monome = monome_platform_load_protocol(proto)) )
		goto err_proto;

	va_start(arguments, dev);
	error = monome->open(monome, dev, serial, m, arguments);
	va_end(arguments);

	if( error )
		goto err_open;

	monome->proto = proto;

	if( !(monome->device = m_strdup(dev)) )
		goto err_nomem;

	monome->rotation = MONOME_ROTATE_0;
	if( serial ) m_free(serial);
	return monome;

err_nomem:
	monome->free(monome);
	goto err_proto;

err_open:
	monome_platform_free(monome);

err_proto:
	if( serial ) m_free(serial);
	return NULL;
}

void monome_close(monome_t *monome) {
	assert(monome);

	if( monome->serial )
		m_free((char *) monome->serial);

	if( monome->device )
		m_free((char *) monome->device);

	monome->close(monome);
	monome_platform_free(monome);
}

const char *monome_get_serial(monome_t *monome) {
	return monome->serial;
}

const char *monome_get_devpath(monome_t *monome) {
	return monome->device;
}

const char *monome_get_friendly_name(monome_t *monome) {
	return monome->friendly;
}

const char *monome_get_proto(monome_t *monome) {
	return monome->proto;
}

int monome_get_rows(monome_t *monome) {
	if( ROTSPEC(monome).flags & ROW_COL_SWAP )
		return monome->cols;
	else
		return monome->rows;
}

int monome_get_cols(monome_t *monome) {
	if( ROTSPEC(monome).flags & ROW_COL_SWAP )
		return monome->rows;
	else
		return monome->cols;
}

monome_rotate_t monome_get_rotation(monome_t *monome) {
	return monome->rotation;
}

void monome_set_rotation(monome_t *monome, monome_rotate_t rotation) {
	monome->rotation = rotation & 3;
}

int monome_register_handler(monome_t *monome, monome_event_type_t event_type,
                            monome_event_callback_t cb, void *data) {
	monome_callback_t *handler;

	if( event_type >= MONOME_EVENT_MAX )
		return EINVAL;

	handler       = &monome->handlers[event_type];
	handler->cb   = cb;
	handler->data = data;

	return 0;
}

int monome_unregister_handler(monome_t *monome,
                              monome_event_type_t event_type) {
	return monome_register_handler(monome, event_type, NULL, NULL);
}

int monome_event_next(monome_t *monome, monome_event_t *e) {
	e->monome = monome;

	return monome->next_event(monome, e);
}

int monome_event_handle_next(monome_t *monome) {
	monome_callback_t *handler;
	monome_event_t e;
	int status;

	status = monome_event_next(monome, &e);
	if (status <= 0)
		return status;

	handler = &monome->handlers[e.event_type];

	if( !handler->cb )
		return 0;

	handler->cb(&e, handler->data);
	return 1;
}

int monome_get_fd(monome_t *monome) {
	return monome->fd;
}

monome_poll_group_t *monome_poll_group_new(void) {
	monome_poll_group_t *group;

	group = m_calloc(1, sizeof(*group));
	if( !group )
		return NULL;

	group->monomes = m_calloc(MONOME_POLL_GROUP_INITIAL_CAP,
	                          sizeof(monome_t *));
	if( !group->monomes ) {
		m_free(group);
		return NULL;
	}

	group->capacity = MONOME_POLL_GROUP_INITIAL_CAP;
	group->count = 0;
	return group;
}

void monome_poll_group_free(monome_poll_group_t *group) {
	if( !group )
		return;

	m_free(group->monomes);
	m_free(group);
}

int monome_poll_group_add(monome_poll_group_t *group, monome_t *monome) {
	unsigned int i;
	monome_t **new_arr;

	if( !group || !monome )
		return MONOME_ERROR_INVALID_ARG;

	for( i = 0; i < group->count; i++ )
		if( group->monomes[i] == monome )
			return MONOME_ERROR_INVALID_ARG;

	if( group->count == group->capacity ) {
		new_arr = realloc(group->monomes,
		                  group->capacity * 2 * sizeof(monome_t *));
		if( !new_arr )
			return MONOME_ERROR_GENERIC;
		group->monomes = new_arr;
		group->capacity *= 2;
	}

	group->monomes[group->count++] = monome;
	return MONOME_OK;
}

int monome_poll_group_remove(monome_poll_group_t *group, monome_t *monome) {
	unsigned int i;

	if( !group || !monome )
		return MONOME_ERROR_INVALID_ARG;

	for( i = 0; i < group->count; i++ ) {
		if( group->monomes[i] == monome ) {
			group->monomes[i] = group->monomes[group->count - 1];
			group->count--;
			return MONOME_OK;
		}
	}

	return MONOME_ERROR_INVALID_ARG;
}

#define REQUIRE(capability) if (!monome->capability) return MONOME_ERROR_UNSUPPORTED

#define CHECK_BOUNDS(x, y) \
	do { \
		if ((x) >= (uint_t)monome_get_cols(monome) || \
		    (y) >= (uint_t)monome_get_rows(monome)) \
			return MONOME_ERROR_OUT_OF_RANGE; \
	} while (0)

int monome_led_set(monome_t *monome, uint_t x, uint_t y, uint_t on) {
	REQUIRE(led);
	CHECK_BOUNDS(x, y);
	return monome->led->set(monome, x, y, on);
}

int monome_led_on(monome_t *monome, uint_t x, uint_t y) {
	return monome_led_set(monome, x, y, 1);
}

int monome_led_off(monome_t *monome, uint_t x, uint_t y) {
	return monome_led_set(monome, x, y, 0);
}

int monome_led_all(monome_t *monome, uint_t status) {
	REQUIRE(led);
	return monome->led->all(monome, status);
}

int monome_led_map(monome_t *monome, uint_t x_off, uint_t y_off,
                   const uint8_t *data) {
	REQUIRE(led);
	if (x_off >= (uint_t)monome_get_cols(monome) ||
	    y_off >= (uint_t)monome_get_rows(monome))
		return MONOME_ERROR_OUT_OF_RANGE;
	return monome->led->map(monome, x_off, y_off, data);
}

int monome_led_row(monome_t *monome, uint_t x_off, uint_t y,
				   size_t count, const uint8_t *data) {
	REQUIRE(led);
	if (y >= (uint_t)monome_get_rows(monome))
		return MONOME_ERROR_OUT_OF_RANGE;
	return monome->led->row(monome, x_off, y, count, data);
}

int monome_led_col(monome_t *monome, uint_t x, uint_t y_off,
				   size_t count, const uint8_t *data) {
	REQUIRE(led);
	if (x >= (uint_t)monome_get_cols(monome))
		return MONOME_ERROR_OUT_OF_RANGE;
	return monome->led->col(monome, x, y_off, count, data);
}

int monome_led_intensity(monome_t *monome, uint_t brightness) {
	REQUIRE(led);
	return monome->led->intensity(monome, brightness);
}

int monome_led_level_set(monome_t *monome, uint_t x, uint_t y, uint_t level) {
	REQUIRE(led_level);
	CHECK_BOUNDS(x, y);
	return monome->led_level->set(monome, x, y, level);
}

int monome_led_level_all(monome_t *monome, uint_t level) {
	REQUIRE(led_level);
	return monome->led_level->all(monome, level);
}

int monome_led_level_map(monome_t *monome, uint_t x_off, uint_t y_off,
                         const uint8_t *data) {
	REQUIRE(led_level);
	if (x_off >= (uint_t)monome_get_cols(monome) ||
	    y_off >= (uint_t)monome_get_rows(monome))
		return MONOME_ERROR_OUT_OF_RANGE;
	return monome->led_level->map(monome, x_off, y_off, data);
}

int monome_led_level_row(monome_t *monome, uint_t x_off, uint_t y,
                         size_t count, const uint8_t *data) {
	REQUIRE(led_level);
	if (y >= (uint_t)monome_get_rows(monome))
		return MONOME_ERROR_OUT_OF_RANGE;
	return monome->led_level->row(monome, x_off, y, count, data);
}

int monome_led_level_col(monome_t *monome, uint_t x, uint_t y_off,
                         size_t count, const uint8_t *data) {
	REQUIRE(led_level);
	if (x >= (uint_t)monome_get_cols(monome))
		return MONOME_ERROR_OUT_OF_RANGE;
	return monome->led_level->col(monome, x, y_off, count, data);
}

int monome_event_get_grid(const monome_event_t *e, unsigned int *out_x, unsigned int *out_y, monome_t **monome) {
	*out_x = e->grid.x;
	*out_y = e->grid.y;
	*monome = e->monome;
	return 0;
}

int monome_led_ring_set(monome_t *monome, uint_t ring, uint_t led,
                        uint_t level) {
	REQUIRE(led_ring);
	return monome->led_ring->set(monome, ring, led, level);
}

int monome_led_ring_all(monome_t *monome, uint_t ring, uint_t level) {
	REQUIRE(led_ring);
	return monome->led_ring->all(monome, ring, level);
}

int monome_led_ring_map(monome_t *monome, uint_t ring, const uint8_t *levels) {
	REQUIRE(led_ring);
	return monome->led_ring->map(monome, ring, levels);
}

int monome_led_ring_range(monome_t *monome, uint_t ring, uint_t start,
                          uint_t end, uint_t level) {
	REQUIRE(led_ring);
	return monome->led_ring->range(monome, ring, start, end, level);
}

int monome_led_ring_intensity(monome_t *monome, uint_t brightness) {
	REQUIRE(led_ring);
	return monome->led_ring->intensity(monome, brightness);
}

int monome_tilt_enable(monome_t *monome, uint_t sensor) {
	REQUIRE(tilt);
	return monome->tilt->enable(monome, sensor);
}

int monome_tilt_disable(monome_t *monome, uint_t sensor) {
	REQUIRE(tilt);
	return monome->tilt->disable(monome, sensor);
}
