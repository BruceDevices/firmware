/*
 * config.c - parse the ssh config file
 *
 * This file is part of the SSH Library
 *
 * Copyright (c) 2009-2013    by Andreas Schneider <asn@cryptomilk.org>
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

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#ifdef HAVE_GLOB_H
# include <glob.h>
#endif
#include <stdbool.h>
#include <limits.h>
#ifndef _WIN32
# include <sys/types.h>
# include <sys/stat.h>
# include <fcntl.h>
# include <errno.h>
# include <signal.h>
# include <sys/wait.h>
# include <net/if.h>
# include <netinet/in.h>
#endif
#ifdef HAVE_IFADDRS_H
#include <ifaddrs.h>
#endif

#include "libssh/config_parser.h"
#include "libssh/libssh_esp32_config.h"
#include "libssh/priv.h"
#include "libssh/session.h"
#include "libssh/misc.h"
#include "libssh/options.h"

#ifndef MAX_LINE_SIZE
#define MAX_LINE_SIZE 1024
#endif

struct ssh_config_keyword_table_s {
  const char *name;
  enum ssh_config_opcode_e opcode;
};

static struct ssh_config_keyword_table_s ssh_config_keyword_table[] = {
  { "host", SOC_HOST },
  { "match", SOC_MATCH },
  { "hostname", SOC_HOSTNAME },
  { "port", SOC_PORT },
  { "user", SOC_USERNAME },
  { "identityfile", SOC_IDENTITY },
  { "ciphers", SOC_CIPHERS },
  { "macs", SOC_MACS },
  { "compression", SOC_COMPRESSION },
  { "connecttimeout", SOC_TIMEOUT },
  { "stricthostkeychecking", SOC_STRICTHOSTKEYCHECK },
  { "userknownhostsfile", SOC_KNOWNHOSTS },
  { "proxycommand", SOC_PROXYCOMMAND },
  { "gssapiserveridentity", SOC_GSSAPISERVERIDENTITY },
  { "gssapiclientidentity", SOC_GSSAPICLIENTIDENTITY },
  { "gssapidelegatecredentials", SOC_GSSAPIDELEGATECREDENTIALS },
  { "include", SOC_INCLUDE },
  { "bindaddress", SOC_BINDADDRESS},
  { "globalknownhostsfile", SOC_GLOBALKNOWNHOSTSFILE},
  { "loglevel", SOC_LOGLEVEL},
  { "hostkeyalgorithms", SOC_HOSTKEYALGORITHMS},
  { "kexalgorithms", SOC_KEXALGORITHMS},
  { "gssapiauthentication", SOC_GSSAPIAUTHENTICATION},
  { "kbdinteractiveauthentication", SOC_KBDINTERACTIVEAUTHENTICATION},
  { "passwordauthentication", SOC_PASSWORDAUTHENTICATION},
  { "pubkeyauthentication", SOC_PUBKEYAUTHENTICATION},
  { "addkeystoagent", SOC_UNSUPPORTED},
  { "addressfamily", SOC_UNSUPPORTED},
  { "batchmode", SOC_UNSUPPORTED},
  { "canonicaldomains", SOC_UNSUPPORTED},
  { "canonicalizefallbacklocal", SOC_UNSUPPORTED},
  { "canonicalizehostname", SOC_UNSUPPORTED},
  { "canonicalizemaxdots", SOC_UNSUPPORTED},
  { "canonicalizepermittedcnames", SOC_UNSUPPORTED},
  { "certificatefile", SOC_CERTIFICATE},
  { "kbdinteractiveauthentication", SOC_UNSUPPORTED},
  { "checkhostip", SOC_UNSUPPORTED},
  { "connectionattempts", SOC_UNSUPPORTED},
  { "enablesshkeysign", SOC_UNSUPPORTED},
  { "fingerprinthash", SOC_UNSUPPORTED},
  { "forwardagent", SOC_UNSUPPORTED},
  { "hashknownhosts", SOC_UNSUPPORTED},
  { "hostbasedauthentication", SOC_UNSUPPORTED},
  { "hostbasedacceptedalgorithms", SOC_UNSUPPORTED},
  { "hostkeyalias", SOC_UNSUPPORTED},
  { "identitiesonly", SOC_IDENTITIESONLY},
  { "identityagent", SOC_IDENTITYAGENT},
  { "ipqos", SOC_UNSUPPORTED},
  { "kbdinteractivedevices", SOC_UNSUPPORTED},
  { "nohostauthenticationforlocalhost", SOC_UNSUPPORTED},
  { "numberofpasswordprompts", SOC_UNSUPPORTED},
  { "pkcs11provider", SOC_UNSUPPORTED},
  { "preferredauthentications", SOC_UNSUPPORTED},
  { "proxyjump", SOC_PROXYJUMP},
  { "proxyusefdpass", SOC_UNSUPPORTED},
  { "pubkeyacceptedalgorithms", SOC_PUBKEYACCEPTEDKEYTYPES},
  { "rekeylimit", SOC_REKEYLIMIT},
  { "remotecommand", SOC_UNSUPPORTED},
  { "revokedhostkeys", SOC_UNSUPPORTED},
  { "serveralivecountmax", SOC_UNSUPPORTED},
  { "serveraliveinterval", SOC_UNSUPPORTED},
  { "streamlocalbindmask", SOC_UNSUPPORTED},
  { "streamlocalbindunlink", SOC_UNSUPPORTED},
  { "syslogfacility", SOC_UNSUPPORTED},
  { "tcpkeepalive", SOC_UNSUPPORTED},
  { "updatehostkeys", SOC_UNSUPPORTED},
  { "verifyhostkeydns", SOC_UNSUPPORTED},
  { "visualhostkey", SOC_UNSUPPORTED},
  { "clearallforwardings", SOC_NA},
  { "controlmaster", SOC_NA},
  { "controlpersist", SOC_NA},
  { "controlpath", SOC_NA},
  { "dynamicforward", SOC_NA},
  { "escapechar", SOC_NA},
  { "exitonforwardfailure", SOC_NA},
  { "forwardx11", SOC_NA},
  { "forwardx11timeout", SOC_NA},
  { "forwardx11trusted", SOC_NA},
  { "gatewayports", SOC_NA},
  { "ignoreunknown", SOC_NA},
  { "localcommand", SOC_NA},
  { "localforward", SOC_NA},
  { "permitlocalcommand", SOC_NA},
  { "remoteforward", SOC_NA},
  { "requesttty", SOC_NA},
  { "sendenv", SOC_NA},
  { "tunnel", SOC_NA},
  { "tunneldevice", SOC_NA},
  { "xauthlocation", SOC_NA},
  { "pubkeyacceptedkeytypes", SOC_PUBKEYACCEPTEDKEYTYPES},
  { NULL, SOC_UNKNOWN }
};

enum ssh_config_match_e {
    MATCH_UNKNOWN = -1,
    MATCH_ALL,
    MATCH_FINAL,
    MATCH_CANONICAL,
    MATCH_EXEC,
    MATCH_HOST,
    MATCH_ORIGINALHOST,
    MATCH_USER,
    MATCH_LOCALUSER,
    MATCH_LOCALNETWORK
};

struct ssh_config_match_keyword_table_s {
    const char *name;
    enum ssh_config_match_e opcode;
};

static struct ssh_config_match_keyword_table_s
    ssh_config_match_keyword_table[] = {
        {"all", MATCH_ALL},
        {"canonical", MATCH_CANONICAL},
        {"final", MATCH_FINAL},
        {"exec", MATCH_EXEC},
        {"host", MATCH_HOST},
        {"originalhost", MATCH_ORIGINALHOST},
        {"user", MATCH_USER},
        {"localuser", MATCH_LOCALUSER},
        {"localnetwork", MATCH_LOCALNETWORK},
        {NULL, MATCH_UNKNOWN},
};

static int ssh_config_parse_line(ssh_session session, const char *line,
    unsigned int count, int *parsing, unsigned int depth, bool global);

static enum ssh_config_opcode_e ssh_config_get_opcode(char *keyword) {
  int i;

  for (i = 0; ssh_config_keyword_table[i].name != NULL; i++) {
    if (strcasecmp(keyword, ssh_config_keyword_table[i].name) == 0) {
      return ssh_config_keyword_table[i].opcode;
    }
  }

  return SOC_UNKNOWN;
}

#define LIBSSH_CONF_MAX_DEPTH 16
static void
local_parse_file(ssh_session session,
                 const char *filename,
                 int *parsing,
                 unsigned int depth,
                 bool global)
{
    FILE *f = NULL;
    char line[MAX_LINE_SIZE] = {0};
    unsigned int count = 0;
    int rv;

    if (depth > LIBSSH_CONF_MAX_DEPTH) {
        ssh_set_error(session, SSH_FATAL,
                      "ERROR - Too many levels of configuration includes "
                      "when processing file '%s'", filename);
        return;
    }

    f = ssh_strict_fopen(filename, SSH_MAX_CONFIG_FILE_SIZE);
    if (f == NULL) {
        /* The underlying function logs the reasons */
        return;
    }

    SSH_LOG(SSH_LOG_PACKET, "Reading additional configuration data from %s", filename);
    while (fgets(line, sizeof(line), f)) {
        count++;
        rv = ssh_config_parse_line(session, line, count, parsing, depth, global);
        if (rv < 0) {
            fclose(f);
            return;
        }
    }

    fclose(f);
    return;
}

