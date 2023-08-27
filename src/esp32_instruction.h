#include <fstream>
#include <stdarg.h>
#include <stdio.h>
#include "esp32_device.h"

struct esp32_instruction_t {
    char* name;
    esp32_instruction_t* sub[16];
    uint32_t mask;
    uint8_t shift;
    void (*pFunc)(esp32_device_t*);
};

extern esp32_instruction_t* instruction_base;

void esp32_instruction_init();