#pragma once

#if !defined(ESP_PLATFORM)
# error "NimBLEBondMigration.h currently requires ESP_PLATFORM (NVS backend)."
#endif

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <string>
#include <vector>

#include "esp_err.h"
#include "esp_log.h"
#include "nvs.h"
#include "syscfg/syscfg.h"

/**
 * @brief Helpers for converting NimBLE bond data stored in ESP NVS between
 *        the legacy 1.x layout and the current 2.x layout.
 *
 * These helpers are intended for one-time migration firmware and must be used
 * before calling `NimBLEDevice::init()`.
 */
namespace NimBLEBondMigration {

namespace detail {

static constexpr const char* kLogTag        = "NimBLEBondMigration";
static constexpr const char* kBondNamespace = "nimble_bond";
static constexpr const char* kOurSecPrefix  = "our_sec";
static constexpr const char* kPeerSecPrefix = "peer_sec";
static constexpr const char* kLocalIrkPrefix = "local_irk";
static constexpr size_t      kNvsKeyMaxLen  = 16;

typedef struct {
    uint8_t type;
    uint8_t val[6];
} ble_addr_t;

struct BleStoreValueSecV1 {
    ble_addr_t peer_addr;

    uint8_t key_size;
    uint16_t ediv;
    uint64_t rand_num;
    uint8_t ltk[16];
    uint8_t ltk_present : 1;

    uint8_t irk[16];
    uint8_t irk_present : 1;

    uint8_t csrk[16];
    uint8_t csrk_present : 1;

    unsigned authenticated : 1;
    uint8_t sc : 1;
};

struct BleStoreValueSecCurrent {
    ble_addr_t peer_addr;
    uint16_t   bond_count;

    uint8_t key_size;
    uint16_t ediv;
    uint64_t rand_num;
    uint8_t ltk[16];
    uint8_t ltk_present : 1;

    uint8_t irk[16];
    uint8_t irk_present : 1;

    uint8_t csrk[16];
    uint8_t csrk_present : 1;
    uint32_t sign_counter;

