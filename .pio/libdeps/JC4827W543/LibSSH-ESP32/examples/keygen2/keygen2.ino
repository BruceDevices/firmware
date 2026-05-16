// ESP32 libssh port.
//
// Ewan Parker, created 20th April 2020.
// Simple port of examples/keygen2.c on SPIFFS.  Run with a serial monitor at
// 115200 BAUD.
//
// Copyright (C) 2016–2024 Ewan Parker.

/*
 * keygen2.c - Generate SSH keys using libssh
 * Author: Anderson Toshiyuki Sasaki <ansasaki@redhat.com>
 */

/*
 * Copyright (c) 2019 Red Hat, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see http://www.gnu.org/licenses/.
 */

// The command line you would use to run this from a shell prompt.
#define EX_CMD "keygen2", "--type", "ed25519", "--file", "/spiffs/.ssh/id_ed25519"

// Stack size needed to run SSH and the command parser.
const unsigned int configSTACK = 16384;

// Include the Arduino library.
#include "libssh_esp32.h"

// EXAMPLE includes/defines START
#include "libssh_esp32_config.h"

#include <libssh/libssh.h>
#include <stdio.h>
#include <stdlib.h>
#include "argp.h"
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>

#include <sys/stat.h>
// EXAMPLE includes/defines FINISH

#include <sys/reent.h>
struct _reent reent_data_esp32;
struct _reent *_impure_ptr = &reent_data_esp32;

#include "SPIFFS.h"
#include "driver/uart.h"
#include "esp_vfs_dev.h"

// EXAMPLE functions START
struct arguments_st {
    enum ssh_keytypes_e type;
    unsigned long bits;
    char *file;
    char *passphrase;
    char *format;
    int action_list;
};

static struct argp_option options[] = {
    {
        .name  = "bits",
        .key   = 'b',
        .arg   = "BITS",
        .flags = 0,
        .doc   = "The size of the key to be generated. "
                 "If omitted, a default value is used depending on the TYPE. "
                 "Accepted values are: "
                 "1024, 2048, 3072 (default), 4096, and 8192 for TYPE=\"rsa\"; "
                 "256 (default), 384, and 521 for TYPE=\"ecdsa\"; "
                 "can be omitted for TYPE=\"ed25519\" "
                 "(it will be ignored if provided).\n",
        .group = 0
    },
    {
        .name  = "file",
        .key   = 'f',
        .arg   = "FILE",
        .flags = 0,
        .doc   = "The output file. "
                 "If not provided, the used file name will be generated "
                 "according to the key type as \"id_TYPE\" "
                 "(e.g. \"id_rsa\" for type \"rsa\"). "
                 "The public key file name is generated from the private key "
                 "file name by appending \".pub\".\n",
        .group = 0
    },
    {
        .name  = "passphrase",
        .key   = 'p',
        .arg   = "PASSPHRASE",
        .flags = 0,
        .doc   = "The passphrase used to encrypt the private key. "
                 "If omitted the file will not be encrypted.\n",
        .group = 0
    },
    {
        .name  = "type",
        .key   = 't',
        .arg   = "TYPE",
        .flags = 0,
        .doc   = "The type of the key to be generated. "
                 "Accepted values are: "
                 "\"rsa\", \"ecdsa\", and \"ed25519\".\n",
        .group = 0
    },
    {
        .name  = "list",
        .key   = 'l',
        .arg   = NULL,
        .flags = 0,
        .doc   = "List the Fingerprint of the given key\n",
        .group = 0
    },
    {
        .name  = "format",
        .key   = 'm',
        .arg   = "FORMAT",
        .flags = 0,
        .doc   = "Write the file in specific format. The supported values are "
                "'PEM'and 'OpenSSH' file format. By default Ed25519 "
                "keys are exported in OpenSSH format and others in PEM.\n",
        .group = 0
    },
    {
        /* End of the options */
        0
    },
};

