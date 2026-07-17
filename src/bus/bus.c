// src/bus/bus.c
#include "bus.h"
#include "../cache/cache.h"
#include "handlers.h"
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include "../mesi/mesi.h"   // for mesi_state_to_str in debug printing

// Convierte un mensaje del bus a su representación en cadena
static const char* bus_msg_to_str(BusMsg msg) {
    switch (msg) {
        case BUS_RD:   return "BusRd";
        case BUS_RDX:  return "BusRdX";
        case BUS_UPGR: return "BusUpgr";
        case BUS_WB:   return "BusWB";
        default:       return "Unknown";
    }
}

/* Dispatcher: saca peticiones de la cola y las procesa secuencialmente.
   Garantiza FIFO y evita que dos handlers se ejecuten "simultáneamente"
   intercalando efectos (además de usar locks internos de caches). */
static void* bus_dispatcher(void* arg) {
    Bus *bus = (Bus*)arg;
    if (!bus) return NULL;

    while (1) {
        pthread_mutex_lock(&bus->queue_lock);
        while (!bus->req_head && bus->running) {
            pthread_cond_wait(&bus->queue_cv, &bus->queue_lock);
        }

        if (!bus->running && !bus->req_head) {
            pthread_mutex_unlock(&bus->queue_lock);
            break; // salir limpiamente
        }

        // sacar petición
        BusRequest *req = bus->req_head;
        if (req) {
            bus->req_head = req->next;
            if (!bus->req_head) bus->req_tail = NULL;
        }
        // dejamos queue_lock liberado durante la ejecución del handler
        pthread_mutex_unlock(&bus->queue_lock);

        if (req) {
            // contabilizar tráfico
            bus->traffic_count++;
            if (req->src_pe >= 0 && req->src_pe < NUM_PES && req->msg >= 0 && req->msg <= BUS_WB)
                bus->per_pe_bus_msgs[req->src_pe][req->msg]++;

            // Resetear shared flag antes del handler (uso interno)
            bus->last_shared = 0;

            // Ejecutar handler (los handlers usan locks propios para proteger caches)
            if (bus->handlers[req->msg]) {
                bus->handlers[req->msg](bus, req->addr, req->src_pe);
            } else {
                printf("[BUS] ⚠️ No hay handler definido para %s\n", bus_msg_to_str(req->msg));
            }

            // Copiar el resultado del handler (bus->last_shared) dentro de la request
            pthread_mutex_lock(&req->mutex);
            req->shared = bus->last_shared ? 1 : 0;
            req->processed = 1;
            pthread_cond_signal(&req->cond);
            pthread_mutex_unlock(&req->mutex);
            // El thread solicitante liberará y destruirá req (liberación de memoria)
        }
    }

    return NULL;
}

void bus_init(Bus* bus, Cache* caches[]) {
    if (!bus || !caches) {
        fprintf(stderr, "[BUS] Error: Parámetros inválidos en bus_init.\n");
        exit(EXIT_FAILURE);
    }

    memset(bus, 0, sizeof(Bus));

    for (int i = 0; i < NUM_PES; i++) {
        if (!caches[i]) {
            fprintf(stderr, "[BUS] Error: Caché PE%d no inicializada.\n", i);
            exit(EXIT_FAILURE);
        }
        bus->caches[i] = caches[i];
        for (int j = 0; j <= BUS_WB; j++) {
            bus->per_pe_bus_msgs[i][j] = 0;  // Inicializar contadores de mensajes
        }
    }

    // Registrar handlers (implementado en handlers.c)
    bus_register_handlers(bus);

    bus->last_shared = 0;
    bus->traffic_count = 0;
    bus->req_head = bus->req_tail = NULL;
    bus->running = 1;

    pthread_mutex_init(&bus->queue_lock, NULL);
    pthread_cond_init(&bus->queue_cv, NULL);

    // Lanzar dispatcher
    if (pthread_create(&bus->dispatcher_thread, NULL, bus_dispatcher, (void*)bus) != 0) {
        perror("[BUS] Error creando dispatcher thread");
        exit(EXIT_FAILURE);
    }

    printf("[BUS] Initialized (dispatcher running).\n");
}