    unsigned authenticated : 1;
    uint8_t sc : 1;
};

struct BleStoreValueLocalIrkV1 {
    uint8_t irk[16];
};

struct BleStoreValueLocalIrkCurrent {
    ble_addr_t addr;
    uint8_t irk[16];
};

static constexpr uint8_t kDefaultLocalIrk[16] = {
    0xef, 0x8d, 0xe2, 0x16, 0x4f, 0xec, 0x43, 0x0d,
    0xbf, 0x5b, 0xdd, 0x34, 0xc0, 0x53, 0x1e, 0xb8,
};

struct MigrationStats {
    uint16_t scanned;
    uint16_t converted;
    uint16_t alreadyTarget;
    uint16_t skippedUnknownSize;
};

inline void appendLine(std::string& out, const char* fmt, ...) {
    char    buf[256];
    va_list args;
    va_start(args, fmt);
    const int written = vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    if (written > 0) {
        out.append(buf, strlen(buf));
    }
}

inline void appendHex(std::string& out, const uint8_t* data, size_t len) {
    static constexpr char hex[] = "0123456789ABCDEF";
    out.reserve(out.size() + (len * 2));
    for (size_t i = 0; i < len; ++i) {
        const uint8_t value = data[i];
        out.push_back(hex[(value >> 4) & 0x0F]);
        out.push_back(hex[value & 0x0F]);
    }
}

inline void appendAddr(std::string& out, const ble_addr_t& addr) {
    appendLine(out,
               "%02X:%02X:%02X:%02X:%02X:%02X(type=%u)",
               addr.val[5],
               addr.val[4],
               addr.val[3],
               addr.val[2],
               addr.val[1],
               addr.val[0],
               addr.type);
}

inline void appendV1Record(std::string& out, const BleStoreValueSecV1& sec) {
    out += "  peer_addr=";
    appendAddr(out, sec.peer_addr);
    out += "\n";
    appendLine(out,
               "  key_size=%u ediv=%u rand_num=%llu auth=%u sc=%u\n",
               sec.key_size,
               sec.ediv,
               static_cast<unsigned long long>(sec.rand_num),
               sec.authenticated,
               sec.sc);
    appendLine(out,
               "  ltk_present=%u irk_present=%u csrk_present=%u\n",
               sec.ltk_present,
               sec.irk_present,
               sec.csrk_present);
    if (sec.ltk_present) {
        out += "  ltk=";
        appendHex(out, sec.ltk, sizeof(sec.ltk));
        out += "\n";
    }
    if (sec.irk_present) {
        out += "  irk=";
        appendHex(out, sec.irk, sizeof(sec.irk));
        out += "\n";
    }
    if (sec.csrk_present) {
        out += "  csrk=";
        appendHex(out, sec.csrk, sizeof(sec.csrk));
        out += "\n";
    }
}

inline void appendCurrentRecord(std::string& out, const BleStoreValueSecCurrent& sec) {
    out += "  peer_addr=";
    appendAddr(out, sec.peer_addr);
    out += "\n";
    appendLine(out,
               "  bond_count=%u sign_counter=%u key_size=%u ediv=%u rand_num=%llu auth=%u sc=%u\n",
               sec.bond_count,
               sec.sign_counter,
               sec.key_size,
               sec.ediv,
               static_cast<unsigned long long>(sec.rand_num),
               sec.authenticated,
               sec.sc);
    appendLine(out,
               "  ltk_present=%u irk_present=%u csrk_present=%u\n",
               sec.ltk_present,
               sec.irk_present,
               sec.csrk_present);
    if (sec.ltk_present) {
        out += "  ltk=";
        appendHex(out, sec.ltk, sizeof(sec.ltk));
        out += "\n";
    }
    if (sec.irk_present) {
        out += "  irk=";
        appendHex(out, sec.irk, sizeof(sec.irk));
        out += "\n";
    }
    if (sec.csrk_present) {
        out += "  csrk=";
        appendHex(out, sec.csrk, sizeof(sec.csrk));
        out += "\n";
    }
}

inline void appendLocalIrkV1Record(std::string& out, const BleStoreValueLocalIrkV1& irkRecord) {
    out += "  irk=";
    appendHex(out, irkRecord.irk, sizeof(irkRecord.irk));
    out += "\n";
}

inline void appendLocalIrkCurrentRecord(std::string& out, const BleStoreValueLocalIrkCurrent& irkRecord) {
    out += "  addr=";
    appendAddr(out, irkRecord.addr);
    out += "\n";
    out += "  irk=";
    appendHex(out, irkRecord.irk, sizeof(irkRecord.irk));
    out += "\n";
}

inline bool makeBondKey(char* out, size_t outLen, const char* prefix, uint16_t index) {
    const int written = snprintf(out, outLen, "%s_%u", prefix, index);
    return written > 0 && static_cast<size_t>(written) < outLen;
}

inline bool migrateEntryToCurrent(nvs_handle_t nvsHandle,
                                  const char* key,
                                  uint16_t index,
                                  MigrationStats* stats) {
    size_t blobSize = 0;
    esp_err_t err = nvs_get_blob(nvsHandle, key, nullptr, &blobSize);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        return true;
    }
    if (err != ESP_OK) {
        ESP_LOGE(kLogTag, "nvs_get_blob size failed key=%s err=%d", key, static_cast<int>(err));
        return false;
    }

    stats->scanned++;

    if (blobSize == sizeof(BleStoreValueSecCurrent)) {
        stats->alreadyTarget++;
        return true;
    }

    if (blobSize != sizeof(BleStoreValueSecV1)) {
        stats->skippedUnknownSize++;
        ESP_LOGW(kLogTag,
                 "Skipping key=%s due to unexpected size=%u",
                 key,
                 static_cast<unsigned>(blobSize));
        return true;
    }

    BleStoreValueSecV1 oldValue{};
    size_t readSize = sizeof(oldValue);
    err = nvs_get_blob(nvsHandle, key, &oldValue, &readSize);
    if (err != ESP_OK) {
        ESP_LOGE(kLogTag, "nvs_get_blob value failed key=%s err=%d", key, static_cast<int>(err));
        return false;
    }

