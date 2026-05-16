/*
 * socket.c - socket functions for the library
 *
 * This file is part of the SSH Library
 *
 * Copyright (c) 2008-2010      by Aris Adamantiadis
 *
 * The SSH Library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or (at your
 * option) any later version.
 *
 * The SSH Library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with the SSH Library; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
 * MA 02111-1307, USA.
 */

#include "libssh_esp32_config.h"

#include <errno.h>
#include <stdio.h>
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#ifndef UNIX_PATH_MAX
 /* Inlining the key portions of afunix.h in Windows 10 SDK;
  * that header isn't available in the mingw environment. */
#define UNIX_PATH_MAX 108
struct sockaddr_un {
  ADDRESS_FAMILY sun_family;
  char sun_path[UNIX_PATH_MAX];
};
#endif
#else /* _WIN32 */
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <signal.h>
#ifdef HAVE_PTHREAD
#include <pthread.h>
#endif
#endif /* _WIN32 */

#include "libssh/priv.h"
#include "libssh/callbacks.h"
#include "libssh/socket.h"
#include "libssh/buffer.h"
#include "libssh/poll.h"
#include "libssh/session.h"

/**
 * @defgroup libssh_socket The SSH socket functions.
 * @ingroup libssh
 *
 * Functions for handling sockets.
 *
 * @{
 */

enum ssh_socket_states_e {
	SSH_SOCKET_NONE,
	SSH_SOCKET_CONNECTING,
	SSH_SOCKET_CONNECTED,
	SSH_SOCKET_EOF,
	SSH_SOCKET_ERROR,
	SSH_SOCKET_CLOSED
};

struct ssh_socket_struct {
  socket_t fd;
  int fd_is_socket;
  int last_errno;
  int read_wontblock; /* reading now on socket will
                       not block */
  int write_wontblock;
  int data_except;
  enum ssh_socket_states_e state;
  ssh_buffer out_buffer;
  ssh_buffer in_buffer;
  ssh_session session;
  ssh_socket_callbacks callbacks;
  ssh_poll_handle poll_handle;
#ifndef _WIN32
  pid_t proxy_pid;
#endif
};

#ifdef HAVE_PTHREAD
struct jump_thread_data_struct {
    ssh_session session;
    socket_t fd;
};

int proxy_disconnect = 0;
#endif /* HAVE_PTHREAD */

static int sockets_initialized = 0;

static ssize_t ssh_socket_unbuffered_read(ssh_socket s,
                                          void *buffer,
                                          uint32_t len);
static ssize_t ssh_socket_unbuffered_write(ssh_socket s,
                                           const void *buffer,
                                           uint32_t len);

/**
 * \internal
 * \brief inits the socket system (windows specific)
 */
int ssh_socket_init(void)
{
    if (sockets_initialized == 0) {
#ifdef _WIN32
        struct WSAData wsaData;

        /* Initiates use of the Winsock DLL by a process. */
        if (WSAStartup(MAKEWORD(2, 0), &wsaData) != 0) {
            return -1;
        }
#endif
        ssh_poll_init();

        sockets_initialized = 1;
    }

    return 0;
}

/**
 * @brief Cleanup the socket system.
 */
void ssh_socket_cleanup(void)
{
    if (sockets_initialized == 1) {
        ssh_poll_cleanup();
#ifdef _WIN32
        WSACleanup();
#endif
        sockets_initialized = 0;
    }
}


/**
 * \internal
 * \brief creates a new Socket object
 */
ssh_socket ssh_socket_new(ssh_session session)
{
    ssh_socket s;

    s = calloc(1, sizeof(struct ssh_socket_struct));
    if (s == NULL) {
        ssh_set_error_oom(session);
        return NULL;
    }
    s->fd = SSH_INVALID_SOCKET;
    s->last_errno = -1;
    s->fd_is_socket = 1;
    s->session = session;
    s->in_buffer = ssh_buffer_new();
    if (s->in_buffer == NULL) {
        ssh_set_error_oom(session);
        SAFE_FREE(s);
        return NULL;
    }
    s->out_buffer=ssh_buffer_new();
    if (s->out_buffer == NULL) {
        ssh_set_error_oom(session);
        SSH_BUFFER_FREE(s->in_buffer);
        SAFE_FREE(s);
        return NULL;
    }
    s->read_wontblock = 0;
    s->write_wontblock = 0;
    s->data_except = 0;
    s->poll_handle = NULL;
    s->state=SSH_SOCKET_NONE;
    return s;
}

/**
 * @internal
 * @brief Reset the state of a socket so it looks brand-new
 * @param[in] s socket to rest
 */
void ssh_socket_reset(ssh_socket s)
{
    s->fd = SSH_INVALID_SOCKET;
    s->last_errno = -1;
    s->fd_is_socket = 1;
    ssh_buffer_reinit(s->in_buffer);
    ssh_buffer_reinit(s->out_buffer);
    s->read_wontblock = 0;
    s->write_wontblock = 0;
    s->data_except = 0;
    s->poll_handle = NULL;
    s->state=SSH_SOCKET_NONE;
#ifndef _WIN32
    s->proxy_pid = 0;
#endif
}

