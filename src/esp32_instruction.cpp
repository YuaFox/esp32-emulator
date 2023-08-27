#include "esp32_instruction.h"
#include <iostream>

esp32_instruction_t* instruction_base = new esp32_instruction_t;

void esp32_instruction_register(void (*pFunc)(esp32_device_t*), esp32_instruction_t* instruction) {
    instruction->pFunc = pFunc;
}

template<typename ... Strings>
void esp32_instruction_register(void (*pFunc)(esp32_device_t*), esp32_instruction_t* instruction, uint8_t ivalue, uint32_t imask, uint8_t shift, const Strings&... rest) {
    if(instruction->sub[ivalue] == nullptr){
        instruction->sub[ivalue] = new esp32_instruction_t;
        instruction->mask = imask;
        instruction->shift = shift;
    }

    esp32_instruction_register(pFunc, instruction->sub[ivalue],rest...);
}

template<typename ... Strings>
void esp32_instruction_register(void (*pFunc)(esp32_device_t*), uint8_t ivalue, uint32_t imask, uint8_t shift, const Strings&... rest) {
    esp32_instruction_register(pFunc,instruction_base,ivalue,imask,shift,rest...);
}

uint16_t endian_swap_16(uint16_t n){
    return ((n & 0xff00) >> 8) | ((n & 0x00ff) << 8);
}

/* -------- INSTRUCTIONS --------- */
void esp32_instruction_X(esp32_device_t device){

}

void esp32_instruction_ILL(esp32_device_t* device){
    puts("\x1b[31mILLEGAL INSTRUCTION");
    exit(0);
}

void esp32_instruction_RETW(esp32_device_t* device){
    uint16_t n = esp32_register_a_read(device, 0) >> 30;
    if(n == 0){
        puts("\x1b[31mWRONG RETW");
        exit(0);
    }
    device->ps_owb = device->special[ESP32_REG_WINDOWBASE];
    device->program_counter = (device->program_counter >> 30 << 30) | (esp32_register_a_read(device, 0) & 0x3fffffff);
    device->special[ESP32_REG_WINDOWBASE] = device->special[ESP32_REG_WINDOWBASE] - n;

    if(device->special[ESP32_REG_WINDOWSTART] >> device->special[ESP32_REG_WINDOWBASE] & 1 != 0){
        device->special[ESP32_REG_WINDOWSTART] &= ~(1UL << device->ps_owb);
    }else{
        puts("\x1b[31mRETW UNDERFLOW EXCEPTION");
        exit(0);
    }
    printf("RETW              ; PC = %#01x\n", device->program_counter);

}

void esp32_instruction_CALLX8(esp32_device_t* device){
    device->ps_callinc = 0b10;
    device->temp = esp32_register_a_read(device, device->instruction >> 8 &0xf);
    esp32_register_a_write(device, 8, ((device->program_counter + 3) & 0x3fffffff) | (0b10 << 30));
    device->program_counter = device->temp;
    if(device->print_instr) printf("CALLX8 a%i         ; PC = %#01x\n", device->instruction >> 8 &0xf,  device->program_counter);
}

void esp32_instruction_MEMW(esp32_device_t* device){
    if(device->print_instr) printf("MEMW\n");
    device->program_counter += 3;
}

void esp32_instruction_OR(esp32_device_t* device){
    if(device->print_instr) printf("OR\n");
    esp32_register_a_write(
        device,
        device->instruction >> 12 & 0x0f,
        esp32_register_a_read(device, device->instruction >> 8 & 0x0f) |  esp32_register_a_read(device, device->instruction >> 4 & 0x0f)
    );
    device->program_counter += 3;
}

void esp32_instruction_RSIL(esp32_device_t* device){
    if(device->print_instr) printf("RSIL [NOT]\n");
    device->program_counter += 3;
}

void esp32_instruction_WSR(esp32_device_t* device){
    device->special[device->instruction >> 8 & 0xff] = esp32_register_a_read(device, device->instruction >> 4 & 0x0f);
    device->program_counter += 3;

    if(device->print_instr) printf("WSR s%i, a%i      ; a = %02x\n", device->instruction >> 8 & 0xff, device->instruction >> 4 & 0x0f ,esp32_register_a_read(device, device->instruction >> 4 & 0x0f));
}