    BleStoreValueSecCurrent newValue{};
    newValue.peer_addr = oldValue.peer_addr;
    newValue.bond_count = index;
    newValue.key_size = oldValue.key_size;
    newValue.ediv = oldValue.ediv;
    newValue.rand_num = oldValue.rand_num;
    memcpy(newValue.ltk, oldValue.ltk, sizeof(newValue.ltk));
    newValue.ltk_present = oldValue.ltk_present;
    memcpy(newValue.irk, oldValue.irk, sizeof(newValue.irk));
    newValue.irk_present = oldValue.irk_present;
    memcpy(newValue.csrk, oldValue.csrk, sizeof(newValue.csrk));
    newValue.csrk_present = oldValue.csrk_present;
    newValue.sign_counter = 0;
    newValue.authenticated = oldValue.authenticated;
    newValue.sc = oldValue.sc;

    err = nvs_set_blob(nvsHandle, key, &newValue, sizeof(newValue));
    if (err != ESP_OK) {
        ESP_LOGE(kLogTag, "nvs_set_blob failed key=%s err=%d", key, static_cast<int>(err));
        return false;
    }

    stats->converted++;
    return true;
}

inline bool migrateEntryToV1(nvs_handle_t nvsHandle,
                             const char* key,
                             MigrationStats* stats) {
    size_t blobSize = 0;
    esp_err_t err = nvs_get_blob(nvsHandle, key, nullptr, &blobSize);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        return true;
    }
    if (err != ESP_OK) {
        ESP_LOGE(kLogTag, "nvs_get_blob size failed key=%s err=%d", key, static_cast<int>(err));
        return false;
    }

    stats->scanned++;

    if (blobSize == sizeof(BleStoreValueSecV1)) {
        stats->alreadyTarget++;
        return true;
    }

    if (blobSize != sizeof(BleStoreValueSecCurrent)) {
        stats->skippedUnknownSize++;
        ESP_LOGW(kLogTag,
                 "Skipping key=%s due to unexpected size=%u",
                 key,
                 static_cast<unsigned>(blobSize));
        return true;
    }

    BleStoreValueSecCurrent curValue{};
    size_t readSize = sizeof(curValue);
    err = nvs_get_blob(nvsHandle, key, &curValue, &readSize);
    if (err != ESP_OK) {
        ESP_LOGE(kLogTag, "nvs_get_blob value failed key=%s err=%d", key, static_cast<int>(err));
        return false;
    }

    BleStoreValueSecV1 oldValue{};
    oldValue.peer_addr = curValue.peer_addr;
    oldValue.key_size = curValue.key_size;
    oldValue.ediv = curValue.ediv;
    oldValue.rand_num = curValue.rand_num;
    memcpy(oldValue.ltk, curValue.ltk, sizeof(oldValue.ltk));
    oldValue.ltk_present = curValue.ltk_present;
    memcpy(oldValue.irk, curValue.irk, sizeof(oldValue.irk));
    oldValue.irk_present = curValue.irk_present;
    memcpy(oldValue.csrk, curValue.csrk, sizeof(oldValue.csrk));
    oldValue.csrk_present = curValue.csrk_present;
    oldValue.authenticated = curValue.authenticated;
    oldValue.sc = curValue.sc;

    err = nvs_set_blob(nvsHandle, key, &oldValue, sizeof(oldValue));
    if (err != ESP_OK) {
        ESP_LOGE(kLogTag, "nvs_set_blob failed key=%s err=%d", key, static_cast<int>(err));
        return false;
    }

    stats->converted++;
    return true;
}

