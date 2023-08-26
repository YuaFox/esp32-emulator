#include <iostream>
#include <fstream>
#include <stdio.h>
#include <cstring>
#include <bitset>

#include "src/esp32_instruction.h"


#define ESP_MEMORY 0x6008598c


int main(int argc, char *argv[]) {

    static uint8_t memory[ESP_MEMORY];

    // Emulator options
    esp32_status->print_instr = false;
    for(int i = 0; i < argc; i++){
        if(!strcmp(argv[i], "--show-instructions")){
            esp32_status->print_instr = true;
        }
    }
    
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

    // Simulate bootloader
    esp32_status->program_counter = firmware[0x7] << 24 | firmware[0x6] << 16 | firmware[0x5] << 8 | firmware[0x4]; // Don't match with esp32
    printf("ets %s %s\n\n", (char*)&firmware[0x80], (char*)&firmware[0x70]);
    printf("\x1b[31m");
    printf("rst:0x1 (POWERON_RESET),boot:0x13 (SPI_FAST_FLASH_BOOT)\nconfigsip: 0, SPIWP:0xee\nclk_drv:0x00,q_drv:0x00,d_drv:0x00,cs0_drv:0x00,hd_drv:0x00,wp_drv:0x00\nmode:DIO, clock div:2\nload:0x3fff0030,len:6940\nho 0 tail 12 room 4\nload:0x40078000,len:15500\nload:0x40080400,len:3844\n");
    printf("\x1b[0m");
    printf("entry %#010x\n", esp32_status->program_counter);


    std::memcpy(&memory[0x3f400020], &firmware[0x00000020], 0x087e8); // 1 DROM
    std::memcpy(&memory[0x40080000], &firmware[0x0000a68c], 0x0598c); // 3 IRAM
    std::memcpy(&memory[0x400d0020], &firmware[0x00010020], 0x15be0); // 4 IROM
    std::memcpy(&memory[0x4008598c], &firmware[0x00025c08], 0x05ed8); // 5 IRAM

    esp32_status->special[ESP32_REG_LITBASE] = 0;

    esp32_status->program_counter = 0x40081044; // call_start_cpu0
    //esp32_status->program_counter = 0x400d0c10; // app_main

    uint16_t oks = 0;

    printf("\x1B[32m");
    while(true){
        esp32_status->instruction = memory[esp32_status->program_counter+2] << 16 | memory[esp32_status->program_counter+1] << 8 | memory[esp32_status->program_counter];
        // int puts(char * __s)
        if(esp32_status->program_counter == 0x400d5014){
            printf("\x1b[0m");
            const char* output = (char*) &memory[esp32_status->ar[10]];
            puts(output); 
            exit(0);
        }
        if(esp32_instruction_parse(memory, esp32_status)){
            oks++;
        }else{
            puts("\x1b[31m");
            std::cout << "\nERROR: Operation not supported" << std::endl;
            std::cout << "Got " << oks << " OKS!" << std::endl << std::endl;
            std::cout << "PC    : 0x" << std::hex << esp32_status->program_counter << std::endl;
            std::cout << "INSTR : 0x" << std::hex << esp32_status->instruction << std::endl;
            std::cout << "        24   20   16   12    8    4    0" << std::endl;
            std::cout << "        |    |    |    |    |    |    |" << std::endl;
            std::cout << "        "
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