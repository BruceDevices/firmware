// ESP32 libssh port.
//
// Ewan Parker, created 25th September 2020.
// Use SCP to download and flash new firmware over the air ('OTA').
// The example makes an SCP connection to a defined server, pulls the firmware
// image, and writes it to flash.
//
// Copyright (C) 2016–2023 Ewan Parker.

// EXAMPLE copyright START
// Some SCP code borrowed shamelessly from libssh example libssh_scp.c
// EXAMPLE copyright FINISH

// Set local WiFi credentials below.
const char *configSTASSID = "YourWiFiSSID";
const char *configSTAPSK = "YourWiFiPSK";

// Set remote SCP server access credentials below.
const char *configOTAServer = "your.scpserver.local";
const char *configOTAUser = "scpuser";
const char *configOTAPath = "/srv/ota/firmware.ino.bin";

// The command line you would use to run this from a shell prompt.
#define EX_CMD "blank"

// Stack size needed to run SSH and the OTA processing.
const unsigned int configSTACKctl = 10240;
const unsigned int configSTACKota = 32768;
const int verbosity = 0;

#include <arpa/inet.h>
#include "esp_netif.h"
#include "IPv6Address.h"
#include "WiFi.h"
// Include the Arduino library.
#include "libssh_esp32.h"

// EXAMPLE includes/defines START
// EXAMPLE includes/defines FINISH

volatile bool wifiPhyConnected;

// Timing and timeout configuration.
#define WIFI_TIMEOUT_S 10
#define NET_WAIT_MS 100
#define OTA_SCP_TIMEOUT_S 15
#define OTA_REBOOT_TIMEOUT_S 5

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
#include "driver/uart.h"
#include "esp_vfs_dev.h"
#include "esp_ota_ops.h"

// EXAMPLE functions START
/*
 * authentication.c
 * This file contains an example of how to do an authentication to a
 * SSH server using libssh
 */

