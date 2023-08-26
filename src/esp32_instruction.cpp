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

void esp32_instruction_CALLX8(uint8_t* memory, esp32_status_t* status){
    status->program_counter = esp32_register_a_read(status, status->instruction >> 8 &0xf);
    status->ps_callinc = 0b10;
    esp32_register_a_write(status, 4, (status->program_counter + 3) & 0x3ff | 0b10 << 30);
    if(status->print_instr) printf("CALLX8 a%i         ; PC = %#01x\n", status->instruction >> 8 &0xf, esp32_register_a_read(status, status->instruction >> 8 &0xf));
}

void esp32_instruction_OR(uint8_t* memory, esp32_status_t* status){
    if(status->print_instr) printf("OR\n");
    esp32_register_a_write(
        status,
        status->instruction >> 12 & 0x0f,
        esp32_register_a_read(status, status->instruction >> 8 & 0x0f) |  esp32_register_a_read(status, status->instruction >> 4 & 0x0f)
    );
    status->program_counter += 3;
}

void esp32_instruction_RSIL(uint8_t* memory, esp32_status_t* status){
    if(status->print_instr) printf("RSIL [NOT]\n");
    status->program_counter += 3;
}

void esp32_instruction_WSR(uint8_t* memory, esp32_status_t* status){
    status->special[status->instruction >> 8 & 0xff] = esp32_register_a_read(status, status->instruction >> 4 & 0x0f);
    status->program_counter += 3;

    if(status->print_instr) printf("WSR s%i, a%i      ; a = %02x\n", status->instruction >> 8 & 0xff, status->instruction >> 4 & 0x0f ,esp32_register_a_read(status, status->instruction >> 4 & 0x0f));
}

void esp32_instruction_L32R(uint8_t* memory, esp32_status_t* status){
    status->vAddr = ((status->instruction >> 8 << 2)) | (0b11111111111111 << 18);

    if(status->special[ESP32_REG_LITBASE]&0x1 == 1){
        status->vAddr += (int32_t)(status->special[ESP32_REG_LITBASE] >> 12 << 12);
    }else{
        status->vAddr += (int32_t)(status->program_counter + 3 >> 2 << 2);
    }
    
    esp32_register_a_write(
        status,
        status->instruction >> 4 & 0x0f,
        memory[status->vAddr+3] << 24 | memory[status->vAddr+2] << 16 | memory[status->vAddr+1] << 8 | memory[status->vAddr]
    );
    status->program_counter += 3;

    if(status->print_instr) printf("L32R a%i, %i    ; mem[%#010x] = (%02x %02x %02x %02x)\n", status->instruction >> 4 & 0x0f, status->instruction >> 8, status->vAddr, memory[status->vAddr+3], memory[status->vAddr+2], memory[status->vAddr+1], memory[status->vAddr+0]);
}

void esp32_instruction_CALL8(uint8_t* memory, esp32_status_t* status){
    if(status->print_instr) printf("CALL8\n");
    status->temp = status->instruction >> 6;
    if(status->temp >> 17 & 1 == 1){
        status->temp |= 0xfff << 18;
    }
    status->ps_callinc = 0b10;
    esp32_register_a_write(status, 4, (status->program_counter + 3) & 0x3ff | 0x8000);
    status->temp++;
    status->program_counter = (status->program_counter >> 2 ) + status->temp << 2;
}

void esp32_instruction_J(uint8_t* memory, esp32_status_t* status){
    if(status->print_instr) printf("J\n");
    status->temp = status->instruction >> 6;
    if(status->temp >> 17 & 1 == 1){
        status->temp |= 0xfffc << 16;
    }
    status->temp += 4;
    status->program_counter += status->temp;
}

void esp32_instruction_ENTRY(uint8_t* memory, esp32_status_t* status){
    if(status->print_instr) printf("ENTRY\n");
    status->temp = (status->instruction >> 8) & 4;
    if(status->temp > 3){
        std::cout << "ENTRY ERROR: AS > 3" << std::endl;
        exit(0);
    }
    esp32_register_a_write(
        status,
        (status->ps_callinc << 2) + status->temp,
        esp32_register_a_read(status, status->temp) - (status->instruction >> 12 << 3)
    );
    status->special[ESP32_REG_WINDOWBASE] = status->special[ESP32_REG_WINDOWBASE] + status->ps_callinc;
    status->special[ESP32_REG_WINDOWSTART] = 1 << status->special[ESP32_REG_WINDOWBASE];
    status->program_counter += 3;
}

void esp32_instruction_L32IN(uint8_t* memory, esp32_status_t* status){
    if(status->print_instr) printf("L32IN\n");
    // TODO: Check
    status->temp = esp32_register_a_read(
        status,
        (status->instruction >> 8) & 0x0f
    );
    status->vAddr = status->temp + (((status->instruction >> 12) & 0x0f) << 2);
    esp32_register_a_write(
        status,
        (status->instruction >> 4) & 0x0f,
        status->vAddr
    );
    status->program_counter += 2;
}

void esp32_instruction_MOVIN(uint8_t* memory, esp32_status_t* status){
    if(status->print_instr) printf("MOVI.N\n");
    
    status->temp = ((status->instruction >> 12) & 0x0f) | ((status->instruction >> 4) & 0x0f);
    esp32_register_a_write(
        status,
        (status->instruction >> 8) & 0x0f,
        status->temp 
    );

    status->program_counter += 2;
}

void esp32_instruction_init(){
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
        esp32_instruction_L32IN,
        0b1000,     0xf,    0
    );
    esp32_instruction_register(
        esp32_instruction_MOVIN,
        0b1100,     0xf,  0,
        0b0000,     0xf,  4
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