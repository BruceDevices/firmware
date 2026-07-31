#include "camera_brands.h"

#define ARRSZ(a) (sizeof(a) / sizeof((a)[0]))

// ---------------------------------------------------------------------------
// OUI tables (lowercase). Vendors that ship almost exclusively cameras / NVRs.
// ---------------------------------------------------------------------------

// Hangzhou Hikvision Digital Technology (IEEE registry, 85 prefixes).
static const char *const oui_hikvision[] = {
    "e4:d5:8b", "c8:a7:02", "08:54:11", "08:a1:89", "08:cc:81", "24:0f:9b", "24:32:ae", "24:48:45",
    "2c:a5:9c", "40:ac:bf", "44:19:b6", "44:a6:42", "4c:62:df", "4c:bd:8f", "4c:f5:dc", "50:e5:38",
    "58:03:fb", "58:50:ed", "5c:34:5b", "64:db:8b", "68:6d:bc", "80:f5:ae", "8c:e7:48", "94:e1:ac",
    "98:9d:e5", "98:df:82", "98:f1:12", "a0:ff:0c", "a4:14:37", "a4:d5:c2", "ac:cb:51", "b4:a3:82",
    "bc:5e:33", "bc:ad:28", "bc:ba:c2", "c0:51:7e", "c0:56:e3", "c0:6d:ed", "dc:07:f8", "10:12:fb",
    "e0:ba:ad", "e0:ca:3c", "e0:df:13", "ec:a9:71", "a4:29:02", "a4:a4:59", "80:48:9f", "e8:a0:ed",
    "fc:9f:fd", "04:03:12", "0c:75:d2", "18:68:cb", "18:80:25", "24:28:fd", "28:57:be", "34:09:62",
    "3c:1b:f8", "44:47:cc", "54:8c:81", "54:c4:15", "74:3f:c2", "80:7c:62", "80:be:af", "84:9a:40",
    "98:8b:0a", "ac:b9:2f", "bc:9b:5e", "c4:2f:90", "d4:e8:53", "dc:d2:6a", "ec:c8:9c", "f8:4d:fc",
    "4c:1f:86", "a4:4b:d9", "88:de:39", "84:94:59", "08:3b:c1", "00:bc:99", "04:ee:cd", "8c:22:d2",
    "48:78:5b", "cc:13:f3", "40:b5:70", "24:b1:05", "bc:29:78"
};

// Zhejiang Dahua Technology (33 prefixes).
static const char *const oui_dahua[] = {
    "08:ed:ed", "14:a7:8b", "38:af:29", "3c:e3:6b", "3c:ef:8c", "5c:f5:1a", "64:fd:29", "74:c9:29",
    "8c:e9:b4", "9c:14:63", "a0:bd:1d", "bc:32:5f", "c0:39:5a", "c4:aa:c4", "d4:43:0e", "e0:50:8b",
    "e4:24:6c", "f8:ce:07", "30:dd:aa", "f4:b1:c2", "fc:b6:9d", "24:52:6a", "4c:11:bf", "6c:1c:71",
    "90:02:a9", "98:f9:cc", "b4:4c:3b", "e0:2e:fe", "fc:5f:49", "4c:99:e8", "40:7a:a4", "20:2c:05",
    "a8:ca:87"
};

// Hangzhou EZVIZ Software (Hikvision consumer brand, 14 prefixes).
static const char *const oui_ezviz[] = {
    "94:ec:13", "20:bb:bc", "58:8f:cf", "64:f2:fb", "78:c1:ae", "34:c6:dd", "ec:97:e0",
    "54:d6:0d", "78:a6:a0", "0c:a6:4c", "ac:1c:26", "64:24:4d", "f4:70:18", "fc:24:22"
};

// Hangzhou Huacheng Network Technology (Imou / Dahua consumer brand, 5).
static const char *const oui_imou[] = {"90:6a:94", "a8:31:62", "1c:4d:89", "30:24:50", "ac:3d:fa"};

// Reolink Innovation Limited (1).
static const char *const oui_reolink[] = {"ec:71:db"};

// Arlo Technology (3).
static const char *const oui_arlo[] = {"48:62:64", "a4:11:62", "fc:9c:98"};

// Ring LLC (Amazon, 13).
static const char *const oui_ring[] = {
    "ac:9f:c3", "18:7f:88", "34:3e:a4", "54:e0:19", "5c:47:5e", "64:9a:63", "90:48:6c",
    "9c:76:13", "cc:3b:fb", "c4:db:ad", "24:2b:d6", "00:b4:63", "50:e4:67"
};

// Wyze Labs (5). (a4:da:22 removed - it is an IEEE MA-M parent block, not Wyze.)
static const char *const oui_wyze[] = {"2c:aa:8e", "7c:78:b2", "80:48:2c", "d0:3f:27", "f0:c8:8b"};

