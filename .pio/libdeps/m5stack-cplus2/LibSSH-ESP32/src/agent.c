/*
 * agent.c - ssh agent functions
 *
 * This file is part of the SSH Library
 *
 * Copyright (c) 2008-2013 by Andreas Schneider <asn@cryptomilk.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

/* This file is based on authfd.c from OpenSSH */

/*
 * How does the ssh-agent work?
 *
 * a) client sends a request to get a list of all keys
 *    the agent returns the count and all public keys
 * b) iterate over them to check if the server likes one
 * c) the client sends a sign request to the agent
 *    type, pubkey as blob, data to sign, flags
 *    the agent returns the signed data
 */

#include "libssh_esp32_config.h"

#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifndef _WIN32
#include <netinet/in.h>
#include <lwip/inet.h>
#include <sys/socket.h>
#else
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

#include "libssh/agent.h"
#include "libssh/priv.h"
#include "libssh/socket.h"
#include "libssh/buffer.h"
#include "libssh/session.h"
#include "libssh/poll.h"
#include "libssh/pki.h"
#include "libssh/bytearray.h"

/* macro to check for "agent failure" message */
#define agent_failed(x) \
  (((x) == SSH_AGENT_FAILURE) || ((x) == SSH_COM_AGENT2_FAILURE) || \
   ((x) == SSH2_AGENT_FAILURE))

static uint32_t atomicio(struct ssh_agent_struct *agent, void *buf, uint32_t n, int do_read) {
  char *b = buf;
  uint32_t pos = 0;
  ssize_t res;
  ssh_pollfd_t pfd;
  ssh_channel channel = agent->channel;
  socket_t fd;

  /* Using a socket ? */
  if (channel == NULL) {
    fd = ssh_socket_get_fd(agent->sock);
    pfd.fd = fd;
    pfd.events = do_read ? POLLIN : POLLOUT;

    while (n > pos) {
      if (do_read) {
        res = recv(fd, b + pos, n - pos, 0);
      } else {
        res = send(fd, b + pos, n - pos, 0);
      }
      switch (res) {
      case -1:
        if (errno == EINTR) {
          continue;
        }
#ifdef EWOULDBLOCK
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
#else
          if (errno == EAGAIN) {
#endif
            (void) ssh_poll(&pfd, 1, -1);
            continue;
          }
          return 0;
      case 0:
        /* read returns 0 on end-of-file */
        errno = do_read ? 0 : EPIPE;
        return pos;
      default:
        pos += (uint32_t) res;
        }
      }
      return pos;
    } else {
      /* using an SSH channel */
      while (n > pos){
        if (do_read)
          res = ssh_channel_read(channel,b + pos, n-pos, 0);
        else
          res = ssh_channel_write(channel, b+pos, n-pos);
        if (res == SSH_AGAIN)
          continue;
        if (res == SSH_ERROR)
          return 0;
        pos += (uint32_t)res;
      }
      return pos;
    }
}

ssh_agent ssh_agent_new(struct ssh_session_struct *session) {
  ssh_agent agent = NULL;

  agent = malloc(sizeof(struct ssh_agent_struct));
  if (agent == NULL) {
    return NULL;
  }
  ZERO_STRUCTP(agent);

  agent->count = 0;
  agent->sock = ssh_socket_new(session);
  if (agent->sock == NULL) {
    SAFE_FREE(agent);
    return NULL;
  }
  agent->channel = NULL;
  return agent;
}

static void agent_set_channel(struct ssh_agent_struct *agent, ssh_channel channel){
  agent->channel = channel;
}

/**
 * @addtogroup libssh_auth
 *
 * @{
 */

/** @brief sets the SSH agent channel.
 * The SSH agent channel will be used to authenticate this client using
 * an agent through a channel, from another session. The most likely use
 * is to implement SSH Agent forwarding into a SSH proxy.
 *
 * @param session the session
 *
 * @param[in] channel a SSH channel from another session.
 *
 * @returns SSH_OK in case of success
 *          SSH_ERROR in case of an error
 */
int ssh_set_agent_channel(ssh_session session, ssh_channel channel){
  if (!session)
    return SSH_ERROR;
  if (!session->agent){
    ssh_set_error(session, SSH_REQUEST_DENIED, "Session has no active agent");
    return SSH_ERROR;
  }
  agent_set_channel(session->agent, channel);
  return SSH_OK;
}

