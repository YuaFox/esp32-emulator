#ifndef ESP32_DEVICE_H
#define ESP32_DEVICE_H

#include <fstream>

#define ESP32_REG_LITBASE       5
#define ESP32_REG_WINDOWBASE    72
#define ESP32_REG_WINDOWSTART   73
#define ESP32_REG_PS            230
    #define ESP32_REG_PS_INTLEVEL 0
#define ESP32_REG_VECBASE       231
#define ESP32_REG_CCOUNT        234
#define ESP32_REG_PRID          235


struct esp32_device_t {
    uint8_t memory[0x7fffffff];

    uint32_t program_counter;
    uint32_t instruction;

    // Registers
    uint32_t ar[16*16];
    uint32_t special[256];
    uint32_t vAddr;
    uint32_t pAddr;
    uint32_t mask;

    // Special ps
    uint8_t ps_callinc = 0;
    uint8_t ps_owb = 0;
    uint8_t ps_intlevel = 0;

    // Helpers
    int32_t temp;
    int32_t s, t;


    // Emulator options
    bool print_instr = false;
};

bool esp32_instruction_parse(esp32_device_t* device);

uint32_t esp32_register_a_read(esp32_device_t* status, uint16_t reg);

void esp32_register_a_write(esp32_device_t* status, uint16_t reg, uint32_t value);

uint32_t esp32_memory_paddr(uint32_t vAddr);

uint32_t esp32_memory_load8(esp32_device_t* device);
uint32_t esp32_memory_load16(esp32_device_t* device);
uint32_t esp32_memory_load32(esp32_device_t* device);
void esp32_memory_write8(esp32_device_t* device, uint8_t val);
void esp32_memory_write16(esp32_device_t* device, uint16_t val);
void esp32_memory_write32(esp32_device_t* device, uint32_t val);

#endif