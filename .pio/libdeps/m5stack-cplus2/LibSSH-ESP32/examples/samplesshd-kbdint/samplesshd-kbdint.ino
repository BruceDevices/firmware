// ESP32 libssh port.
//
// Ewan Parker, created 18th April 2020.
// Simple port of examples/samplesshd-kbdint.c over WiFi.  Run with a serial
// monitor at 115200 BAUD.
//
// Copyright (C) 2016–2025 Ewan Parker.

/* This is a sample implementation of a libssh based SSH server */
/*
Copyright 2003-2011 Aris Adamantiadis

This file is part of the SSH Library

You are free to copy this file, modify it in any way, consider it being public
domain. This does not apply to the rest of the library though, but it is
allowed to cut-and-paste working code from this file to any license of
program.
The goal is to show the API in action. It's not a reference on how terminal
clients must be made or how a client should react.
*/

// Set local WiFi credentials below.
const char *configSTASSID = "YourWiFiSSID";
const char *configSTAPSK = "YourWiFiPSK";

// The command line you would use to run this from a shell prompt.
#define EX_CMD "samplesshd-kbdint", "--hostkey", "/spiffs/.ssh/id_ed25519", \
               "::"

// SSH key storage location.
#define KEYS_FOLDER "/spiffs/"

// Stack size needed to run SSH.
const unsigned int configSTACK = 10240;

#include <arpa/inet.h>
#include "esp_netif.h"
#if ESP_IDF_VERSION_MAJOR <= 4
#include "IPv6Address.h"
#endif
#include "WiFi.h"
// Include the Arduino library.
#include "libssh_esp32.h"

// Use argument parsing.
#define HAVE_ARGP_H

// EXAMPLE includes/defines START
#include "libssh_esp32_config.h"

#include <libssh/libssh.h>
#include <libssh/server.h>

#ifdef HAVE_ARGP_H
#include "argp.h"
#endif
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

#ifndef BUF_SIZE
#define BUF_SIZE 2048
#endif

#define SSHD_USER "libssh"
#define SSHD_PASSWORD "libssh"

#ifndef KEYS_FOLDER
#ifdef _WIN32
#define KEYS_FOLDER
#else
#define KEYS_FOLDER "/etc/ssh/"
#endif
#endif

static int port = 22;
static bool authenticated = false;
// EXAMPLE includes/defines FINISH

#include <sys/reent.h>
struct _reent reent_data_esp32;
struct _reent *_impure_ptr = &reent_data_esp32;

volatile bool wifiPhyConnected;

// Timing and timeout configuration.
#define WIFI_TIMEOUT_S 10
#define NET_WAIT_MS 100

// Networking state of this esp32 device.
typedef enum
{
  STATE_NEW,
  STATE_PHY_CONNECTED,
  STATE_WAIT_IPADDR,
  STATE_GOT_IPADDR,
  STATE_OTA_UPDATING,
  STATE_OTA_COMPLETE,
  STATE_LISTENING,
  STATE_TCP_DISCONNECTED
} devState_t;

static volatile devState_t devState;
static volatile bool gotIpAddr, gotIp6Addr;

#include "SPIFFS.h"

// EXAMPLE functions START
#ifdef WITH_PCAP
static const char *pcap_file = "debug.server.pcap";
static ssh_pcap_file pcap;

static void set_pcap(ssh_session session){
        if(!pcap_file)
                return;
        pcap=ssh_pcap_file_new();
        if(ssh_pcap_file_open(pcap,pcap_file) == SSH_ERROR){
                printf("Error opening pcap file\n");
                ssh_pcap_file_free(pcap);
                pcap=NULL;
                return;
        }
        ssh_set_pcap_file(session,pcap);
}

static void cleanup_pcap(void) {
        ssh_pcap_file_free(pcap);
        pcap=NULL;
}
#endif


