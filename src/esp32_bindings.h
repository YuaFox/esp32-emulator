#include <map>
#include <utility>

#include "esp32_device.h"

#define esp32_binding_map std::map<uint32_t, std::pair<char*, void(*)(esp32_device_t*)>>

extern esp32_binding_map esp32_bindings;

void esp32_binding_init();