// Lorex Technology (1).
static const char *const oui_lorex[] = {"00:1f:54"};

// Zhuhai Dingzhi Electronic — ODM behind Meari / CloudEdge cameras (rebranded
// by many labels, e.g. "SPEED"). Proprietary Meari cloud/P2P, so only OUI/name
// detection applies (no iLnkP2P/TUTK LAN probe).
static const char *const oui_meari[] = {"68:76:27"};

// Zhejiang Uniview (UNV) — top-tier surveillance vendor.
static const char *const oui_uniview[] = {"6c:f1:7e", "48:ea:63", "c4:79:05", "88:26:3f"};

// Amcrest Technologies (US; Dahua-lineage). (34:46:63 removed - IEEE MA-M
// parent block, not Amcrest.)
static const char *const oui_amcrest[] = {"00:65:1e", "9c:8e:cd", "a0:60:32"};

// Axon Enterprise body cameras (from nyanBOX).
static const char *const oui_axon[] = {"00:25:df"};

// Flock Safety surveillance cameras (from nyanBOX).
static const char *const oui_flock[] = {
    "58:8e:81", "cc:cc:cc", "ec:1b:bd", "90:35:ea", "04:0d:84", "f0:82:c0", "1c:34:f1",
    "38:5b:44", "94:34:69", "b4:e3:f9", "70:c9:4e", "3c:91:80", "d8:f3:bc", "80:30:49",
    "14:5a:fc", "74:4c:a1", "08:3a:88", "9c:2f:9d", "94:08:53", "e4:aa:ea"
};

// --- Professional / enterprise network-camera vendors (IEEE registry) --------
// Axis Communications (Sweden) - global market leader in pro network cameras.
static const char *const oui_axis[] = {"b8:a4:4f", "00:40:8c", "e8:27:25", "ac:cc:8e"};
// Hanwha Vision / Wisenet (ex-Samsung Techwin). NxMD (machine tools) excluded.
static const char *const oui_hanwha[] = {"00:09:18", "44:b4:23", "e4:30:22"};
// VIVOTEK (Taiwan).
static const char *const oui_vivotek[] = {"00:02:d1"};
// Tiandy Technologies (China) - fast-growing surveillance vendor.
static const char *const oui_tiandy[] = {"3c:da:6d", "e4:e6:6c"};
// Xiamen Milesight IoT.
static const char *const oui_milesight[] = {"1c:c3:16", "c0:ba:1f", "24:e1:24"};
// MOBOTIX (Germany).
static const char *const oui_mobotix[] = {"00:03:c5"};
// GeoVision (Taiwan).
static const char *const oui_geovision[] = {"00:13:e2"};
// Sunell Electronics (China; thermal / surveillance).
static const char *const oui_sunell[] = {"00:1c:27"};

// --- Consumer brands that also hold dedicated OUI blocks ---------------------
// Blink by Amazon (5). (enblink 28:a1:86 excluded - unrelated vendor.)
static const char *const oui_blink[] = {"70:ad:43", "74:13:48", "f0:74:c1", "74:ab:93", "3c:a0:70"};
// SimpliSafe (US home security).
static const char *const oui_simplisafe[] = {"f8:51:28"};
// Swann Communications (Australia).
static const char *const oui_swann[] = {"bc:51:fe"};

// ---------------------------------------------------------------------------
// Name / SSID substring tables (lowercase). Camera-specific tokens so we don't
// flag a vendor's phones or routers.
// ---------------------------------------------------------------------------

static const char *const pat_hikvision[] = {"hikvision", "hik-"};
static const char *const pat_dahua[] = {"dahua", "dh-ipc", "dh_ipc"};
static const char *const pat_ezviz[] = {"ezviz"};
static const char *const pat_imou[] = {"imou"};
static const char *const pat_reolink[] = {"reolink"};
static const char *const pat_meari[] = {"meari", "cloudedge"};
static const char *const pat_arlo[] = {"arlo"};
static const char *const pat_ring[] = {"ring setup", "ring-", "ring_"};
static const char *const pat_wyze[] = {"wyze"};
static const char *const pat_flock[] = {"flock", "fs ext battery", "penguin", "pigvision"};
static const char *const pat_axon[] = {"axon"};

