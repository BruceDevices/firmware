Import("env")

# The Arduino ESP32 framework injects -fno-lto into LINKFLAGS to prevent LTO
# at link time. Remove it so GCC can perform whole-program LTO optimization
# across all LTO IR objects (user code + framework Arduino source).
# Prebuilt ESP-IDF archives (libfreertos.a etc.) are regular objects and
# participate in LTO linking transparently.
flags = env.get("LINKFLAGS", [])
if "-fno-lto" in flags:
    env.Replace(LINKFLAGS=[f for f in flags if f != "-fno-lto"])
