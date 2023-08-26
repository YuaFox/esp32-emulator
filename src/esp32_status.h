#ifndef ESP32_STATUS_H
#define ESP32_STATUS_H

#include <fstream>

#define ESP32_REG_LITBASE       5
#define ESP32_REG_WINDOWBASE    72
#define ESP32_REG_WINDOWSTART   73
#define VECBASE                 231


struct esp32_status_t {
    uint32_t program_counter;
    uint32_t instruction;

    // Registers
    uint32_t ar[16*16];
    uint32_t special[256];
    int32_t vAddr;
    uint8_t ps_callinc = 0;

    // Emulator
    int32_t temp;
    bool print_instr = false;
};

extern esp32_status_t* esp32_status;

uint32_t esp32_register_a_read(esp32_status_t* status, uint16_t reg);

void esp32_register_a_write(esp32_status_t* status, uint16_t reg, uint32_t value);

#endif