/**
 * @internal
 * @brief the socket callbacks, i.e. callbacks to be called
 * upon a socket event.
 * @param s socket to set callbacks on.
 * @param callbacks a ssh_socket_callback object reference.
 */

void ssh_socket_set_callbacks(ssh_socket s, ssh_socket_callbacks callbacks)
{
    s->callbacks = callbacks;
}

void ssh_socket_set_connected(ssh_socket s, struct ssh_poll_handle_struct *p)
{
    s->state = SSH_SOCKET_CONNECTED;
    /* POLLOUT is the event to wait for in a nonblocking connect */
    if (p != NULL) {
        ssh_poll_set_events(p, POLLIN | POLLOUT);
    }
}

/**
 * @brief               SSH poll callback. This callback will be used when an event
 *                      caught on the socket.
 *
 * @param p             Poll object this callback belongs to.
 * @param fd            The raw socket.
 * @param revents       The current poll events on the socket.
 * @param v_s           Userdata to be passed to the callback function,
 *                      in this case the socket object.
 *
 * @return              0 on success, < 0 when the poll object has been removed
 *                      from its poll context.
 */
int ssh_socket_pollcallback(struct ssh_poll_handle_struct *p,
                            socket_t fd,
                            int revents,
                            void *v_s)
{
    ssh_socket s = (ssh_socket)v_s;
    void *buffer = NULL;
    ssize_t nread = 0;
    int rc;
    int err = 0;
    socklen_t errlen = sizeof(err);

    /* Do not do anything if this socket was already closed */
    if (!ssh_socket_is_open(s)) {
        return -1;
    }
    SSH_LOG(SSH_LOG_TRACE,
            "Poll callback on socket %d (%s%s%s), out buffer %" PRIu32, fd,
            (revents & POLLIN) ? "POLLIN ":"",
            (revents & POLLOUT) ? "POLLOUT ":"",
            (revents & POLLERR) ? "POLLERR":"",
            ssh_buffer_get_len(s->out_buffer));
    if ((revents & POLLERR) || (revents & POLLHUP)) {
        /* Check if we are in a connecting state */
        if (s->state == SSH_SOCKET_CONNECTING) {
            s->state = SSH_SOCKET_ERROR;
            rc = getsockopt(fd, SOL_SOCKET, SO_ERROR, (char *)&err, &errlen);
            if (rc < 0) {
                err = errno;
            }
            ssh_socket_close(s);
            /* Overwrite ssh_socket_close() error with the real socket error */
            s->last_errno = err;
            errno = err;

            if (s->callbacks != NULL && s->callbacks->connected != NULL) {
                s->callbacks->connected(SSH_SOCKET_CONNECTED_ERROR,
                                        err,
                                        s->callbacks->userdata);
            }

            return -1;
        }
        /* Then we are in a more standard kind of error */
        /* force a read to get an explanation */
        revents |= POLLIN;
    }
    if ((revents & POLLIN) && s->state == SSH_SOCKET_CONNECTED) {
        s->read_wontblock = 1;
        buffer = ssh_buffer_allocate(s->in_buffer, MAX_BUF_SIZE);
        if (buffer) {
            nread = ssh_socket_unbuffered_read(s, buffer, MAX_BUF_SIZE);
        }
        if (nread < 0) {
            ssh_buffer_pass_bytes_end(s->in_buffer, MAX_BUF_SIZE);
            if (p != NULL) {
                ssh_poll_remove_events(p, POLLIN);
            }

            if (s->callbacks != NULL && s->callbacks->exception != NULL) {
                s->callbacks->exception(SSH_SOCKET_EXCEPTION_ERROR,
                                        s->last_errno,
                                        s->callbacks->userdata);
            }
            return -2;
        }

        /* Rollback the unused space */
        ssh_buffer_pass_bytes_end(s->in_buffer, MAX_BUF_SIZE - nread);

        if (nread == 0) {
            if (p != NULL) {
                ssh_poll_remove_events(p, POLLIN);
            }
            if (s->callbacks != NULL && s->callbacks->exception != NULL) {
                s->callbacks->exception(SSH_SOCKET_EXCEPTION_EOF,
                                        0,
                                        s->callbacks->userdata);
            }
            return -2;
        }

        if (s->session->socket_counter != NULL) {
            s->session->socket_counter->in_bytes += nread;
        }

        /* Call the callback */
        if (s->callbacks != NULL && s->callbacks->data != NULL) {
            size_t processed;
            do {
                processed = s->callbacks->data(ssh_buffer_get(s->in_buffer),
                                               ssh_buffer_get_len(s->in_buffer),
                                               s->callbacks->userdata);
                ssh_buffer_pass_bytes(s->in_buffer, processed);
            } while ((processed > 0) && (s->state == SSH_SOCKET_CONNECTED));

            /* p may have been freed, so don't use it
             * anymore in this function */
            p = NULL;
        }
    }
#ifdef _WIN32
    if (revents & POLLOUT || revents & POLLWRNORM) {
#else
    if (revents & POLLOUT) {
#endif
        uint32_t len;

        /* First, POLLOUT is a sign we may be connected */
        if (s->state == SSH_SOCKET_CONNECTING) {
            SSH_LOG(SSH_LOG_PACKET, "Received POLLOUT in connecting state");
            ssh_socket_set_connected(s, p);

            rc = ssh_socket_set_blocking(ssh_socket_get_fd(s));
            if (rc < 0) {
                return -1;
            }

            if (s->callbacks != NULL && s->callbacks->connected != NULL) {
                s->callbacks->connected(SSH_SOCKET_CONNECTED_OK,
                                        0,
                                        s->callbacks->userdata);
            }

            return 0;
        }

        /* So, we can write data */
        s->write_wontblock = 1;
        if (p != NULL) {
            ssh_poll_remove_events(p, POLLOUT);
        }

        /* If buffered data is pending, write it */
        len = ssh_buffer_get_len(s->out_buffer);
        if (len > 0) {
            ssh_socket_nonblocking_flush(s);
        } else if (s->callbacks != NULL && s->callbacks->controlflow != NULL) {
            /* Otherwise advertise the upper level that write can be done */
            SSH_LOG(SSH_LOG_TRACE, "sending control flow event");
            s->callbacks->controlflow(SSH_SOCKET_FLOW_WRITEWONTBLOCK,
                                      s->callbacks->userdata);
        }
        /* TODO: Find a way to put back POLLOUT when buffering occurs */
    }

    /* Return -1 if the poll handler disappeared */
    if (s->poll_handle == NULL) {
        return -1;
    }

    return 0;
}

/** @internal
 * @brief returns the poll handle corresponding to the socket,
 * creates it if it does not exist.
 * @returns allocated and initialized ssh_poll_handle object
 */
ssh_poll_handle ssh_socket_get_poll_handle(ssh_socket s)
{
    if (s->poll_handle) {
        return s->poll_handle;
    }
    s->poll_handle = ssh_poll_new(s->fd, 0, ssh_socket_pollcallback, s);
    return s->poll_handle;
}

/** \internal
 * \brief Deletes a socket object
 */
void ssh_socket_free(ssh_socket s)
{
    if (s == NULL) {
        return;
    }
    ssh_socket_close(s);
    SSH_BUFFER_FREE(s->in_buffer);
    SSH_BUFFER_FREE(s->out_buffer);
    SAFE_FREE(s);
}

int ssh_socket_unix(ssh_socket s, const char *path)
{
    struct sockaddr_un sunaddr;
    char err_msg[SSH_ERRNO_MSG_MAX] = {0};
    socket_t fd;
    sunaddr.sun_family = AF_UNIX;
    snprintf(sunaddr.sun_path, sizeof(sunaddr.sun_path), "%s", path);

    fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd == SSH_INVALID_SOCKET) {
        ssh_set_error(s->session, SSH_FATAL,
                      "Error from socket(AF_UNIX, SOCK_STREAM, 0): %s",
                      ssh_strerror(errno, err_msg, SSH_ERRNO_MSG_MAX));
        return -1;
    }

#ifndef _WIN32
    if (fcntl(fd, F_SETFD, 1) == -1) {
        ssh_set_error(s->session, SSH_FATAL,
                      "Error from fcntl(fd, F_SETFD, 1): %s",
                      ssh_strerror(errno, err_msg, SSH_ERRNO_MSG_MAX));
        CLOSE_SOCKET(fd);
        return -1;
    }
#endif

    if (connect(fd, (struct sockaddr *) &sunaddr, sizeof(sunaddr)) < 0) {
        ssh_set_error(s->session, SSH_FATAL, "Error from connect(%s): %s",
                      path,
                      ssh_strerror(errno, err_msg, SSH_ERRNO_MSG_MAX));
        CLOSE_SOCKET(fd);
        return -1;
    }
    ssh_socket_set_fd(s,fd);
    return 0;
}

