#include "bus.h"
#include "../cache/cache.h"  // aquí sí conoce Cache
#include "handlers.h"
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>  // Para EXIT_FAILURE

// Mutex para proteger el acceso al bus
static pthread_mutex_t bus_lock = PTHREAD_MUTEX_INITIALIZER;

// Convierte un mensaje del bus a su representación en cadena
const char* bus_msg_to_str(BusMsg msg) {
    switch (msg) {
        case BUS_RD: return "BusRd";
        case BUS_RDX: return "BusRdX";
        case BUS_UPGR: return "BusUpgr";
        case BUS_WB: return "BusWB";
        default: return "Unknown";
    }
}

// Inicializa el bus y sus estructuras internas
void bus_init(Bus* bus, Cache* caches[]) {
    if (!bus || !caches) {
        fprintf(stderr, "[BUS] Error: Parámetros inválidos en bus_init.\n");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < NUM_PES; i++) {
        if (!caches[i]) {
            fprintf(stderr, "[BUS] Error: Caché PE%d no inicializada.\n", i);
            exit(EXIT_FAILURE);
        }
        bus->caches[i] = caches[i];
        for (int j = 0; j < 4; j++) {
            bus->per_pe_bus_msgs[i][j] = 0;  // Inicializar contadores de mensajes
        }
    }

    // Registrar handlers (implementado en handlers.c)
    bus_register_handlers(bus);

    bus->last_shared = 0;
    bus->traffic_count = 0;
    printf("[BUS] Initialized.\n");
}

// Difunde un mensaje del bus a todos los PEs
void bus_broadcast(Bus* bus, BusMsg msg, int addr, int src_pe) {
    if (!bus) {
        fprintf(stderr, "[BUS] Error: Bus no inicializado en bus_broadcast.\n");
        exit(EXIT_FAILURE);
    }

    pthread_mutex_lock(&bus_lock);
    bus->traffic_count++;
    bus->last_shared = 0; // Reset antes de ejecutar handler

    // Incrementar el contador de mensajes enviados por el PE
    if (src_pe < 0 || src_pe >= NUM_PES) {
        fprintf(stderr, "[BUS] Error: PE inválido (%d) en bus_broadcast.\n", src_pe);
        pthread_mutex_unlock(&bus_lock);
        exit(EXIT_FAILURE);
    }
    bus->per_pe_bus_msgs[src_pe][msg]++;

    printf("[BUS] Señal %s recibida de PE%d para Addr=%d\n", bus_msg_to_str(msg), src_pe, addr);

    if (bus->handlers[msg]) {
        printf("[BUS] Ejecutando handler para %s (PE%d, Addr=%d)\n", bus_msg_to_str(msg), src_pe, addr);
        bus->handlers[msg](bus, addr, src_pe);
    } else {
        printf("[BUS] ⚠️ No hay handler definido para %s\n", bus_msg_to_str(msg));
    }

    pthread_mutex_unlock(&bus_lock);
}

// Procesa una solicitud de un PE al bus
void bus_request(Bus* bus, int addr, BusMsg msg, int src_pe) {
    if (!bus) {
        fprintf(stderr, "[BUS] Error: Bus no inicializado en bus_request.\n");
        exit(EXIT_FAILURE);
    }

    if (addr < 0 || addr >= MEM_SIZE) {
        fprintf(stderr, "[BUS] Error: Dirección fuera de los límites en bus_request (Addr=%d, MEM_SIZE=%d).\n", addr, MEM_SIZE);
        exit(EXIT_FAILURE);
    }

    if (src_pe < 0 || src_pe >= NUM_PES) {
        fprintf(stderr, "[BUS] Error: PE inválido (%d) en bus_request.\n", src_pe);
        exit(EXIT_FAILURE);
    }

    printf("[BUS] PE%d solicita %s para Addr=%d\n", src_pe, bus_msg_to_str(msg), addr);
    bus_broadcast(bus, msg, addr, src_pe);
}

// Imprime las métricas del tráfico del bus
void bus_print_metrics(Bus* bus) {
    if (!bus) {
        fprintf(stderr, "[BUS] Error: Bus no inicializado en bus_print_metrics.\n");
        return;
    }

    printf("\n[TRÁFICO DEL BUS]\n");
    for (int i = 0; i < NUM_PES; i++) {
        printf("[PE%d] Mensajes enviados:\n", i);
        printf("  - BusRd: %lu\n", bus->per_pe_bus_msgs[i][BUS_RD]);
        printf("  - BusRdX: %lu\n", bus->per_pe_bus_msgs[i][BUS_RDX]);
        printf("  - BusUpgr: %lu\n", bus->per_pe_bus_msgs[i][BUS_UPGR]);
        printf("  - BusWB: %lu\n", bus->per_pe_bus_msgs[i][BUS_WB]);
    }
}

// Imprime los estados finales de las cachés
void debug_print_bus_cache_states(Bus* bus) {
    if (!bus) {
        fprintf(stderr, "[BUS] Error: Bus no inicializado en debug_print_bus_cache_states.\n");
        return;
    }

    printf("\n[DEBUG] Estados finales de las cachés:\n");
    for (int i = 0; i < NUM_PES; i++) {
        printf("[DEBUG] Caché PE%d:\n", i);
        for (int set = 0; set < SETS; set++) {
            for (int way = 0; way < WAYS; way++) {
                CacheLine* line = &bus->caches[i]->sets[set].lines[way];
                if (line->valid) {
                    unsigned long addr = (line->tag * SETS + set) * DOUBLES_PER_LINE;
                    printf("  Set %d, Way %d: Addr=%lu, State=%s\n",
                           set, way, addr, mesi_state_to_str(line->state));
                }
            }
        }
    }
}