inline bool migrateLocalIrkEntryToCurrent(nvs_handle_t nvsHandle,
                                          const char* key,
                                          MigrationStats* stats) {
    size_t blobSize = 0;
    esp_err_t err = nvs_get_blob(nvsHandle, key, nullptr, &blobSize);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        blobSize = 0;
    } else if (err != ESP_OK) {
        ESP_LOGE(kLogTag, "nvs_get_blob size failed key=%s err=%d", key, static_cast<int>(err));
        return false;
    } else {
        stats->scanned++;
    }

    if (blobSize != 0 && blobSize != sizeof(BleStoreValueLocalIrkCurrent) &&
        blobSize != sizeof(BleStoreValueLocalIrkV1)) {
        stats->skippedUnknownSize++;
        ESP_LOGW(kLogTag,
                 "Overwriting key=%s with default local_irk despite unexpected size=%u",
                 key,
                 static_cast<unsigned>(blobSize));
    }

    BleStoreValueLocalIrkCurrent newValue{};
    memcpy(newValue.irk, kDefaultLocalIrk, sizeof(newValue.irk));

    err = nvs_set_blob(nvsHandle, key, &newValue, sizeof(newValue));
    if (err != ESP_OK) {
        ESP_LOGE(kLogTag, "nvs_set_blob failed key=%s err=%d", key, static_cast<int>(err));
        return false;
    }

    stats->converted++;
    return true;
}

inline bool migrateLocalIrkEntryToV1(nvs_handle_t nvsHandle,
                                     const char* key,
                                     MigrationStats* stats) {
    size_t blobSize = 0;
    esp_err_t err = nvs_get_blob(nvsHandle, key, nullptr, &blobSize);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        return true;
    }
    if (err != ESP_OK) {
        ESP_LOGE(kLogTag, "nvs_get_blob size failed key=%s err=%d", key, static_cast<int>(err));
        return false;
    }

    stats->scanned++;

    // Rollback to v1 should not persist local IRK entries; erase if present.
    err = nvs_erase_key(nvsHandle, key);
    if (err != ESP_OK) {
        ESP_LOGE(kLogTag, "nvs_erase_key failed key=%s err=%d", key, static_cast<int>(err));
        return false;
    }

    stats->converted++;
    return true;
}

inline bool migrateBondStore(bool toCurrent, uint16_t maxEntries) {
    nvs_handle_t nvsHandle;
    esp_err_t err = nvs_open(kBondNamespace, NVS_READWRITE, &nvsHandle);
    if (err != ESP_OK) {
        ESP_LOGE(kLogTag,
                 "Failed to open NVS namespace '%s', err=%d",
                 kBondNamespace,
                 static_cast<int>(err));
        return false;
    }

    MigrationStats stats{};
    char key[kNvsKeyMaxLen]{};

    for (uint16_t i = 1; i <= maxEntries; ++i) {
        if (!makeBondKey(key, sizeof(key), kOurSecPrefix, i)) {
            nvs_close(nvsHandle);
            return false;
        }

        if (!(toCurrent ? migrateEntryToCurrent(nvsHandle, key, i, &stats)
                        : migrateEntryToV1(nvsHandle, key, &stats))) {
            nvs_close(nvsHandle);
            return false;
        }

        if (!makeBondKey(key, sizeof(key), kPeerSecPrefix, i)) {
            nvs_close(nvsHandle);
            return false;
        }

        if (!(toCurrent ? migrateEntryToCurrent(nvsHandle, key, i, &stats)
                        : migrateEntryToV1(nvsHandle, key, &stats))) {
            nvs_close(nvsHandle);
            return false;
        }
    }

    if (toCurrent) {
        if (!makeBondKey(key, sizeof(key), kLocalIrkPrefix, 1)) {
            nvs_close(nvsHandle);
            return false;
        }

        if (!migrateLocalIrkEntryToCurrent(nvsHandle, key, &stats)) {
            nvs_close(nvsHandle);
            return false;
        }
    } else {
        for (uint16_t i = 1; i <= maxEntries; ++i) {
            if (!makeBondKey(key, sizeof(key), kLocalIrkPrefix, i)) {
                nvs_close(nvsHandle);
                return false;
            }

            if (!migrateLocalIrkEntryToV1(nvsHandle, key, &stats)) {
                nvs_close(nvsHandle);
                return false;
            }
        }
    }

    if (stats.converted > 0) {
        err = nvs_commit(nvsHandle);
        if (err != ESP_OK) {
            ESP_LOGE(kLogTag, "nvs_commit failed err=%d", static_cast<int>(err));
            nvs_close(nvsHandle);
            return false;
        }
    }

    nvs_close(nvsHandle);

    ESP_LOGI(kLogTag,
             "Bond migration %s: scanned=%u converted=%u already=%u skipped=%u",
             toCurrent ? "to-current" : "to-v1",
             stats.scanned,
             stats.converted,
             stats.alreadyTarget,
             stats.skippedUnknownSize);

    return true;
}

} // namespace detail

