#include "bus.h"
#include "handlers.h"
#include <stdio.h>

pthread_mutex_t bus_lock = PTHREAD_MUTEX_INITIALIZER;

void bus_init(Bus* bus, Cache* caches[]) {
    for (int i = 0; i < NUM_PES; i++)
        bus->caches[i] = caches[i];

    // Registrar los handlers
    bus_register_handlers(bus);

    printf("[BUS] Initialized.\n");
}

void bus_broadcast(Bus* bus, BusMsg msg, int addr, int src_pe) {
    pthread_mutex_lock(&bus_lock);
    printf("[BUS] Señal %d recibida de PE%d (addr=%d)\n", msg, src_pe, addr);

    // Llamar al handler correspondiente
    if (bus->handlers[msg])
        bus->handlers[msg](bus, addr, src_pe);
    else
        printf("[BUS] ⚠️ No hay handler definido para la señal %d\n", msg);

    pthread_mutex_unlock(&bus_lock);
}
