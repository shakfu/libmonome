/**
 * Copyright (c) 2010 William Light <wrl@illest.net>
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
#include <stdio.h>
#include <string.h>
#include <sys/select.h>

#include "internal.h"
#include "platform.h"

char *monome_platform_get_dev_serial(const char *path) {
	char *serial;

	assert(path);

	/* osx serial paths are of the form
	   /dev/tty.usbserial-<device serial> or
	   /dev/tty.usbmodem<device serial> (arduino uno)

	   we'll locate to one past the first hyphen
	   or the first occurrence of usbmodem sequence */

	if( (serial = strstr(path, "usbmodem")) )
		serial += 7;
	else if( !(serial = strchr(path, '-')) )
		return NULL;

	return strdup(serial + 1);
}

int monome_platform_wait_for_input(monome_t *monome, uint_t msec) {
	struct timeval timeout[1];
	fd_set rfds[1];
	fd_set efds[1];
	int fd;

	fd = monome_get_fd(monome);

	timeout->tv_sec  = msec / 1000;
	timeout->tv_usec = (msec - (timeout->tv_sec * 1000)) * 1000;

	FD_ZERO(rfds);
	FD_SET(fd, rfds);
	FD_ZERO(efds);
	FD_SET(fd, efds);

	if( !select(fd + 1, rfds, NULL, efds, timeout) )
		return 1;

	if( FD_ISSET(fd, efds) )
		return -1;

	return 0;
}

int monome_poll_group_wait(monome_poll_group_t *group, int timeout_ms) {
	struct timeval tv, *tvp;
	fd_set rfds, efds;
	unsigned int i;
	int maxfd, fd, ret, dispatched;

	if( !group || !group->count )
		return -1;

	FD_ZERO(&rfds);
	FD_ZERO(&efds);
	maxfd = -1;

	for( i = 0; i < group->count; i++ ) {
		fd = monome_get_fd(group->monomes[i]);
		FD_SET(fd, &rfds);
		FD_SET(fd, &efds);
		if( fd > maxfd )
			maxfd = fd;
	}

	if( timeout_ms < 0 ) {
		tvp = NULL;
	} else {
		tv.tv_sec  = timeout_ms / 1000;
		tv.tv_usec = (timeout_ms % 1000) * 1000;
		tvp = &tv;
	}

	ret = select(maxfd + 1, &rfds, NULL, &efds, tvp);
	if( ret < 0 )
		return -1;
	if( ret == 0 )
		return 0;

	dispatched = 0;
	for( i = 0; i < group->count; i++ ) {
		fd = monome_get_fd(group->monomes[i]);
		if( FD_ISSET(fd, &efds) )
			return -1;
		if( FD_ISSET(fd, &rfds) ) {
			ret = monome_event_handle_next(group->monomes[i]);
			if( ret > 0 )
				dispatched += ret;
		}
	}

	return dispatched;
}