/*
Copyright 2003-2009 Aris Adamantiadis

This file is part of the SSH Library

You are free to copy this file, modify it in any way, consider it being public
domain. This does not apply to the rest of the library though, but it is
allowed to cut-and-paste working code from this file to any license of
program.
The goal is to show the API in action. It's not a reference on how terminal
clients must be made or how a client should react.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libssh/libssh.h>
#include "examples_common.h"

int authenticate_kbdint(ssh_session session, const char *password)
{
    int err;

    err = ssh_userauth_kbdint(session, NULL, NULL);
    while (err == SSH_AUTH_INFO) {
        const char *instruction;
        const char *name;
        char buffer[128];
        int i, n;

        name = ssh_userauth_kbdint_getname(session);
        instruction = ssh_userauth_kbdint_getinstruction(session);
        n = ssh_userauth_kbdint_getnprompts(session);

        if (name && strlen(name) > 0) {
            printf("%s\n", name);
        }

        if (instruction && strlen(instruction) > 0) {
            printf("%s\n", instruction);
        }

        for (i = 0; i < n; i++) {
            const char *answer;
            const char *prompt;
            char echo;

            prompt = ssh_userauth_kbdint_getprompt(session, i, &echo);
            if (prompt == NULL) {
                break;
            }

            if (echo) {
                char *p;

                printf("%s", prompt);

                if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
                    return SSH_AUTH_ERROR;
                }

                buffer[sizeof(buffer) - 1] = '\0';
                if ((p = strchr(buffer, '\n'))) {
                    *p = '\0';
                }

                if (ssh_userauth_kbdint_setanswer(session, i, buffer) < 0) {
                    return SSH_AUTH_ERROR;
                }

                memset(buffer, 0, strlen(buffer));
            } else {
                if (password && strstr(prompt, "Password:")) {
                    answer = password;
                } else {
                    buffer[0] = '\0';

                    if (ssh_getpass(prompt, buffer, sizeof(buffer), 0, 0) < 0) {
                        return SSH_AUTH_ERROR;
                    }
                    answer = buffer;
                }
                err = ssh_userauth_kbdint_setanswer(session, i, answer);
                memset(buffer, 0, sizeof(buffer));
                if (err < 0) {
                    return SSH_AUTH_ERROR;
                }
            }
        }
        err=ssh_userauth_kbdint(session,NULL,NULL);
    }

    return err;
}

static int auth_keyfile(ssh_session session, char* keyfile)
{
    ssh_key key = NULL;
    char pubkey[132] = {0}; // +".pub"
    int rc;

    snprintf(pubkey, sizeof(pubkey), "%s.pub", keyfile);

    rc = ssh_pki_import_pubkey_file( pubkey, &key);

    if (rc != SSH_OK)
        return SSH_AUTH_DENIED;

    rc = ssh_userauth_try_publickey(session, NULL, key);

    ssh_key_free(key);

    if (rc!=SSH_AUTH_SUCCESS)
        return SSH_AUTH_DENIED;

    rc = ssh_pki_import_privkey_file(keyfile, NULL, NULL, NULL, &key);

    if (rc != SSH_OK)
        return SSH_AUTH_DENIED;

    rc = ssh_userauth_publickey(session, NULL, key);

    ssh_key_free(key);

    return rc;
}


static void error(ssh_session session)
{
    fprintf(stderr,"Authentication failed: %s\n",ssh_get_error(session));
}

int authenticate_console(ssh_session session)
{
    int rc;
    int method;
    char password[128] = {0};
    char *banner;

    // Try to authenticate
    rc = ssh_userauth_none(session, NULL);
    if (rc == SSH_AUTH_ERROR) {
        error(session);
        return rc;
    }

    method = ssh_userauth_list(session, NULL);
    while (rc != SSH_AUTH_SUCCESS) {
        if (method & SSH_AUTH_METHOD_GSSAPI_MIC){
            rc = ssh_userauth_gssapi(session);
            if(rc == SSH_AUTH_ERROR) {
                error(session);
                return rc;
            } else if (rc == SSH_AUTH_SUCCESS) {
                break;
            }
        }
        // Try to authenticate with public key first
        if (method & SSH_AUTH_METHOD_PUBLICKEY) {
            rc = ssh_userauth_publickey_auto(session, NULL, NULL);
            if (rc == SSH_AUTH_ERROR) {
                error(session);
                return rc;
            } else if (rc == SSH_AUTH_SUCCESS) {
                break;
            }
        }
        {
            char buffer[128] = {0};
            char *p = NULL;

            printf("Automatic pubkey failed. "
                   "Do you want to try a specific key? (y/n)\n");
            if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
                break;
            }
            if ((buffer[0]=='Y') || (buffer[0]=='y')) {
                printf("private key filename: ");

                if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
                    return SSH_AUTH_ERROR;
                }

                buffer[sizeof(buffer) - 1] = '\0';
                if ((p = strchr(buffer, '\n'))) {
                    *p = '\0';
                }

                rc = auth_keyfile(session, buffer);

                if(rc == SSH_AUTH_SUCCESS) {
                    break;
                }
                fprintf(stderr, "failed with key\n");
            }
        }

        // Try to authenticate with keyboard interactive";
        if (method & SSH_AUTH_METHOD_INTERACTIVE) {
            rc = authenticate_kbdint(session, NULL);
            if (rc == SSH_AUTH_ERROR) {
                error(session);
                return rc;
            } else if (rc == SSH_AUTH_SUCCESS) {
                break;
            }
        }

        if (ssh_getpass("Password: ", password, sizeof(password), 0, 0) < 0) {
            return SSH_AUTH_ERROR;
        }

        // Try to authenticate with password
        if (method & SSH_AUTH_METHOD_PASSWORD) {
            rc = ssh_userauth_password(session, NULL, password);
            if (rc == SSH_AUTH_ERROR) {
                error(session);
                return rc;
            } else if (rc == SSH_AUTH_SUCCESS) {
                break;
            }
        }
        memset(password, 0, sizeof(password));
    }

    banner = ssh_get_issue_banner(session);
    if (banner) {
        printf("%s\n",banner);
        SSH_STRING_FREE_CHAR(banner);
    }

    return rc;
}

/*
 * knownhosts.c
 * This file contains an example of how verify the identity of a
 * SSH server using libssh
 */

/*
Copyright 2003-2009 Aris Adamantiadis

This file is part of the SSH Library

You are free to copy this file, modify it in any way, consider it being public
domain. This does not apply to the rest of the library though, but it is
allowed to cut-and-paste working code from this file to any license of
program.
The goal is to show the API in action. It's not a reference on how terminal
clients must be made or how a client should react.
 */

