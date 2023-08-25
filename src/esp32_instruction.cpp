#include "esp32_instruction.h"
#include <iostream>

esp32_instruction_t* instruction_base = new esp32_instruction_t;

void esp32_instruction_register(void (*pFunc)(uint8_t*, esp32_status_t*), esp32_instruction_t* instruction) {
    instruction->pFunc = pFunc;
}

template<typename ... Strings>
void esp32_instruction_register(void (*pFunc)(uint8_t*, esp32_status_t*), esp32_instruction_t* instruction, uint8_t ivalue, uint32_t imask, uint8_t shift, const Strings&... rest) {
    if(instruction->sub[ivalue] == nullptr){
        instruction->sub[ivalue] = new esp32_instruction_t;
        instruction->mask = imask;
        instruction->shift = shift;
    }

    esp32_instruction_register(pFunc, instruction->sub[ivalue],rest...);
}

template<typename ... Strings>
void esp32_instruction_register(void (*pFunc)(uint8_t*, esp32_status_t*), uint8_t ivalue, uint32_t imask, uint8_t shift, const Strings&... rest) {
    esp32_instruction_register(pFunc,instruction_base,ivalue,imask,shift,rest...);
}

uint16_t endian_swap_16(uint16_t n){
    return ((n & 0xff00) >> 8) | ((n & 0x00ff) << 8);
}

/* -------- INSTRUCTIONS --------- */
void esp32_instruction_X(uint8_t* memory, esp32_status_t* status){

}

void esp32_instruction_OR(uint8_t* memory, esp32_status_t* status){
    status->a[(status->instruction >> 12) & 0x0f] =
        status->a[(status->instruction >> 8) & 0x0f] |
        status->a[(status->instruction >> 4) & 0x0f];
    status->program_counter += 3;
}

void esp32_instruction_RSIL(uint8_t* memory, esp32_status_t* status){
    std::cout << "[WARN] RSIL NOT IMPLEMENTED" << std::endl;
    status->program_counter += 3;
}

void esp32_instruction_L32R(uint8_t* memory, esp32_status_t* status){
    // TODO: LITBASE
    std::cout << "[WARN] L32R half implemented" << std::endl;
    status->vAddr = ((status->instruction >> 8) << 2) | (0b11111111111111 << 18);
    status->vAddr += (status->program_counter + 3 >> 2 << 2);
    status->vAddr -= 0xccff00; // TODO: wtf?
    status->a[status->instruction >> 4 &0x0f] = status->vAddr;
    
    status->program_counter += 3;
}

void esp32_instruction_CALL8(uint8_t* memory, esp32_status_t* status){
    std::cout << "[WARN] CALL8 half implemented" << std::endl;
    status->temp = status->instruction >> 6;
    if(status->temp >> 17 & 1 == 1){
        status->temp |= 0xfff << 18;
    }
    status->temp++;
    status->program_counter = (status->program_counter >> 2 ) + status->temp << 2;
}

void esp32_instruction_J(uint8_t* memory, esp32_status_t* status){
    status->temp = status->instruction >> 6;
    if(status->temp >> 17 & 1 == 1){
        status->temp |= 0xfffc << 16;
    }
    status->temp += 4;
    status->program_counter += status->temp;
}

void esp32_instruction_ENTRY(uint8_t* memory, esp32_status_t* status){
    std::cout << "[WARN] ENTRY half implemented" << std::endl;
    status->program_counter += 3;
}

void esp32_instruction_L32IN(uint8_t* memory, esp32_status_t* status){
    // TODO: Check
    status->vAddr = status->a[(status->instruction >> 8) & 0x0f] + (((status->instruction >> 12) & 0x0f) << 2);
    status->a[(status->instruction >> 4) & 0x0f] = status->vAddr;
    status->program_counter += 2;
}

void esp32_instruction_init(){
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
        esp32_instruction_L32IN,
        0b1000,     0xf,    0
    );
}

bool esp32_instruction_parse(uint8_t* memory, esp32_status_t* status, esp32_instruction_t* instruction){
    if(instruction->mask == 0) return false;

    esp32_instruction_t* l = instruction->sub[(status->instruction >> instruction->shift) & instruction->mask];

    if(l == nullptr) return false;

    if(l->pFunc != nullptr){
        l->pFunc(memory, status);
        return true;
    }else{
        return esp32_instruction_parse(memory, status, l);
    }
}

bool esp32_instruction_parse(uint8_t* memory, esp32_status_t* status){
    return esp32_instruction_parse(memory, status, instruction_base);
    
}