/** @brief sets the SSH agent socket.
 * The SSH agent will be used to authenticate this client using
 * the given socket to communicate with the ssh-agent. The caller
 * is responsible for connecting to the socket prior to calling
 * this function.
 * @returns SSH_OK in case of success
 *          SSH_ERROR in case of an error
 */
int ssh_set_agent_socket(ssh_session session, socket_t fd){
  if (!session)
    return SSH_ERROR;
  if (!session->agent){
    ssh_set_error(session, SSH_REQUEST_DENIED, "Session has no active agent");
    return SSH_ERROR;
  }

  ssh_socket_set_fd(session->agent->sock, fd);
  return SSH_OK;
}

/**
 * @}
 */

void ssh_agent_close(struct ssh_agent_struct *agent) {
  if (agent == NULL) {
    return;
  }

  ssh_socket_close(agent->sock);
}

void ssh_agent_free(ssh_agent agent) {
  if (agent) {
    if (agent->ident) {
      SSH_BUFFER_FREE(agent->ident);
    }
    if (agent->sock) {
      ssh_agent_close(agent);
      ssh_socket_free(agent->sock);
    }
    SAFE_FREE(agent);
  }
}

static int agent_connect(ssh_session session) {
  const char *auth_sock = NULL;

  if (session == NULL || session->agent == NULL) {
    return -1;
  }

  if (session->agent->channel != NULL)
    return 0;

  auth_sock = session->opts.agent_socket ?
    session->opts.agent_socket : getenv("SSH_AUTH_SOCK");

  if (auth_sock && *auth_sock) {
    if (ssh_socket_unix(session->agent->sock, auth_sock) < 0) {
      return -1;
    }
    return 0;
  }

  return -1;
}

#if 0
static int agent_decode_reply(struct ssh_session_struct *session, int type) {
  switch (type) {
    case SSH_AGENT_FAILURE:
    case SSH2_AGENT_FAILURE:
    case SSH_COM_AGENT2_FAILURE:
      ssh_log(session, SSH_LOG_RARE, "SSH_AGENT_FAILURE");
      return 0;
    case SSH_AGENT_SUCCESS:
      return 1;
    default:
      ssh_set_error(session, SSH_FATAL,
          "Bad response from authentication agent: %d", type);
      break;
  }

  return -1;
}
#endif

static int agent_talk(struct ssh_session_struct *session,
    struct ssh_buffer_struct *request, struct ssh_buffer_struct *reply) {
  uint32_t len = 0;
  uint8_t tmpbuf[4];
  uint8_t *payload = tmpbuf;
  char err_msg[SSH_ERRNO_MSG_MAX] = {0};

  len = ssh_buffer_get_len(request);
  SSH_LOG(SSH_LOG_TRACE, "Request length: %" PRIu32, len);
  PUSH_BE_U32(payload, 0, len);

  /* send length and then the request packet */
  if (atomicio(session->agent, payload, 4, 0) == 4) {
    if (atomicio(session->agent, ssh_buffer_get(request), len, 0)
        != len) {
      SSH_LOG(SSH_LOG_TRACE, "atomicio sending request failed: %s",
          strerror(errno));
      return -1;
    }
  } else {
    SSH_LOG(SSH_LOG_TRACE,
        "atomicio sending request length failed: %s",
        ssh_strerror(errno, err_msg, SSH_ERRNO_MSG_MAX));
    return -1;
  }

  /* wait for response, read the length of the response packet */
  if (atomicio(session->agent, payload, 4, 1) != 4) {
    SSH_LOG(SSH_LOG_TRACE, "atomicio read response length failed: %s",
        strerror(errno));
    return -1;
  }

  len = PULL_BE_U32(payload, 0);
  if (len > 256 * 1024) {
    ssh_set_error(session, SSH_FATAL,
        "Authentication response too long: %" PRIu32, len);
    return -1;
  }
  SSH_LOG(SSH_LOG_TRACE, "Response length: %" PRIu32, len);

  payload = ssh_buffer_allocate(reply, len);
  if (payload == NULL) {
    SSH_LOG(SSH_LOG_DEBUG, "Not enough space");
    return -1;
  }

  if (atomicio(session->agent, payload, len, 1) != len) {
    SSH_LOG(SSH_LOG_DEBUG,
        "Error reading response from authentication socket.");
    /* Rollback the unused space */
    ssh_buffer_pass_bytes_end(reply, len);
    return -1;
  }

  return 0;
}