static int auth_password(const char *user, const char *password)
{
    int cmp;

    cmp = strcmp(user, SSHD_USER);
    if (cmp != 0) {
        return 0;
    }
    cmp = strcmp(password, SSHD_PASSWORD);
    if (cmp != 0) {
        return 0;
    }

    authenticated = true;
    return 1; // authenticated
}
#ifdef HAVE_ARGP_H
const char *argp_program_version = "libssh server example "
  SSH_STRINGIFY(LIBSSH_VERSION);
const char *argp_program_bug_address = "<libssh@libssh.org>";

/* Program documentation. */
static char doc[] = "libssh -- a Secure Shell protocol implementation";

/* A description of the arguments we accept. */
static char args_doc[] = "BINDADDR";

/* The options we understand. */
static struct argp_option options[] = {
  {
    .name  = "port",
    .key   = 'p',
    .arg   = "PORT",
    .flags = 0,
    .doc   = "Set the port to bind.",
    .group = 0
  },
  {
    .name  = "hostkey",
    .key   = 'k',
    .arg   = "FILE",
    .flags = 0,
    .doc   = "Set the host key.",
    .group = 0
  },
  {
    .name  = "rsakey",
    .key   = 'r',
    .arg   = "FILE",
    .flags = 0,
    .doc   = "Set the rsa key (deprecated alias for 'k').",
    .group = 0
  },
  {
    .name  = "verbose",
    .key   = 'v',
    .arg   = NULL,
    .flags = 0,
    .doc   = "Get verbose output.",
    .group = 0
  },
  {NULL, 0, 0, 0, NULL, 0}
};

/* Parse a single option. */
static error_t parse_opt (int key, char *arg, struct argp_state *state) {
  /* Get the input argument from argp_parse, which we
   * know is a pointer to our arguments structure.
   */
  ssh_bind sshbind = (ssh_bind)state->input;

  switch (key) {
    case 'p':
      ssh_bind_options_set(sshbind, SSH_BIND_OPTIONS_BINDPORT_STR, arg);
      port = atoi(arg);
      break;
    case 'r':
    case 'k':
      ssh_bind_options_set(sshbind, SSH_BIND_OPTIONS_HOSTKEY, arg);
      break;
    case 'v':
      ssh_bind_options_set(sshbind, SSH_BIND_OPTIONS_LOG_VERBOSITY_STR, "3");
      break;
    case ARGP_KEY_ARG:
      if (state->arg_num >= 1) {
        /* Too many arguments. */
        argp_usage (state);
      }
      ssh_bind_options_set(sshbind, SSH_BIND_OPTIONS_BINDADDR, arg);
      break;
    case ARGP_KEY_END:
      if (state->arg_num < 1) {
        /* Not enough arguments. */
        argp_usage (state);
      }
      break;
    default:
      return ARGP_ERR_UNKNOWN;
  }

  return 0;
}

/* Our argp parser. */
static struct argp argp = {options, parse_opt, args_doc, doc, NULL, NULL, NULL};
#endif /* HAVE_ARGP_H */

static const char *name = NULL;
static const char *instruction = NULL;
static const char *prompts[2];
static char echo[] = { 1, 0 };

static int kbdint_check_response(ssh_session session) {
    int count;

    count = ssh_userauth_kbdint_getnanswers(session);
    if(count != 2) {
        instruction = "Something weird happened :(";
        return 0;
    }
    if(strcasecmp("Arthur Dent",
                        ssh_userauth_kbdint_getanswer(session, 0)) != 0) {
        instruction = "OK, this is not YOUR name, "
                                        "but it's a reference to the HGTG...";
        prompts[0] = "The main character's full name: ";
        return 0;
    } 
    if(strcmp("42", ssh_userauth_kbdint_getanswer(session, 1)) != 0) {
        instruction = "Make an effort !!! What is the Answer to the Ultimate "
                            "Question of Life, the Universe, and Everything ?";
        prompts[1] = "Answer to the Ultimate Question of Life, the Universe, "
                            "and Everything: ";
        return 0;
    }

    authenticated = true;
    return 1;
}

