#include "include/config.h"
#include "pe/pe.h"
#include "bus/bus.h"
#include "memory/memory.h"

int main() {
    mem_init();

    Bus bus;
    Cache caches[NUM_PES];
    PE pes[NUM_PES];
    pthread_t threads[NUM_PES];

    // Inicializar cachés
    for (int i = 0; i < NUM_PES; i++) {
        cache_init(&caches[i]);
        caches[i].bus = &bus;
    }

    // Inicializar bus
    Cache* cache_ptrs[NUM_PES];
    for (int i = 0; i < NUM_PES; i++)
        cache_ptrs[i] = &caches[i];
    bus_init(&bus, cache_ptrs);

    // Inicializar PEs
    for (int i = 0; i < NUM_PES; i++) {
        pes[i].id = i;
        pes[i].cache = &caches[i];
        pthread_create(&threads[i], NULL, pe_run, &pes[i]);
    }

    // Esperar hilos
    for (int i = 0; i < NUM_PES; i++)
        pthread_join(threads[i], NULL);

    return 0;
}
