Import("env")
from pathlib import Path
root = Path(env.subst("$PROJECT_DIR")) / "lib/BruceWifite"
defines = {item[0] if isinstance(item, (tuple, list)) else item for item in env.get("CPPDEFINES", [])}
if "T_EMBED_1101" not in defines or "LITE_VERSION" in defines:
    env.Replace(SRC_FILTER=["-<*>"])
else:
    if env.BoardConfig().get("build.mcu") != "esp32s3":
        raise RuntimeError("WiFite currently supports ESP32-S3 only")
    env.Append(CCFLAGS=["-I" + str(root / 'vendor/include'),"-I" + str(root / 'vendor/port/include'),"-I" + str(root / 'vendor/esp_supplicant/include'),"-I" + str(root / 'vendor/src'),"-I" + str(root / 'vendor/src/utils'),"-I" + str(root / 'vendor/src/crypto'),"-I" + str(root / 'vendor/esp_supplicant/src'),'-D__ets__','-DESP_SUPPLICANT','-DIEEE8021X_EAPOL','-DEAP_PEER_METHOD','-DEAP_MSCHAPv2','-DEAP_TTLS','-DEAP_TLS','-DEAP_PEAP','-DUSE_WPA2_TASK','-DCONFIG_WPS','-DESPRESSIF_USE','-DCONFIG_ECC','-DCONFIG_IEEE80211W','-DCONFIG_SHA256','-DCONFIG_NO_RADIUS'])