static int authenticate(ssh_session session) {
    ssh_message message;

    name = "\n\nKeyboard-Interactive Fancy Authentication\n";
    instruction = "Please enter your real name and your password";
    prompts[0] = "Real name: ";
    prompts[1] = "Password: ";

    do {
        message=ssh_message_get(session);
        if(!message)
            break;
        switch(ssh_message_type(message)){
            case SSH_REQUEST_AUTH:
                switch(ssh_message_subtype(message)){
                    case SSH_AUTH_METHOD_PASSWORD:
                        printf("User %s wants to auth with pass %s\n",
                               ssh_message_auth_user(message),
                               ssh_message_auth_password(message));
                        if(auth_password(ssh_message_auth_user(message),
                           ssh_message_auth_password(message))){
                               ssh_message_auth_reply_success(message,0);
                               ssh_message_free(message);
                               return 1;
                           }
                        ssh_message_auth_set_methods(message,
                                                SSH_AUTH_METHOD_PASSWORD |
                                                SSH_AUTH_METHOD_INTERACTIVE);
                        // not authenticated, send default message
                        ssh_message_reply_default(message);
                        break;

                    case SSH_AUTH_METHOD_INTERACTIVE:
                        if(!ssh_message_auth_kbdint_is_response(message)) {
                            printf("User %s wants to auth with kbdint\n",
                                   ssh_message_auth_user(message));
                            ssh_message_auth_interactive_request(message, name,
                                                    instruction, 2, prompts, echo);
                        } else {
                            if(kbdint_check_response(session)) {
                                ssh_message_auth_reply_success(message,0);
                                ssh_message_free(message);
                                return 1;
                            }
                            ssh_message_auth_set_methods(message,
                                                    SSH_AUTH_METHOD_PASSWORD |
                                                    SSH_AUTH_METHOD_INTERACTIVE);
                            ssh_message_reply_default(message);
                        }
                        break;
                    case SSH_AUTH_METHOD_NONE:
                    default:
                        printf("User %s wants to auth with unknown auth %d\n",
                               ssh_message_auth_user(message),
                               ssh_message_subtype(message));
                        ssh_message_auth_set_methods(message,
                                                SSH_AUTH_METHOD_PASSWORD |
                                                SSH_AUTH_METHOD_INTERACTIVE);
                        ssh_message_reply_default(message);
                        break;
                }
                break;
            default:
                ssh_message_auth_set_methods(message,
                                                SSH_AUTH_METHOD_PASSWORD |
                                                SSH_AUTH_METHOD_INTERACTIVE);
                ssh_message_reply_default(message);
        }
        ssh_message_free(message);
    } while (1);
    return 0;
}
// EXAMPLE functions FINISH

