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
    device->stacktrace.pop_back();
    device->ps_owb = device->special[ESP32_REG_WINDOWBASE];
    device->program_counter = (device->program_counter >> 30 << 30) | (esp32_register_a_read(device, 0) & 0x3fffffff);
    device->special[ESP32_REG_WINDOWBASE] = device->special[ESP32_REG_WINDOWBASE] - n;

    if(device->special[ESP32_REG_WINDOWSTART] >> device->special[ESP32_REG_WINDOWBASE] & 1 != 0){
        device->special[ESP32_REG_WINDOWSTART] &= ~(1UL << device->ps_owb);
    }else{
        puts("\x1b[31mRETW UNDERFLOW EXCEPTION");
        exit(0);
    }
    if(device->print_instr) printf("RETW              ; PC = %#01x\n", device->program_counter);
    device->call_depth--;

}

void esp32_instruction_CALLX8(esp32_device_t* device){
    device->ps_callinc = 0b10;
    device->temp = esp32_register_a_read(device, device->instruction >> 8 &0xf);
    esp32_register_a_write(device, 8, ((device->program_counter + 3) & 0x3fffffff) | (0b10 << 30));
    device->program_counter = device->temp;
    if(device->print_instr) printf("CALLX8 a%i         ; PC = %#01x\n", device->instruction >> 8 &0xf,  device->program_counter);
}
void esp32_instruction_RSYNC(esp32_device_t* device){
    if(device->print_instr) printf("RSYNC\n");
    device->program_counter += 3;
}


void esp32_instruction_MEMW(esp32_device_t* device){
    if(device->print_instr) printf("MEMW\n");
    device->program_counter += 3;
}

void esp32_instruction_AND(esp32_device_t* device){
    if(device->print_instr) printf("AND a%i, a%i, a%i\n", device->instruction >> 12 & 0xf, device->instruction >> 8 & 0xf,device->instruction >> 4 & 0xf);
    esp32_register_a_write(
        device,
        device->instruction >> 12 & 0xf,
        esp32_register_a_read(device, device->instruction >> 8 & 0x0f) & esp32_register_a_read(device, device->instruction >> 4 & 0x0f)
    );
    device->program_counter += 3;
}

void esp32_instruction_OR(esp32_device_t* device){
    if(device->print_instr) printf("OR a%i, a%i, a%i\n", device->instruction >> 12 & 0xf, device->instruction >> 8 & 0xf,device->instruction >> 4 & 0xf);
    esp32_register_a_write(
        device,
        device->instruction >> 12 & 0x0f,
        esp32_register_a_read(device, device->instruction >> 8 & 0x0f) | esp32_register_a_read(device, device->instruction >> 4 & 0x0f)
    );
    device->program_counter += 3;
}

void esp32_instruction_MOVSP(esp32_device_t* device){
    // TODO: Reimplement exception
    if(device->print_instr) printf("MOVSP a%i, a%i, a%i\n", device->instruction >> 4 & 0xf, device->instruction >> 8 & 0xf);
    esp32_register_a_write(
        device,
        device->instruction >> 4 & 0xf,
        esp32_register_a_read(device, device->instruction >> 8 & 0xf)
    );
    device->program_counter += 3;
}



void esp32_instruction_ADD(esp32_device_t* device){
    if(device->print_instr) printf("ADD a%i, a%i, a%i\n", device->instruction >> 12 & 0xf, device->instruction >> 8 & 0xf,device->instruction >> 4 & 0xf);
    esp32_register_a_write(
        device,
        device->instruction >> 12 & 0x0f,
        esp32_register_a_read(device, device->instruction >> 8 & 0x0f) + esp32_register_a_read(device, device->instruction >> 4 & 0x0f)
    );
    device->program_counter += 3;
}