/** \internal
 * \brief closes a socket
 */
void ssh_socket_close(ssh_socket s)
{
    if (ssh_socket_is_open(s)) {
#ifdef _WIN32
        CLOSE_SOCKET(s->fd);
        s->last_errno = WSAGetLastError();
#else
        CLOSE_SOCKET(s->fd);
        s->last_errno = errno;
#endif
    }

    if (s->poll_handle != NULL && !ssh_poll_is_locked(s->poll_handle)) {
        ssh_poll_free(s->poll_handle);
        s->poll_handle = NULL;
    }

    s->state = SSH_SOCKET_CLOSED;

#if !defined( _WIN32) && !defined(ESP32)
    /* If the proxy command still runs try to kill it */
    if (s->proxy_pid != 0) {
        int status;
        pid_t pid = s->proxy_pid;

        s->proxy_pid = 0;
        kill(pid, SIGTERM);
        while (waitpid(pid, &status, 0) == -1) {
            if (errno != EINTR) {
                char err_msg[SSH_ERRNO_MSG_MAX] = {0};
                SSH_LOG(SSH_LOG_TRACE, "waitpid failed: %s",
                        ssh_strerror(errno, err_msg, SSH_ERRNO_MSG_MAX));
                return;
            }
        }
        if (!WIFEXITED(status)) {
            SSH_LOG(SSH_LOG_TRACE, "Proxy command exited abnormally");
            return;
        }
        SSH_LOG(SSH_LOG_TRACE, "Proxy command returned %d", WEXITSTATUS(status));
    }
#endif
}

