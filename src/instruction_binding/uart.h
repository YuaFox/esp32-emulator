#include "../esp32_device.h"
#include <cstring>
#include <string.h>

void ets_printf(esp32_device_t* device){
    char * pch;
    char buffer[256];
    pch = strtok ((char*) &device->memory[esp32_register_a_read(device, 2)],"%");

    uint8_t reg = 3;
    fputs(pch, stdout);
    pch = strtok (NULL, "%");
    while (pch != NULL)
    {
        memset(buffer, '%', 1);
        memcpy(buffer+1, pch, 255);
        if(strncmp(buffer, "%lu", 3) == 0){
            printf(buffer, esp32_register_a_read(device, reg++));
        }else if(strncmp(buffer, "%d", 2) == 0){
            printf(buffer, esp32_register_a_read(device, reg++));
        }else if(strncmp(buffer, "%s", 2) == 0){
            printf(buffer, &device->memory[esp32_register_a_read(device, reg++)]);
        }else{
            fputs(buffer, stdout);
        }
        pch = strtok (NULL, "%");
    }
}