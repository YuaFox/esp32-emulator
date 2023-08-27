#ifndef ESP32_DEVICE_H
#define ESP32_DEVICE_H

#include <fstream>

#define ESP32_REG_LITBASE       5
#define ESP32_REG_WINDOWBASE    72
#define ESP32_REG_WINDOWSTART   73
#define VECBASE                 231


struct esp32_device_t {
    uint8_t memory[0x6008598c];

    uint32_t program_counter;
    uint32_t instruction;

    // Registers
    uint32_t ar[16*16];
    uint32_t special[256];
    int32_t vAddr;
    uint8_t ps_callinc = 0;
    uint8_t ps_owb = 0;

    // Emulator
    int32_t temp;
    bool print_instr = false;
};

bool esp32_instruction_parse(esp32_device_t* device);

uint32_t esp32_register_a_read(esp32_device_t* status, uint16_t reg);

void esp32_register_a_write(esp32_device_t* status, uint16_t reg, uint32_t value);

uint32_t esp32_memory_load32(esp32_device_t* device);

#endif