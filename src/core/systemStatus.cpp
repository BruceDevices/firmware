#include "systemStatus.h"

#include <Arduino.h>
#include <vector>

#include "esp32-hal-cpu.h"

extern "C" {
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
}

namespace SystemStatus {
namespace {
constexpr uint32_t SAMPLE_INTERVAL_MS = 1000;

uint32_t lastFreqMhz = 0;
uint8_t lastCpuLoad = 0;
uint32_t lastSampleMs = 0;
uint64_t prevTotalRuntime = 0;
uint64_t prevIdleRuntime = 0;

void sampleRuntimeStats() {
#if (configUSE_TRACE_FACILITY == 1) && (configGENERATE_RUN_TIME_STATS == 1)
    UBaseType_t taskCount = uxTaskGetNumberOfTasks();
    if (taskCount == 0) return;

    std::vector<TaskStatus_t> taskStats(taskCount);
    uint32_t totalRuntime = 0;
    taskCount = uxTaskGetSystemState(taskStats.data(), taskStats.size(), &totalRuntime);
    if (taskCount == 0 || totalRuntime == 0) return;

    uint64_t idleRuntime = 0;
    TaskHandle_t idleHandles[portNUM_PROCESSORS] = {};
    for (int cpu = 0; cpu < portNUM_PROCESSORS; ++cpu) {
        idleHandles[cpu] = xTaskGetIdleTaskHandleForCPU(cpu);
    }

    for (const auto &task : taskStats) {
        for (int cpu = 0; cpu < portNUM_PROCESSORS; ++cpu) {
            if (task.xHandle == idleHandles[cpu]) { idleRuntime += task.ulRunTimeCounter; }
        }
    }

    if (prevTotalRuntime == 0 || prevIdleRuntime == 0 || idleRuntime < prevIdleRuntime ||
        totalRuntime < prevTotalRuntime) {
        prevTotalRuntime = totalRuntime;
        prevIdleRuntime = idleRuntime;
        return;
    }

    uint64_t deltaTotal = totalRuntime - prevTotalRuntime;
    uint64_t deltaIdle = idleRuntime - prevIdleRuntime;
    prevTotalRuntime = totalRuntime;
    prevIdleRuntime = idleRuntime;

    if (deltaTotal == 0) return;
    if (deltaIdle > deltaTotal) deltaIdle = deltaTotal;
    lastCpuLoad = static_cast<uint8_t>(100 - (deltaIdle * 100ULL / deltaTotal));
#else
    lastCpuLoad = 0;
#endif
}
} // namespace

void begin() {
    lastFreqMhz = getCpuFrequencyMhz();
    lastSampleMs = millis();
    prevTotalRuntime = 0;
    prevIdleRuntime = 0;
}

void update() {
    uint32_t now = millis();
    if (now - lastSampleMs < SAMPLE_INTERVAL_MS) return;
    lastSampleMs = now;
    lastFreqMhz = getCpuFrequencyMhz();
    sampleRuntimeStats();
}

uint32_t cpuFrequencyMhz() { return lastFreqMhz; }

uint8_t cpuLoadPercent() { return lastCpuLoad; }
} // namespace SystemStatus
