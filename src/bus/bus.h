#ifndef BUS_H
#define BUS_H

#include <pthread.h>        // <<-- necesario para pthread_mutex_t / pthread_cond_t / pthread_t
#include "../include/config.h"  // Ajusta ruta si hace falta

// Forward declaration de Cache
struct Cache;
typedef struct Cache Cache;

// Enum para mensajes del bus
typedef enum { BUS_RD = 0, BUS_RDX = 1, BUS_UPGR = 2, BUS_WB = 3 } BusMsg;

typedef struct BusRequest {
    BusMsg msg;
    int addr;
    int src_pe;
    int processed;             // 0 = pendiente, 1 = procesado
    struct BusRequest *next;
    // sincronización específica de la petición (espera del thread solicitante)
    pthread_mutex_t mutex;
    pthread_cond_t  cond;
} BusRequest;

// Estructura Bus
typedef struct Bus {
    Cache* caches[NUM_PES];                       // Punteros a las cachés
    void (*handlers[4])(struct Bus*, int, int);   // Handlers para BUS_RD, BUS_RDX, etc.
    int last_shared;
    unsigned long traffic_count;
    unsigned long per_pe_bus_msgs[NUM_PES][4];    // Contadores de mensajes enviados por PE

    // Cola de peticiones para arbitraje FIFO
    BusRequest *req_head;
    BusRequest *req_tail;

    // Sincronización de la cola / dispatcher
    pthread_mutex_t queue_lock;
    pthread_cond_t  queue_cv;
    pthread_t       dispatcher_thread;
    int             running;
} Bus;

// Prototipos de funciones
void bus_init(Bus* bus, Cache* caches[]);
void bus_broadcast(Bus* bus, BusMsg msg, int addr, int src_pe); // bloquea hasta procesar
void bus_print_metrics(Bus* bus);
void debug_print_bus_cache_states(Bus* bus);
void bus_destroy(Bus* bus); // parar dispatcher (recomendado llamar antes de exit)

#endif // BUS_H
