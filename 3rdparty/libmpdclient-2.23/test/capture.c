// SPDX-License-Identifier: BSD-2-Clause
// Copyright The Music Player Daemon Project

#include "capture.h"

#include <mpd/connection.h>
#include <mpd/async.h>

#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#  include <winsock2.h>
#  include <basetsd.h> /* for SSIZE_T */
typedef SSIZE_T ssize_t;
#else
#  include <sys/socket.h>
#  include <unistd.h>
#endif

struct mpd_connection *
test_capture_init(struct test_capture *tc)
{
	int sv[2];

	if (socketpair(AF_LOCAL, SOCK_STREAM, 0, sv) < 0) {
		perror("socketpair() failed");
		return NULL;
	}

	tc->fd = sv[0];

	struct mpd_async *async = mpd_async_new(sv[1]);
	if (async == NULL) {
		close(sv[0]);
		close(sv[1]);
		return NULL;
	}

	struct mpd_connection *c =
		mpd_connection_new_async(async, "OK MPD 0.21.0");
	if (c == NULL) {
		mpd_async_free(async);
		close(sv[0]);
		return NULL;
	}

	return c;
}

void
test_capture_deinit(struct test_capture *tc)
{
	close(tc->fd);
}

const char *
test_capture_receive(struct test_capture *tc)
{
	ssize_t nbytes = recv(tc->fd, tc->buffer, sizeof(tc->buffer) - 1,
			      MSG_DONTWAIT);
	if (nbytes < 0) {
		perror("recv() failed");
		return NULL;
	}

	tc->buffer[nbytes] = 0;
	return tc->buffer;
}

bool
test_capture_send(struct test_capture *tc, const char *response)
{
	ssize_t nbytes = send(tc->fd, response, strlen(response),
			      MSG_DONTWAIT);
	if (nbytes < 0) {
		perror("send() failed");
		return false;
	}

	return true;
}