/**
 * @internal
 * @brief sets the file descriptor of the socket.
 * @param[out] s ssh_socket to update
 * @param[in] fd file descriptor to set
 * @warning this function updates both the input and output
 * file descriptors
 */
void ssh_socket_set_fd(ssh_socket s, socket_t fd)
{
    ssh_poll_handle h = NULL;

    s->fd = fd;

    if (s->poll_handle) {
        ssh_poll_set_fd(s->poll_handle,fd);
    } else {
        s->state = SSH_SOCKET_CONNECTING;
        h = ssh_socket_get_poll_handle(s);
        if (h == NULL) {
            return;
        }

        /* POLLOUT is the event to wait for in a nonblocking connect */
        ssh_poll_set_events(h, POLLOUT);
#ifdef _WIN32
        ssh_poll_add_events(h, POLLWRNORM);
#endif
    }
}

/** \internal
 * \brief returns the input file descriptor of the socket
 */
socket_t ssh_socket_get_fd(ssh_socket s)
{
    return s->fd;
}

/** \internal
 * \brief returns nonzero if the socket is open
 */
int ssh_socket_is_open(ssh_socket s)
{
    return s->fd != SSH_INVALID_SOCKET;
}

/** \internal
 * \brief read len bytes from socket into buffer
 */
static ssize_t ssh_socket_unbuffered_read(ssh_socket s,
                                          void *buffer,
                                          uint32_t len)
{
    ssize_t rc = -1;

    if (s->data_except) {
        return -1;
    }
    if (s->fd_is_socket) {
        rc = recv(s->fd, buffer, len, 0);
    } else {
        rc = read(s->fd, buffer, len);
    }
#ifdef _WIN32
    s->last_errno = WSAGetLastError();
#else
    s->last_errno = errno;
#endif
    s->read_wontblock = 0;

    if (rc < 0) {
        s->data_except = 1;
    } else {
        SSH_LOG(SSH_LOG_TRACE, "read %zd", rc);
    }

    return rc;
}

/** \internal
 * \brief writes len bytes from buffer to socket
 */
static ssize_t ssh_socket_unbuffered_write(ssh_socket s,
                                           const void *buffer,
                                           uint32_t len)
{
    ssize_t w = -1;
    int flags = 0;

#ifdef MSG_NOSIGNAL
    flags |= MSG_NOSIGNAL;
#endif

    if (s->data_except) {
        return -1;
    }

    if (s->fd_is_socket) {
        w = send(s->fd, buffer, len, flags);
    } else {
        w = write(s->fd, buffer, len);
    }
#ifdef _WIN32
    s->last_errno = WSAGetLastError();
#else
    s->last_errno = errno;
#endif
    s->write_wontblock = 0;
    /* Reactive the POLLOUT detector in the poll multiplexer system */
    if (s->poll_handle) {
        SSH_LOG(SSH_LOG_PACKET, "Enabling POLLOUT for socket");
        ssh_poll_add_events(s->poll_handle, POLLOUT);
    }
    if (w < 0) {
        s->data_except = 1;
    }

    SSH_LOG(SSH_LOG_TRACE, "wrote %zd", w);
    return w;
}

/** \internal
 * \brief returns nonzero if the current socket is in the fd_set
 */
int ssh_socket_fd_isset(ssh_socket s, fd_set *set)
{
    if(s->fd == SSH_INVALID_SOCKET) {
        return 0;
    }
    return FD_ISSET(s->fd,set);
}

/** \internal
 * \brief sets the current fd in a fd_set and updates the max_fd
 */

void ssh_socket_fd_set(ssh_socket s, fd_set *set, socket_t *max_fd)
{
    if (s->fd == SSH_INVALID_SOCKET) {
        return;
    }

    FD_SET(s->fd,set);

    if (s->fd >= 0 &&
        s->fd >= *max_fd &&
        s->fd != SSH_INVALID_SOCKET) {
        *max_fd = s->fd + 1;
    }
}

/** \internal
 * \brief buffered write of data
 * \returns SSH_OK, or SSH_ERROR
 * \warning has no effect on socket before a flush
 */
int ssh_socket_write(ssh_socket s, const void *buffer, uint32_t len)
{
    if (len > 0) {
        if (ssh_buffer_add_data(s->out_buffer, buffer, len) < 0) {
            ssh_set_error_oom(s->session);
            return SSH_ERROR;
        }
        ssh_socket_nonblocking_flush(s);
    }

    return SSH_OK;
}