uint32_t ssh_agent_get_ident_count(struct ssh_session_struct *session)
{
    ssh_buffer request = NULL;
    ssh_buffer reply = NULL;
    unsigned int type = 0;
    uint32_t count = 0;
    uint32_t rc;

    /* send message to the agent requesting the list of identities */
    request = ssh_buffer_new();
    if (request == NULL) {
        ssh_set_error_oom(session);
        return 0;
    }
    if (ssh_buffer_add_u8(request, SSH2_AGENTC_REQUEST_IDENTITIES) < 0) {
        ssh_set_error_oom(session);
        SSH_BUFFER_FREE(request);
        return 0;
    }

    reply = ssh_buffer_new();
    if (reply == NULL) {
        SSH_BUFFER_FREE(request);
        ssh_set_error(session, SSH_FATAL, "Not enough space");
        return 0;
    }

    if (agent_talk(session, request, reply) < 0) {
        SSH_BUFFER_FREE(request);
        SSH_BUFFER_FREE(reply);
        return 0;
    }
    SSH_BUFFER_FREE(request);

    /* get message type and verify the answer */
    rc = ssh_buffer_get_u8(reply, (uint8_t *) &type);
    if (rc != sizeof(uint8_t)) {
        ssh_set_error(session, SSH_FATAL,
                "Bad authentication reply size: %" PRIu32, rc);
        SSH_BUFFER_FREE(reply);
        return 0;
    }
#ifdef WORDS_BIGENDIAN
    type = bswap_32(type);
#endif

    SSH_LOG(SSH_LOG_TRACE,
            "Answer type: %d, expected answer: %d",
            type, SSH2_AGENT_IDENTITIES_ANSWER);

    if (agent_failed(type)) {
        SSH_BUFFER_FREE(reply);
        return 0;
    } else if (type != SSH2_AGENT_IDENTITIES_ANSWER) {
        ssh_set_error(session, SSH_FATAL,
                "Bad authentication reply message type: %u", type);
        SSH_BUFFER_FREE(reply);
        return 0;
    }

    rc = ssh_buffer_get_u32(reply, &count);
    if (rc != 4) {
        ssh_set_error(session,
                SSH_FATAL,
                "Failed to read count");
        SSH_BUFFER_FREE(reply);
        return 0;
    }
    session->agent->count = ntohl(count);
    SSH_LOG(SSH_LOG_DEBUG, "Agent count: %d",
            session->agent->count);
    if (session->agent->count > 1024) {
        ssh_set_error(session, SSH_FATAL,
                "Too many identities in authentication reply: %d",
                session->agent->count);
        SSH_BUFFER_FREE(reply);
        return 0;
    }

    ssh_buffer_free(session->agent->ident);
    session->agent->ident = reply;

    return session->agent->count;
}

/* caller has to free comment */
ssh_key ssh_agent_get_first_ident(struct ssh_session_struct *session,
                              char **comment) {
    if (ssh_agent_get_ident_count(session) > 0) {
        return ssh_agent_get_next_ident(session, comment);
    }

    return NULL;
}

/* caller has to free comment */
ssh_key ssh_agent_get_next_ident(struct ssh_session_struct *session,
                                 char **comment)
{
    struct ssh_key_struct *key = NULL;
    struct ssh_string_struct *blob = NULL;
    struct ssh_string_struct *tmp = NULL;
    int rc;

    if (session->agent->count == 0) {
        return NULL;
    }

    /* get the blob */
    blob = ssh_buffer_get_ssh_string(session->agent->ident);
    if (blob == NULL) {
        return NULL;
    }

    /* get the comment */
    tmp = ssh_buffer_get_ssh_string(session->agent->ident);
    if (tmp == NULL) {
        SSH_STRING_FREE(blob);

        return NULL;
    }

    if (comment) {
        *comment = ssh_string_to_char(tmp);
    } else {
        SSH_STRING_FREE(blob);
        SSH_STRING_FREE(tmp);

        return NULL;
    }
    SSH_STRING_FREE(tmp);

    /* get key from blob */
    rc = ssh_pki_import_pubkey_blob(blob, &key);
    if (rc == SSH_ERROR) {
        /* Try again as a cert. */
        rc = ssh_pki_import_cert_blob(blob, &key);
    }
    SSH_STRING_FREE(blob);
    if (rc == SSH_ERROR) {
        return NULL;
    }

    return key;
}

