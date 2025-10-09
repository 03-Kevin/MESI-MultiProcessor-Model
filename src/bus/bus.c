#include "bus.h"
#include <stdio.h>

void bus_init() {
    printf("[BUS] Initialized.\n");
}

void bus_broadcast(BusMsg msg, int addr, int src_pe) {
    const char* msg_str[] = {"BusRd", "BusRdX", "Invalidate", "WriteBack"};
    printf("[BUS] PE%d sent %s for address %d\n", src_pe, msg_str[msg], addr);
}