// Multi-product vendors: name match only.
static const char *const pat_tplink_cam[] = {"tapo", "tp-link_cam", "tplink_cam", "tplinkcam"};
static const char *const pat_xiaomi_cam[] = {
    "chuangmi", "isa_camera", "xiaovv", "imilab", "mijia camera", "xiaomi camera", "mi camera", "miir"
};
static const char *const pat_eufy[] = {"eufy"};
static const char *const pat_swann[] = {"swann"};
static const char *const pat_aqara[] = {"aqara", "lumi-"};
static const char *const pat_nest[] = {"nest cam", "nestcam", "google-nest"};
static const char *const pat_dlink[] = {"dcs-", "d-link cam", "dlink_cam"};
static const char *const pat_uniview[] = {"uniview", "unv-", "unv_"};
static const char *const pat_amcrest[] = {"amcrest", "amc-"};
static const char *const pat_foscam[] = {"foscam", "fosbaby"};
static const char *const pat_annke[] = {"annke"};
static const char *const pat_vivotek[] = {"vivotek"};
static const char *const pat_blink[] = {"blink-", "blink_"};
static const char *const pat_simplisafe[] = {"simplisafe", "simplicam"};
// Professional network-camera vendors. "axis-" (not bare "axis") is the Axis
// default hostname prefix, avoiding false matches on words containing "axis".
static const char *const pat_axis[] = {"axis-", "axiscam"};
static const char *const pat_hanwha[] = {"wisenet", "hanwha", "samsung techwin"};
static const char *const pat_tiandy[] = {"tiandy"};
static const char *const pat_milesight[] = {"milesight"};
static const char *const pat_mobotix[] = {"mobotix"};
static const char *const pat_geovision[] = {"geovision"};
static const char *const pat_sunell[] = {"sunell"};
// Big OEM / white-label ecosystems (Xiongmai/XMeye, CamHi, V380, Yoosee/Gwell,
// iCSee, Ubox, Tuya). These ship generic Wi-Fi chips, so name/SSID/app only.
static const char *const pat_xmeye[] = {"xiongmai", "xmeye", "xm-", "camhi", "ipcam"};
static const char *const pat_v380[] = {"v380", "macrovideo"};
static const char *const pat_yoosee[] = {"yoosee", "gwell", "yhcam"};
static const char *const pat_icsee[] = {"icsee", "ubox", "bccam"};
static const char *const pat_tuyacam[] = {"tuya", "smartlife", "smart_cam", "smartcam"};

// ---------------------------------------------------------------------------
// Brand table. OUI-backed vendors first (most precise), then name-only vendors.
// ---------------------------------------------------------------------------

static const CameraBrand kBrands[] = {
    {"Hikvision", oui_hikvision, ARRSZ(oui_hikvision), pat_hikvision, ARRSZ(pat_hikvision)},
    {"Dahua", oui_dahua, ARRSZ(oui_dahua), pat_dahua, ARRSZ(pat_dahua)},
    {"EZVIZ", oui_ezviz, ARRSZ(oui_ezviz), pat_ezviz, ARRSZ(pat_ezviz)},
    {"Imou", oui_imou, ARRSZ(oui_imou), pat_imou, ARRSZ(pat_imou)},
    {"Reolink", oui_reolink, ARRSZ(oui_reolink), pat_reolink, ARRSZ(pat_reolink)},
    {"Arlo", oui_arlo, ARRSZ(oui_arlo), pat_arlo, ARRSZ(pat_arlo)},
    {"Ring", oui_ring, ARRSZ(oui_ring), pat_ring, ARRSZ(pat_ring)},
    {"Wyze", oui_wyze, ARRSZ(oui_wyze), pat_wyze, ARRSZ(pat_wyze)},
    {"Uniview", oui_uniview, ARRSZ(oui_uniview), pat_uniview, ARRSZ(pat_uniview)},
    {"Amcrest", oui_amcrest, ARRSZ(oui_amcrest), pat_amcrest, ARRSZ(pat_amcrest)},
    {"Axis", oui_axis, ARRSZ(oui_axis), pat_axis, ARRSZ(pat_axis)},
    {"Hanwha/Wisenet", oui_hanwha, ARRSZ(oui_hanwha), pat_hanwha, ARRSZ(pat_hanwha)},
    {"Tiandy", oui_tiandy, ARRSZ(oui_tiandy), pat_tiandy, ARRSZ(pat_tiandy)},
    {"Milesight", oui_milesight, ARRSZ(oui_milesight), pat_milesight, ARRSZ(pat_milesight)},
    {"Mobotix", oui_mobotix, ARRSZ(oui_mobotix), pat_mobotix, ARRSZ(pat_mobotix)},
    {"GeoVision", oui_geovision, ARRSZ(oui_geovision), pat_geovision, ARRSZ(pat_geovision)},
    {"Sunell", oui_sunell, ARRSZ(oui_sunell), pat_sunell, ARRSZ(pat_sunell)},
    {"Lorex", oui_lorex, ARRSZ(oui_lorex), nullptr, 0},
    {"Meari/CloudEdge", oui_meari, ARRSZ(oui_meari), pat_meari, ARRSZ(pat_meari)},
    {"Axon", oui_axon, ARRSZ(oui_axon), pat_axon, ARRSZ(pat_axon)},
    {"Flock", oui_flock, ARRSZ(oui_flock), pat_flock, ARRSZ(pat_flock)},
    // Name-only (multi-product vendors + OEM ecosystems). Ordered roughly by
    // popularity; matched only on camera-specific SSID/BLE/app tokens.
    {"Tapo", nullptr, 0, pat_tplink_cam, ARRSZ(pat_tplink_cam)},
    {"Xiaomi", nullptr, 0, pat_xiaomi_cam, ARRSZ(pat_xiaomi_cam)},
    {"Eufy", nullptr, 0, pat_eufy, ARRSZ(pat_eufy)},
    {"Swann", oui_swann, ARRSZ(oui_swann), pat_swann, ARRSZ(pat_swann)},
    {"Aqara", nullptr, 0, pat_aqara, ARRSZ(pat_aqara)},
    {"Nest", nullptr, 0, pat_nest, ARRSZ(pat_nest)},
    {"D-Link", nullptr, 0, pat_dlink, ARRSZ(pat_dlink)},
    {"Foscam", nullptr, 0, pat_foscam, ARRSZ(pat_foscam)},
    {"Annke", nullptr, 0, pat_annke, ARRSZ(pat_annke)},
    {"Vivotek", oui_vivotek, ARRSZ(oui_vivotek), pat_vivotek, ARRSZ(pat_vivotek)},
    {"Blink", oui_blink, ARRSZ(oui_blink), pat_blink, ARRSZ(pat_blink)},
    {"SimpliSafe", oui_simplisafe, ARRSZ(oui_simplisafe), pat_simplisafe, ARRSZ(pat_simplisafe)},
    {"XMeye/CamHi", nullptr, 0, pat_xmeye, ARRSZ(pat_xmeye)},
    {"V380", nullptr, 0, pat_v380, ARRSZ(pat_v380)},
    {"Yoosee", nullptr, 0, pat_yoosee, ARRSZ(pat_yoosee)},
    {"iCSee", nullptr, 0, pat_icsee, ARRSZ(pat_icsee)},
    {"Tuya Cam", nullptr, 0, pat_tuyacam, ARRSZ(pat_tuyacam)},
};

