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

#include <poll.h>
#include <stdlib.h>

#include <monome.h>
#include "internal.h"
#include "platform.h"

int monome_platform_wait_for_input(monome_t *monome, uint_t msec) {
	struct pollfd fds[1];

	if( !msec )
		return 0;

	fds->fd = monome_get_fd(monome);
	fds->events = POLLIN;

	if( !poll(fds, 1, msec) )
		return 1;

	if (fds->revents & POLLERR)
		return -1;

	return 0;
}

int monome_poll_group_wait(monome_poll_group_t *group, int timeout_ms) {
	struct pollfd *fds;
	unsigned int i;
	int ret, dispatched;

	if( !group || !group->count )
		return -1;

	fds = calloc(group->count, sizeof(struct pollfd));
	if( !fds )
		return -1;

	for( i = 0; i < group->count; i++ ) {
		fds[i].fd = monome_get_fd(group->monomes[i]);
		fds[i].events = POLLIN;
	}

	ret = poll(fds, group->count, timeout_ms);
	if( ret < 0 ) {
		free(fds);
		return -1;
	}
	if( ret == 0 ) {
		free(fds);
		return 0;
	}

	dispatched = 0;
	for( i = 0; i < group->count; i++ ) {
		if( fds[i].revents & POLLERR ) {
			free(fds);
			return -1;
		}
		if( fds[i].revents & POLLIN ) {
			ret = monome_event_handle_next(group->monomes[i]);
			if( ret > 0 )
				dispatched += ret;
		}
	}

	free(fds);
	return dispatched;
}