#include "libssh_esp32_config.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libssh/priv.h"
#include <libssh/libssh.h>
#include "examples_common.h"

#ifdef _WIN32
#define strncasecmp _strnicmp
#endif

int verify_knownhost(ssh_session session)
{
    enum ssh_known_hosts_e state;
    char buf[10];
    unsigned char *hash = NULL;
    size_t hlen;
    ssh_key srv_pubkey;
    int rc;

    rc = ssh_get_server_publickey(session, &srv_pubkey);
    if (rc < 0) {
        return -1;
    }

    rc = ssh_get_publickey_hash(srv_pubkey,
                                SSH_PUBLICKEY_HASH_SHA256,
                                &hash,
                                &hlen);
    ssh_key_free(srv_pubkey);
    if (rc < 0) {
        return -1;
    }

    state = ssh_session_is_known_server(session);

    switch(state) {
    case SSH_KNOWN_HOSTS_CHANGED:
        fprintf(stderr,"Host key for server changed : server's one is now :\n");
        ssh_print_hash(SSH_PUBLICKEY_HASH_SHA256, hash, hlen);
        ssh_clean_pubkey_hash(&hash);
        fprintf(stderr,"For security reason, connection will be stopped\n");
        return -1;
    case SSH_KNOWN_HOSTS_OTHER:
        fprintf(stderr,"The host key for this server was not found but an other type of key exists.\n");
        fprintf(stderr,"An attacker might change the default server key to confuse your client"
                "into thinking the key does not exist\n"
                "We advise you to rerun the client with -d or -r for more safety.\n");
        return -1;
    case SSH_KNOWN_HOSTS_NOT_FOUND:
        fprintf(stderr,"Could not find known host file. If you accept the host key here,\n");
        fprintf(stderr,"the file will be automatically created.\n");
        /* fallback to SSH_SERVER_NOT_KNOWN behavior */
        FALL_THROUGH;
    case SSH_SERVER_NOT_KNOWN:
        fprintf(stderr,
                "The server is unknown. Do you trust the host key (yes/no)?\n");
        ssh_print_hash(SSH_PUBLICKEY_HASH_SHA256, hash, hlen);

        if (fgets(buf, sizeof(buf), stdin) == NULL) {
            ssh_clean_pubkey_hash(&hash);
            return -1;
        }
        if(strncasecmp(buf,"yes",3)!=0){
            ssh_clean_pubkey_hash(&hash);
            return -1;
        }
        fprintf(stderr,"This new key will be written on disk for further usage. do you agree ?\n");
        if (fgets(buf, sizeof(buf), stdin) == NULL) {
            ssh_clean_pubkey_hash(&hash);
            return -1;
        }
        if(strncasecmp(buf,"yes",3)==0){
            rc = ssh_session_update_known_hosts(session);
            if (rc != SSH_OK) {
                ssh_clean_pubkey_hash(&hash);
                fprintf(stderr, "error %s\n", strerror(errno));
                return -1;
            }
        }

        break;
    case SSH_KNOWN_HOSTS_ERROR:
        ssh_clean_pubkey_hash(&hash);
        fprintf(stderr,"%s",ssh_get_error(session));
        return -1;
    case SSH_KNOWN_HOSTS_OK:
        break; /* ok */
    }

    ssh_clean_pubkey_hash(&hash);

    return 0;
}

/*
 * connect_ssh.c
 * This file contains an example of how to connect to a
 * SSH server using libssh
 */

/*
Copyright 2009 Aris Adamantiadis

This file is part of the SSH Library

You are free to copy this file, modify it in any way, consider it being public
domain. This does not apply to the rest of the library though, but it is
allowed to cut-and-paste working code from this file to any license of
program.
The goal is to show the API in action. It's not a reference on how terminal
clients must be made or how a client should react.
 */

#include <libssh/libssh.h>
#include "examples_common.h"
#include <stdio.h>