int ssh_agent_is_running(ssh_session session) {
  if (session == NULL || session->agent == NULL) {
    return 0;
  }

  if (ssh_socket_is_open(session->agent->sock)) {
    return 1;
  } else {
    if (agent_connect(session) < 0) {
      return 0;
    } else {
      return 1;
    }
  }

  return 0;
}

ssh_string ssh_agent_sign_data(ssh_session session,
                               const ssh_key pubkey,
                               struct ssh_buffer_struct *data)
{
    ssh_buffer request = NULL;
    ssh_buffer reply = NULL;
    ssh_string key_blob = NULL;
    ssh_string sig_blob = NULL;
    unsigned int type = 0;
    unsigned int flags = 0;
    uint32_t dlen;
    int rc;

    request = ssh_buffer_new();
    if (request == NULL) {
        return NULL;
    }

    /* create request */
    if (ssh_buffer_add_u8(request, SSH2_AGENTC_SIGN_REQUEST) < 0) {
        SSH_BUFFER_FREE(request);
        return NULL;
    }

    rc = ssh_pki_export_pubkey_blob(pubkey, &key_blob);
    if (rc < 0) {
        SSH_BUFFER_FREE(request);
        return NULL;
    }

    /*
     * make sure it already can contain all the expected content:
     * - 1 x uint8_t
     * - 2 x uint32_t
     * - 1 x ssh_string (uint8_t + data)
     */
    rc = ssh_buffer_allocate_size(request,
                                  sizeof(uint8_t) * 2 +
                                  sizeof(uint32_t) * 2 +
                                  ssh_string_len(key_blob));
    if (rc < 0) {
        SSH_BUFFER_FREE(request);
        return NULL;
    }

    /* adds len + blob */
    rc = ssh_buffer_add_ssh_string(request, key_blob);
    SSH_STRING_FREE(key_blob);
    if (rc < 0) {
        SSH_BUFFER_FREE(request);
        return NULL;
    }

    /* Add data */
    dlen = ssh_buffer_get_len(data);
    if (ssh_buffer_add_u32(request, htonl(dlen)) < 0) {
        SSH_BUFFER_FREE(request);
        return NULL;
    }
    if (ssh_buffer_add_data(request, ssh_buffer_get(data), dlen) < 0) {
        SSH_BUFFER_FREE(request);
        return NULL;
    }

    /* Add Flags: SHA2 extension (RFC 8332) if negotiated */
    if (ssh_key_type_plain(pubkey->type) == SSH_KEYTYPE_RSA) {
        if (session->extensions & SSH_EXT_SIG_RSA_SHA512) {
            flags |= SSH_AGENT_RSA_SHA2_512;
        } else if (session->extensions & SSH_EXT_SIG_RSA_SHA256) {
            flags |= SSH_AGENT_RSA_SHA2_256;
        }
    }
    if (ssh_buffer_add_u32(request, htonl(flags)) < 0) {
        SSH_BUFFER_FREE(request);
        return NULL;
    }

    reply = ssh_buffer_new();
    if (reply == NULL) {
        SSH_BUFFER_FREE(request);
        return NULL;
    }

    /* send the request */
    if (agent_talk(session, request, reply) < 0) {
        SSH_BUFFER_FREE(request);
        SSH_BUFFER_FREE(reply);
        return NULL;
    }
    SSH_BUFFER_FREE(request);

    /* check if reply is valid */
    if (ssh_buffer_get_u8(reply, (uint8_t *) &type) != sizeof(uint8_t)) {
        SSH_BUFFER_FREE(reply);
        return NULL;
    }
#ifdef WORDS_BIGENDIAN
    type = bswap_32(type);
#endif

    if (agent_failed(type)) {
        SSH_LOG(SSH_LOG_DEBUG, "Agent reports failure in signing the key");
        SSH_BUFFER_FREE(reply);
        return NULL;
    } else if (type != SSH2_AGENT_SIGN_RESPONSE) {
        ssh_set_error(session,
                      SSH_FATAL,
                      "Bad authentication response: %u",
                      type);
        SSH_BUFFER_FREE(reply);
        return NULL;
    }

    sig_blob = ssh_buffer_get_ssh_string(reply);
    SSH_BUFFER_FREE(reply);

    return sig_blob;
}