// EXAMPLE main START
int ex_main(int argc, char **argv)
{
    ssh_session session = NULL;
    ssh_bind sshbind = NULL;
    ssh_message message = NULL;
    ssh_channel chan = NULL;
    char buf[BUF_SIZE];
    int auth=0;
    int shell=0;
    int i;
    int r;

    sshbind=ssh_bind_new();
    session=ssh_new();

    ssh_bind_options_set(sshbind, SSH_BIND_OPTIONS_HOSTKEY,
                         KEYS_FOLDER "ssh_host_rsa_key");

#ifdef HAVE_ARGP_H
    /*
     * Parse our arguments; every option seen by parse_opt will
     * be reflected in arguments.
     */
    argp_parse (&argp, argc, argv, 0, 0, sshbind);
#else
    (void) argc;
    (void) argv;
#endif
#ifdef WITH_PCAP
    set_pcap(session);
#endif

    if(ssh_bind_listen(sshbind)<0){
        printf("Error listening to socket: %s\n", ssh_get_error(sshbind));
        return 1;
    }
    printf("Started sample libssh sshd on port %d\n", port);
    printf("You can login as the user %s with the password %s\n", SSHD_USER,
                                                            SSHD_PASSWORD);
    r = ssh_bind_accept(sshbind, session);
    if(r==SSH_ERROR){
      printf("Error accepting a connection: %s\n", ssh_get_error(sshbind));
      return 1;
    }
    if (ssh_handle_key_exchange(session)) {
        printf("ssh_handle_key_exchange: %s\n", ssh_get_error(session));
        return 1;
    }

    /* proceed to authentication */
    auth = authenticate(session);
    if (!auth || !authenticated) {
        printf("Authentication error: %s\n", ssh_get_error(session));
        ssh_disconnect(session);
        return 1;
    }


    /* wait for a channel session */
    do {
        message = ssh_message_get(session);
        if(message){
            if(ssh_message_type(message) == SSH_REQUEST_CHANNEL_OPEN &&
                    ssh_message_subtype(message) == SSH_CHANNEL_SESSION) {
                chan = ssh_message_channel_request_open_reply_accept(message);
                ssh_message_free(message);
                break;
            } else {
                ssh_message_reply_default(message);
                ssh_message_free(message);
            }
        } else {
            break;
        }
    } while(!chan);

    if (!chan) {
        printf("Error: client did not ask for a channel session (%s)\n",
               ssh_get_error(session));
        ssh_finalize();
        return 1;
    }


    /* wait for a shell */
    do {
        message = ssh_message_get(session);
        if(message != NULL) {
            if(ssh_message_type(message) == SSH_REQUEST_CHANNEL &&
                    ssh_message_subtype(message) == SSH_CHANNEL_REQUEST_SHELL) {
                shell = 1;
                ssh_message_channel_request_reply_success(message);
                ssh_message_free(message);
                break;
            }
            ssh_message_reply_default(message);
            ssh_message_free(message);
        } else {
            break;
        }
    } while(!shell);

    if(!shell) {
        printf("Error: No shell requested (%s)\n", ssh_get_error(session));
        return 1;
    }


    printf("it works !\n");
    do{
        i=ssh_channel_read(chan,buf, sizeof(buf), 0);
        if(i>0) {
            if(*buf == '' || *buf == '')
                    break;
            if(i == 1 && *buf == '\r')
                ssh_channel_write(chan, "\r\n", 2);
            else
                ssh_channel_write(chan, buf, i);
            if (write(1,buf,i) < 0) {
                printf("error writing to buffer\n");
                return 1;
            }
        }
    } while (i>0);
    ssh_channel_close(chan);
    ssh_disconnect(session);
    ssh_bind_free(sshbind);
#ifdef WITH_PCAP
    cleanup_pcap();
#endif
    ssh_finalize();
    return 0;
}
// EXAMPLE main FINISH

#define newDevState(s) (devState = s)