/* Parse a single option. */
static error_t parse_opt (int key, char *arg, struct argp_state *state)
{
    /* Get the input argument from argp_parse, which we
     * know is a pointer to our arguments structure.
     */
    struct arguments_st *arguments = NULL;
    error_t rc = 0;

    if (state == NULL) {
        return EINVAL;
    }

    arguments = (arguments_st*)state->input;
    if (arguments == NULL) {
        fprintf(stderr, "Error: NULL pointer to arguments structure "
                "provided\n");
        rc = EINVAL;
        goto end;
    }

    switch (key) {
        case 'b':
            errno = 0;
            arguments->bits = strtoul(arg, NULL, 10);
            if (errno != 0) {
                rc = errno;
                goto end;
            }
            break;
        case 'f':
            arguments->file = strdup(arg);
            if (arguments->file == NULL) {
                fprintf(stderr, "Error: Out of memory\n");
                rc = ENOMEM;
                goto end;
            }
            break;
        case 'p':
            arguments->passphrase = strdup(arg);
            if (arguments->passphrase == NULL) {
                fprintf(stderr, "Error: Out of memory\n");
                rc = ENOMEM;
                goto end;
            }
            break;
        case 't':
            if (!strcmp(arg, "rsa")) {
                arguments->type = SSH_KEYTYPE_RSA;
            }
            else if (!strcmp(arg, "ecdsa")) {
                arguments->type = SSH_KEYTYPE_ECDSA;
            }
            else if (!strcmp(arg, "ed25519")) {
                arguments->type = SSH_KEYTYPE_ED25519;
            }
            else {
                fprintf(stderr, "Error: Invalid key type\n");
                argp_usage(state);
                rc = EINVAL;
                goto end;
            }
            break;
        case 'l':
            arguments->action_list = 1;
            break;
        case 'm':
            arguments->format = strdup(arg);
            break;
        case ARGP_KEY_ARG:
            if (state->arg_num > 0) {
                /* Too many arguments. */
                printf("Error: Too many arguments\n");
                argp_usage(state);
            }
            break;
        case ARGP_KEY_END:
            break;
        default:
            return ARGP_ERR_UNKNOWN;
    }

end:
    return rc;
}

static int validate_args(struct arguments_st *args)
{
    int rc = 0;

    if (args == NULL) {
        return EINVAL;
    }

    /* no other arguments needed for listing key fingerprints */
    if (args->action_list) {
        return 0;
    }

    switch (args->type) {
    case SSH_KEYTYPE_RSA:
        switch (args->bits) {
        case 0:
            /* If not provided, use default value */
            args->bits = 3072;
            break;
        case 1024:
        case 2048:
        case 3072:
        case 4096:
        case 8192:
            break;
        default:
            fprintf(stderr, "Error: Invalid bits parameter provided\n");
            rc = EINVAL;
            break;
        }

        if (args->file == NULL) {
            args->file = strdup("id_rsa");
            if (args->file == NULL) {
                rc = ENOMEM;
                break;
            }
        }

        break;
    case SSH_KEYTYPE_ECDSA:
        switch (args->bits) {
        case 0:
            /* If not provided, use default value */
            args->bits = 256;
            break;
        case 256:
        case 384:
        case 521:
            break;
        default:
            fprintf(stderr, "Error: Invalid bits parameter provided\n");
            rc = EINVAL;
            break;
        }
        if (args->file == NULL) {
            args->file = strdup("id_ecdsa");
            if (args->file == NULL) {
                rc = ENOMEM;
                break;
            }
        }

        break;
    case SSH_KEYTYPE_ED25519:
        /* Ignore value and overwrite with a zero */
        args->bits = 0;

        if (args->file == NULL) {
            args->file = strdup("id_ed25519");
            if (args->file == NULL) {
                rc = ENOMEM;
                break;
            }
        }

        break;
    default:
        fprintf(stderr, "Error: unknown key type\n");
        rc = EINVAL;
        break;
    }

    return rc;
}

/* Program documentation. */
static char doc[] = "Generate an SSH key pair. "
                    "The \"--type\" (short: \"-t\") option is required.";

/* Our argp parser */
static struct argp argp = {options, parse_opt, NULL, doc, NULL, NULL, NULL};

static void list_fingerprint(char *file)
{
    ssh_key key = NULL;
    unsigned char *hash = NULL;
    size_t hlen = 0;
    int rc;

    rc = ssh_pki_import_privkey_file(file, NULL, NULL, NULL, &key);
    if (rc != SSH_OK) {
        fprintf(stderr, "Failed to import private key %s\n", file);
        return;
    }

    rc = ssh_get_publickey_hash(key, SSH_PUBLICKEY_HASH_SHA256, &hash, &hlen);
    if (rc != SSH_OK) {
        fprintf(stderr, "Failed to get key fingerprint\n");
        ssh_key_free(key);
        return;
    }
    ssh_print_hash(SSH_PUBLICKEY_HASH_SHA256, hash, hlen);

    ssh_clean_pubkey_hash(&hash);
    ssh_key_free(key);
}
// EXAMPLE functions FINISH

