#!/usr/bin/env bash
set -euo pipefail

cd /work

# Match .github/workflows/PR_All_envs.yml matrix envs
DEFAULT_ENVS=(
  m5stack-cardputer
  m5stack-sticks3
  m5stack-cplus2
  m5stack-cplus1_1
  LAUNCHER_m5stack-cplus1_1
  m5stack-core2
  m5stack-core16mb
  m5stack-core4mb
  m5stack-cores3
  esp32-s3-devkitc-1
  esp32-c5
  CYD-2432S028
  CYD-2USB
  CYD-2432W328C
  CYD-2432W328C_2
  CYD-2432W328R-or-S024R
  CYD-3248S035R
  CYD-3248S035C
  LAUNCHER_CYD-2432W328R-or-S024R
  LAUNCHER_CYD-2432S028
  LAUNCHER_CYD-2USB
  LAUNCHER_CYD-2432W328C
  LAUNCHER_CYD-3248S035R
  LAUNCHER_CYD-3248S035C
  lilygo-t-embed-cc1101
  lilygo-t-embed
  lilygo-t-deck
  lilygo-t-watch-s3
  lilygo-t-deck-pro
  lilygo-t-display-s3
  lilygo-t-display-s3-touch
  lilygo-t-display-s3-mmc
  lilygo-t-display-s3-touch-mmc
  lilygo-t-display-S3-pro
  lilygo-t-display-ttgo
  lilygo-t-hmi
  lilygo-t-lora-pager
  smoochiee-board
  Phantom_S024R
  LAUNCHER_Phantom_S024R
  Marauder-Mini
  LAUNCHER_Marauder-Mini
  Awok-Mini
  Marauder-v7
  LAUNCHER_Marauder-v7
  Marauder-V4-V6
  Marauder-v61
  LAUNCHER_Marauder-V4-V6
  LAUNCHER_Marauder-v61
  Awok-Touch
  WaveSentry-R1
  LAUNCHER_WaveSentry-R1
)

if [[ -n "${PIO_ENVS:-}" ]]; then
  # Space-separated list
  read -r -a ENVS <<<"${PIO_ENVS}"
else
  ENVS=("${DEFAULT_ENVS[@]}")
fi

# Ensure platformio sees all needed platforms/packages
platformio --version

JOBS="${PIO_JOBS:-5}"

failed=()

for ((i = 0; i < ${#ENVS[@]}; i += JOBS)); do
  batch=("${ENVS[@]:i:JOBS}")

  pids=()
  pid_envs=()
  for e in "${batch[@]}"; do
    echo "===== platformio run -e ${e} ====="
    platformio run -e "${e}" &
    pids+=("$!")
    pid_envs+=("${e}")
  done

  for idx in "${!pids[@]}"; do
    pid="${pids[$idx]}"
    e="${pid_envs[$idx]}"
    if ! wait "${pid}"; then
      failed+=("${e}")
    fi
  done
done

if (( ${#failed[@]} > 0 )); then
  echo "Build failures: ${failed[*]}" >&2
  exit 1
fi

exit 0