// ---------------------------------------------------------------------------
// iLnkP2P / CS2 Network P2P UID prefixes. Any device answering the LAN-search
// probe on UDP 32108 is a P2P camera; the prefix adds vendor colour. Only a few
// mappings are asserted with confidence (VStarcam's VSTx family); the rest are
// labelled by P2P family. Extend from published iLnkP2P prefix research.
// ---------------------------------------------------------------------------
struct P2PPrefix {
    const char *prefix;
    const char *brand;
};

static const P2PPrefix kP2PPrefixes[] = {
    // VStarcam / Eye4 (well documented)
    {"VSTA", "VStarcam"},
    {"VSTB", "VStarcam"},
    {"VSTC", "VStarcam"},
    {"VSTD", "VStarcam"},
    {"VSTF", "VStarcam"},
    {"VSTG", "VStarcam"},
    {"VSTH", "VStarcam"},
    // Generic iLnkP2P / Yunni families recognised by the reference tool.
    // Brand is white-label/OEM, so only the family is asserted.
    {"EEEE", "iLnkP2P OEM"},
    {"FFFF", "iLnkP2P OEM"},
    {"QHSV", "iLnkP2P"},
    {"ROSS", "iLnkP2P"},
    {"ISRP", "iLnkP2P"},
    {"GCMN", "iLnkP2P"},
    {"ELSA", "iLnkP2P"},
    {"MEYE", "iLnkP2P"},
    {"PPBB", "iLnkP2P"},
    {"HWAA", "iLnkP2P OEM"},
};

const char *identifyP2PPrefix(const String &prefixUpper) {
    for (const auto &e : kP2PPrefixes) {
        if (prefixUpper.equalsIgnoreCase(e.prefix)) return e.brand;
    }
    return nullptr;
}

const char *identifyCamera(const String &macLower, const String &nameLower, const char **methodOut) {
    // Pass 1: OUI (high confidence) across all brands.
    if (macLower.length() >= 8) {
        for (const auto &b : kBrands) {
            for (uint16_t i = 0; i < b.ouiCount; i++) {
                if (macLower.startsWith(b.ouis[i])) {
                    if (methodOut) *methodOut = "OUI";
                    return b.name;
                }
            }
        }
    }
    // Pass 2: name / SSID substring.
    if (!nameLower.isEmpty()) {
        for (const auto &b : kBrands) {
            for (uint16_t i = 0; i < b.patternCount; i++) {
                if (nameLower.indexOf(b.patterns[i]) >= 0) {
                    if (methodOut) *methodOut = "name";
                    return b.name;
                }
            }
        }
    }
    return nullptr;
}