ssh_session connect_ssh(const char *host, const char *user,int verbosity){
  ssh_session session;
  int auth=0;

  session=ssh_new();
  if (session == NULL) {
    return NULL;
  }

  if(user != NULL){
    if (ssh_options_set(session, SSH_OPTIONS_USER, user) < 0) {
      ssh_free(session);
      return NULL;
    }
  }

  if (ssh_options_set(session, SSH_OPTIONS_HOST, host) < 0) {
    ssh_free(session);
    return NULL;
  }
  ssh_options_set(session, SSH_OPTIONS_LOG_VERBOSITY, &verbosity);
  if(ssh_connect(session)){
    fprintf(stderr,"Connection failed : %s\n",ssh_get_error(session));
    ssh_disconnect(session);
    ssh_free(session);
    return NULL;
  }
  if(verify_knownhost(session)<0){
    ssh_disconnect(session);
    ssh_free(session);
    return NULL;
  }
  auth=authenticate_console(session);
  if(auth==SSH_AUTH_SUCCESS){
    return session;
  } else if(auth==SSH_AUTH_DENIED){
    fprintf(stderr,"Authentication failed\n");
  } else {
    fprintf(stderr,"Error while authenticating : %s\n",ssh_get_error(session));
  }
  ssh_disconnect(session);
  ssh_free(session);
  return NULL;
}
// EXAMPLE functions FINISH

// EXAMPLE main START
int ex_main(int argc, char **argv){
  printf("Your existing application would be executed here.\n");
  return 0;
}
// EXAMPLE main FINISH

#define newDevState(s) (devState = s)