#if defined(HAVE_GLOB) && defined(HAVE_GLOB_GL_FLAGS_MEMBER)
static void local_parse_glob(ssh_session session,
                             const char *fileglob,
                             int *parsing,
                             unsigned int depth,
                             bool global)
{
    glob_t globbuf = {
        .gl_flags = 0,
    };
    int rt;
    size_t i;

    rt = glob(fileglob, GLOB_TILDE, NULL, &globbuf);
    if (rt == GLOB_NOMATCH) {
        globfree(&globbuf);
        return;
    } else if (rt != 0) {
        SSH_LOG(SSH_LOG_RARE, "Glob error: %s",
                fileglob);
        globfree(&globbuf);
        return;
    }

    for (i = 0; i < globbuf.gl_pathc; i++) {
        local_parse_file(session, globbuf.gl_pathv[i], parsing, depth, global);
    }

    globfree(&globbuf);
}
#endif /* HAVE_GLOB HAVE_GLOB_GL_FLAGS_MEMBER */

static enum ssh_config_match_e
ssh_config_get_match_opcode(const char *keyword)
{
    size_t i;

    for (i = 0; ssh_config_match_keyword_table[i].name != NULL; i++) {
        if (strcasecmp(keyword, ssh_config_match_keyword_table[i].name) == 0) {
            return ssh_config_match_keyword_table[i].opcode;
        }
    }

    return MATCH_UNKNOWN;
}

static int
ssh_config_match(char *value, const char *pattern, bool negate)
{
    int ok, result = 0;

    ok = match_pattern_list(value, pattern, strlen(pattern), 0);
    if (ok <= 0 && negate == true) {
        result = 1;
    } else if (ok > 0 && negate == false) {
        result = 1;
    }
    SSH_LOG(SSH_LOG_TRACE, "%s '%s' against pattern '%s'%s (ok=%d)",
            result == 1 ? "Matched" : "Not matched", value, pattern,
            negate == true ? " (negated)" : "", ok);
    return result;
}