/** \internal
 * \brief starts a nonblocking flush of the output buffer
 *
 */
int ssh_socket_nonblocking_flush(ssh_socket s)
{
    ssh_session session = s->session;
    uint32_t len;

    if (!ssh_socket_is_open(s)) {
        session->alive = 0;
        if (s->callbacks && s->callbacks->exception) {
            s->callbacks->exception(SSH_SOCKET_EXCEPTION_ERROR,
                                    s->last_errno,
                                    s->callbacks->userdata);
        } else {
            char err_msg[SSH_ERRNO_MSG_MAX] = {0};
            ssh_set_error(session,
                          SSH_FATAL,
                          "Writing packet: error on socket (or connection "
                          "closed): %s",
                          ssh_strerror(errno, err_msg, SSH_ERRNO_MSG_MAX));
        }

        return SSH_ERROR;
    }

    len = ssh_buffer_get_len(s->out_buffer);
    if (!s->write_wontblock && s->poll_handle && len > 0) {
        /* force the poll system to catch pollout events */
        ssh_poll_add_events(s->poll_handle, POLLOUT);

        return SSH_AGAIN;
    }

    if (s->write_wontblock && len > 0) {
        ssize_t bwritten;

        bwritten = ssh_socket_unbuffered_write(s,
                                               ssh_buffer_get(s->out_buffer),
                                               len);
        if (bwritten < 0) {
            session->alive = 0;
            ssh_socket_close(s);

            if (s->callbacks && s->callbacks->exception) {
                s->callbacks->exception(SSH_SOCKET_EXCEPTION_ERROR,
                                        s->last_errno,
                                        s->callbacks->userdata);
            } else {
                char err_msg[SSH_ERRNO_MSG_MAX] = {0};
                ssh_set_error(session,
                              SSH_FATAL,
                              "Writing packet: error on socket (or connection "
                              "closed): %s",
                              ssh_strerror(errno, err_msg, SSH_ERRNO_MSG_MAX));
            }

            return SSH_ERROR;
        }

        ssh_buffer_pass_bytes(s->out_buffer, bwritten);
        if (s->session->socket_counter != NULL) {
            s->session->socket_counter->out_bytes += bwritten;
        }
    }

    /* Is there some data pending? */
    len = ssh_buffer_get_len(s->out_buffer);
    if (s->poll_handle && len > 0) {
        SSH_LOG(SSH_LOG_TRACE,
                "did not send all the data, queuing pollout event");
        /* force the poll system to catch pollout events */
        ssh_poll_add_events(s->poll_handle, POLLOUT);

        return SSH_AGAIN;
    }

    /* all data written */
    return SSH_OK;
}

void ssh_socket_set_write_wontblock(ssh_socket s)
{
    s->write_wontblock = 1;
}

void ssh_socket_set_read_wontblock(ssh_socket s)
{
    s->read_wontblock = 1;
}

void ssh_socket_set_except(ssh_socket s)
{
    s->data_except = 1;
}

int ssh_socket_data_available(ssh_socket s)
{
    return s->read_wontblock;
}

int ssh_socket_data_writable(ssh_socket s)
{
    return s->write_wontblock;
}

/** @internal
 * @brief returns the number of outgoing bytes currently buffered
 * @param s the socket
 * @returns numbers of bytes buffered, or 0 if the socket isn't connected
 */
int ssh_socket_buffered_write_bytes(ssh_socket s)
{
    if (s==NULL || s->out_buffer == NULL) {
        return 0;
    }

    return ssh_buffer_get_len(s->out_buffer);
}


int ssh_socket_get_status(ssh_socket s)
{
    int r = 0;

    if (ssh_buffer_get_len(s->in_buffer) > 0) {
        r |= SSH_READ_PENDING;
    }

    if (ssh_buffer_get_len(s->out_buffer) > 0) {
        r |= SSH_WRITE_PENDING;
    }

    if (s->data_except) {
        r |= SSH_CLOSED_ERROR;
    }

    return r;
}

int ssh_socket_get_poll_flags(ssh_socket s)
{
    int r = 0;
    if (s->poll_handle != NULL && (ssh_poll_get_events (s->poll_handle) & POLLIN) > 0) {
        r |= SSH_READ_PENDING;
    }
    if (s->poll_handle != NULL && (ssh_poll_get_events (s->poll_handle) & POLLOUT) > 0) {
        r |= SSH_WRITE_PENDING;
    }
    return r;
}

#ifdef _WIN32
int ssh_socket_set_nonblocking(socket_t fd)
{
    u_long nonblocking = 1;
    return ioctlsocket(fd, FIONBIO, &nonblocking);
}

int ssh_socket_set_blocking(socket_t fd)
{
    u_long nonblocking = 0;
    return ioctlsocket(fd, FIONBIO, &nonblocking);
}