void otaTask(void *pvParameter)
{
  printf("ota%% Task started\n");
  char buffer[4096];

  const esp_partition_t *bootPart = esp_ota_get_boot_partition();
  const esp_partition_t *runPart = esp_ota_get_running_partition();
  esp_ota_handle_t update_handle = 0;

  printf("ota%% Boot Partition type=%d subtype=%d addr=%d size=%d label=%s\n",
    bootPart->type, bootPart->subtype, bootPart->address, bootPart->size,
    bootPart->label);
  printf("ota%% Run Partition type=%d subtype=%d addr=%d size=%d label=%s\n",
    runPart->type, runPart->subtype, runPart->address, runPart->size,
    runPart->label);

  const esp_partition_t *newPart = esp_ota_get_next_update_partition(NULL);

  unsigned int appBytes = 0, otaBytes, otaRet;
  int scpRet;
  bool aborting;
  aborting = false;

  // Initialize the Arduino library.
  libssh_begin();
  ssh_session session = NULL;
  ssh_scp scp = NULL;
  long ssh_sess_opt_timeout = OTA_SCP_TIMEOUT_S;

  session = connect_ssh(configOTAServer, configOTAUser, verbosity);
  if (!session)
  {
    fprintf(stderr, "ota%% Unable to connect to %s\n", configOTAServer);
    aborting = true;
  }
  else ssh_options_set(session, SSH_OPTIONS_TIMEOUT, &ssh_sess_opt_timeout);

  if (!aborting)
  {
    scp = ssh_scp_new(session, SSH_SCP_READ, configOTAPath);
    if (!scp)
    {
      fprintf(stderr, "ota%% scp error: %s\n", ssh_get_error(session));
      aborting = true;
    }
  }

  if (!aborting)
  {
    if (ssh_scp_init(scp) == SSH_ERROR)
    {
      fprintf(stderr, "ota%% scp error: %s\n", ssh_get_error(session));
      aborting = true;
    }
  }

  if (!aborting)
  {
    printf("ota%% OTA Partition type=%d subtype=%d addr=%d size=%d label=%s\n",
      newPart->type, newPart->subtype, newPart->address, newPart->size,
      newPart->label);
    otaRet = esp_ota_begin(newPart, OTA_SIZE_UNKNOWN, &update_handle);
    if (otaRet != ESP_OK)
    {
      fprintf(stderr, "ota%% esp_ota_begin failed: rc=%d\n", otaRet);
      aborting = true;
    }
  }

  if (!aborting)
  {
    scpRet = ssh_scp_pull_request(scp);
    if (scpRet == SSH_SCP_REQUEST_NEWFILE)
    {
       otaBytes = ssh_scp_request_get_size(scp);
       printf("ota%% Image size = %d bytes\n", otaBytes);
       ssh_scp_accept_request(scp);
    }
    else
    {
      fprintf(stderr, "ota%% ssh_scp_pull_request failed rc=%d %s\n",
        scpRet, ssh_get_error(session));
      aborting = true;
    }
  }

  if (!aborting)
  {
    // We have found the data so attempt to write it to the flash of the
    // new partition.
    long startMillis = millis();
    short previousPercent = -1, currentPercent;
    do
    {
      scpRet = ssh_scp_read(scp, buffer, sizeof(buffer));
      if (scpRet == SSH_ERROR)
      {
        fprintf(stderr, "ota%% ssh_scp_read error: %s\n",
          ssh_get_error(session));
        aborting = true;
        break;
      }

      if (scpRet == 0) {
        printf("\nota%% Timeout after %ld seconds\r", ssh_sess_opt_timeout);
        aborting = true;
        break;
      }
      otaRet = esp_ota_write(update_handle, buffer, scpRet);
      appBytes += scpRet;
      if (otaRet != ESP_OK)
      {
        fprintf(stderr, "ota%% esp_ota_write failed: rc=%d\n", otaRet);
        aborting = true;
        break;
      }
      else
      {
        currentPercent = 100 * appBytes/otaBytes;
        if (currentPercent != previousPercent && currentPercent%1 == 0)
        {
          printf("ota%% [%d%%] Write %d bytes, so far %d (%d kBytes/s)%s\r",
            currentPercent, scpRet, appBytes,
            1000 * appBytes/(millis() - startMillis)/1024, "        ");
          fflush(NULL);
          previousPercent = currentPercent;
        }
      }
    }
    while (appBytes < otaBytes);
    printf("\n");
  }

  if (!aborting)
  {
    otaRet = esp_ota_end(update_handle);
    if (otaRet != ESP_OK)
    {
      fprintf(stderr, "ota%% esp_ota_end failed: rc=%d\n", otaRet);
      aborting = true;
    }
  }

  if (!aborting)
  {
    otaRet = esp_ota_set_boot_partition(newPart);
    if (otaRet != ESP_OK)
    {
      fprintf(stderr, "ota%% esp_ota_set_boot_partition failed: rc=%d\n",
        otaRet);
      aborting = true;
    }
  }

  if (scp) ssh_scp_free(scp);
  if (session) ssh_disconnect(session);

  if (!aborting)
  {
    printf("ota%% Flash complete, restarting in %ds\n", OTA_REBOOT_TIMEOUT_S);
    delay(OTA_REBOOT_TIMEOUT_S * 1000);
    esp_restart();
  }
  else
  {
    printf("ota%% Skipped\n");
  }

  if (session) ssh_free(session);
  ssh_finalize();

  newDevState(STATE_OTA_COMPLETE);

  printf("ota%% Task deleted\n");
  vTaskDelete(NULL);
}

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
        Serial.println(IPv6Address(event->ip6_info.ip.addr));
      }
      break;
    case IP_EVENT_STA_GOT_IP:
      {
        WiFi.enableIpV6(); // Under IDF 5 we need to get IPv4 address first.
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
    "%% Mounted SPIFFS used=%d total=%d\n", SPIFFS.usedBytes(),
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
        // Run the OTA checks on core#1 to prevent watchdog timeout.
        xTaskCreatePinnedToCore(otaTask, "ota", configSTACKota, NULL,
          (tskIDLE_PRIORITY + 3), NULL, portNUM_PROCESSORS - 1);
        newDevState(STATE_OTA_UPDATING);
        break;
      case STATE_OTA_UPDATING :
        vTaskDelay(NET_WAIT_MS / portTICK_PERIOD_MS);
        break;
      case STATE_OTA_COMPLETE :
        aborting = false;

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

  esp_netif_init();
  esp_event_loop_create_default();
  esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, event_cb, NULL, NULL);
  esp_event_handler_instance_register(IP_EVENT, ESP_EVENT_ANY_ID, event_cb, NULL, NULL);

  // Stack size needs to be larger, so continue in a new task.
  xTaskCreatePinnedToCore(controlTask, "ctl", configSTACKctl, NULL,
    (tskIDLE_PRIORITY + 3), NULL, portNUM_PROCESSORS - 1);
}

void loop()
{
  // Nothing to do here since controlTask has taken over.
  vTaskDelay(60000 / portTICK_PERIOD_MS);
}