#ifdef WITH_EXEC
/* FIXME reuse the ssh_execute_command() from socket.c */
static int
ssh_exec_shell(char *cmd)
{
    char *shell = NULL;
    pid_t pid;
    int status, devnull, rc;
    char err_msg[SSH_ERRNO_MSG_MAX] = {0};

    shell = getenv("SHELL");
    if (shell == NULL || shell[0] == '\0') {
        shell = (char *)"/bin/sh";
    }

    rc = access(shell, X_OK);
    if (rc != 0) {
        SSH_LOG(SSH_LOG_WARN, "The shell '%s' is not executable", shell);
        return -1;
    }

    /* Need this to redirect subprocess stdin/out */
    devnull = open("/dev/null", O_RDWR);
    if (devnull == -1) {
        SSH_LOG(SSH_LOG_WARN, "Failed to open(/dev/null): %s",
                ssh_strerror(errno, err_msg, SSH_ERRNO_MSG_MAX));
        return -1;
    }

    SSH_LOG(SSH_LOG_DEBUG, "Running command '%s'", cmd);
    pid = fork();
    if (pid == 0) { /* Child */
        char *argv[4];

        /* Redirect child stdin and stdout. Leave stderr */
        rc = dup2(devnull, STDIN_FILENO);
        if (rc == -1) {
            SSH_LOG(SSH_LOG_WARN, "dup2: %s",
                    ssh_strerror(errno, err_msg, SSH_ERRNO_MSG_MAX));
            exit(1);
        }
        rc = dup2(devnull, STDOUT_FILENO);
        if (rc == -1) {
            SSH_LOG(SSH_LOG_WARN, "dup2: %s",
                    ssh_strerror(errno, err_msg, SSH_ERRNO_MSG_MAX));
            exit(1);
        }
        if (devnull > STDERR_FILENO) {
            close(devnull);
        }

        argv[0] = shell;
        argv[1] = (char *) "-c";
        argv[2] = strdup(cmd);
        argv[3] = NULL;

        rc = execv(argv[0], argv);
        if (rc == -1) {
            SSH_LOG(SSH_LOG_WARN, "Failed to execute command '%s': %s", cmd,
                    ssh_strerror(errno, err_msg, SSH_ERRNO_MSG_MAX));
            /* Die with signal to make this error apparent to parent. */
            signal(SIGTERM, SIG_DFL);
            kill(getpid(), SIGTERM);
            _exit(1);
        }
    }

    /* Parent */
    close(devnull);
    if (pid == -1) { /* Error */
        SSH_LOG(SSH_LOG_WARN, "Failed to fork child: %s",
                ssh_strerror(errno, err_msg, SSH_ERRNO_MSG_MAX));
        return -1;

    }

    while (waitpid(pid, &status, 0) == -1) {
        if (errno != EINTR) {
            SSH_LOG(SSH_LOG_WARN, "waitpid failed: %s",
                    ssh_strerror(errno, err_msg, SSH_ERRNO_MSG_MAX));
            return -1;
        }
    }
    if (!WIFEXITED(status)) {
        SSH_LOG(SSH_LOG_WARN, "Command %s exited abnormally", cmd);
        return -1;
    }
    SSH_LOG(SSH_LOG_TRACE, "Command '%s' returned %d", cmd, WEXITSTATUS(status));
    return WEXITSTATUS(status);
}

static int
ssh_match_exec(ssh_session session, const char *command, bool negate)
{
    int rv, result = 0;
    char *cmd = NULL;

    /* TODO There should be more supported expansions */
    cmd = ssh_path_expand_escape(session, command);
    if (cmd == NULL) {
        return 0;
    }
    rv = ssh_exec_shell(cmd);
    if (rv > 0 && negate == true) {
        result = 1;
    } else if (rv == 0 && negate == false) {
        result = 1;
    }
    SSH_LOG(SSH_LOG_TRACE, "%s 'exec' command '%s'%s (rv=%d)",
            result == 1 ? "Matched" : "Not matched", cmd,
            negate == true ? " (negated)" : "", rv);
    free(cmd);
    return result;
}
#else
static int
ssh_match_exec(ssh_session session, const char *command, bool negate)
{
    (void)session;
    (void)command;
    (void)negate;

    SSH_LOG(SSH_LOG_TRACE,
            "Unsupported 'exec' command on Windows '%s'",
            command);
    return 0;
}
#endif /* WITH_EXEC */

/**
 * @brief: Parse the ProxyJump configuration line and if parsing,
 * stores the result in the configuration option
 *
 * @param[in]   session    The ssh session
 * @param[in]   s          The string to be parsed.
 * @param[in]   do_parsing Whether to parse or not.
 *
 * @returns     SSH_OK if the provided string is formatted and parsed correctly
 *              SSH_ERROR on failure
 */
int
ssh_config_parse_proxy_jump(ssh_session session, const char *s, bool do_parsing)
{
    char *c = NULL, *cp = NULL, *endp = NULL;
    char *username = NULL;
    char *hostname = NULL;
    char *port = NULL;
    char *next = NULL;
    int cmp, rv = SSH_ERROR;
    struct ssh_jump_info_struct *jump_host = NULL;
    bool parse_entry = do_parsing;
    bool libssh_proxy_jump = ssh_libssh_proxy_jumps();

    /* Special value none disables the proxy */
    cmp = strcasecmp(s, "none");
    if (cmp == 0) {
        if (!libssh_proxy_jump && do_parsing) {
            ssh_options_set(session, SSH_OPTIONS_PROXYCOMMAND, s);
        }
        return SSH_OK;
    }

    /* This is comma-separated list of [user@]host[:port] entries */
    c = strdup(s);
    if (c == NULL) {
        ssh_set_error_oom(session);
        return SSH_ERROR;
    }

    cp = c;
    do {
        endp = strchr(cp, ',');
        if (endp != NULL) {
            /* Split out the token */
            *endp = '\0';
        }
        if (parse_entry && libssh_proxy_jump) {
            jump_host = calloc(1, sizeof(struct ssh_jump_info_struct));
            if (jump_host == NULL) {
                ssh_set_error_oom(session);
                rv = SSH_ERROR;
                goto out;
            }

            rv = ssh_config_parse_uri(cp,
                                      &jump_host->username,
                                      &jump_host->hostname,
                                      &port,
                                      false);
            if (rv != SSH_OK) {
                ssh_set_error_invalid(session);
                SAFE_FREE(jump_host);
                goto out;
            }
            if (port == NULL) {
                jump_host->port = 22;
            } else {
                jump_host->port = strtol(port, NULL, 10);
                SAFE_FREE(port);
            }

            /* Prepend because we will recursively proxy jump */
            rv = ssh_list_prepend(session->opts.proxy_jumps, jump_host);
            if (rv != SSH_OK) {
                ssh_set_error_oom(session);
                SAFE_FREE(jump_host);
                goto out;
            }
        } else if (parse_entry) {
            /* We actually care only about the first item */
            rv = ssh_config_parse_uri(cp, &username, &hostname, &port, false);
            if (rv != SSH_OK) {
                ssh_set_error_invalid(session);
                goto out;
            }
            /* The rest of the list needs to be passed on */
            if (endp != NULL) {
                next = strdup(endp + 1);
                if (next == NULL) {
                    ssh_set_error_oom(session);
                    rv = SSH_ERROR;
                    goto out;
                }
            }
        } else {
            /* The rest is just sanity-checked to avoid failures later */
            rv = ssh_config_parse_uri(cp, NULL, NULL, NULL, false);
            if (rv != SSH_OK) {
                ssh_set_error_invalid(session);
                goto out;
            }
        }
        if (!libssh_proxy_jump) {
            parse_entry = 0;
        }
        if (endp != NULL) {
            cp = endp + 1;
        } else {
            cp = NULL; /* end */
        }
    } while (cp != NULL);

    if (!libssh_proxy_jump && hostname != NULL && do_parsing) {
        char com[512] = {0};

        rv = snprintf(com, sizeof(com), "ssh%s%s%s%s%s%s -W '[%%h]:%%p' %s",
                      username ? " -l " : "",
                      username ? username : "",
                      port ? " -p " : "",
                      port ? port : "",
                      next ? " -J " : "",
                      next ? next : "",
                      hostname);
        if (rv < 0 || rv >= (int)sizeof(com)) {
            SSH_LOG(SSH_LOG_TRACE, "Too long ProxyJump configuration line");
            rv = SSH_ERROR;
            goto out;
        }
        rv = ssh_options_set(session, SSH_OPTIONS_PROXYCOMMAND, com);
        if (rv != SSH_OK) {
            ssh_set_error_oom(session);
            goto out;
        }
    }

    rv = SSH_OK;

out:
    if (rv != SSH_OK) {
        ssh_proxyjumps_free(session->opts.proxy_jumps);
    }
    SAFE_FREE(username);
    SAFE_FREE(hostname);
    SAFE_FREE(port);
    SAFE_FREE(next);
    SAFE_FREE(c);
    return rv;
}

