#include <iostream>
#include <fstream>
#include <stdio.h>
#include <cstring>
#include <bitset>
#include "src/esp32_instruction.h"


#define ESP_MEMORY 0x6008598c





int main() {

    static uint8_t memory[ESP_MEMORY];
    
    // read firmware
    std::ifstream file("demo/hello_world.bin", std::ios::binary);
    file.seekg(0, std::ios::end);
    size_t fileSize = file.tellg();
    file.seekg(0, std::ios::beg);
    uint8_t firmware[fileSize];
    file.read((char*) &firmware[0], fileSize);

    // copy to memory
    std::memcpy(&memory[0x3f400020], &firmware[0x00000018], 0x087e8); // 1 DROM
    std::memcpy(&memory[0x40080000], &firmware[0x0000a684], 0x0598c); // 3 IRAM
    std::memcpy(&memory[0x400d0020], &firmware[0x00010018], 0x15be0); // 4 IROM
    std::memcpy(&memory[0x4008598c], &firmware[0x00025c00], 0x05ed8); // 5 IRAM

    esp32_instruction_init();

    //uint32_t program_counter = 0x400e556e; // app_main
    //                             400d0c18
    esp32_status->program_counter = 0x400d0020+0x10C10-0x00010018; // elf_main

    uint16_t oks = 0;

    std::cout << "[EMU] Run started!" << std::endl;

    while(true){
        esp32_status->instruction = memory[esp32_status->program_counter+2] << 16 | memory[esp32_status->program_counter+1] << 8 | memory[esp32_status->program_counter];
        if(esp32_instruction_parse(memory, esp32_status)){
            oks++;
        }else{
            std::cout << "ERROR: Operation not supported" << std::endl;
            std::cout << "Got " << oks << " OKS!" << std::endl << std::endl;
            std::cout << "24   20   16   12    8    4    0" << std::endl;
            std::cout << "|    |    |    |    |    |    |" << std::endl;
            std::cout   
                    << " " << std::bitset<4>((esp32_status->instruction >> 20) &0xf)
                    << " " << std::bitset<4>((esp32_status->instruction >> 16) &0xf)
                    << " " << std::bitset<4>((esp32_status->instruction >> 12) &0xf)
                    << " " << std::bitset<4>((esp32_status->instruction >>  8) &0xf)
                    << " " << std::bitset<4>((esp32_status->instruction >>  4) &0xf)
                    << " " << std::bitset<4>((esp32_status->instruction >>  0) &0xf) << std::endl;
            exit(0);
            break;
        }

    }
}