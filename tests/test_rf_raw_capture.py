#!/usr/bin/env python3
"""Run production rfReceiveSignal and RAW serialization against a fake radio.

Requires g++; no radio transmissions or ESP32 dependencies.
"""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
source = (root / 'src/modules/rf/rf_scan.cpp').read_text()
receive = source[source.index('String rfReceiveSignal('):]
presets = (root / 'src/modules/rf/protocols/rf_presets.cpp').read_text()
presets = '\n'.join(line for line in presets.splitlines() if not line.startswith('#include'))
shim = r'''
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>
#include "rf_raw_capture.h"
struct String : std::string {
    using std::string::string;
    String(const std::string &s) : std::string(s) {}
    String(int v) : std::string(std::to_string(v)) {}
    String(float v) : std::string(std::to_string(v)) {}
    bool isEmpty() const { return empty(); }
    void replace(const char *a, const char *b) {
        size_t p = 0;
        while ((p = find(a, p)) != npos) std::string::replace(p, std::string(a).size(), b);
    }
};
struct RfPreset {
    const char *name; uint8_t modulation; float deviation, rxBW, dataRate; uint8_t legacyProto;
};
struct RfCodes {
    uint64_t key=0; int frequency=0, te=0, Bit=0, hop=0, serial=0, cnt=0, fix=0, btn=0;
    String filepath, data, protocol, preset, mf_name;
};
constexpr int CC1101_SPI_MODULE=1, BORDER_PAD_X=0, BORDER_PAD_Y=0, FP=1;
struct { float rfFreq=433.92f; int rfModule=CC1101_SPI_MODULE; } bruceConfigPins;
bool EscPress=false, deliver=true, full=false;
uint32_t clockMs=0;
int inits=0, deinits=0, normalizations=0, polls=0;
float tuned=0;
std::vector<int> capture;
uint32_t millis() { return clockMs; }
void vTaskDelay(int ticks) { clockMs += ticks; }
bool check(bool &flag) { bool result=flag; flag=false; return result; }
void drawMainBorder() {}
struct { void setCursor(int,int){} void setTextSize(int){} void println(String){} } tft;
struct {
    int modulation=2; float deviation=0, bw=0, rate=0;
    void setSidle() {} void SetRx() {} void setPktFormat(int) {}
    void setModulation(int m) { modulation=m; }
    void setDeviation(float d) { deviation=d; }
    void setRxBW(float b) { bw=b; }
    void setDRate(float r) { rate=r; }
    uint8_t SpiReadReg(uint8_t address) { return address; }
} ELECHOUSE_cc1101;
bool initRfModule(String, float frequency) {
    ++inits; tuned=frequency; ELECHOUSE_cc1101.modulation=2; return true;
}
void deinitRfModule() { ++deinits; }
struct RfRxSession {
    bool begin(bool radioReady=false) {
        if (!radioReady) initRfModule("rx",bruceConfigPins.rfFreq);
        return true;
    }
    bool poll(std::vector<int> &v) {
        ++polls;
        if (!deliver) return false;
        v=capture; return true;
    }
    bool bufferFull() { return full; }
    void end() {}
};
bool rf_try_keeloq(const std::vector<int>&, RfCodes&) { return false; }
bool rf_decode_ook(const std::vector<int>&, RfCodes&) { return false; }
int rf_build_raw(const std::vector<int>&, String&, bool&, uint64_t&, std::vector<int>&, int&, int&) {
    ++normalizations; return 0;
}
void decimalToHexString(uint64_t v, char *out) { std::sprintf(out,"%llX",(unsigned long long)v); }
void display_info(RfCodes,int,bool,bool,bool,String,bool) {}
String rf_subghz_header(float f) { return "Filetype: Flipper SubGhz RAW File\nFrequency: " + String(f) + "\n"; }
String rf_flipper_protocol_name(String p) { return p; }
'''
checks = r'''
std::vector<int> readRaw(const std::string &file) {
    std::istringstream lines(file); std::string line; std::vector<int> pulses;
    while (std::getline(lines,line)) {
        if (line.rfind("RAW_Data:",0)) continue;
        std::istringstream values(line.substr(9)); int value; size_t count=0;
        while (values>>value) { pulses.push_back(value); ++count; }
        assert(count <= 512);
    }
    return pulses;
}
int main() {
    for (int i=0;i<1301;++i) capture.push_back(i%2 ? -30000 : 31+i);
    auto file=rfReceiveSignal(315.0f,1,true,true,"FuriHalSubGhzPreset2FSKDev476Async");
    assert(inits==1 && deinits==1 && tuned==315.0f);
    assert(ELECHOUSE_cc1101.modulation==0 && ELECHOUSE_cc1101.bw==476.f);
    assert(file.find("Bruce_RX_Preset: 2FSKDev476Async\n")!=std::string::npos);
    assert(file.find("Bruce_RX_Registers: 00 01 02")!=std::string::npos);
    assert(readRaw(file)==capture && normalizations==0);
    full=true;
    file=rfReceiveSignal(433.92f,1,true,true,"Ook650Async");
    assert(ELECHOUSE_cc1101.modulation==2 && ELECHOUSE_cc1101.bw==650.f);
    assert(file.find("Bruce_Buffer_Full: 1\n")!=std::string::npos);
    assert(readRaw(file)==capture);
    int previous=inits;
    assert(rfReceiveSignal(433.92f,1,true,true,"invalid").empty());
    bruceConfigPins.rfModule=0;
    assert(rfReceiveSignal(433.92f,1,true,true,"Ook650Async").empty());
    assert(inits==previous);
    bruceConfigPins.rfModule=CC1101_SPI_MODULE;
    file=rfReceiveSignal(433.92f,1,true,true,"");
    assert(file.find("Bruce_RX_Preset:")==std::string::npos);
    assert(readRaw(file)==capture);
    EscPress=true;
    assert(rfReceiveSignal(433.92f,1,true,true,"").empty() && EscPress);
    EscPress=false; deliver=false; clockMs=UINT32_MAX-500; polls=0;
    assert(rfReceiveSignal(433.92f,1,true,true,"").empty());
    assert(clockMs==499 && polls==1001); // wrap-safe one-second timeout, prompt polling
    assert(rf_capture::rawLines({}).empty());
    std::puts("PASS: presets, preserved radio setup, raw timing/line limits, full-buffer reporting, timeout and Back");
}
'''
with tempfile.TemporaryDirectory() as directory:
    cpp, exe = Path(directory) / 'raw.cpp', Path(directory) / 'raw-test'
    cpp.write_text(shim + presets + receive + checks)
    subprocess.run(['g++', '-std=c++17', '-Wall', '-Wextra', '-Werror',
                    '-fsanitize=address,undefined', '-fno-omit-frame-pointer', '-no-pie',
                    '-I', str(root / 'src/modules/rf/protocols'), str(cpp), '-o', str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