static char *
ssh_config_make_absolute(ssh_session session,
                         const char *path,
                         bool global)
{
    size_t outlen = 0;
    char *out = NULL;
    int rv;

    /* Looks like absolute path */
    if (path[0] == '/') {
        return strdup(path);
    }

    /* relative path */
    if (global) {
        /* Parsing global config */
        outlen = strlen(path) + strlen("/etc/ssh/") + 1;
        out = malloc(outlen);
        if (out == NULL) {
            ssh_set_error_oom(session);
            return NULL;
        }
        rv = snprintf(out, outlen, "/etc/ssh/%s", path);
        if (rv < 1) {
            free(out);
            return NULL;
        }
        return out;
    }

    /* paths starting with tilde are already absolute */
    if (path[0] == '~') {
        return ssh_path_expand_tilde(path);
    }

    /* Parsing user config relative to home directory (generally ~/.ssh) */
    if (session->opts.sshdir == NULL) {
        ssh_set_error_invalid(session);
        return NULL;
    }
    outlen = strlen(path) + strlen(session->opts.sshdir) + 1 + 1;
    out = malloc(outlen);
    if (out == NULL) {
        ssh_set_error_oom(session);
        return NULL;
    }
    rv = snprintf(out, outlen, "%s/%s", session->opts.sshdir, path);
    if (rv < 1) {
        free(out);
        return NULL;
    }
    return out;
}

#ifdef HAVE_IFADDRS_H
/**
 * @brief Checks if host address matches the local network specified.
 *
 * Verify whether a local network interface address matches any of the CIDR
 * patterns.
 *
 * @param addrlist The CIDR pattern-list to be checked, can contain both
 *                 IPv4 and IPv6 addresses and has to be comma separated
 *                 (',' only, space after comma not allowed).
 *
 * @param negate   The negate condition. The return value is negated
 *                 (returns 1 instead of 0 and vice versa).
 *
 * @return 1 if match found.
 * @return 0 if no match found.
 * @return -1 on errors.
 */
static int
ssh_match_localnetwork(const char *addrlist, bool negate)
{
    struct ifaddrs *ifa = NULL, *ifaddrs = NULL;
    int r, found = 0;
    char address[NI_MAXHOST], err_msg[SSH_ERRNO_MSG_MAX] = {0};
    socklen_t sa_len;

    r = getifaddrs(&ifaddrs);
    if (r != 0) {
        SSH_LOG(SSH_LOG_WARN,
                "Match localnetwork: getifaddrs() failed: %s",
                ssh_strerror(r, err_msg, SSH_ERRNO_MSG_MAX));
        return -1;
    }

    for (ifa = ifaddrs; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL || (ifa->ifa_flags & IFF_UP) == 0) {
            continue;
        }

        switch (ifa->ifa_addr->sa_family) {
        case AF_INET:
            sa_len = sizeof(struct sockaddr_in);
            break;
        case AF_INET6:
            sa_len = sizeof(struct sockaddr_in6);
            break;
        default:
            SSH_LOG(SSH_LOG_TRACE,
                    "Interface %s: unsupported address family %d",
                    ifa->ifa_name,
                    ifa->ifa_addr->sa_family);
            continue;
        }

        r = getnameinfo(ifa->ifa_addr,
                        sa_len,
                        address,
                        sizeof(address),
                        NULL,
                        0,
                        NI_NUMERICHOST);
        if (r != 0) {
            SSH_LOG(SSH_LOG_TRACE,
                    "Interface %s getnameinfo failed: %s",
                    ifa->ifa_name,
                    gai_strerror(r));
            continue;
        }
        SSH_LOG(SSH_LOG_TRACE,
                "Interface %s address %s",
                ifa->ifa_name,
                address);

        r = match_cidr_address_list(address,
                                    addrlist,
                                    ifa->ifa_addr->sa_family);
        if (r == 1) {
            SSH_LOG(SSH_LOG_TRACE,
                    "Matched interface %s: address %s in %s",
                    ifa->ifa_name,
                    address,
                    addrlist);
            found = 1;
            break;
        }
    }

    freeifaddrs(ifaddrs);

    return (found == (negate ? 0 : 1));
}
#endif /* HAVE_IFADDRS_H */

