#include <fstream>
#include <stdarg.h>
#include <stdio.h>
#include "esp32_status.h"

struct esp32_instruction_t {
    char* name;
    esp32_instruction_t* sub[16];
    uint32_t mask;
    uint8_t shift;
    void (*pFunc)(uint8_t*, esp32_status_t*);
};

extern esp32_instruction_t* instruction_base;

void esp32_instruction_init();

bool esp32_instruction_parse(uint8_t* memory, esp32_status_t* status);