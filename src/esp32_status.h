#ifndef ESP32_STATUS_H
#define ESP32_STATUS_H

#include <fstream>

struct esp32_status_t {
    uint32_t program_counter;
    uint32_t instruction;

    // Registers
    uint32_t a[16];
    uint32_t litbase;
    int32_t vAddr;
};

extern esp32_status_t* esp32_status;

#endif