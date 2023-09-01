#include "esp32_bindings.h"
#include <iostream>

#include "instruction_binding/uart.h"
#include "instruction_binding/mem.h"
#include "instruction_binding/misc.h"

esp32_binding_map esp32_bindings;

// 
void _xtos_set_min_intlevel(esp32_device_t* device){
    device->ps_intlevel = esp32_register_a_read(device, 2);
}

void rtc_get_reset_reason(esp32_device_t* device){

}

void esp32_binding_init(){
    esp32_bindings[0x400081d4] = {"rtc_get_reset_reason", rtc_get_reset_reason};
    esp32_bindings[0x4000bfdc] = {"unsigned _xtos_set_min_intlevel(int intlevel)", _xtos_set_min_intlevel};
    esp32_bindings[0x40007d54] = {"int ets_printf(const char *fmt, ...)", ets_printf};
    esp32_bindings[0x4000c44c] = {"memset", esp32_f_memset};
    esp32_bindings[0x4000c2c8] = {"void * memcpy ( void * destination, const void * source, size_t num )", esp32_f_memcpy};
    esp32_bindings[0x40056424] = {"void qsort(void *base, size_t nitems, size_t size, int (*compar)(const void *, const void*))", esp32_f_qsort};
    esp32_bindings[0x400566b4] = {"char *  itoa ( int value, char * str, int base );", esp32_f_itoa};
    esp32_bindings[0x400014c0] = {"strlen", esp32_f_strlen};
    
}