/**
 * @brief Read and format the contents of the NimBLE bond store.
 *
 * This is intended for diagnostics before or after migration and returns a
 * human-readable summary of each `our_sec_*`, `peer_sec_*`, and `local_irk_*`
 * record found in the NVS bond namespace.
 *
 * @param[in] maxEntries The maximum number of bond slots to inspect.
 * @return A formatted string describing the stored bond data, or an error
 *         message if the NVS namespace could not be opened.
 */
inline std::string dumpBondData(uint16_t maxEntries = MYNEWT_VAL(BLE_STORE_MAX_BONDS)) {
    std::string out;
    out.reserve(2048);

    nvs_handle_t nvsHandle;
    esp_err_t    err = nvs_open(detail::kBondNamespace, NVS_READONLY, &nvsHandle);
    if (err != ESP_OK) {
        detail::appendLine(out,
                           "Failed to open NVS namespace '%s', err=%d\n",
                           detail::kBondNamespace,
                           static_cast<int>(err));
        return out;
    }

    out += "NimBLE bond dump\n";
    detail::appendLine(out,
                       "v1_size=%u current_size=%u max_entries=%u\n",
                       static_cast<unsigned>(sizeof(detail::BleStoreValueSecV1)),
                       static_cast<unsigned>(sizeof(detail::BleStoreValueSecCurrent)),
                       maxEntries);

    uint16_t foundCount = 0;
    char     key[detail::kNvsKeyMaxLen]{};

    for (uint16_t i = 1; i <= maxEntries; ++i) {
        const char* prefixes[] = {detail::kOurSecPrefix, detail::kPeerSecPrefix};
        const char* types[]    = {"our", "peer"};

        for (size_t j = 0; j < 2; ++j) {
            if (!detail::makeBondKey(key, sizeof(key), prefixes[j], i)) {
                out += "Key format error\n";
                continue;
            }

            size_t blobSize = 0;
            err             = nvs_get_blob(nvsHandle, key, nullptr, &blobSize);
            if (err == ESP_ERR_NVS_NOT_FOUND) {
                continue;
            }
            if (err != ESP_OK) {
                detail::appendLine(out,
                                   "[%s:%u] key=%s read-size error err=%d\n",
                                   types[j],
                                   i,
                                   key,
                                   static_cast<int>(err));
                continue;
            }

            std::vector<uint8_t> blob(blobSize);
            size_t               readSize = blobSize;
            err = nvs_get_blob(nvsHandle, key, blob.data(), &readSize);
            if (err != ESP_OK) {
                detail::appendLine(out,
                                   "[%s:%u] key=%s read error err=%d\n",
                                   types[j],
                                   i,
                                   key,
                                   static_cast<int>(err));
                continue;
            }

            foundCount++;
            detail::appendLine(out,
                               "[%s:%u] key=%s size=%u ",
                               types[j],
                               i,
                               key,
                               static_cast<unsigned>(blobSize));

            if (blobSize == sizeof(detail::BleStoreValueSecCurrent)) {
                out += "format=current\n";
                detail::BleStoreValueSecCurrent sec{};
                memcpy(&sec, blob.data(), sizeof(sec));
                detail::appendCurrentRecord(out, sec);
            } else if (blobSize == sizeof(detail::BleStoreValueSecV1)) {
                out += "format=v1\n";
                detail::BleStoreValueSecV1 sec{};
                memcpy(&sec, blob.data(), sizeof(sec));
                detail::appendV1Record(out, sec);
            } else {
                out += "format=unknown\n";
                out += "  raw=";
                const size_t dumpLen = blobSize > 64 ? 64 : blobSize;
                detail::appendHex(out, blob.data(), dumpLen);
                if (blobSize > dumpLen) {
                    out += "...";
                }
                out += "\n";
            }
        }

        if (!detail::makeBondKey(key, sizeof(key), detail::kLocalIrkPrefix, i)) {
            out += "Key format error\n";
            continue;
        }

        size_t blobSize = 0;
        err             = nvs_get_blob(nvsHandle, key, nullptr, &blobSize);
        if (err == ESP_ERR_NVS_NOT_FOUND) {
            continue;
        }
        if (err != ESP_OK) {
            detail::appendLine(out,
                               "[local_irk:%u] key=%s read-size error err=%d\n",
                               i,
                               key,
                               static_cast<int>(err));
            continue;
        }

        std::vector<uint8_t> blob(blobSize);
        size_t               readSize = blobSize;
        err = nvs_get_blob(nvsHandle, key, blob.data(), &readSize);
        if (err != ESP_OK) {
            detail::appendLine(out,
                               "[local_irk:%u] key=%s read error err=%d\n",
                               i,
                               key,
                               static_cast<int>(err));
            continue;
        }

        foundCount++;
        detail::appendLine(out,
                           "[local_irk:%u] key=%s size=%u ",
                           i,
                           key,
                           static_cast<unsigned>(blobSize));

        if (blobSize == sizeof(detail::BleStoreValueLocalIrkCurrent)) {
            out += "format=local_irk_current\n";
            detail::BleStoreValueLocalIrkCurrent irkRecord{};
            memcpy(&irkRecord, blob.data(), sizeof(irkRecord));
            detail::appendLocalIrkCurrentRecord(out, irkRecord);
        } else if (blobSize == sizeof(detail::BleStoreValueLocalIrkV1)) {
            out += "format=local_irk_v1\n";
            detail::BleStoreValueLocalIrkV1 irkRecord{};
            memcpy(&irkRecord, blob.data(), sizeof(irkRecord));
            detail::appendLocalIrkV1Record(out, irkRecord);
        } else {
            out += "format=unknown\n";
            out += "  raw=";
            const size_t dumpLen = blobSize > 64 ? 64 : blobSize;
            detail::appendHex(out, blob.data(), dumpLen);
            if (blobSize > dumpLen) {
                out += "...";
            }
            out += "\n";
        }
    }

    if (foundCount == 0) {
        out += "No bond entries found\n";
    } else {
        detail::appendLine(out, "Total entries found: %u\n", foundCount);
    }

    nvs_close(nvsHandle);
    return out;
}

/**
 * @brief Convert legacy 1.x bond data in NVS to the current 2.x format.
 *
 * This function should be called once, before `NimBLEDevice::init()`, by a
 * temporary migration firmware when upgrading an existing installation that
 * needs to preserve bonded peers.
 *
 * @param[in] maxEntries The maximum number of bond slots to inspect and
 *            convert.
 * @return `true` if migration completed successfully, otherwise `false`.
 */
inline bool migrateBondStoreToCurrent(uint16_t maxEntries = MYNEWT_VAL(BLE_STORE_MAX_BONDS)) {
    return detail::migrateBondStore(true, maxEntries);
}

/**
 * @brief Convert current 2.x bond data in NVS back to the legacy 1.x format.
 *
 * This is intended for rollback scenarios where bond data must be preserved
 * before flashing firmware built against the 1.x library.
 *
 * @param[in] maxEntries The maximum number of bond slots to inspect and
 *            convert.
 * @return `true` if migration completed successfully, otherwise `false`.
 */
inline bool migrateBondStoreToV1(uint16_t maxEntries = MYNEWT_VAL(BLE_STORE_MAX_BONDS)) {
    return detail::migrateBondStore(false, maxEntries);
}

} // namespace NimBLEBondMigration
