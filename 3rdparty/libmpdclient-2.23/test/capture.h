// SPDX-License-Identifier: BSD-2-Clause
// Copyright The Music Player Daemon Project

#ifndef CAPTURE_H
#define CAPTURE_H

#include <stdbool.h>

struct test_capture {
	int fd;

	char buffer[4096];
};

struct mpd_connection *
test_capture_init(struct test_capture *tc);

void
test_capture_deinit(struct test_capture *tc);

const char *
test_capture_receive(struct test_capture *tc);

bool
test_capture_send(struct test_capture *tc, const char *response);

#endif