#else /* _WIN32 */
int ssh_socket_set_nonblocking(socket_t fd)
{
    return fcntl(fd, F_SETFL, O_NONBLOCK);
}

int ssh_socket_set_blocking(socket_t fd)
{
    return fcntl(fd, F_SETFL, 0);
}
#endif /* _WIN32 */

/**
 * @internal
 * @brief Launches a socket connection
 * If the socket connected callback has been defined and
 * a poll object exists, this call will be non blocking.
 * @param s    socket to connect.
 * @param host hostname or ip address to connect to.
 * @param port port number to connect to.
 * @param bind_addr address to bind to, or NULL for default.
 * @returns SSH_OK socket is being connected.
 * @returns SSH_ERROR error while connecting to remote host.
 */
int ssh_socket_connect(ssh_socket s,
                       const char *host,
                       uint16_t port,
                       const char *bind_addr)
{
    socket_t fd;

    if (s->state != SSH_SOCKET_NONE) {
        ssh_set_error(s->session, SSH_FATAL,
                      "ssh_socket_connect called on socket not unconnected");
        return SSH_ERROR;
    }
    fd = ssh_connect_host_nonblocking(s->session, host, bind_addr, port);
    SSH_LOG(SSH_LOG_DEBUG, "Nonblocking connection socket: %d", fd);
    if (fd == SSH_INVALID_SOCKET) {
        return SSH_ERROR;
    }
    ssh_socket_set_fd(s,fd);

    return SSH_OK;
}

#ifdef WITH_EXEC
/**
 * @internal
 * @brief executes a command and redirect input and outputs
 * @param command command to execute
 * @param in input file descriptor
 * @param out output file descriptor
 */
void
ssh_execute_command(const char *command, socket_t in, socket_t out)
{
    const char *shell = NULL;
    const char *args[] = {NULL/*shell*/, "-c", command, NULL};
    int devnull;
    int rc;

    /* Prepare /dev/null socket for the stderr redirection */
    devnull = open("/dev/null", O_WRONLY);
    if (devnull == -1) {
        SSH_LOG(SSH_LOG_TRACE, "Failed to open /dev/null");
        exit(1);
    }

    /*
     * By default, use the current users shell. This could fail with some
     * shells like zsh or dash ...
     */
    shell = getenv("SHELL");
    if (shell == NULL || shell[0] == '\0') {
        /* Fall back to the /bin/sh only if the bash is not available. But there are
         * issues with dash or whatever people tend to link to /bin/sh */
        rc = access("/bin/bash", 0);
        if (rc != 0) {
            shell = "/bin/sh";
        } else {
            shell = "/bin/bash";
        }
    }
    args[0] = shell;

    /* redirect in and out to stdin, stdout */
    dup2(in, 0);
    dup2(out, 1);
    /* Ignore anything on the stderr */
    dup2(devnull, STDERR_FILENO);
    close(in);
    close(out);
    rc = execv(args[0], (char * const *)args);
    if (rc < 0) {
        char err_msg[SSH_ERRNO_MSG_MAX] = {0};

        SSH_LOG(SSH_LOG_WARN, "Failed to execute command %s: %s",
                command, ssh_strerror(errno, err_msg, SSH_ERRNO_MSG_MAX));
    }
    exit(1);
}

#ifndef ESP32

/**
 * @internal
 * @brief Open a socket on a ProxyCommand
 * This call will always be nonblocking.
 * @param s    socket to connect.
 * @param command Command to execute.
 * @returns SSH_OK socket is being connected.
 * @returns SSH_ERROR error while executing the command.
 */
int
ssh_socket_connect_proxycommand(ssh_socket s, const char *command)
{
    socket_t pair[2];
    ssh_poll_handle h = NULL;
    int pid;
    int rc;

    if (s->state != SSH_SOCKET_NONE) {
        return SSH_ERROR;
    }

    rc = socketpair(AF_UNIX, SOCK_STREAM, 0, pair);
    if (rc < 0) {
        return SSH_ERROR;
    }

    SSH_LOG(SSH_LOG_DEBUG, "Executing proxycommand '%s'", command);
    pid = fork();
    if (pid == 0) {
        ssh_execute_command(command, pair[0], pair[0]);
        /* Does not return */
    }
    s->proxy_pid = pid;
    close(pair[0]);
    SSH_LOG(SSH_LOG_DEBUG, "ProxyCommand connection pipe: [%d,%d]",pair[0],pair[1]);
    ssh_socket_set_fd(s, pair[1]);
    s->fd_is_socket = 0;
    h = ssh_socket_get_poll_handle(s);
    if (h == NULL) {
        return SSH_ERROR;
    }
    ssh_socket_set_connected(s, h);
    if (s->callbacks && s->callbacks->connected) {
        s->callbacks->connected(SSH_SOCKET_CONNECTED_OK, 0, s->callbacks->userdata);
    }

    return SSH_OK;
}
#endif /* ESP32 */
#endif /* WITH_EXEC */