static int
ssh_config_parse_line(ssh_session session,
                      const char *line,
                      unsigned int count,
                      int *parsing,
                      unsigned int depth,
                      bool global)
{
  enum ssh_config_opcode_e opcode;
  const char *p = NULL, *p2 = NULL;
  char *s = NULL, *x = NULL;
  char *keyword = NULL;
  char *lowerhost = NULL;
  size_t len;
  int i, rv;
  uint8_t *seen = session->opts.options_seen;
  long l;
  int64_t ll;

  /* Ignore empty lines */
  if (line == NULL || *line == '\0') {
    return 0;
  }

  x = s = strdup(line);
  if (s == NULL) {
    ssh_set_error_oom(session);
    return -1;
  }

  /* Remove trailing spaces */
  for (len = strlen(s) - 1; len > 0; len--) {
    if (! isspace((unsigned char)s[len])) {
      break;
    }
    s[len] = '\0';
  }

  keyword = ssh_config_get_token(&s);
  if (keyword == NULL || *keyword == '#' ||
      *keyword == '\0' || *keyword == '\n') {
    SAFE_FREE(x);
    return 0;
  }

  opcode = ssh_config_get_opcode(keyword);
  if (*parsing == 1 &&
      opcode != SOC_HOST &&
      opcode != SOC_MATCH &&
      opcode != SOC_INCLUDE &&
      opcode != SOC_IDENTITY &&
      opcode != SOC_CERTIFICATE &&
      opcode > SOC_UNSUPPORTED &&
      opcode < SOC_MAX) { /* Ignore all unknown types here */
      /* Skip all the options that were already applied */
      if (seen[opcode] != 0) {
          SAFE_FREE(x);
          return 0;
      }
      seen[opcode] = 1;
  }

  switch (opcode) {
    case SOC_INCLUDE: /* recursive include of other files */

      p = ssh_config_get_str_tok(&s, NULL);
      if (p && *parsing) {
        char *path = ssh_config_make_absolute(session, p, global);
        if (path == NULL) {
          SSH_LOG(SSH_LOG_WARN, "line %d: Failed to allocate memory "
                  "for the include path expansion", count);
          SAFE_FREE(x);
          return -1;
        }
#if defined(HAVE_GLOB) && defined(HAVE_GLOB_GL_FLAGS_MEMBER)
        local_parse_glob(session, path, parsing, depth + 1, global);
#else
        local_parse_file(session, path, parsing, depth + 1, global);
#endif /* HAVE_GLOB */
        free(path);
      }
      break;

    case SOC_MATCH: {
        bool negate;
        int result = 1;
        size_t args = 0;
        enum ssh_config_match_e opt;
        char *localuser = NULL;

        *parsing = 0;
        do {
            p = p2 = ssh_config_get_str_tok(&s, NULL);
            if (p == NULL || p[0] == '\0') {
                break;
            }
            args++;
            SSH_LOG(SSH_LOG_DEBUG, "line %d: Processing Match keyword '%s'",
                    count, p);

            /* If the option is prefixed with ! the result should be negated */
            negate = false;
            if (p[0] == '!') {
                negate = true;
                p++;
            }

            opt = ssh_config_get_match_opcode(p);
            switch (opt) {
            case MATCH_ALL:
                p = ssh_config_get_str_tok(&s, NULL);
                if (args <= 2 && (p == NULL || p[0] == '\0')) {
                    /* The first or second, but last argument. The "all" keyword
                     * can be prefixed with either "final" or "canonical"
                     * keywords which do not have any effect here. */
                    if (negate == true) {
                        result = 0;
                    }
                    break;
                }

                ssh_set_error(session, SSH_FATAL,
                              "line %d: ERROR - Match all cannot be combined with "
                              "other Match attributes", count);
                SAFE_FREE(x);
                return -1;

            case MATCH_FINAL:
            case MATCH_CANONICAL:
                SSH_LOG(SSH_LOG_DEBUG,
                        "line %d: Unsupported Match keyword '%s', skipping",
                        count,
                        p);
                /* Not set any result here -- the result is dependent on the
                 * following matches after this keyword */
                break;

            case MATCH_EXEC:
                /* Skip one argument (including in quotes) */
                p = ssh_config_get_token(&s);
                if (p == NULL || p[0] == '\0') {
                    SSH_LOG(SSH_LOG_TRACE, "line %d: Match keyword "
                            "'%s' requires argument", count, p2);
                    SAFE_FREE(x);
                    return -1;
                }
                if (result != 1) {
                    SSH_LOG(SSH_LOG_DEBUG, "line %d: Skipped match exec "
                            "'%s' as previous conditions already failed.",
                            count, p2);
                    continue;
                }
                result &= ssh_match_exec(session, p, negate);
                args++;
                break;

            case MATCH_LOCALUSER:
                /* Here we match only one argument */
                p = ssh_config_get_str_tok(&s, NULL);
                if (p == NULL || p[0] == '\0') {
                    ssh_set_error(session,
                                  SSH_FATAL,
                                  "line %d: ERROR - Match localuser keyword "
                                  "requires argument",
                                  count);
                    SAFE_FREE(x);
                    return -1;
                }
                localuser = ssh_get_local_username();
                if (localuser == NULL) {
                    SSH_LOG(SSH_LOG_TRACE, "line %d: Can not get local username "
                            "for conditional matching.", count);
                    SAFE_FREE(x);
                    return -1;
                }
                result &= ssh_config_match(localuser, p, negate);
                SAFE_FREE(localuser);
                args++;
                break;

            case MATCH_ORIGINALHOST:
                /* Skip one argument */
                p = ssh_config_get_str_tok(&s, NULL);
                if (p == NULL || p[0] == '\0') {
                    SSH_LOG(SSH_LOG_TRACE, "line %d: Match keyword "
                            "'%s' requires argument", count, p2);
                    SAFE_FREE(x);
                    return -1;
                }
                args++;
                SSH_LOG(SSH_LOG_TRACE,
                        "line %d: Unsupported Match keyword '%s', ignoring",
                        count,
                        p2);
                result = 0;
                break;

            case MATCH_HOST:
                /* Here we match only one argument */
                p = ssh_config_get_str_tok(&s, NULL);
                if (p == NULL || p[0] == '\0') {
                    ssh_set_error(session, SSH_FATAL,
                                  "line %d: ERROR - Match host keyword "
                                  "requires argument", count);
                    SAFE_FREE(x);
                    return -1;
                }
                result &= ssh_config_match(session->opts.host, p, negate);
                args++;
                break;

            case MATCH_USER:
                /* Here we match only one argument */
                p = ssh_config_get_str_tok(&s, NULL);
                if (p == NULL || p[0] == '\0') {
                    ssh_set_error(session, SSH_FATAL,
                                  "line %d: ERROR - Match user keyword "
                                  "requires argument", count);
                    SAFE_FREE(x);
                    return -1;
                }
                result &= ssh_config_match(session->opts.username, p, negate);
                args++;
                break;

            case MATCH_LOCALNETWORK:
                /* Here we match only one argument */
                p = ssh_config_get_str_tok(&s, NULL);
                if (p == NULL || p[0] == '\0') {
                    ssh_set_error(session,
                                  SSH_FATAL,
                                  "line %d: ERROR - Match local network keyword"
                                  "requires argument",
                                  count);
                    SAFE_FREE(x);
                    return -1;
                }
#ifdef HAVE_IFADDRS_H
                rv = match_cidr_address_list(NULL, p, -1);
                if (rv == -1) {
                    ssh_set_error(session,
                                  SSH_FATAL,
                                  "line %d: ERROR - List invalid entry: %s",
                                  count,
                                  p);
                    SAFE_FREE(x);
                    return -1;
                }
                rv = ssh_match_localnetwork(p, negate);
                if (rv == -1) {
                    ssh_set_error(session,
                                  SSH_FATAL,
                                  "line %d: ERROR - Error while retrieving "
                                  "network interface information -"
                                  " List entry: %s",
                                  count,
                                  p);
                    SAFE_FREE(x);
                    return -1;
                }

                result &= rv;
#else /* HAVE_IFADDRS_H */
                ssh_set_error(session,
                              SSH_FATAL,
                              "line %d: ERROR - match localnetwork "
                              "not supported on this platform",
                              count);
                SAFE_FREE(x);
                return -1;
#endif /* HAVE_IFADDRS_H */
                args++;
                break;

            case MATCH_UNKNOWN:
            default:
                SSH_LOG(SSH_LOG_WARN,
                        "Unknown argument '%s' for Match keyword. Not matching",
                        p);
                result = 0;
                break;
            }
        } while (p != NULL && p[0] != '\0');
        if (args == 0) {
            SSH_LOG(SSH_LOG_WARN,
                    "ERROR - Match keyword requires an argument. Not matching");
            result = 0;
        }
        *parsing = result;
        break;
    }
    case SOC_HOST: {
        int ok = 0, result = -1;

        *parsing = 0;
        lowerhost = (session->opts.host) ? ssh_lowercase(session->opts.host) : NULL;
        for (p = ssh_config_get_str_tok(&s, NULL);
             p != NULL && p[0] != '\0';
             p = ssh_config_get_str_tok(&s, NULL)) {
             if (ok >= 0) {
               ok = match_hostname(lowerhost, p, strlen(p));
               if (result == -1 && ok < 0) {
                   result = 0;
               } else if (result == -1 && ok > 0) {
                   result = 1;
               }
            }
        }
        SAFE_FREE(lowerhost);
        if (result != -1) {
            *parsing = result;
        }
        break;
    }
    case SOC_HOSTNAME:
      p = ssh_config_get_str_tok(&s, NULL);
      if (p && *parsing) {
        char *z = ssh_path_expand_escape(session, p);
        if (z == NULL) {
            z = strdup(p);
        }
        ssh_options_set(session, SSH_OPTIONS_HOST, z);
        free(z);
      }
      break;
    case SOC_PORT:
        p = ssh_config_get_str_tok(&s, NULL);
        if (p && *parsing) {
            ssh_options_set(session, SSH_OPTIONS_PORT_STR, p);
        }
        break;
    case SOC_USERNAME:
        p = ssh_config_get_str_tok(&s, NULL);
        if (p && *parsing) {
            ssh_options_set(session, SSH_OPTIONS_USER, p);
        }
        break;
    case SOC_IDENTITY:
      p = ssh_config_get_str_tok(&s, NULL);
      if (p && *parsing) {
        ssh_options_set(session, SSH_OPTIONS_ADD_IDENTITY, p);
      }
      break;
    case SOC_CIPHERS:
      p = ssh_config_get_str_tok(&s, NULL);
      if (p && *parsing) {
        ssh_options_set(session, SSH_OPTIONS_CIPHERS_C_S, p);
        ssh_options_set(session, SSH_OPTIONS_CIPHERS_S_C, p);
      }
      break;
    case SOC_MACS:
      p = ssh_config_get_str_tok(&s, NULL);
      if (p && *parsing) {
        ssh_options_set(session, SSH_OPTIONS_HMAC_C_S, p);
        ssh_options_set(session, SSH_OPTIONS_HMAC_S_C, p);
      }
      break;
    case SOC_COMPRESSION:
      i = ssh_config_get_yesno(&s, -1);
      if (i >= 0 && *parsing) {
        if (i) {
          ssh_options_set(session, SSH_OPTIONS_COMPRESSION, "yes");
        } else {
          ssh_options_set(session, SSH_OPTIONS_COMPRESSION, "no");
        }
      }
      break;
    case SOC_TIMEOUT:
      l = ssh_config_get_long(&s, -1);
      if (l >= 0 && *parsing) {
        ssh_options_set(session, SSH_OPTIONS_TIMEOUT, &l);
      }
      break;
    case SOC_STRICTHOSTKEYCHECK:
      i = ssh_config_get_yesno(&s, -1);
      if (i >= 0 && *parsing) {
        ssh_options_set(session, SSH_OPTIONS_STRICTHOSTKEYCHECK, &i);
      }
      break;
    case SOC_KNOWNHOSTS:
      p = ssh_config_get_str_tok(&s, NULL);
      if (p && *parsing) {
        ssh_options_set(session, SSH_OPTIONS_KNOWNHOSTS, p);
      }
      break;
    case SOC_PROXYCOMMAND:
      p = ssh_config_get_cmd(&s);
      /* We share the seen value with the ProxyJump */
      if (p && *parsing && !seen[SOC_PROXYJUMP]) {
        ssh_options_set(session, SSH_OPTIONS_PROXYCOMMAND, p);
      }
      break;
    case SOC_PROXYJUMP:
        p = ssh_config_get_str_tok(&s, NULL);
        if (p == NULL) {
            SAFE_FREE(x);
            return -1;
        }
        /* We share the seen value with the ProxyCommand */
        rv = ssh_config_parse_proxy_jump(session,
                                         p,
                                         (*parsing && !seen[SOC_PROXYCOMMAND]));
        if (rv != SSH_OK) {
            SAFE_FREE(x);
            return -1;
        }
        break;
    case SOC_GSSAPISERVERIDENTITY:
      p = ssh_config_get_str_tok(&s, NULL);
      if (p && *parsing) {
        ssh_options_set(session, SSH_OPTIONS_GSSAPI_SERVER_IDENTITY, p);
      }
      break;
    case SOC_GSSAPICLIENTIDENTITY:
      p = ssh_config_get_str_tok(&s, NULL);
      if (p && *parsing) {
        ssh_options_set(session, SSH_OPTIONS_GSSAPI_CLIENT_IDENTITY, p);
      }
      break;
    case SOC_GSSAPIDELEGATECREDENTIALS:
      i = ssh_config_get_yesno(&s, -1);
      if (i >=0 && *parsing) {
        ssh_options_set(session, SSH_OPTIONS_GSSAPI_DELEGATE_CREDENTIALS, &i);
      }
      break;
    case SOC_BINDADDRESS:
        p = ssh_config_get_str_tok(&s, NULL);
        if (p && *parsing) {
            ssh_options_set(session, SSH_OPTIONS_BINDADDR, p);
        }
        break;
    case SOC_GLOBALKNOWNHOSTSFILE:
        p = ssh_config_get_str_tok(&s, NULL);
        if (p && *parsing) {
            ssh_options_set(session, SSH_OPTIONS_GLOBAL_KNOWNHOSTS, p);
        }
        break;
    case SOC_LOGLEVEL:
        p = ssh_config_get_str_tok(&s, NULL);
        if (p && *parsing) {
            int value = -1;

            if (strcasecmp(p, "quiet") == 0) {
                value = SSH_LOG_NONE;
            } else if (strcasecmp(p, "fatal") == 0 ||
                    strcasecmp(p, "error")== 0) {
                value = SSH_LOG_WARN;
            } else if (strcasecmp(p, "verbose") == 0 ||
                    strcasecmp(p, "info") == 0) {
                value = SSH_LOG_INFO;
            } else if (strcasecmp(p, "DEBUG") == 0 ||
                    strcasecmp(p, "DEBUG1") == 0) {
                value = SSH_LOG_DEBUG;
            } else if (strcasecmp(p, "DEBUG2") == 0 ||
                    strcasecmp(p, "DEBUG3") == 0) {
                value = SSH_LOG_TRACE;
            }
            if (value != -1) {
                ssh_options_set(session, SSH_OPTIONS_LOG_VERBOSITY, &value);
            }
        }
        break;
    case SOC_HOSTKEYALGORITHMS:
        p = ssh_config_get_str_tok(&s, NULL);
        if (p && *parsing) {
            ssh_options_set(session, SSH_OPTIONS_HOSTKEYS, p);
        }
        break;
    case SOC_PUBKEYACCEPTEDKEYTYPES:
        p = ssh_config_get_str_tok(&s, NULL);
        if (p && *parsing) {
            ssh_options_set(session, SSH_OPTIONS_PUBLICKEY_ACCEPTED_TYPES, p);
        }
        break;
    case SOC_KEXALGORITHMS:
        p = ssh_config_get_str_tok(&s, NULL);
        if (p && *parsing) {
            ssh_options_set(session, SSH_OPTIONS_KEY_EXCHANGE, p);
        }
        break;
    case SOC_REKEYLIMIT:
        /* Parse the data limit */
        p = ssh_config_get_str_tok(&s, NULL);
        if (p == NULL) {
            break;
        } else if (strcmp(p, "default") == 0) {
            /* Default rekey limits enforced automatically */
            ll = 0;
        } else {
            char *endp = NULL;
            ll = strtoll(p, &endp, 10);
            if (p == endp || ll < 0) {
                /* No number or negative */
                SSH_LOG(SSH_LOG_TRACE, "Invalid argument to rekey limit");
                break;
            }
            switch (*endp) {
            case 'G':
                if (ll > LLONG_MAX / 1024) {
                    SSH_LOG(SSH_LOG_TRACE, "Possible overflow of rekey limit");
                    ll = -1;
                    break;
                }
                ll = ll * 1024;
                FALL_THROUGH;
            case 'M':
                if (ll > LLONG_MAX / 1024) {
                    SSH_LOG(SSH_LOG_TRACE, "Possible overflow of rekey limit");
                    ll = -1;
                    break;
                }
                ll = ll * 1024;
                FALL_THROUGH;
            case 'K':
                if (ll > LLONG_MAX / 1024) {
                    SSH_LOG(SSH_LOG_TRACE, "Possible overflow of rekey limit");
                    ll = -1;
                    break;
                }
                ll = ll * 1024;
                endp++;
                FALL_THROUGH;
            case '\0':
                /* just the number */
                break;
            default:
                /* Invalid suffix */
                ll = -1;
                break;
            }
            if (*endp != ' ' && *endp != '\0') {
                SSH_LOG(SSH_LOG_TRACE,
                        "Invalid trailing characters after the rekey limit: %s",
                        endp);
                break;
            }
        }
        if (ll > -1 && *parsing) {
            uint64_t v = (uint64_t)ll;
            ssh_options_set(session, SSH_OPTIONS_REKEY_DATA, &v);
        }
        /* Parse the time limit */
        p = ssh_config_get_str_tok(&s, NULL);
        if (p == NULL) {
            break;
        } else if (strcmp(p, "none") == 0) {
            ll = 0;
        } else {
            char *endp = NULL;
            ll = strtoll(p, &endp, 10);
            if (p == endp || ll < 0) {
                /* No number or negative */
                SSH_LOG(SSH_LOG_TRACE, "Invalid argument to rekey limit");
                break;
            }
            switch (*endp) {
            case 'w':
            case 'W':
                if (ll > LLONG_MAX / 7) {
                    SSH_LOG(SSH_LOG_TRACE, "Possible overflow of rekey limit");
                    ll = -1;
                    break;
                }
                ll = ll * 7;
                FALL_THROUGH;
            case 'd':
            case 'D':
                if (ll > LLONG_MAX / 24) {
                    SSH_LOG(SSH_LOG_TRACE, "Possible overflow of rekey limit");
                    ll = -1;
                    break;
                }
                ll = ll * 24;
                FALL_THROUGH;
            case 'h':
            case 'H':
                if (ll > LLONG_MAX / 60) {
                    SSH_LOG(SSH_LOG_TRACE, "Possible overflow of rekey limit");
                    ll = -1;
                    break;
                }
                ll = ll * 60;
                FALL_THROUGH;
            case 'm':
            case 'M':
                if (ll > LLONG_MAX / 60) {
                    SSH_LOG(SSH_LOG_TRACE, "Possible overflow of rekey limit");
                    ll = -1;
                    break;
                }
                ll = ll * 60;
                FALL_THROUGH;
            case 's':
            case 'S':
                endp++;
                FALL_THROUGH;
            case '\0':
                /* just the number */
                break;
            default:
                /* Invalid suffix */
                ll = -1;
                break;
            }
            if (*endp != '\0') {
                SSH_LOG(SSH_LOG_TRACE, "Invalid trailing characters after the"
                        " rekey limit: %s", endp);
                break;
            }
        }
        if (ll > -1 && *parsing) {
            uint32_t v = (uint32_t)ll;
            ssh_options_set(session, SSH_OPTIONS_REKEY_TIME, &v);
        }
        break;
    case SOC_GSSAPIAUTHENTICATION:
    case SOC_KBDINTERACTIVEAUTHENTICATION:
    case SOC_PASSWORDAUTHENTICATION:
    case SOC_PUBKEYAUTHENTICATION:
        i = ssh_config_get_yesno(&s, 0);
        if (i>=0 && *parsing) {
            switch(opcode){
            case SOC_GSSAPIAUTHENTICATION:
                ssh_options_set(session, SSH_OPTIONS_GSSAPI_AUTH, &i);
                break;
            case SOC_KBDINTERACTIVEAUTHENTICATION:
                ssh_options_set(session, SSH_OPTIONS_KBDINT_AUTH, &i);
                break;
            case SOC_PASSWORDAUTHENTICATION:
                ssh_options_set(session, SSH_OPTIONS_PASSWORD_AUTH, &i);
                break;
            case SOC_PUBKEYAUTHENTICATION:
                ssh_options_set(session, SSH_OPTIONS_PUBKEY_AUTH, &i);
                break;
            /* make gcc happy */
            default:
                break;
            }
        }
        break;
    case SOC_NA:
      SSH_LOG(SSH_LOG_TRACE, "Unapplicable option: %s, line: %d",
              keyword, count);
      break;
    case SOC_UNSUPPORTED:
      SSH_LOG(SSH_LOG_RARE, "Unsupported option: %s, line: %d",
              keyword, count);
      break;
    case SOC_UNKNOWN:
      SSH_LOG(SSH_LOG_TRACE, "Unknown option: %s, line: %d",
              keyword, count);
      break;
    case SOC_IDENTITYAGENT:
      p = ssh_config_get_str_tok(&s, NULL);
      if (p && *parsing) {
          ssh_options_set(session, SSH_OPTIONS_IDENTITY_AGENT, p);
      }
      break;
    case SOC_IDENTITIESONLY:
      i = ssh_config_get_yesno(&s, -1);
      if (i >= 0 && *parsing) {
        bool b = i;
        ssh_options_set(session, SSH_OPTIONS_IDENTITIES_ONLY, &b);
      }
      break;
    case SOC_CONTROLMASTER:
      p = ssh_config_get_str_tok(&s, NULL);
      if (p && *parsing) {
          int value = -1;

          if (strcasecmp(p, "auto") == 0) {
              value = SSH_CONTROL_MASTER_AUTO;
          } else if (strcasecmp(p, "yes") == 0) {
              value = SSH_CONTROL_MASTER_YES;
          } else if (strcasecmp(p, "no") == 0) {
              value = SSH_CONTROL_MASTER_NO;
          } else if (strcasecmp(p, "autoask") == 0) {
              value = SSH_CONTROL_MASTER_AUTOASK;
          } else if (strcasecmp(p, "ask") == 0) {
              value = SSH_CONTROL_MASTER_ASK;
          }

          if (value != -1) {
              ssh_options_set(session, SSH_OPTIONS_CONTROL_MASTER, &value);
          }
      }
      break;
    case SOC_CONTROLPATH:
      p = ssh_config_get_str_tok(&s, NULL);
      if (p == NULL) {
        SAFE_FREE(x);
        return -1;
      }
      if (*parsing) {
          ssh_options_set(session, SSH_OPTIONS_CONTROL_PATH, p);
      }
      break;
    case SOC_CERTIFICATE:
        p = ssh_config_get_str_tok(&s, NULL);
        if (p && *parsing) {
            ssh_options_set(session, SSH_OPTIONS_CERTIFICATE, p);
        }
        break;
    default:
      ssh_set_error(session, SSH_FATAL, "ERROR - unimplemented opcode: %d",
              opcode);
      SAFE_FREE(x);
      return -1;
      break;
  }

  SAFE_FREE(x);
  return 0;
}

