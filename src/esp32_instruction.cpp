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

void esp32_instruction_L32R(uint8_t* memory, esp32_status_t* status){
    // TODO: LITBASE
    status->vAddr = ((status->instruction >> 8) << 2) | (0b11111111111111 << 18);
    status->vAddr += (status->program_counter + 3 >> 2 << 2);
    status->vAddr -= 0xccff00; // TODO: wtf?
    status->a[status->instruction >> 4 &0x0f] = status->vAddr;
    
    status->program_counter += 3;
}

void esp32_instruction_CALL8(uint8_t* memory, esp32_status_t* status){
    status->program_counter = (status->program_counter>>2<<2) + (int16_t)((status->instruction >> 6)+1 << 2);
}

void esp32_instruction_ENTRY(uint8_t* memory, esp32_status_t* status){
    status->program_counter += 3;
}

void esp32_instruction_init(){
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
        esp32_instruction_ENTRY,
        0b0110,     0xf,    0,
        0b11,       0b11,   4,
        0b00,       0b11,   6
    );

    /*
    esp32_instruction_t* entry = new esp32_instruction_t;
    instruction_base->sub[0b0001] = new esp32_instruction_t;
    instruction_base->sub[0b0001]->name = "L32R";
    instruction_base->sub[0b0001]->pFunc = esp32_instruction_L32R;

    instruction_base->sub[0b0110] = new esp32_instruction_t;
    instruction_base->sub[0b0110]->sub[0b11] = new esp32_instruction_t;
    instruction_base->sub[0b0110]->sub[0b11]->sub[0b00] = new esp32_instruction_t;
    instruction_base->sub[0b0110]->sub[0b11]->sub[0b00]->name = "ENTRY";
    instruction_base->sub[0b0110]->sub[0b11]->sub[0b00]->pFunc = esp32_instruction_ENTRY;
    */
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