void esp32_instruction_L32R(esp32_device_t* device){
    device->vAddr = ((device->instruction >> 8 << 2)) | (0b11111111111111 << 18);

    if(device->special[ESP32_REG_LITBASE]&0x1 == 1){
        device->vAddr += (int32_t)(device->special[ESP32_REG_LITBASE] >> 12 << 12);
    }else{
        device->vAddr += (int32_t)(device->program_counter + 3 >> 2 << 2);
    }
    
    esp32_register_a_write(
        device,
        device->instruction >> 4 & 0x0f,
        esp32_memory_load32(device)
    );
    device->program_counter += 3;

    if(device->print_instr) printf("L32R a%i, %i    ; mem[%#010x] = (%#08x)\n", device->instruction >> 4 & 0x0f, device->instruction >> 8, device->vAddr, esp32_register_a_read(device, device->instruction >> 4 & 0x0f));
}

void esp32_instruction_CALL8(esp32_device_t* device){
    if(device->print_instr) printf("CALL8\n");
    device->temp = device->instruction >> 6;
    if(device->temp >> 17 & 1 == 1){
        device->temp |= 0xfff << 18;
    }
    device->ps_callinc = 0b10;
    esp32_register_a_write(device, 4, (device->program_counter + 3) & 0x3ff | 0x8000);
    device->temp++;
    device->program_counter = (device->program_counter >> 2 ) + device->temp << 2;
}

void esp32_instruction_J(esp32_device_t* device){
    if(device->print_instr) printf("J\n");
    device->temp = device->instruction >> 6;
    if(device->temp >> 17 & 1 == 1){
        device->temp |= 0xfffc << 16;
    }
    device->temp += 4;
    device->program_counter += device->temp;
}

void esp32_instruction_ENTRY(esp32_device_t* device){
    device->temp = (device->instruction >> 8) & 0xf;
    if(device->print_instr) printf("ENTRY %i %#01x\n", device->temp, device->instruction >> 12 << 3);
    if(device->temp > 3){
        std::cout << "ENTRY ERROR: AS > 3" << std::endl;
        exit(0);
    }
    esp32_register_a_write(
        device,
        (device->ps_callinc << 2) + device->temp,
        esp32_register_a_read(device, device->temp) - (device->instruction >> 12 << 3)
    );
    device->special[ESP32_REG_WINDOWBASE] = device->special[ESP32_REG_WINDOWBASE] + device->ps_callinc;
    device->special[ESP32_REG_WINDOWSTART] |= 1 << device->special[ESP32_REG_WINDOWBASE];
    device->program_counter += 3;
}

void esp32_instruction_BLTUI(esp32_device_t* device){
    device->temp = device->instruction >> 12 & 0xf;
    switch (device->temp) {
        case  0: device->temp = 3268; break;
        case  1: device->temp = 65536; break;
        case  2: device->temp = 2; break;
        case  3: device->temp = 3; break;
        case  4: device->temp = 4; break;
        case  5: device->temp = 5; break;
        case  6: device->temp = 6; break;
        case  7: device->temp = 7; break;
        case  8: device->temp = 8; break;
        case  9: device->temp = 10; break;
        case 10: device->temp = 12; break;
        case 11: device->temp = 16; break;
        case 12: device->temp = 32; break;
        case 13: device->temp = 64; break;
        case 14: device->temp = 128; break;
        case 15: device->temp = 256; break;
    }

    device->vAddr = esp32_register_a_read(device, device->instruction >> 8 & 0xf);
    
    if(device->print_instr) printf("BLTUI a%i, %i, %i   ; [ %i < %i  ]", device->instruction >> 8 & 0xf, device->instruction >> 16, device->temp, esp32_memory_load32(device), device->temp);
    
    if(esp32_memory_load32(device) < device->temp){
        device->temp = device->instruction >> 16;
        if(device->temp >> 7 & 1 == 1){
            device->temp |= 0xffffff << 8;
        }
        device->program_counter += device->temp + 4;
        if(device->print_instr) printf(" jump to PC = %#010x\n", device->program_counter);
    }else{
        device->program_counter += 3;
        if(device->print_instr) puts(" skip\n");
    }
}

void esp32_instruction_L32IN(esp32_device_t* device){
    if(device->print_instr) printf("L32IN\n");
    // TODO: Check
    device->temp = esp32_register_a_read(
        device,
        (device->instruction >> 8) & 0x0f
    );
    device->vAddr = device->temp + (((device->instruction >> 12) & 0x0f) << 2);
    esp32_register_a_write(
        device,
        (device->instruction >> 4) & 0x0f,
        device->vAddr
    );
    device->program_counter += 2;
}

