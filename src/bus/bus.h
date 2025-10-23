#ifndef BUS_H
#define BUS_H

#include "../include/config.h"  // Ajusta ruta si hace falta

// Forward declaration de Cache
struct Cache;
typedef struct Cache Cache;

// Enum para mensajes del bus
typedef enum { BUS_RD, BUS_RDX, BUS_UPGR, BUS_WB } BusMsg;

// Estructura Bus
typedef struct Bus {
    Cache* caches[NUM_PES];                       // Punteros a las cachés
    void (*handlers[4])(struct Bus*, int, int);   // Handlers para BUS_RD, BUS_RDX, etc.
    int last_shared;
    unsigned long traffic_count;
    unsigned long per_pe_bus_msgs[NUM_PES][4];    // Contadores de mensajes enviados por PE
} Bus;

// Prototipos de funciones
void bus_init(Bus* bus, Cache* caches[]);
void bus_broadcast(Bus* bus, BusMsg msg, int addr, int src_pe);
void bus_request(Bus* bus, int addr, BusMsg msg, int src_pe);
void bus_print_metrics(Bus* bus);
void debug_print_bus_cache_states(Bus* bus);  // Renombrado para evitar conflictos

#endif // BUS_H