/*
 * Encola una petición y bloquea hasta que se procese.
 * Devuelve 1 si el handler encontró que la línea estaba compartida (shared), 0 si no.
 */
int bus_broadcast(Bus* bus, BusMsg msg, int addr, int src_pe) {
    if (!bus) {
        fprintf(stderr, "[BUS] Error: Bus no inicializado en bus_broadcast.\n");
        exit(EXIT_FAILURE);
    }

    if (addr < 0 || addr >= MEM_SIZE) {
        fprintf(stderr, "[BUS] Error: Dirección fuera de los límites en bus_broadcast (Addr=%d, MEM_SIZE=%d).\n", addr, MEM_SIZE);
        exit(EXIT_FAILURE);
    }

    if (src_pe < 0 || src_pe >= NUM_PES) {
        fprintf(stderr, "[BUS] Error: PE inválido (%d) en bus_broadcast.\n", src_pe);
        exit(EXIT_FAILURE);
    }

    // Construir petición
    BusRequest *req = (BusRequest*)malloc(sizeof(BusRequest));
    if (!req) {
        perror("[BUS] malloc BusRequest");
        exit(EXIT_FAILURE);
    }
    req->msg = msg;
    req->addr = addr;
    req->src_pe = src_pe;
    req->processed = 0;
    req->shared = 0;
    req->next = NULL;
    pthread_mutex_init(&req->mutex, NULL);
    pthread_cond_init(&req->cond, NULL);

    // Encolar la petición
    pthread_mutex_lock(&bus->queue_lock);
    if (bus->req_tail) bus->req_tail->next = req;
    else bus->req_head = req;
    bus->req_tail = req;
    // Señalizar dispatcher
    pthread_cond_signal(&bus->queue_cv);
    pthread_mutex_unlock(&bus->queue_lock);

    printf("[BUS] Señal %s encolada por PE%d para Addr=%d\n", bus_msg_to_str(msg), src_pe, addr);

    // Esperar a que el dispatcher procese la petición (comportamiento bloqueante)
    pthread_mutex_lock(&req->mutex);
    while (!req->processed) {
        pthread_cond_wait(&req->cond, &req->mutex);
    }
    int was_shared = req->shared; // lectura sincronizada
    pthread_mutex_unlock(&req->mutex);

    // Liberar recursos de la petición
    pthread_cond_destroy(&req->cond);
    pthread_mutex_destroy(&req->mutex);
    free(req);

    return was_shared;
}

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
    printf("[BUS] Total mensajes procesados: %lu\n", bus->traffic_count);
}

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
                    unsigned long phys = (line->tag * SETS + set) * DOUBLES_PER_LINE;
                    printf("  Set %d, Way %d: Addr=%lu, State=%s\n",
                           set, way, phys, mesi_state_to_str(line->state));
                }
            }
        }
    }
}

void bus_destroy(Bus* bus) {
    if (!bus) return;
    // parar dispatcher limpiamente
    pthread_mutex_lock(&bus->queue_lock);
    bus->running = 0;
    pthread_cond_signal(&bus->queue_cv);
    pthread_mutex_unlock(&bus->queue_lock);

    pthread_join(bus->dispatcher_thread, NULL);

    pthread_mutex_destroy(&bus->queue_lock);
    pthread_cond_destroy(&bus->queue_cv);

    // Si quedaron requests en cola (no deberían), liberarlas
    BusRequest *r = bus->req_head;
    while (r) {
        BusRequest *n = r->next;
        pthread_cond_destroy(&r->cond);
        pthread_mutex_destroy(&r->mutex);
        free(r);
        r = n;
    }
    bus->req_head = bus->req_tail = NULL;

    printf("[BUS] Destroyed (dispatcher stopped).\n");
}
