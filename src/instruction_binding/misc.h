#include "../esp32_device.h"
#include <stdlib.h>
#include <stdio.h>


void esp32_f_qsort(esp32_device_t* device){
    /*
    for(int i = 0; i < 8; i++){
        printf("%i %#01x\n", i, esp32_register_a_read(device, i));
    }*/

    uint32_t pbase = esp32_register_a_read(device, 2);

    device->vAddr = pbase;
    esp32_register_a_write(device, 10, esp32_memory_load32(device));
    esp32_register_a_write(device, 11, pbase+4);

    // Run function
    esp32_run_instruction(device, 0b000000000000010111100000);
    esp32_run(device);
}

void esp32_f_itoa(esp32_device_t* device){
    if(esp32_register_a_read(device, 4) == 10){
        sprintf((char*) &device->memory[esp32_register_a_read(device, 3)], "%d", esp32_register_a_read(device, 2));
    }else{
        exit(0);
    }
}

void esp32_f_strlen(esp32_device_t* device){
    esp32_register_a_write(device, 2, strlen((char*) &device->memory[esp32_memory_paddr(esp32_register_a_read(device, 2))]));
}