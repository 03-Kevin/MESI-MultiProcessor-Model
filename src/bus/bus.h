#ifndef BUS_H
#define BUS_H

#include "memory/memory.h"
#include "cache/cache.h"

#define NUM_PES 4

typedef enum { BUS_RD, BUS_RDX, BUS_UPGR, BUS_WB } BusMsg;

struct Bus; // Declaración adelantada

// Tipo de puntero a función handler
typedef void (*BusHandler)(struct Bus* bus, int addr, int src_pe);

// Estructura principal del bus
typedef struct Bus {
    Cache* caches[NUM_PES];
    BusHandler handlers[4];  // Dispatch table
} Bus;

// Funciones públicas
void bus_init(Bus* bus, Cache* caches[]);
void bus_broadcast(Bus* bus, BusMsg msg, int addr, int src_pe);

#endif