/* @brief Parse configuration file and set the options to the given session
 *
 * @params[in] session   The ssh session
 * @params[in] filename  The path to the ssh configuration file
 *
 * @returns    0 on successful parsing the configuration file, -1 on error
 */
int ssh_config_parse_file(ssh_session session, const char *filename)
{
    char line[MAX_LINE_SIZE] = {0};
    unsigned int count = 0;
    FILE *f = NULL;
    int parsing, rv;
    bool global = 0;

    f = ssh_strict_fopen(filename, SSH_MAX_CONFIG_FILE_SIZE);
    if (f == NULL) {
        /* The underlying function logs the reasons */
        return 0;
    }

    rv = strcmp(filename, GLOBAL_CLIENT_CONFIG);
    if (rv == 0) {
        global = true;
    }

    SSH_LOG(SSH_LOG_PACKET, "Reading configuration data from %s", filename);

    parsing = 1;
    while (fgets(line, sizeof(line), f)) {
        count++;
        rv = ssh_config_parse_line(session, line, count, &parsing, 0, global);
        if (rv < 0) {
            fclose(f);
            return -1;
        }
    }

    fclose(f);
    return 0;
}

/* @brief Parse configuration string and set the options to the given session
 *
 * @params[in] session   The ssh session
 * @params[in] input     Null terminated string containing the configuration
 *
 * @returns    SSH_OK on successful parsing the configuration string,
 *             SSH_ERROR on error
 */
