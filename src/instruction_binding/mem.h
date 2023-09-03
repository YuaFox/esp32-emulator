#include "../esp32_device.h"

#include <string.h>

void esp32_f_memset(esp32_device_t* device){
    memset(
        &device->memory[esp32_memory_paddr(esp32_register_a_read(device, 2))],
        esp32_memory_paddr(esp32_register_a_read(device, 3)),
        esp32_register_a_read(device, 4)
    );
}

void esp32_f_memcpy(esp32_device_t* device){
    memcpy(
        &device->memory[esp32_memory_paddr(esp32_register_a_read(device, 2))],
        &device->memory[esp32_memory_paddr(esp32_register_a_read(device, 3))],
        esp32_register_a_read(device, 4)
    );
    printf("memo %x\n", esp32_register_a_read(device, 2));
}