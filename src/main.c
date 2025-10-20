#include "include/config.h"
#include "pe/pe.h"
#include "bus/bus.h"
#include "memory/memory.h"
#include <stdio.h> // Para printf

// Función para imprimir los estados finales de las cachés
void debug_print_cache_states(Bus* bus) {
    printf("\n[DEBUG] Estados finales de las cachés:\n");
    for (int i = 0; i < NUM_PES; i++) {
        printf("[DEBUG] Caché PE%d:\n", i);
        for (int set = 0; set < SETS; set++) {
            for (int way = 0; way < WAYS; way++) {
                CacheLine* line = &bus->caches[i]->sets[set].lines[way];
                if (line->valid) {
                    printf("  Set %d, Way %d: Addr=%lu, State=%s\n",
                           set, way, line->tag, mesi_state_to_str(line->state));
                }
            }
        }
    }
}

int main() {
    // Inicializar memoria
    mem_init();
    printf("[MAIN] Memoria inicializada.\n");

    Bus bus;
    Cache caches[NUM_PES];
    PE pes[NUM_PES];
    pthread_t threads[NUM_PES];

    // Inicializar cachés
    for (int i = 0; i < NUM_PES; i++) {
        cache_init(&caches[i]);
        caches[i].bus = &bus;
        printf("[MAIN] Caché %d inicializada y conectada al bus.\n", i);
    }

    // Inicializar bus
    Cache* cache_ptrs[NUM_PES];
    for (int i = 0; i < NUM_PES; i++)
        cache_ptrs[i] = &caches[i];
    bus_init(&bus, cache_ptrs);
    printf("[MAIN] Bus inicializado y cachés registradas.\n");

    // Crear hilos para los PEs
    for (int i = 0; i < NUM_PES; i++) {
        pes[i].id = i;
        pes[i].cache = &caches[i];
        pthread_create(&threads[i], NULL, pe_run, &pes[i]);
        printf("[MAIN] PE%d inicializado y ejecutándose en un hilo.\n", i);
    }

    // Esperar hilos
    for (int i = 0; i < NUM_PES; i++) {
        pthread_join(threads[i], NULL);
        printf("[MAIN] Hilo de PE%d finalizado.\n", i);
    }

    // Imprimir estados finales de las cachés
    debug_print_cache_states(&bus);

    printf("[MAIN] Todos los hilos finalizados. Programa terminado.\n");
    return 0;
}