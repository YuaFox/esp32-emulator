#include <iostream>
#include <fstream>
#include <stdio.h>
#include <cstring>
#include <bitset>
#include <string.h>

#include "src/esp32_instruction.h"


int main(int argc, char *argv[]) {

    esp32_device_t* esp32_device = new esp32_device_t;

    // Emulator options
    esp32_device->print_instr = false;
    for(int i = 0; i < argc; i++){
        if(!strcmp(argv[i], "--show-instructions")){
            esp32_device->print_instr = true;
        }
    }
    
    // read firmware
    std::ifstream file("demo/hello_world.bin", std::ios::binary);
    file.seekg(0, std::ios::end);
    size_t fileSize = file.tellg();
    file.seekg(0, std::ios::beg);
    uint8_t firmware[fileSize];
    file.read((char*) &firmware[0], fileSize);

    // copy to memory from esptool
    /*
    std::memcpy(&esp32_device->memory[0x3f400020], &firmware[0x00000018], 0x087e8); // 1 DROM
    std::memcpy(&esp32_device->memory[0x40080000], &firmware[0x0000a684], 0x0598c); // 3 IRAM
    std::memcpy(&esp32_device->memory[0x400d0020], &firmware[0x00010018], 0x15be0); // 4 IROM
    std::memcpy(&esp32_device->memory[0x4008598c], &firmware[0x00025c00], 0x05ed8); // 5 IRAM
    */

    esp32_instruction_init();

    // Simulate bootloader
    esp32_device->program_counter = firmware[0x7] << 24 | firmware[0x6] << 16 | firmware[0x5] << 8 | firmware[0x4]; // Don't match with esp32
    printf("ets %s %s\n\n", (char*)&firmware[0x80], (char*)&firmware[0x70]);
    printf("\x1b[31m");
    printf("rst:0x1 (POWERON_RESET),boot:0x13 (SPI_FAST_FLASH_BOOT)\nconfigsip: 0, SPIWP:0xee\nclk_drv:0x00,q_drv:0x00,d_drv:0x00,cs0_drv:0x00,hd_drv:0x00,wp_drv:0x00\nmode:DIO, clock div:2\nload:0x3fff0030,len:6940\nho 0 tail 12 room 4\nload:0x40078000,len:15500\nload:0x40080400,len:3844\n");
    printf("\x1b[0m");
    printf("entry %#010x\n", esp32_device->program_counter);

    // copy from real esp32
    std::memcpy(&esp32_device->memory[0x3f400020], &firmware[0x00000020], 0x087e8); // 1 DROM
    std::memcpy(&esp32_device->memory[0x3ffb0000], &firmware[0x00018810], 0x01e74); // 2
    std::memcpy(&esp32_device->memory[0x40080000], &firmware[0x0000a68c], 0x0598c); // 3 IRAM
    std::memcpy(&esp32_device->memory[0x400d0020], &firmware[0x00010020], 0x15be0); // 4 IROM
    std::memcpy(&esp32_device->memory[0x4008598c], &firmware[0x00025c08], 0x05ed8); // 5 IRAM

    esp32_device->program_counter = 0x400d1248;

    esp32_device->memory[0x3ffe01e0] = 0x1; // g_ticks_per_us_pro
    esp32_device->special[ESP32_REG_LITBASE] = 0;
    esp32_device->special[ESP32_REG_CCOUNT] = 0;
    esp32_register_a_write(esp32_device, 1, 0x3ffe3f20);



    uint16_t oks = 0;

    printf("\x1B[34m");
    while(true){
        esp32_device->instruction = esp32_device->memory[esp32_device->program_counter+2] << 16 |
                                    esp32_device->memory[esp32_device->program_counter+1] << 8 |
                                    esp32_device->memory[esp32_device->program_counter];
        // int puts(char * __s)
        if(esp32_device->program_counter == 0x400d5014){
            printf("\x1b[0m");
            const char* output = (char*) &esp32_device->memory[esp32_device->ar[10]];
            puts(output); 
            exit(0);
        }
        // rtc_get_reset_reason
        if(esp32_device->program_counter == 0x400081d4){
            esp32_device->instruction = 0x236;
            esp32_instruction_parse(esp32_device);
            esp32_device->instruction = 0x000090;
        }

        // unsigned _xtos_set_min_intlevel(int intlevel)
        if(esp32_device->program_counter == 0x4000bfdc){
            if(esp32_device->print_instr) puts("; -- binded unsigned _xtos_set_min_intlevel(int intlevel)");
            esp32_device->instruction = 0x236;
            esp32_instruction_parse(esp32_device);
            esp32_device->ps_intlevel = esp32_register_a_read(esp32_device, 2);
            esp32_device->instruction = 0x000090;
        }

        // ets_printf
        if(esp32_device->program_counter == 0x40007d54){
            if(esp32_device->print_instr) puts("; -- binded ets_printf");
            esp32_device->instruction = 0x136;
            esp32_instruction_parse(esp32_device);

            fputs((char*) &esp32_device->memory[esp32_register_a_read(esp32_device, 2)], stdout);
            esp32_device->instruction = 0x000090;
            printf("\x1b[34m");
        }

        // memcpy
        if(esp32_device->program_counter == 0x4000c2c8){
            if(esp32_device->print_instr) puts("; -- binded memcpy");
            esp32_device->instruction = 0x136;
            esp32_instruction_parse(esp32_device);

            for(int i = 0; i < 16; i++){
                printf("%i %#01x\n", i, esp32_register_a_read(esp32_device, i));
            }
            memcpy(
                &esp32_device->memory[esp32_memory_paddr(esp32_register_a_read(esp32_device, 2))],
                &esp32_device->memory[esp32_memory_paddr(esp32_register_a_read(esp32_device, 3))],
                esp32_register_a_read(esp32_device, 4)
            );
            exit(0);

            esp32_device->instruction = 0x000090;
            printf("\x1b[34m");
        }
        
        if(esp32_instruction_parse(esp32_device)){
            esp32_device->special[ESP32_REG_CCOUNT] += 1;
            oks++;
        }else{
            puts("\x1b[31m");
            std::cout << "\nERROR: Operation not supported" << std::endl;
            std::cout << "Got " << oks << " OKS!" << std::endl << std::endl;
            std::cout << "PC    : 0x" << std::hex << esp32_device->program_counter << std::endl;
            std::cout << "INSTR : 0x" << std::hex << esp32_device->instruction << std::endl;
            std::cout << "        24   20   16   12    8    4    0" << std::endl;
            std::cout << "        |    |    |    |    |    |    |" << std::endl;
            std::cout << "        "
                    << " " << std::bitset<4>((esp32_device->instruction >> 20) &0xf)
                    << " " << std::bitset<4>((esp32_device->instruction >> 16) &0xf)
                    << " " << std::bitset<4>((esp32_device->instruction >> 12) &0xf)
                    << " " << std::bitset<4>((esp32_device->instruction >>  8) &0xf)
                    << " " << std::bitset<4>((esp32_device->instruction >>  4) &0xf)
                    << " " << std::bitset<4>((esp32_device->instruction >>  0) &0xf) << std::endl;
            exit(0);
            break;
        }

    }
}