int ssh_config_parse_string(ssh_session session, const char *input)
{
    char line[MAX_LINE_SIZE] = {0};
    const char *c = input, *line_start = input;
    unsigned int line_num = 0, line_len;
    int parsing, rv;

    SSH_LOG(SSH_LOG_DEBUG, "Reading configuration data from string:");
    SSH_LOG(SSH_LOG_DEBUG, "START\n%s\nEND", input);

    parsing = 1;
    while (1) {
        line_num++;
        line_start = c;
        c = strchr(line_start, '\n');
        if (c == NULL) {
            /* if there is no newline at the end of the string */
            c = strchr(line_start, '\0');
        }
        if (c == NULL) {
            /* should not happen, would mean a string without trailing '\0' */
            SSH_LOG(SSH_LOG_TRACE, "No trailing '\\0' in config string");
            return SSH_ERROR;
        }
        line_len = c - line_start;
        if (line_len > MAX_LINE_SIZE - 1) {
            SSH_LOG(SSH_LOG_TRACE, "Line %u too long: %u characters",
                    line_num, line_len);
            return SSH_ERROR;
        }
        memcpy(line, line_start, line_len);
        line[line_len] = '\0';
        SSH_LOG(SSH_LOG_DEBUG, "Line %u: %s", line_num, line);
        rv = ssh_config_parse_line(session, line, line_num, &parsing, 0, false);
        if (rv < 0) {
            return SSH_ERROR;
        }
        if (*c == '\0') {
            break;
        }
        c++;
    }

    return SSH_OK;
}