void esp32_instruction_S32IN(esp32_device_t* device){
    device->vAddr = ((device->instruction >> 12 & 0x0f) << 2) + esp32_register_a_read(device, device->instruction >> 8 & 0xf);
    esp32_memory_load32(device);
    esp32_register_a_write(device, device->instruction >> 4 & 0xf, esp32_memory_load32(device));
    device->program_counter += 2;
    if(device->print_instr) printf("S32IN\n");
}



void esp32_instruction_MOVIN(esp32_device_t* device){
    device->temp = ((device->instruction >> 12) & 0x0f) | ((device->instruction >> 4) & 0x0f);
    esp32_register_a_write(
        device,
        (device->instruction >> 8) & 0x0f,
        device->temp 
    );
    device->program_counter += 2;
    if(device->print_instr) printf("MOVI.N a%i, %#08x\n",  (device->instruction >> 8) & 0x0f, device->temp);
}

void esp32_instruction_MOVN(esp32_device_t* device){
     esp32_register_a_write(
        device,
        (device->instruction >> 4) & 0x0f,
        esp32_register_a_read(device, (device->instruction >> 8) & 0x0f)
    );
    if(device->print_instr) printf("MOV.N a%i, a%i\n", (device->instruction >> 4) & 0x0f, (device->instruction >> 8) & 0x0f);
    device->program_counter += 2;
}

void esp32_instruction_init(){
    esp32_instruction_register(
        esp32_instruction_ILL,
        0b0000,     0xf,    0,
        0b0000,     0xf,   16,
        0b0000,     0xf,   20,
        0b0000,     0xf,   12,
        0b00,       0b11,   6,
        0b00,       0b11,   4,
        0b00,       0xf,    8
    );
    esp32_instruction_register(
        esp32_instruction_RETW,
        0b0000,     0xf,    0,
        0b0000,     0xf,   16,
        0b0000,     0xf,   20,
        0b0000,     0xf,   12,
        0b10,       0b11,   6,
        0b01,       0b11,   4
    );
    esp32_instruction_register(
        esp32_instruction_CALLX8,
        0b0000,     0xf,    0,
        0b0000,     0xf,   16,
        0b0000,     0xf,   20,
        0b0000,     0xf,   12,
        0b11,       0b11,   6,
        0b10,       0b11,   4
    );
    esp32_instruction_register(
        esp32_instruction_MEMW,
        0b0000,     0xf,    0,
        0b0000,     0xf,   16,
        0b0000,     0xf,   20,
        0b0010,     0xf,   12,
        0b1100,     0xf,    4
    );
    esp32_instruction_register(
        esp32_instruction_OR,
        0b0000,     0xf,  0,
        0b0000,     0xf,  16,
        0b0010,     0xf,  20
    );
    esp32_instruction_register(
        esp32_instruction_RSIL,
        0b0000,     0xf,  0,
        0b0000,     0xf,  16,
        0b0000,     0xf,  20,
        0b0110,     0xf,  12
    );
    esp32_instruction_register(
        esp32_instruction_WSR,
        0b0000,     0xf,  0,
        0b0011,     0xf,  16,
        0b0001,     0xf,  20
    );
    esp32_instruction_register(
        esp32_instruction_L32R,
        0b0001,     0xf,  0
    );
    esp32_instruction_register(
        esp32_instruction_CALL8,
        0b0101,     0xf,    0,
        0b10,       0b11,   4
    );
    esp32_instruction_register(
        esp32_instruction_J,
        0b0110,     0xf,    0,
        0b00,       0b11,   4
    );
    esp32_instruction_register(
        esp32_instruction_ENTRY,
        0b0110,     0xf,    0,
        0b11,       0b11,   4,
        0b00,       0b11,   6
    );
    esp32_instruction_register(
        esp32_instruction_BLTUI,
        0b0110,     0xf,    0,
        0b11,       0b11,   4,
        0b10,       0b11,   6
    );

    esp32_instruction_register(
        esp32_instruction_L32IN,
        0b1000,     0xf,    0
    );
    esp32_instruction_register(
        esp32_instruction_S32IN,
        0b1001,     0xf,    0
    );
    esp32_instruction_register(
        esp32_instruction_MOVIN,
        0b1100,     0xf,  0,
           0b0,     0x1,  7
    );
    esp32_instruction_register(
        esp32_instruction_MOVN,
        0b1101,     0xf,  0,
        0b0000,     0xf,  12
    );
}