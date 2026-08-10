/**
 * playos_lifecycle.c — Lifecycle events (IPC-based implementation)
 *
 * Connects to /run/playos/control.sock (Unix domain socket) to receive
 * lifecycle events from playos-init. The fd returned by playos_lifecycle_fd()
 * can be used with poll(2)/select(2) for non-blocking event detection.
 *
 * SPDX-License-Identifier: MIT
 */

#define _POSIX_C_SOURCE 199309L
#include "playos/playos_lifecycle.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/un.h>

#define PLAYOS_IPC_SOCKET "/run/playos/control.sock"

static int g_socket_fd = -1;
static int g_socket_connected = 0;

static int connect_ipc(void)
{
    if (g_socket_connected)
        return g_socket_fd;

    int fd = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);
    if (fd < 0)
        return -1;

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    snprintf(addr.sun_path, sizeof(addr.sun_path), "%s", PLAYOS_IPC_SOCKET);

    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
        close(fd);
        return -1;
    }

    g_socket_fd = fd;
    g_socket_connected = 1;
    return fd;
}

int playos_lifecycle_poll(PlayOSLifecycleEvent *event)
{
    int fd = connect_ipc();
    if (fd < 0)
        return -1;

    /* Non-blocking read — check if a lifecycle message is available */
    char buf[64];
    ssize_t n = recv(fd, buf, sizeof(buf) - 1, MSG_DONTWAIT);
    if (n < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
            return 0; /* No event pending */
        return -1;
    }
    if (n == 0)
        return 0; /* EOF — no event */

    buf[n] = '\0';

    /* Parse: messages are "type:event_name\n" */
    int evt = -1;
    if (strncmp(buf, "foreground", 10) == 0)
        evt = PLAYOS_LIFECYCLE_FOREGROUND;
    else if (strncmp(buf, "background", 10) == 0)
        evt = PLAYOS_LIFECYCLE_BACKGROUND;
    else if (strncmp(buf, "suspend", 7) == 0)
        evt = PLAYOS_LIFECYCLE_SUSPEND;
    else if (strncmp(buf, "resume", 6) == 0)
        evt = PLAYOS_LIFECYCLE_RESUME;
    else if (strncmp(buf, "terminate", 9) == 0)
        evt = PLAYOS_LIFECYCLE_TERMINATE;

    if (evt >= 0 && event) {
        *event = (PlayOSLifecycleEvent)evt;
        return 1;
    }

    return 0; /* Unknown message, ignore */
}

int playos_lifecycle_wait(PlayOSLifecycleEvent *event, int timeout_ms)
{
    int fd = connect_ipc();
    if (fd < 0)
        return -1;

    /* Set up timeout for recv */
    if (timeout_ms >= 0) {
        struct timeval tv;
        tv.tv_sec = timeout_ms / 1000;
        tv.tv_usec = (timeout_ms % 1000) * 1000;
        setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    }

    char buf[64];
    ssize_t n = recv(fd, buf, sizeof(buf) - 1, 0);

    /* Reset timeout to blocking (or set to infinite if timeout_ms < 0) */
    if (timeout_ms >= 0) {
        struct timeval tv = {0, 0};
        setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    }

    if (n < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
            return 0; /* Timeout */
        return -1;
    }
    if (n == 0)
        return 0;

    buf[n] = '\0';

    int evt = -1;
    if (strncmp(buf, "foreground", 10) == 0)
        evt = PLAYOS_LIFECYCLE_FOREGROUND;
    else if (strncmp(buf, "background", 10) == 0)
        evt = PLAYOS_LIFECYCLE_BACKGROUND;
    else if (strncmp(buf, "suspend", 7) == 0)
        evt = PLAYOS_LIFECYCLE_SUSPEND;
    else if (strncmp(buf, "resume", 6) == 0)
        evt = PLAYOS_LIFECYCLE_RESUME;
    else if (strncmp(buf, "terminate", 9) == 0)
        evt = PLAYOS_LIFECYCLE_TERMINATE;

    if (evt >= 0 && event) {
        *event = (PlayOSLifecycleEvent)evt;
        return 1;
    }

    return 0;
}

int playos_lifecycle_fd(void)
{
    return connect_ipc();
}