#ifndef _WIN32
#ifdef HAVE_PTHREAD
static int
verify_knownhost(ssh_session session)
{
    enum ssh_known_hosts_e state;

    state = ssh_session_is_known_server(session);

    switch (state) {
    case SSH_KNOWN_HOSTS_OK:
        break; /* ok */
    default:
        SSH_LOG(SSH_LOG_WARN, "Couldn't verify knownhost during proxyjump.");
        return SSH_ERROR;
    }

    return SSH_OK;
}

static void *
jump_thread_func(void *arg)
{
    struct jump_thread_data_struct *jump_thread_data = NULL;
    struct ssh_jump_info_struct *jis = NULL;
    struct ssh_jump_callbacks_struct *cb = NULL;
    ssh_session jump_session = NULL;
    ssh_channel caa = NULL;
    int rc;
    ssh_event event = NULL;
    ssh_connector connector_in = NULL, connector_out = NULL;
    ssh_session session = NULL;
    int next_port;
    char *next_hostname = NULL;

    jump_thread_data = (struct jump_thread_data_struct *)arg;
    session = jump_thread_data->session;

    next_port = session->opts.port;
    next_hostname = strdup(session->opts.host);

    jump_session = ssh_new();
    if (jump_session == NULL) {
        goto exit;
    }

    jump_session->proxy_root = false;
    /* Reset the global variable if it was previously 1 */
    if (session->proxy_root) {
        proxy_disconnect = 0;
    }

    for (jis = ssh_list_pop_head(struct ssh_jump_info_struct *,
                                 session->opts.proxy_jumps);
         jis != NULL;
         jis = ssh_list_pop_head(struct ssh_jump_info_struct *,
                                 session->opts.proxy_jumps)) {
        rc = ssh_list_append(jump_session->opts.proxy_jumps, jis);
        if (rc != SSH_OK) {
            ssh_set_error_oom(session);
            goto exit;
        }
    }
    for (jis =
            ssh_list_pop_head(struct ssh_jump_info_struct *,
                              session->opts.proxy_jumps_user_cb);
         jis != NULL;
         jis = ssh_list_pop_head(struct ssh_jump_info_struct *,
                                 session->opts.proxy_jumps_user_cb)) {
        rc = ssh_list_append(jump_session->opts.proxy_jumps_user_cb, jis);
        if (rc != SSH_OK) {
            ssh_set_error_oom(session);
            goto exit;
        }
    }

    ssh_options_set(jump_session,
                    SSH_OPTIONS_LOG_VERBOSITY,
                    &session->common.log_verbosity);

    /* Pop the information about the current jump */
    jis = ssh_list_pop_head(struct ssh_jump_info_struct *,
                            jump_session->opts.proxy_jumps);
    if (jis == NULL) {
        SSH_LOG(SSH_LOG_WARN, "Inconsistent list of proxy jumps received");
        goto exit;
    }

    ssh_options_set(jump_session, SSH_OPTIONS_HOST, jis->hostname);
    ssh_options_set(jump_session, SSH_OPTIONS_USER, jis->username);
    ssh_options_set(jump_session, SSH_OPTIONS_PORT, &jis->port);

    /* Pop the callbacks for the current jump */
    cb = ssh_list_pop_head(struct ssh_jump_callbacks_struct *,
                           jump_session->opts.proxy_jumps_user_cb);

    if (cb != NULL && cb->before_connection != NULL) {
        rc = cb->before_connection(jump_session, cb->userdata);
        if (rc != SSH_OK) {
            SSH_LOG(SSH_LOG_WARN, "%s", ssh_get_error(jump_session));
            goto exit;
        }
    }

    /* If there are more jumps then this will make a new thread and call the
     * current function again, until there are no jumps. When there are no jumps
     * it connects normally. */
    rc = ssh_connect(jump_session);
    if (rc != SSH_OK) {
        SSH_LOG(SSH_LOG_WARN, "%s", ssh_get_error(jump_session));
        goto exit;
    }

    /* Use the callback or default implementation for verifying knownhost */
    if (cb != NULL && cb->verify_knownhost != NULL) {
        rc = cb->verify_knownhost(jump_session, cb->userdata);
    } else {
        rc = verify_knownhost(jump_session);
    }
    if (rc != SSH_OK) {
        goto exit;
    }

    /* Use the callback or publickey method to authenticate */
    if (cb != NULL && cb->authenticate != NULL) {
        rc = cb->authenticate(jump_session, cb->userdata);
    } else {
        rc = ssh_userauth_publickey_auto(jump_session, NULL, NULL);
    }
    if (rc != SSH_OK) {
        SSH_LOG(SSH_LOG_WARN, "%s", ssh_get_error(jump_session));
        goto exit;
    }

    caa = ssh_channel_new(jump_session);
    if (caa == NULL) {
        goto exit;
    }
    /* The origin hostname and port are set to match OpenSSH implementation
     * they are only used for logging on the server */
    rc = ssh_channel_open_forward(caa,
                                  next_hostname,
                                  next_port,
                                  "127.0.0.1",
                                  65535);
    if (rc != SSH_OK) {
        SSH_LOG(SSH_LOG_WARN,
                "Error opening port forwarding channel: %s",
                ssh_get_error(jump_session));
        goto exit;
    }

    event = ssh_event_new();
    if (event == NULL) {
        goto exit;
    }

    connector_in = ssh_connector_new(jump_session);
    if (connector_in == NULL) {
        goto exit;
    }
    ssh_connector_set_out_channel(connector_in, caa, SSH_CONNECTOR_STDINOUT);
    ssh_connector_set_in_fd(connector_in, jump_thread_data->fd);
    ssh_event_add_connector(event, connector_in);

    connector_out = ssh_connector_new(jump_session);
    if (connector_out == NULL) {
        goto exit;
    }
    ssh_connector_set_out_fd(connector_out, jump_thread_data->fd);
    ssh_connector_set_in_channel(connector_out, caa, SSH_CONNECTOR_STDINOUT);
    ssh_event_add_connector(event, connector_out);

    while (ssh_channel_is_open(caa)) {
        if (proxy_disconnect == 1) {
            break;
        }
        rc = ssh_event_dopoll(event, 60000);
        if (rc == SSH_ERROR) {
            SSH_LOG(SSH_LOG_WARN,
                    "Error in ssh_event_dopoll() during proxy jump");
            break;
        }
    }

exit:
    if (connector_in != NULL) {
        ssh_event_remove_connector(event, connector_in);
        ssh_connector_free(connector_in);
    }
    if (connector_out != NULL) {
        ssh_event_remove_connector(event, connector_out);
        ssh_connector_free(connector_out);
    }
    SAFE_FREE(next_hostname);
    if (jis != NULL) {
        SAFE_FREE(jis->hostname);
        SAFE_FREE(jis->username);
    }
    SAFE_FREE(jis);

    ssh_disconnect(jump_session);
    ssh_event_free(event);
    ssh_free(jump_session);

    shutdown(jump_thread_data->fd, SHUT_RDWR);
    close(jump_thread_data->fd);
    SAFE_FREE(jump_thread_data);

    pthread_exit(NULL);
}

