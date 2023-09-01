#include "esp32_device.h"
#include "esp32_instruction.h"
#include "esp32_bindings.h"
#include <iostream>
#include <bitset>

int32_t temp;

int esp32_run(esp32_device_t* device){
    uint16_t start_depth = device->call_depth;

    esp32_run_instruction(device);

    while(device->call_depth > start_depth){
        esp32_binding_map::iterator it = esp32_bindings.find(device->program_counter);

        if (it == esp32_bindings.end()) {
            esp32_run_instruction(device);
        } else {
            if(device->print_instr) printf("; -- binded %s\n", it->second.first);
            esp32_run_instruction(device, 0x136);
            it->second.second(device);
            esp32_run_instruction(device, 0x90);
            if(device->print_instr) puts("; -- end binding");
        }

        if(device->program_counter == 0x4008a48b+2){
            getchar();
        }
    }

    return 0;
}

void esp32_run_instruction(esp32_device_t* device){
    esp32_run_instruction(device, device->memory[device->program_counter+2] << 16 |
                                    device->memory[device->program_counter+1] << 8 |
                                    device->memory[device->program_counter]);
}

void esp32_run_instruction(esp32_device_t* device, uint32_t instruction){
    device->instruction = instruction;
    if(esp32_instruction_parse(device)){
        device->special[ESP32_REG_CCOUNT] += 1;
    }else{
        puts("\x1b[31m");
        std::cout << "\nERROR: Operation not supported" << std::endl;
        std::cout << "Got " << 0 << " OKS!" << std::endl << std::endl;
        std::cout << "PC    : 0x" << std::hex << device->program_counter << std::endl;
        std::cout << "INSTR : 0x" << std::hex << device->instruction << std::endl;
        std::cout << "        24   20   16   12    8    4    0" << std::endl;
        std::cout << "        |    |    |    |    |    |    |" << std::endl;
        std::cout << "        "
                << " " << std::bitset<4>((device->instruction >> 20) &0xf)
                << " " << std::bitset<4>((device->instruction >> 16) &0xf)
                << " " << std::bitset<4>((device->instruction >> 12) &0xf)
                << " " << std::bitset<4>((device->instruction >>  8) &0xf)
                << " " << std::bitset<4>((device->instruction >>  4) &0xf)
                << " " << std::bitset<4>((device->instruction >>  0) &0xf) << std::endl;
        exit(0);
    }
}


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

uint32_t esp32_memory_paddr(uint32_t vAddr){
    return vAddr & 0x7fffffff;
}

uint32_t esp32_memory_load8(esp32_device_t* device){
    device->pAddr = device->vAddr & 0x7fffffff;
    return device->memory[device->pAddr];
}
uint32_t esp32_memory_load16(esp32_device_t* device){
    device->pAddr = device->vAddr & 0x7fffffff;
    return device->memory[device->pAddr+1] << 8 | device->memory[device->pAddr];
}
uint32_t esp32_memory_load32(esp32_device_t* device){
    device->pAddr = device->vAddr & 0x7fffffff;
    return device->memory[device->pAddr+3] << 24 | device->memory[device->pAddr+2] << 16 | device->memory[device->pAddr+1] << 8 | device->memory[device->pAddr];
}

void esp32_memory_write8(esp32_device_t* device, uint8_t val){
    device->pAddr = device->vAddr & 0x7fffffff;
    device->memory[device->pAddr+0] = val >> 0 & 0xff;
}

void esp32_memory_write16(esp32_device_t* device, uint16_t val){
    device->pAddr = device->vAddr & 0x7fffffff;
    device->memory[device->pAddr+1] = val >> 8 & 0xff;
    device->memory[device->pAddr+0] = val >> 0 & 0xff;
}

void esp32_memory_write32(esp32_device_t* device, uint32_t val){
    device->pAddr = device->vAddr & 0x7fffffff;
    device->memory[device->pAddr+3] = val >> 24 & 0xff;
    device->memory[device->pAddr+2] = val >> 16 & 0xff;
    device->memory[device->pAddr+1] = val >> 8 & 0xff;
    device->memory[device->pAddr+0] = val >> 0 & 0xff;
}

