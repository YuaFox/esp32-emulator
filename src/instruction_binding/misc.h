#include "../esp32_device.h"
#include <stdlib.h>
#include <stdio.h>


void esp32_f_qsort(esp32_device_t* device){
    const uint32_t pbase = esp32_register_a_read(device, 2);
    const uint32_t items = esp32_register_a_read(device, 3);

    for(int i = items; i > 0; i--){
        for(int j = 0; j < i-1; j++){
            esp32_register_a_write(device, 10, pbase+j*8);
            esp32_register_a_write(device, 11, pbase+(j+1)*8);
            esp32_run_instruction(device, 0b000000000000010111100000);
            esp32_run(device);
            if((int32_t) esp32_register_a_read(device, 10) > 0){
                esp32_memory_swap32(device, pbase+j*8, pbase+(j+1)*8, 2);
            }
        }
    }
    device->vAddr = pbase;
    
    for(int i = 0; i < 7; i++){
        printf("%i %#01x ", i, esp32_memory_load32(device));
        device->vAddr += 4;
        printf("%#01x\n", esp32_memory_load32(device));
        device->vAddr += 4;
    }
}

void esp32_f_itoa(esp32_device_t* device){
    if(esp32_register_a_read(device, 4) == 10){
        sprintf((char*) &device->memory[esp32_register_a_read(device, 3)], "%d", esp32_register_a_read(device, 2));
    }else if(esp32_register_a_read(device, 4) == 16){
        sprintf((char*) &device->memory[esp32_register_a_read(device, 3)], "%x", esp32_register_a_read(device, 2));
    }else{
        printf("Error: itoa with base %i\n", esp32_register_a_read(device, 4));
        exit(0);
    }
}

void esp32_f_strlen(esp32_device_t* device){
    esp32_register_a_write(device, 2, strlen((char*) &device->memory[esp32_memory_paddr(esp32_register_a_read(device, 2))]));
}