int
ssh_socket_connect_proxyjump(ssh_socket s)
{
    ssh_poll_handle h = NULL;
    int rc;
    pthread_t jump_thread;
    struct jump_thread_data_struct *jump_thread_data = NULL;
    socket_t pair[2];

    if (s->state != SSH_SOCKET_NONE) {
        ssh_set_error(
            s->session,
            SSH_FATAL,
            "ssh_socket_connect_proxyjump called on socket not unconnected");
        return SSH_ERROR;
    }

    jump_thread_data = calloc(1, sizeof(struct jump_thread_data_struct));
    if (jump_thread_data == NULL) {
        ssh_set_error_oom(s->session);
        return SSH_ERROR;
    }

    rc = socketpair(AF_UNIX, SOCK_STREAM, 0, pair);
    if (rc == -1) {
        char err_msg[SSH_ERRNO_MSG_MAX] = {0};

        ssh_set_error(s->session,
                      SSH_FATAL,
                      "Creating socket pair failed: %s",
                      ssh_strerror(errno, err_msg, SSH_ERRNO_MSG_MAX));
        SAFE_FREE(jump_thread_data);
        return SSH_ERROR;
    }

    jump_thread_data->session = s->session;
    jump_thread_data->fd = pair[0];

    rc = pthread_create(&jump_thread, NULL, jump_thread_func, jump_thread_data);
    if (rc != 0) {
        char err_msg[SSH_ERRNO_MSG_MAX] = {0};

        ssh_set_error(s->session,
                      SSH_FATAL,
                      "Creating new thread failed: %s",
                      ssh_strerror(rc, err_msg, SSH_ERRNO_MSG_MAX));
        SAFE_FREE(jump_thread_data);
        return SSH_ERROR;
    }
    rc = pthread_detach(jump_thread);
    if (rc != 0) {
        char err_msg[SSH_ERRNO_MSG_MAX] = {0};

        ssh_set_error(s->session,
                      SSH_FATAL,
                      "Failed to detach thread: %s",
                      ssh_strerror(rc, err_msg, SSH_ERRNO_MSG_MAX));
        SAFE_FREE(jump_thread_data);
        return SSH_ERROR;
    }

    SSH_LOG(SSH_LOG_DEBUG,
            "ProxyJump connection pipe: [%d,%d]",
            pair[0],
            pair[1]);
    ssh_socket_set_fd(s, pair[1]);
    s->fd_is_socket = 1;
    h = ssh_socket_get_poll_handle(s);
    if (h == NULL) {
        return SSH_ERROR;
    }
    ssh_socket_set_connected(s, h);
    if (s->callbacks && s->callbacks->connected) {
        s->callbacks->connected(SSH_SOCKET_CONNECTED_OK,
                                0,
                                s->callbacks->userdata);
    }

    return SSH_OK;
}

#endif /* HAVE_PTHREAD */

#endif /* _WIN32 */
/** @} */
