/**
 * playos_lifecycle.c — Lifecycle events (pipe-based implementation)
 *
 * Reads single-byte lifecycle events from the file descriptor passed by
 * playos-init via the PLAYOS_LIFECYCLE_FD environment variable. The byte
 * value maps directly onto the PlayOSLifecycleEvent enum (FOREGROUND=0 …
 * TERMINATE=4). The fd returned by playos_lifecycle_fd() can be used with
 * poll(2)/select(2) for non-blocking event detection.
 *
 * SPDX-License-Identifier: MIT
 */

#define _POSIX_C_SOURCE 200809L
#include "playos/playos_lifecycle.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>

static int g_fd = -1;
static int g_fd_resolved = 0;

/* Last successfully-read lifecycle event. Used by playos_audio.c to enforce
 * "volume setters are honored only while foreground". Processes that never
 * receive an event (the shell/overlay) remain foreground by default. */
static PlayOSLifecycleEvent g_last_event = PLAYOS_LIFECYCLE_FOREGROUND;

/* Resolve the lifecycle fd once from the environment. Returns the fd, or
 * -1 if it is not present/valid. */
static int resolve_fd(void)
{
    if (g_fd_resolved)
        return g_fd;

    g_fd_resolved = 1;

    const char *fd_str = getenv("PLAYOS_LIFECYCLE_FD");
    if (!fd_str || !fd_str[0])
        return -1;

    char *end = NULL;
    long fd = strtol(fd_str, &end, 10);
    if (!end || *end != '\0' || fd < 0)
        return -1;

    /* Make the read end non-blocking so playos_lifecycle_poll() never
     * blocks; playos_lifecycle_wait() uses poll(2) for its timeout. */
    int flags = fcntl((int)fd, F_GETFL, 0);
    if (flags >= 0)
        (void)fcntl((int)fd, F_SETFL, flags | O_NONBLOCK);

    g_fd = (int)fd;
    return g_fd;
}

int playos_lifecycle_fd(void)
{
    return resolve_fd();
}

int playos_lifecycle_poll(PlayOSLifecycleEvent *event)
{
    int fd = resolve_fd();
    if (fd < 0)
        return -1;

    unsigned char byte = 0;
    ssize_t n = read(fd, &byte, 1);
    if (n < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
            return 0; /* No event pending */
        return -1;
    }
    if (n == 0)
        return 0; /* EOF — init closed the pipe */

    if (event)
        *event = (PlayOSLifecycleEvent)byte;
    g_last_event = (PlayOSLifecycleEvent)byte;
    return 1;
}

int playos_lifecycle_wait(PlayOSLifecycleEvent *event, int timeout_ms)
{
    int fd = resolve_fd();
    if (fd < 0)
        return -1;

    struct pollfd pfd;
    pfd.fd = fd;
    pfd.events = POLLIN;
    pfd.revents = 0;

    int ret = poll(&pfd, 1, timeout_ms);
    if (ret < 0)
        return -1;
    if (ret == 0)
        return 0; /* Timeout */

    unsigned char byte = 0;
    ssize_t n = read(fd, &byte, 1);
    if (n < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
            return 0;
        return -1;
    }
    if (n == 0)
        return 0;

    if (event)
        *event = (PlayOSLifecycleEvent)byte;
    g_last_event = (PlayOSLifecycleEvent)byte;
    return 1;
}

int playos_lifecycle_is_foreground(void)
{
    return g_last_event == PLAYOS_LIFECYCLE_FOREGROUND;
}
