#include "esp32_status.h"

esp32_status_t* esp32_status = new esp32_status_t;

uint32_t esp32_register_a_read(esp32_status_t* status, uint16_t reg){
    return status->ar[((((reg >> 2) & 0b11)+status->special[ESP32_REG_WINDOWBASE]) << 2)|(reg & 0b11)];
}

void esp32_register_a_write(esp32_status_t* status, uint16_t reg, uint32_t value){
    status->ar[((((reg >> 2) & 0b11)+status->special[ESP32_REG_WINDOWBASE]) << 2)|(reg & 0b11)] = value;
}