void esp32_instruction_RSIL(esp32_device_t* device){
    if(device->print_instr) printf("RSIL [NOT] !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
    device->program_counter += 3;
}

void esp32_instruction_ADDX2(esp32_device_t* device){
    esp32_register_a_write(
        device,
        device->instruction >> 12 & 0xf,
        (esp32_register_a_read(device, device->instruction >> 8 & 0xf) << 1) + esp32_register_a_read(device, device->instruction >> 4 & 0xf)
    );
    if(device->print_instr) printf("ADDX4 a%i, a%i, a%i  ; a = %#08x\n", device->instruction >> 12 & 0xf, device->instruction >> 8 & 0xf, device->instruction >> 4 & 0xf,  esp32_register_a_read(device, device->instruction >> 12 & 0xf));
    device->program_counter += 3;
}

void esp32_instruction_ADDX4(esp32_device_t* device){
    esp32_register_a_write(
        device,
        device->instruction >> 12 & 0xf,
        (esp32_register_a_read(device, device->instruction >> 8 & 0xf) << 2) + esp32_register_a_read(device, device->instruction >> 4 & 0xf)
    );
    if(device->print_instr) printf("ADDX4 a%i, a%i, a%i  ; a = %#08x\n", device->instruction >> 12 & 0xf, device->instruction >> 8 & 0xf, device->instruction >> 4 & 0xf,  esp32_register_a_read(device, device->instruction >> 12 & 0xf));
    device->program_counter += 3;
}

void esp32_instruction_ADDX8(esp32_device_t* device){
    esp32_register_a_write(
        device,
        device->instruction >> 12 & 0xf,
        (esp32_register_a_read(device, device->instruction >> 8 & 0xf) << 3) + esp32_register_a_read(device, device->instruction >> 4 & 0xf)
    );
    if(device->print_instr) printf("ADDX8 a%i, a%i, a%i  ; a = %#08x\n", device->instruction >> 12 & 0xf, device->instruction >> 8 & 0xf, device->instruction >> 4 & 0xf,  esp32_register_a_read(device, device->instruction >> 12 & 0xf));
    device->program_counter += 3;
}

void esp32_instruction_SUB(esp32_device_t* device){
    esp32_register_a_write(
        device,
        device->instruction >> 12 & 0xf,
        esp32_register_a_read(device, device->instruction >> 8 & 0xf) - esp32_register_a_read(device, device->instruction >> 4 & 0xf)
    );
    if(device->print_instr) printf("SUB a%i, a%i, a%i    ; a = %#08x\n", device->instruction >> 12 & 0xf, device->instruction >> 8 & 0xf, device->instruction >> 4 & 0xf,  esp32_register_a_read(device, device->instruction >> 12 & 0xf));
    device->program_counter += 3;
}



void esp32_instruction_SLLI(esp32_device_t* device){
    device->temp =  ((device->instruction >> 16 & 1) << 4) | (device->instruction >> 4 & 0xf);
    esp32_register_a_write(
        device,
        device->instruction >> 12 & 0xf,
        esp32_register_a_read(device, device->instruction >> 8 & 0xf) << (32-device->temp)
    );
    if(device->print_instr) printf("SLLI a%i, a%i, %i   ; a = %#08x\n", device->instruction >> 12 & 0xf, device->instruction >> 8 & 0xf, device->temp, esp32_register_a_read(device, device->instruction >> 12 & 0xf));
    device->program_counter += 3;
}

void esp32_instruction_SRAI(esp32_device_t* device){
    device->temp =  ((device->instruction >> 20 & 1) << 4) | (device->instruction >> 8 & 0xf);
    uint64_t temp = esp32_register_a_read(device, device->instruction >> 4 & 0xf);
    if(temp >> 31 & 1 == 1){
        temp |= 0xffffffff << 32;
    }
    temp = temp >> device->temp;

    esp32_register_a_write(
        device,
        device->instruction >> 12 & 0xf,
        temp
    );
    if(device->print_instr) printf("SRAI a%i, a%i, %i   ; a = %#08x\n", device->instruction >> 12 & 0xf, device->instruction >> 4 & 0xf, device->temp, esp32_register_a_read(device, device->instruction >> 12 & 0xf));
    device->program_counter += 3;
}

void esp32_instruction_SRLI(esp32_device_t* device){
    esp32_register_a_write(
        device,
        device->instruction >> 12 & 0xf,
        esp32_register_a_read(device, device->instruction >> 4 & 0xf) >> (device->instruction >> 8 & 0xf)
    );
    if(device->print_instr) printf("SRLI a%i, a%i, %i   ; a = %#08x\n", device->instruction >> 12 & 0xf, device->instruction >> 4 & 0xf, device->instruction >> 8 & 0xf, esp32_register_a_read(device, device->instruction >> 12 & 0xf));
    device->program_counter += 3;
}

void esp32_instruction_SRC(esp32_device_t* device){
    device->temp = esp32_register_a_read(device, device->instruction >> 12 & 0xf) & 0b111111;
    uint64_t temp = esp32_register_a_read(device, device->instruction >> 8 & 0xf) << 32 | esp32_register_a_read(device, device->instruction >> 4 & 0xf);

    esp32_register_a_write(
        device,
        device->instruction >> 12 & 0xf,
        temp >> device->temp
    );
    if(device->print_instr) printf("SRC a%i, a%i, a%i   ; a = %#08x\n", device->instruction >> 12 & 0xf, device->instruction >> 8 & 0xf, device->instruction >> 4 & 0xf, esp32_register_a_read(device, device->instruction >> 12 & 0xf));
    device->program_counter += 3;
}

void esp32_instruction_MULL(esp32_device_t* device){
    if(device->print_instr) printf("MULL a%i, a%i, a%i   ; ", device->instruction >> 12 & 0xf, device->instruction >> 8 & 0xf, device->instruction >> 4 & 0xf);
    esp32_register_a_write(
        device,
        device->instruction >> 12 & 0xf,
        (esp32_register_a_read(device, device->instruction >> 8 & 0xf) << 1 >> 1) * (esp32_register_a_read(device, device->instruction >> 4 & 0xf) << 1 >> 1)
    );
    if(device->print_instr) printf("a = %#08x\n", esp32_register_a_read(device, device->instruction >> 12 & 0xf));
    device->program_counter += 3;
}

void esp32_instruction_MULLUH(esp32_device_t* device){
    if(device->print_instr) printf("MULLUH a%i, a%i, a%i   ; ", device->instruction >> 12 & 0xf, device->instruction >> 8 & 0xf, device->instruction >> 4 & 0xf);
    uint64_t temp = (uint64_t) esp32_register_a_read(device, device->instruction >> 8 & 0xf) * (uint64_t) esp32_register_a_read(device, device->instruction >> 4 & 0xf);
    esp32_register_a_write(
        device,
        device->instruction >> 12 & 0xf,
        temp >> 32
    );
    if(device->print_instr) printf("a = %#08x\n", esp32_register_a_read(device, device->instruction >> 12 & 0xf));
    device->program_counter += 3;
}

void esp32_instruction_QUOU(esp32_device_t* device){
    if(device->print_instr) printf("QUOU a%i, a%i, a%i   ; ", device->instruction >> 12 & 0xf, device->instruction >> 8 & 0xf, device->instruction >> 4 & 0xf);
    if(esp32_register_a_read(device, device->instruction >> 4 & 0xf) == 0){
        printf("Error IntegerDivideByZero\n");
        exit(0);
    }

    esp32_register_a_write(
        device,
        device->instruction >> 12 & 0xf,
        (esp32_register_a_read(device, device->instruction >> 8 & 0xf) << 1 >> 1) / (esp32_register_a_read(device, device->instruction >> 4 & 0xf) << 1 >> 1)
    );
    if(device->print_instr) printf("a = %#08x\n", esp32_register_a_read(device, device->instruction >> 12 & 0xf));
    device->program_counter += 3;
}


void esp32_instruction_WSR(esp32_device_t* device){
    device->special[device->instruction >> 8 & 0xff] = esp32_register_a_read(device, device->instruction >> 4 & 0x0f);
    device->program_counter += 3;
    if(device->print_instr) printf("WSR s%i, a%i      ; a = %02x\n", device->instruction >> 8 & 0xff, device->instruction >> 4 & 0x0f ,esp32_register_a_read(device, device->instruction >> 4 & 0x0f));
}

void esp32_instruction_RSR(esp32_device_t* device){
    device->temp = device->special[device->instruction >> 8 & 0xff];
    esp32_register_a_write(device, device->instruction >> 4 & 0x0f, device->temp);
    device->program_counter += 3;
    if(device->print_instr) printf("RSR s%i, a%i  ; a = %#08x\n", device->instruction >> 8 & 0xff, device->instruction >> 4 & 0x0f ,device->temp);
}

void esp32_instruction_MINU(esp32_device_t* device){
    if(esp32_register_a_read(device, device->instruction >> 8 & 0xf) < esp32_register_a_read(device, device->instruction >> 4 & 0xf)){
        esp32_register_a_write(device, device->instruction >> 12 & 0x0f, esp32_register_a_read(device, device->instruction >> 8 & 0xf));
    }else{
        esp32_register_a_write(device, device->instruction >> 12 & 0x0f, esp32_register_a_read(device, device->instruction >> 4 & 0xf));
    }
    device->program_counter += 3;
    if(device->print_instr) printf("MINU a%i, a%i, a%i; a = %#08x\n", device->instruction >> 12 & 0xf, device->instruction >> 8 & 0xf, device->instruction >> 4 & 0xf ,device->temp);
}




void esp32_instruction_MOVEQZ(esp32_device_t* device){
    if(device->print_instr) printf("MOVEQZ a%i, a%i, a%i      ; ", device->instruction >> 12 & 0xf, device->instruction >> 8 & 0xf ,device->instruction >> 4 & 0xf);
    if(esp32_register_a_read(device, device->instruction >> 4 & 0xf) == 0){
        esp32_register_a_write(device, device->instruction >> 12 & 0xf, esp32_register_a_read(device, device->instruction >> 8 & 0xf));
        if(device->print_instr) printf("a = %#08x\n", esp32_register_a_read(device, device->instruction >> 12 & 0xf));
    }else{
        if(device->print_instr) puts("skip");
    }
    device->program_counter += 3;
}

void esp32_instruction_MOVENEZ(esp32_device_t* device){
    if(device->print_instr) printf("MOVENEZ a%i, a%i, a%i      ; ", device->instruction >> 12 & 0xf, device->instruction >> 8 & 0xf ,device->instruction >> 4 & 0xf);
    if(esp32_register_a_read(device, device->instruction >> 4 & 0xf) != 0){
        esp32_register_a_write(device, device->instruction >> 12 & 0xf, esp32_register_a_read(device, device->instruction >> 8 & 0xf));
        if(device->print_instr) printf("a = %#08x\n", esp32_register_a_read(device, device->instruction >> 12 & 0xf));
    }else{
        if(device->print_instr) puts("skip");
    }
    device->program_counter += 3;
}

void esp32_instruction_EXTUI(esp32_device_t* device){
    device->mask = (1 << (device->instruction >> 20)+1) - 1;
    device->temp = ((device->instruction >> 16 & 1) << 4) | (device->instruction >> 8 & 0xf); // shiftimm
    esp32_register_a_write(
        device,
        device->instruction >> 12 & 0xf,
        esp32_register_a_read(device, device->instruction >> 4 & 0xf) >> device->temp & device->mask
    );
    device->program_counter += 3;
    if(device->print_instr) printf("EXTUI a%i, a%i, %i, %#01x     ; a = %#08x\n", device->instruction >> 12 & 0xf, device->instruction >> 4 & 0xf, device->temp, device->mask, esp32_register_a_read(device, device->instruction >> 4 & 0xf) >> device->temp & device->mask);
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

    if(device->print_instr) printf("L32R a%i, %i    ; mem[%#010x] = %#08x\n", device->instruction >> 4 & 0x0f, device->instruction >> 8, device->vAddr, esp32_register_a_read(device, device->instruction >> 4 & 0x0f));
}

void esp32_instruction_L8UI(esp32_device_t* device){
    if(device->print_instr) printf("L8UI a%i, a%i, %#01x ", (device->instruction >> 4) & 0x0f, (device->instruction >> 8) & 0x0f, device->instruction >> 16);
    device->vAddr = esp32_register_a_read(device, device->instruction >> 8 & 0x0f) + (device->instruction >> 16);
    esp32_register_a_write(device, device->instruction >> 4 & 0xf, esp32_memory_load8(device));
    device->program_counter += 3;
    if(device->print_instr) printf("   ; mem[%#01x] = %#01x\n", device->vAddr, esp32_memory_load8(device));
}

void esp32_instruction_L16UI(esp32_device_t* device){
    if(device->print_instr) printf("L16UI a%i, a%i, %#01x ", (device->instruction >> 4) & 0x0f, (device->instruction >> 8) & 0x0f, device->instruction >> 16);
    device->vAddr = esp32_register_a_read(device, device->instruction >> 8 & 0x0f) + (device->instruction >> 16 << 1);
    esp32_register_a_write(device, device->instruction >> 4 & 0xf, esp32_memory_load16(device));
    device->program_counter += 3;
    if(device->print_instr) printf("   ; mem[%#01x] = %#01x\n", device->vAddr, esp32_memory_load16(device));
}

void esp32_instruction_L32I(esp32_device_t* device){
    if(device->print_instr) printf("L32I a%i, a%i, %#01x ", (device->instruction >> 4) & 0x0f, (device->instruction >> 8) & 0x0f, device->instruction >> 16);
    device->vAddr = esp32_register_a_read(device, device->instruction >> 8 & 0x0f) + (device->instruction >> 16 << 2);
    esp32_register_a_write(device, device->instruction >> 4 & 0x0f, esp32_memory_load32(device));
    device->program_counter += 3;
    if(device->print_instr) printf("   ; mem[%#01x] = %#01x\n", device->vAddr, esp32_memory_load32(device));
}

void esp32_instruction_S8I(esp32_device_t* device){
    if(device->print_instr) printf("S8I a%i, a%i, %#01x ", device->instruction >> 4 & 0xf, device->instruction >> 8 & 0xf, device->instruction >> 16);
    device->vAddr = esp32_register_a_read(device, device->instruction >> 8 & 0xf) + (device->instruction >> 16 << 0);
    esp32_memory_write8(device, esp32_register_a_read(device, device->instruction >> 8 & 0xf) & 0x7f);
    device->program_counter += 3;
    if(device->print_instr) printf("   ; mem[%#01x] = %#01x\n", device->vAddr, esp32_memory_load8(device));
}

void esp32_instruction_S16I(esp32_device_t* device){
    if(device->print_instr) printf("S16I a%i, a%i, %#01x ", device->instruction >> 4 & 0xf, device->instruction >> 8 & 0xf, device->instruction >> 16);
    device->vAddr = esp32_register_a_read(device, device->instruction >> 8 & 0xf) + (device->instruction >> 16 << 1);
    esp32_memory_write16(device, esp32_register_a_read(device, device->instruction >> 8 & 0xf) & 0x7fff);
    device->program_counter += 3;
    if(device->print_instr) printf("   ; mem[%#01x] = %#01x\n", device->vAddr, esp32_memory_load16(device));
}

void esp32_instruction_S32I(esp32_device_t* device){
    if(device->print_instr) printf("S8I a%i, a%i, %#01x ", device->instruction >> 4 & 0xf, device->instruction >> 8 & 0xf, device->instruction >> 16);
    device->vAddr = esp32_register_a_read(device, device->instruction >> 8 & 0xf) + (device->instruction >> 16 << 2);
    esp32_memory_write32(device, esp32_register_a_read(device, device->instruction >> 8 & 0xf));
    device->program_counter += 3;
    if(device->print_instr) printf("   ; mem[%#01x] = %#01x\n", device->vAddr, esp32_memory_load32(device));
}



void esp32_instruction_MOVI(esp32_device_t* device){
    device->temp = ((device->instruction >> 8 & 0xf) << 8) | (device->instruction >> 16 & 0xff);
    if(device->temp >> 11 & 1 == 1){
        device->temp |= 0xfffff << 12;
    }
    if(device->print_instr) printf("MOVI a%i, %#01x ", (device->instruction >> 4) & 0x0f, device->temp);
    esp32_register_a_write(device, device->instruction >> 4 & 0xf, device->temp);
    device->program_counter += 3;
    if(device->print_instr) printf("   ; a = %#08x\n", esp32_register_a_read(device, device->instruction >> 4 & 0xf));
}

void esp32_instruction_ADDI(esp32_device_t* device){
   if(device->print_instr) printf("ADDI a%i, a%i, a%i   ; ", device->instruction >> 12 & 0xf, device->instruction >> 8 & 0xf, device->instruction >> 4 & 0xf);
   device->temp = device->instruction >> 16;
   if(device->temp >> 7 & 1){
        device->temp |= 0xffffff << 8;
   }
    esp32_register_a_write(
        device,
        device->instruction >> 4 & 0xf,
        esp32_register_a_read(device, device->instruction >> 8 & 0xf) + device->temp
    );
    if(device->print_instr) printf("a = %#08x\n", esp32_register_a_read(device, device->instruction >> 4 & 0xf));
    device->program_counter += 3;
}



void esp32_instruction_CALL8(esp32_device_t* device){
    device->ps_callinc = 0b10;
    esp32_register_a_write(device, 8, ((device->program_counter + 3) & 0x3fffffff) | (0b10 << 30));
    device->temp = device->instruction >> 6;
    if(device->temp >> 17 & 1 == 1){
        device->temp |= 0xfff << 18;
    }
    device->program_counter = ((device->program_counter >> 2) + device->temp + 1) << 2;
    if(device->print_instr) printf("CALL8 %#01x     ; PC = %#08x\n", device->instruction >> 6, device->program_counter);
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

void esp32_instruction_BEQZ(esp32_device_t* device){
    if(device->print_instr) printf("BEQZ a%i\n", device->instruction >> 8 & 0xf);
    if(esp32_register_a_read(device, device->instruction >> 8 & 0xf) == 0){
        exit(0);
        device->temp = device->instruction >> 12;
        if(device->temp >> 11 & 1 == 1){
            device->temp |= 0xffffff << 12;
        }
        device->program_counter += device->temp + 4;
    }else{
        device->program_counter += 3;
    }
    
}

void esp32_instruction_BEQI(esp32_device_t* device){
    switch (device->temp) {
        case  0: device->temp = -1; break;
        case  1: device->temp = 1; break;
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

    if(device->print_instr) printf("BEQI a%i, %i, %i   ; [ %i == %i  ]", device->instruction >> 8 & 0xf, device->instruction >> 16, device->temp, esp32_register_a_read(device, device->instruction >> 8 & 0xf), device->temp);
    

    if(esp32_register_a_read(device, device->instruction >> 8 & 0xf) == device->temp){
        device->temp = device->instruction >> 16;
        if(device->temp >> 7 & 1 == 1){
            device->temp |= 0xffffff << 8;
        }
        device->program_counter += device->temp + 4;
        if(device->print_instr) printf(" jump to PC = %#010x\n", device->program_counter);
    }else{
        device->program_counter += 3;
        if(device->print_instr) printf(" skip\n");
    }
    device->temp = device->instruction >> 6;
    if(device->temp >> 17 & 1 == 1){
        device->temp |= 0xfffc << 16;
    }
    device->temp += 4;
    
}

void esp32_instruction_BNEI(esp32_device_t* device){
    switch (device->temp) {
        case  0: device->temp = -1; break;
        case  1: device->temp = 1; break;
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

    if(device->print_instr) printf("BNEI a%i, %i, %i   ; [ %i != %i  ]", device->instruction >> 8 & 0xf, device->instruction >> 16, device->temp, esp32_register_a_read(device, device->instruction >> 8 & 0xf), device->temp);
    

    if(esp32_register_a_read(device, device->instruction >> 8 & 0xf) != device->temp){
        device->temp = device->instruction >> 16;
        if(device->temp >> 7 & 1 == 1){
            device->temp |= 0xffffff << 8;
        }
        device->program_counter += device->temp + 4;
        if(device->print_instr) printf(" jump to PC = %#010x\n", device->program_counter);
    }else{
        device->program_counter += 3;
        if(device->print_instr) printf(" skip\n");
    }
    device->temp = device->instruction >> 6;
    if(device->temp >> 17 & 1 == 1){
        device->temp |= 0xfffc << 16;
    }
    device->temp += 4;
    
}

void esp32_instruction_ENTRY(esp32_device_t* device){
    device->stacktrace.push_back(device->program_counter);
    device->temp = (device->instruction >> 8) & 0xf;
    if(device->print_instr) printf("ENTRY a%i %#01x     ; ", device->temp, device->instruction >> 12 << 3);
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
    device->call_depth++;

    if(device->print_instr) printf("STACK = %#08x\n", esp32_register_a_read(device, device->temp));
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
    
    if(device->print_instr) printf("BLTUI a%i, %i, %i   ; [ %i < %i  ]", device->instruction >> 8 & 0xf, device->instruction >> 16, device->temp, esp32_register_a_read(device, device->instruction >> 8 & 0xf), device->temp);
    
    if(esp32_register_a_read(device, device->instruction >> 8 & 0xf) < device->temp){
        device->temp = device->instruction >> 16;
        if(device->temp >> 7 & 1 == 1){
            device->temp |= 0xffffff << 8;
        }
        device->program_counter += device->temp + 4;
        if(device->print_instr) printf(" jump to PC = %#010x\n", device->program_counter);
    }else{
        device->program_counter += 3;
        if(device->print_instr) puts(" skip");
    }
}

void esp32_instruction_BGEUI(esp32_device_t* device){
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
    
    if(device->print_instr) printf("BGEUI a%i, %i, %i   ; [ %i >= %i  ]", device->instruction >> 8 & 0xf, device->instruction >> 16, device->temp, esp32_register_a_read(device, device->instruction >> 8 & 0xf), device->temp);
    
    if(esp32_register_a_read(device, device->instruction >> 8 & 0xf) >= device->temp){
        device->temp = device->instruction >> 16;
        if(device->temp >> 7 & 1 == 1){
            device->temp |= 0xffffff << 8;
        }
        device->program_counter += device->temp + 4;
        if(device->print_instr) printf(" jump to PC = %#010x\n", device->program_counter);
    }else{
        device->program_counter += 3;
        if(device->print_instr) puts(" skip");
    }
}

void esp32_instruction_BEQ(esp32_device_t* device){
    if(device->print_instr) printf("BEQ a%i, a%i, %#01x   ; ", (device->instruction >> 4) & 0x0f, (device->instruction >> 8) & 0x0f, device->instruction >> 16);
    if(esp32_register_a_read(device, device->instruction >> 8 & 0xf) == esp32_register_a_read(device, device->instruction >> 4 & 0xf)){
        device->temp = device->instruction >> 16;
        if(device->temp >> 7 & 1 == 1){
            device->temp |= 0xffffff << 8;
        }
        device->program_counter += device->temp + 4;
        if(device->print_instr) printf("PC = %#01x\n", device->program_counter);
    }else{
        device->program_counter += 3;
        if(device->print_instr) puts("skip");
    }
}

void esp32_instruction_BLTU(esp32_device_t* device){
    if(device->print_instr) printf("BLTU a%i, a%i, %#01x   ; ", (device->instruction >> 4) & 0x0f, (device->instruction >> 8) & 0x0f, device->instruction >> 16);
    if(esp32_register_a_read(device, device->instruction >> 8 & 0xf) < esp32_register_a_read(device, device->instruction >> 4 & 0xf)){
        device->temp = device->instruction >> 16;
        if(device->temp >> 7 & 1 == 1){
            device->temp |= 0xffffff << 8;
        }
        device->program_counter += device->temp + 4;
        if(device->print_instr) printf("PC = %#01x\n", device->program_counter);
    }else{
        device->program_counter += 3;
        if(device->print_instr) puts("skip");
    }
}

void esp32_instruction_BBCI(esp32_device_t* device){
    // TODO: Not implemented
    if(device->print_instr) printf("BBCI a%i, a%i, %#01x   ; ", (device->instruction >> 4) & 0x0f, (device->instruction >> 8) & 0x0f, device->instruction >> 16);
    //exit(0);
    device->program_counter += 3;
    /*
    if(esp32_register_a_read(device, device->instruction >> 8 & 0xf) < esp32_register_a_read(device, device->instruction >> 4 & 0xf)){
        device->temp = device->instruction >> 16;
        if(device->temp >> 7 & 1 == 1){
            device->temp |= 0xffffff << 8;
        }
        device->program_counter += device->temp + 4;
        if(device->print_instr) printf("PC = %#01x\n", device->program_counter);
    }else{
        device->program_counter += 3;
        if(device->print_instr) puts("skip");
    }*/
}



void esp32_instruction_BNE(esp32_device_t* device){
    if(device->print_instr) printf("BNE a%i, a%i, %#01x   ; ", (device->instruction >> 4) & 0x0f, (device->instruction >> 8) & 0x0f, device->instruction >> 16);
    if(esp32_register_a_read(device, device->instruction >> 4 & 0xf) != esp32_register_a_read(device, device->instruction >> 8 & 0xf)){
        device->temp = device->instruction >> 16;
        if(device->temp >> 7 & 1 == 1){
            device->temp |= 0xffffff << 8;
        }
        device->program_counter += device->temp + 4;
        if(device->print_instr) printf("PC = %#01x\n", device->program_counter);
    }else{
        device->program_counter += 3;
        if(device->print_instr) puts("skip");
    }
}


void esp32_instruction_BGE(esp32_device_t* device){
    if(device->print_instr) printf("BGE a%i, a%i, %#01x   ; ", (device->instruction >> 4) & 0x0f, (device->instruction >> 8) & 0x0f, device->instruction >> 16);

    if((int32_t) esp32_register_a_read(device, device->instruction >> 4 & 0xf) >= (int32_t) esp32_register_a_read(device, device->instruction >> 8 & 0xf)){
        device->temp = device->instruction >> 16;
        if(device->temp >> 7 & 1 == 1){
            device->temp |= 0xffffff << 8;
        }
        device->program_counter += device->temp + 4;
        if(device->print_instr) printf("PC = %#01x\n", device->program_counter);
    }else{
        device->program_counter += 3;
        if(device->print_instr) puts("skip");
    }
}

void esp32_instruction_BGEU(esp32_device_t* device){
    if(device->print_instr) printf("BGEU a%i, a%i, %#01x   ; [ %#01x >= %#01x ]", (device->instruction >> 8) & 0x0f, (device->instruction >> 4) & 0x0f, device->instruction >> 16, esp32_register_a_read(device, device->instruction >> 8 & 0xf), esp32_register_a_read(device, device->instruction >> 4 & 0xf));

    if(esp32_register_a_read(device, device->instruction >> 8 & 0xf) >= esp32_register_a_read(device, device->instruction >> 4 & 0xf)){
        device->temp = device->instruction >> 16;
        if(device->temp >> 7 & 1 == 1){
            device->temp |= 0xffffff << 8;
        }
        device->program_counter += device->temp + 4;
        if(device->print_instr) printf("PC = %#01x\n", device->program_counter);
    }else{
        device->program_counter += 3;
        if(device->print_instr) puts("skip");
    }
}





void esp32_instruction_L32IN(esp32_device_t* device){
    if(device->print_instr) printf("L32IN a%i, a%i, %#01x", (device->instruction >> 4) & 0x0f, (device->instruction >> 8) & 0x0f, (device->instruction >> 12) & 0x0f);
    device->temp = esp32_register_a_read(
        device,
        (device->instruction >> 8) & 0x0f
    );
    device->vAddr = device->temp + (((device->instruction >> 12) & 0x0f) << 2);
    esp32_register_a_write(
        device,
        (device->instruction >> 4) & 0x0f,
        esp32_memory_load32(device)
    );
    device->program_counter += 2;
    if(device->print_instr) printf("   ; mem[%#01x] = %#01x\n", device->vAddr, esp32_memory_load32(device));
}

void esp32_instruction_S32IN(esp32_device_t* device){
    device->vAddr = ((device->instruction >> 12 & 0x0f) << 2) + esp32_register_a_read(device, device->instruction >> 8 & 0xf);
    esp32_memory_write32(device, esp32_register_a_read(device, device->instruction >> 4 & 0xf));
    device->program_counter += 2;
    if(device->print_instr) printf("S32IN a%i, a%i, %i; mem[%#08x] <- %#08x\n", device->instruction >> 4 & 0xf, device->instruction >> 8 & 0xf, device->instruction >> 12 & 0xf, device->vAddr, esp32_register_a_read(device, device->instruction >> 4 & 0xf));
}

void esp32_instruction_ADDN(esp32_device_t* device){
    esp32_register_a_write(
        device,
        device->instruction >> 12 & 0xf,
        esp32_register_a_read(device, device->instruction >> 8 & 0xf) + esp32_register_a_read(device, device->instruction >> 4 & 0xf)
    );
    if(device->print_instr) printf("ADD.N a%i, a%i, a%i  ; a = %#08x\n", device->instruction >> 12 & 0xf, device->instruction >> 8 & 0xf, device->instruction >> 4 & 0xf,  esp32_register_a_read(device, device->instruction >> 12 & 0xf));
    device->program_counter += 2;
}

void esp32_instruction_ADDIN(esp32_device_t* device){
    if(device->instruction >> 4 & 0xf == 0){
        esp32_register_a_write(
            device,
            device->instruction >> 12 & 0xf,
            esp32_register_a_read(device, device->instruction >> 8 & 0xf) + (device->instruction >> 4 & 0xf)
        );
    }else{
        esp32_register_a_write(
            device,
            device->instruction >> 12 & 0xf,
            esp32_register_a_read(device, device->instruction >> 8 & 0xf) + 0xffffffff
        );
    }
    if(device->print_instr) printf("ADDI.N a%i, a%i, %i  ; a = %#08x\n", device->instruction >> 12 & 0xf, device->instruction >> 8 & 0xf, device->instruction >> 4 & 0xf,  esp32_register_a_read(device, device->instruction >> 12 & 0xf));
    device->program_counter += 2;
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

void esp32_instruction_BEQZN(esp32_device_t* device){
    device->temp = ((device->instruction >> 12 & 0xf)) + ((device->instruction >> 4 & 0b11)  << 4);

    if(device->print_instr) printf("BEQZ.N a%i, %i     ", device->instruction >> 8 & 0x0f, device->temp);

    if(esp32_register_a_read(device, (device->instruction >> 8) & 0xf) == 0){
        device->program_counter += device->temp +4;
        if(device->print_instr) printf("; PC = %#01x\n", device->program_counter);
    }else{
        device->program_counter += 2;
        if(device->print_instr) printf("; skip\n");
    }
}

void esp32_instruction_BNEZN(esp32_device_t* device){
    device->temp = (device->instruction >> 12 & 0xf) + ((device->instruction >> 6 & 0b11)) << 4;

    if(device->print_instr) printf("BNEZ.N a%i, %i     ", device->instruction >> 8 & 0x0f, device->temp);

    if(esp32_register_a_read(device, (device->instruction >> 8) & 0x0f) != 0){
        device->program_counter += device->temp;
        if(device->print_instr) printf("; PC = %#01x\n", device->program_counter);
    }else{
        device->program_counter += 2;
        if(device->print_instr) printf("; skip\n");
    }
}

void esp32_instruction_MOVN(esp32_device_t* device){
     esp32_register_a_write(
        device,
        (device->instruction >> 4) & 0xf,
        esp32_register_a_read(device, (device->instruction >> 8) & 0x0f)
    );
    if(device->print_instr) printf("MOV.N a%i, a%i         ; %#01x\n", (device->instruction >> 4) & 0x0f, (device->instruction >> 8) & 0x0f, esp32_register_a_read(device, (device->instruction >> 8) & 0x0f));
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
        esp32_instruction_RSYNC,
        0b0000,     0xf,    0,
        0b0000,     0xf,   16,
        0b0000,     0xf,   20,
        0b0010,     0xf,   12,
        0b0001,     0xf,    4
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
        esp32_instruction_AND,
        0b0000,     0xf,  0,
        0b0000,     0xf,  16,
        0b0001,     0xf,  20
    );
    esp32_instruction_register(
        esp32_instruction_OR,
        0b0000,     0xf,  0,
        0b0000,     0xf,  16,
        0b0010,     0xf,  20
    );
    esp32_instruction_register(
        esp32_instruction_MOVSP,
        0b0000,     0xf,  0,
        0b0000,     0xf,  16,
        0b0000,     0xf,  20,
        0b0001,     0xf,  12
    );
    esp32_instruction_register(
        esp32_instruction_ADD,
        0b0000,     0xf,  0,
        0b0000,     0xf,  16,
        0b1000,     0xf,  20
    );
    esp32_instruction_register(
        esp32_instruction_RSIL,
        0b0000,     0xf,  0,
        0b0000,     0xf,  16,
        0b0000,     0xf,  20,
        0b0110,     0xf,  12
    );
    esp32_instruction_register(
        esp32_instruction_ADDX2,
        0b0000,     0xf,    0,
        0b0000,     0xf,   16,
        0b1001,     0xf,   20
    );
    esp32_instruction_register(
        esp32_instruction_ADDX4,
        0b0000,     0xf,    0,
        0b0000,     0xf,   16,
        0b1010,     0xf,   20
    );
    esp32_instruction_register(
        esp32_instruction_ADDX8,
        0b0000,     0xf,    0,
        0b0000,     0xf,   16,
        0b1011,     0xf,   20
    );
    
    esp32_instruction_register(
        esp32_instruction_SUB,
        0b0000,     0xf,    0,
        0b0000,     0xf,   16,
        0b1100,     0xf,   20
    );
    esp32_instruction_register(
        esp32_instruction_SLLI,
        0b0000,     0xf,  0,
        0b0001,     0xf,  16,
        0b0000,     0xf,  20
    );
    esp32_instruction_register(
        esp32_instruction_SLLI,
        0b0000,     0xf,  0,
        0b0001,     0xf,  16,
        0b0001,     0xf,  20
    );
    esp32_instruction_register(
        esp32_instruction_SRAI,
        0b0000,     0xf,  0,
        0b0001,     0xf,  16,
        0b0010,     0xf,  20
    );
    esp32_instruction_register(
        esp32_instruction_SRAI,
        0b0000,     0xf,  0,
        0b0001,     0xf,  16,
        0b0011,     0xf,  20
    );
    esp32_instruction_register(
        esp32_instruction_SRLI,
        0b0000,     0xf,    0,
        0b0001,     0xf,   16,
        0b0100,     0xf,   20
    );
    esp32_instruction_register(
        esp32_instruction_SRC,
        0b0000,     0xf,    0,
        0b0001,     0xf,   16,
        0b1000,     0xf,   20
    );
    esp32_instruction_register(
        esp32_instruction_MULL,
        0b0000,     0xf,    0,
        0b0010,     0xf,   16,
        0b1000,     0xf,   20
    );
    esp32_instruction_register(
        esp32_instruction_MULLUH,
        0b0000,     0xf,    0,
        0b0010,     0xf,   16,
        0b1010,     0xf,   20
    );
    esp32_instruction_register(
        esp32_instruction_QUOU,
        0b0000,     0xf,    0,
        0b0010,     0xf,   16,
        0b1100,     0xf,   20
    );
    esp32_instruction_register(
        esp32_instruction_WSR,
        0b0000,     0xf,  0,
        0b0011,     0xf,  16,
        0b0001,     0xf,  20
    );
    esp32_instruction_register(
        esp32_instruction_RSR,
        0b0000,     0xf,  0,
        0b0011,     0xf,  16,
        0b0000,     0xf,  20
    );
    esp32_instruction_register(
        esp32_instruction_MINU,
        0b0000,     0xf,    0,
        0b0011,     0xf,   16,
        0b0110,     0xf,   20
    );
    esp32_instruction_register(
        esp32_instruction_MOVEQZ,
        0b0000,     0xf,    0,
        0b0011,     0xf,   16,
        0b1000,     0xf,   20
    );
    esp32_instruction_register(
        esp32_instruction_MOVENEZ,
        0b0000,     0xf,    0,
        0b0011,     0xf,   16,
        0b1001,     0xf,   20
    );
    esp32_instruction_register(
        esp32_instruction_EXTUI,
        0b0000,     0xf,  0,
        0b0100,     0xf,  16
    );
    esp32_instruction_register(
        esp32_instruction_EXTUI,
        0b0000,     0xf,  0,
        0b0101,     0xf,  16
    );
    esp32_instruction_register(
        esp32_instruction_L32R,
        0b0001,     0xf,  0
    );
    esp32_instruction_register(
        esp32_instruction_L8UI,
        0b0010,     0xf,  0,
        0b0000,     0xf,  12
    );
    esp32_instruction_register(
        esp32_instruction_L16UI,
        0b0010,     0xf,  0,
        0b0001,     0xf,  12
    );
    esp32_instruction_register(
        esp32_instruction_L32I,
        0b0010,     0xf,  0,
        0b0010,     0xf,  12
    );
    esp32_instruction_register(
        esp32_instruction_S8I,
        0b0010,     0xf,  0,
        0b0100,     0xf,  12
    );
    esp32_instruction_register(
        esp32_instruction_S16I,
        0b0010,     0xf,  0,
        0b0101,     0xf,  12
    );
    esp32_instruction_register(
        esp32_instruction_S32I,
        0b0010,     0xf,  0,
        0b0110,     0xf,  12
    );
    esp32_instruction_register(
        esp32_instruction_MOVI,
        0b0010,     0xf,  0,
        0b1010,     0xf,  12
    );
    esp32_instruction_register(
        esp32_instruction_ADDI,
        0b0010,     0xf,  0,
        0b1100,     0xf,  12
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
        esp32_instruction_BEQZ,
        0b0110,     0xf,    0,
        0b01,       0b11,   4,
        0b00,       0b11,   6
    );
    esp32_instruction_register(
        esp32_instruction_BEQI,
        0b0110,     0xf,    0,
        0b10,       0b11,   4,
        0b00,       0b11,   6
    );
    
    esp32_instruction_register(
        esp32_instruction_BNEI,
        0b0110,     0xf,    0,
        0b10,       0b11,   4,
        0b01,       0b11,   6
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
        esp32_instruction_BGEUI,
        0b0110,     0xf,    0,
        0b11,       0b11,   4,
        0b11,       0b11,   6
    );

     esp32_instruction_register(
        esp32_instruction_BEQ,
        0b0111,     0xf,  0,
        0b0001,     0xf,  12
    );

    esp32_instruction_register(
        esp32_instruction_BLTU,
        0b0111,     0xf,  0,
        0b0011,     0xf,  12
    );

    esp32_instruction_register(
        esp32_instruction_BBCI,
        0b0111,     0xf,  0,
        0b0110,     0xf,  12
    );

    esp32_instruction_register(
        esp32_instruction_BBCI,
        0b0111,     0xf,  0,
        0b0111,     0xf,  12
    );

    esp32_instruction_register(
        esp32_instruction_BNE,
        0b0111,     0xf,  0,
        0b1001,     0xf,  12
    );

    esp32_instruction_register(
        esp32_instruction_BGE,
        0b0111,     0xf,  0,
        0b1010,     0xf,  12
    );

    esp32_instruction_register(
        esp32_instruction_BGEU,
        0b0111,     0xf,  0,
        0b1011,     0xf,  12
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
        esp32_instruction_ADDN,
        0b1010,     0xf,  0
    );
    esp32_instruction_register(
        esp32_instruction_ADDIN,
        0b1011,     0xf,  0
    );
    esp32_instruction_register(
        esp32_instruction_MOVIN,
        0b1100,     0xf,  0,
        0b00,       0b11,  6
    );
    esp32_instruction_register(
        esp32_instruction_MOVIN,
        0b1100,     0xf,  0,
        0b01,       0b11,  6
    );
    esp32_instruction_register(
        esp32_instruction_BEQZN,
        0b1100,     0xf,  0,
        0b10,      0b11,  6
    );
    esp32_instruction_register(
        esp32_instruction_BNEZN,
        0b1100,     0xf,    0,
        0b11,       0b11,   6
    );
    esp32_instruction_register(
        esp32_instruction_MOVN,
        0b1101,     0xf,  0,
        0b0000,     0xf,  12
    );
    esp32_instruction_register(
        esp32_instruction_RETW, // RETW.N is equal
        0b1101,     0xf,  0,
        0b1111,     0xf,  12,
        0b0001,     0xf,  4
    );
}