// EXAMPLE main START
int ex_main(int argc, char **argv){
    ssh_key key = NULL;
    int rc = 0;
    char overwrite[1024] = "";

    char *pubkey_file = NULL;

    struct arguments_st arguments = {
        .type = SSH_KEYTYPE_UNKNOWN,
        .bits = 0,
        .file = NULL,
        .passphrase = NULL,
        .action_list = 0,
    };

    if (argc < 2) {
        argp_help(&argp, stdout, ARGP_HELP_DOC | ARGP_HELP_USAGE, argv[0]);
        goto end;
    }

    rc = argp_parse(&argp, argc, argv, 0, 0, &arguments);
    if (rc != 0) {
        goto end;
    }

    rc = validate_args(&arguments);
    if (rc != 0) {
        goto end;
    }

    if (arguments.action_list && arguments.file) {
        list_fingerprint(arguments.file);
        goto end;
    }

    errno = 0;
    rc = open(arguments.file, O_CREAT | O_EXCL | O_WRONLY, S_IRUSR | S_IWUSR);
    if (rc < 0) {
        if (errno == EEXIST) {
            printf("File \"%s\" exists. Overwrite it? (y|n) ", arguments.file);
            rc = scanf("%1023s", overwrite);
            if (rc > 0 && tolower(overwrite[0]) == 'y') {
                rc = open(arguments.file, O_WRONLY);
                if (rc > 0) {
                    close(rc);
                    errno = 0;
                    /*
                    rc = chmod(arguments.file, S_IRUSR | S_IWUSR);
                    if (rc != 0) {
                        fprintf(stderr,
                                "Error(%d): Could not set file permissions\n",
                                errno);
                        goto end;
                    }
                    */
                } else {
                    fprintf(stderr,
                            "Error: Could not create private key file\n");
                    goto end;
                }
            } else {
                goto end;
            }
        } else {
            fprintf(stderr, "Error opening \"%s\" file\n", arguments.file);
            goto end;
        }
    } else {
        close(rc);
    }

    /* Generate a new private key */
    rc = ssh_pki_generate(arguments.type, arguments.bits, &key);
    if (rc != SSH_OK) {
        fprintf(stderr, "Error: Failed to generate keys");
        goto end;
    }

    /* Write the private key */
    if (arguments.format != NULL) {
        if (strcasecmp(arguments.format, "PEM") == 0) {
            rc = ssh_pki_export_privkey_file_format(key,
                                                    arguments.passphrase,
                                                    NULL,
                                                    NULL,
                                                    arguments.file,
                                                    SSH_FILE_FORMAT_PEM);
        } else if (strcasecmp(arguments.format, "OpenSSH") == 0) {
            rc = ssh_pki_export_privkey_file_format(key,
                                                    arguments.passphrase,
                                                    NULL,
                                                    NULL,
                                                    arguments.file,
                                                    SSH_FILE_FORMAT_OPENSSH);
        } else {
            rc = ssh_pki_export_privkey_file_format(key,
                                                    arguments.passphrase,
                                                    NULL,
                                                    NULL,
                                                    arguments.file,
                                                    SSH_FILE_FORMAT_DEFAULT);
        }
    } else {
        rc = ssh_pki_export_privkey_file(key,
                                         arguments.passphrase,
                                         NULL,
                                         NULL,
                                         arguments.file);
    }
    if (rc != SSH_OK) {
        fprintf(stderr, "Error: Failed to write private key file");
        goto end;
    }

    /* If a passphrase was provided, overwrite and free it as it is not needed
     * anymore */
    if (arguments.passphrase != NULL) {
#ifdef HAVE_EXPLICIT_BZERO
        explicit_bzero(arguments.passphrase, strlen(arguments.passphrase));
#else
        bzero(arguments.passphrase, strlen(arguments.passphrase));
#endif
        free(arguments.passphrase);
        arguments.passphrase = NULL;
    }

    pubkey_file = (char *)malloc(strlen(arguments.file) + 5);
    if (pubkey_file == NULL) {
        rc = ENOMEM;
        goto end;
    }

    sprintf(pubkey_file, "%s.pub", arguments.file);

    errno = 0;
    rc = open(pubkey_file,
              O_CREAT | O_EXCL | O_WRONLY,
              S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (rc < 0) {
        if (errno == EEXIST) {
            printf("File \"%s\" exists. Overwrite it? (y|n) ", pubkey_file);
            rc = scanf("%1023s", overwrite);
            if (rc > 0 && tolower(overwrite[0]) == 'y') {
                rc = open(pubkey_file, O_WRONLY);
                if (rc > 0) {
                    close(rc);
                    errno = 0;
                    /*
                    rc = chmod(pubkey_file,
                               S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
                    if (rc != 0) {
                        fprintf(stderr,
                                "Error(%d): Could not set file permissions\n",
                                errno);
                        goto end;
                    }
                    */
                } else {
                    fprintf(stderr,
                            "Error: Could not create public key file\n");
                    goto end;
                }
            } else {
                goto end;
            }
        } else {
            fprintf(stderr, "Error opening \"%s\" file\n", pubkey_file);
            goto end;
        }
    } else {
        close(rc);
    }

    /* Write the public key */
    rc = ssh_pki_export_pubkey_file(key, pubkey_file);
    if (rc != SSH_OK) {
        fprintf(stderr, "Error: Failed to write public key file");
        goto end;
    }

end:
    if (key != NULL) {
        ssh_key_free(key);
    }

    if (arguments.file != NULL) {
        free(arguments.file);
    }

    if (arguments.passphrase != NULL) {
#ifdef HAVE_EXPLICIT_BZERO
        explicit_bzero(arguments.passphrase, strlen(arguments.passphrase));
#else
        bzero(arguments.passphrase, strlen(arguments.passphrase));
#endif
        free(arguments.passphrase);
    }

    if (pubkey_file != NULL) {
        free(pubkey_file);
    }
    return rc;
}
// EXAMPLE main FINISH

void controlTask(void *pvParameter)
{
  _REENT_INIT_PTR((&reent_data_esp32));

  // Mount the file system.
  boolean fsGood = SPIFFS.begin();
  if (!fsGood)
  {
    printf("%% No formatted SPIFFS filesystem found to mount.\n");
    printf("%% Format SPIFFS and mount now (NB. may cause data loss) [y/n]?\n");
    while (!Serial.available()) {}
    char c = Serial.read();
    if (c == 'y' || c == 'Y')
    {
      printf("%% Formatting...\n");
      fsGood = SPIFFS.format();
      if (fsGood) SPIFFS.begin();
    }
  }
  if (!fsGood)
  {
    printf("%% Aborting now.\n");
    while (1) vTaskDelay(60000 / portTICK_PERIOD_MS);
  }
  printf(
    "%% Mounted SPIFFS used=%d total=%d\r\n", SPIFFS.usedBytes(),
    SPIFFS.totalBytes());

  // Initialize the Arduino library.
  libssh_begin();

  // Call the EXAMPLE main code.
  {
          const char *ex_argv[] = { EX_CMD, NULL };
          int ex_argc = sizeof ex_argv/sizeof ex_argv[0] - 1;
          printf("%% Execution in progress:");
          short a; for (a = 0; a < ex_argc; a++) printf(" %s", ex_argv[a]);
          long start_millis = millis();
          printf("\n[SNIP STDOUT START]\n");
          int ex_rc = ex_main(ex_argc, (char**)ex_argv);
          printf("[SNIP STDOUT FINISH]\n");
          printf("%% Execution completed: rc=%d, elapsed=%ldms\n",
            ex_rc, (long)millis() - start_millis);
  }
  while (1) vTaskDelay(60000 / portTICK_PERIOD_MS);
  // Finished the EXAMPLE main code.
}

void setup()
{
  // Use the expected blocking I/O behavior.
  setvbuf(stdin, NULL, _IONBF, 0);
  setvbuf(stdout, NULL, _IONBF, 0);
  #if ESP_IDF_VERSION_MAJOR >= 4
  Serial.begin(115200);
  uart_driver_install
    ((uart_port_t)CONFIG_ESP_CONSOLE_UART_NUM, 256, 0, 0, NULL, 0);
  esp_vfs_dev_uart_use_driver(CONFIG_ESP_CONSOLE_UART_NUM);
  #else
  uart_driver_install((uart_port_t)CONFIG_CONSOLE_UART_NUM, 256, 0, 0, NULL, 0);
  esp_vfs_dev_uart_use_driver(CONFIG_CONSOLE_UART_NUM);
  Serial.begin(115200);
  #endif

  // Stack size needs to be larger, so continue in a new task.
  xTaskCreatePinnedToCore(controlTask, "ctl", configSTACK, NULL,
    (tskIDLE_PRIORITY + 3), NULL, portNUM_PROCESSORS - 1);
}

void loop()
{
  // Nothing to do here since controlTask has taken over.
  vTaskDelay(60000 / portTICK_PERIOD_MS);
}