void event_cb(void *args, esp_event_base_t base, int32_t id, void* event_data)
{
  switch(id)
  {
    case WIFI_EVENT_STA_START:
      Serial.print("% WiFi enabled with SSID=");
      Serial.println(configSTASSID);
      break;
    case WIFI_EVENT_STA_CONNECTED:
      Serial.println("% WiFi connected");
      wifiPhyConnected = true;
      if (devState < STATE_PHY_CONNECTED) newDevState(STATE_PHY_CONNECTED);
      break;
    case WIFI_EVENT_STA_DISCONNECTED:
      if (devState < STATE_WAIT_IPADDR) newDevState(STATE_NEW);
      if (wifiPhyConnected)
      {
        Serial.println("% WiFi disconnected");
        wifiPhyConnected = false;
      }
      WiFi.begin(configSTASSID, configSTAPSK);
      break;
    case IP_EVENT_GOT_IP6:
      {
        ip_event_got_ip6_t* event = (ip_event_got_ip6_t*) event_data;
        if (event->ip6_info.ip.addr[0] != htons(0xFE80) && !gotIp6Addr)
        {
          gotIp6Addr = true;
        }
        Serial.print("% IPv6 Address: ");
        #if ESP_IDF_VERSION_MAJOR >= 5
        Serial.println(IPAddress((const uint8_t*)&event->ip6_info.ip.addr));
        #else
        Serial.println(IPv6Address(event->ip6_info.ip.addr));
        #endif
      }
      break;
    case IP_EVENT_STA_GOT_IP:
      {
        #if ESP_IDF_VERSION_MAJOR >= 5
        WiFi.enableIPv6(); // Under IDF 5 we need to get IPv4 address first.
        #else
        WiFi.enableIpV6(); // Under IDF 5 we need to get IPv4 address first.
        #endif
        gotIpAddr = true;
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        Serial.print("% IPv4 Address: ");
        Serial.println(IPAddress(event->ip_info.ip.addr));
      }
      break;
    case IP_EVENT_STA_LOST_IP:
      //gotIpAddr = false;
    default:
      break;
  }
}

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

  wifiPhyConnected = false;
  WiFi.disconnect(true);
  WiFi.mode(WIFI_MODE_STA);
  gotIpAddr = false; gotIp6Addr = false;
  WiFi.begin(configSTASSID, configSTAPSK);

  TickType_t xStartTime;
  xStartTime = xTaskGetTickCount();
  const TickType_t xTicksTimeout = WIFI_TIMEOUT_S*1000/portTICK_PERIOD_MS;
  bool aborting;

  while (1)
  {
    switch (devState)
    {
      case STATE_NEW :
        vTaskDelay(NET_WAIT_MS / portTICK_PERIOD_MS);
        break;
      case STATE_PHY_CONNECTED :
        newDevState(STATE_WAIT_IPADDR);
        // Set the initial time, where timeout will be started
        xStartTime = xTaskGetTickCount();
        break;
      case STATE_WAIT_IPADDR :
        if (gotIpAddr && gotIp6Addr)
          newDevState(STATE_GOT_IPADDR);
        else
        {
          // Check the timeout.
          if (xTaskGetTickCount() >= xStartTime + xTicksTimeout)
          {
            printf("%% Timeout waiting for all IP addresses\n");
            if (gotIpAddr || gotIp6Addr)
              newDevState(STATE_GOT_IPADDR);
            else
              newDevState(STATE_NEW);
          }
          else
          {
            vTaskDelay(NET_WAIT_MS / portTICK_PERIOD_MS);
          }
        }
        break;
      case STATE_GOT_IPADDR :
        newDevState(STATE_OTA_UPDATING);
        break;
      case STATE_OTA_UPDATING :
        // No OTA for this sketch.
        newDevState(STATE_OTA_COMPLETE);
        break;
      case STATE_OTA_COMPLETE :
        aborting = false;
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
        if (!aborting)
          newDevState(STATE_LISTENING);
        else
          newDevState(STATE_TCP_DISCONNECTED);
        break;
      case STATE_LISTENING :
        aborting = false;
        newDevState(STATE_TCP_DISCONNECTED);
        break;
      case STATE_TCP_DISCONNECTED :
        // This would be the place to free net resources, if needed,
        newDevState(STATE_LISTENING);
        break;
      default :
        break;
    }
  }
}

void setup()
{
  devState = STATE_NEW;

  Serial.begin(115200);

  esp_netif_init();
  esp_event_loop_create_default();
  esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, event_cb, NULL, NULL);
  esp_event_handler_instance_register(IP_EVENT, ESP_EVENT_ANY_ID, event_cb, NULL, NULL);

  // Stack size needs to be larger, so continue in a new task.
  xTaskCreatePinnedToCore(controlTask, "ctl", configSTACK, NULL,
    (tskIDLE_PRIORITY + 3), NULL, portNUM_PROCESSORS - 1);
}

void loop()
{
  // Nothing to do here since controlTask has taken over.
  vTaskDelay(60000 / portTICK_PERIOD_MS);
}
