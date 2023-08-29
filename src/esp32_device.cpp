#include "esp32_device.h"
#include "esp32_instruction.h"
#include <iostream>

int32_t temp;

bool esp32_instruction_parse(esp32_device_t* device, esp32_instruction_t* instruction){
    if(instruction->mask == 0) return false;

    esp32_instruction_t* l = instruction->sub[(device->instruction >> instruction->shift) & instruction->mask];

    if(l == nullptr) return false;

    if(l->pFunc != nullptr){ 
        l->pFunc(device);
        return true;
    }else{
        
        return esp32_instruction_parse(device, l);
    }
}

bool esp32_instruction_parse(esp32_device_t* device){
    return esp32_instruction_parse(device, instruction_base);
    
}

uint32_t esp32_register_a_read(esp32_device_t* status, uint16_t reg){
    return status->ar[((((reg >> 2) & 0b11)+status->special[ESP32_REG_WINDOWBASE]) << 2)|(reg & 0b11)];
}

void esp32_register_a_write(esp32_device_t* status, uint16_t reg, uint32_t value){
    status->ar[((((reg >> 2) & 0b11)+status->special[ESP32_REG_WINDOWBASE]) << 2)|(reg & 0b11)] = value;
}

uint32_t esp32_memory_load32(esp32_device_t* device){
    device->pAddr = device->vAddr & 0x7fffffff;
    return device->memory[device->pAddr+3] << 24 | device->memory[device->pAddr+2] << 16 | device->memory[device->pAddr+1] << 8 | device->memory[device->pAddr];
}

void esp32_memory_write32(esp32_device_t* device, uint32_t val){
    device->pAddr = device->vAddr & 0x7fffffff;
    device->memory[device->pAddr+3] = val >> 24 & 0xff;
    device->memory[device->pAddr+2] = val >> 16 & 0xff;
    device->memory[device->pAddr+1] = val >> 8 & 0xff;
    device->memory[device->pAddr+0] = val >> 0 & 0xff;
}

