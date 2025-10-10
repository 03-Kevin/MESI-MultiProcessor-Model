#ifndef BUS_H
#define BUS_H

#include "include/config.h"
#include "memory/memory.h"
#include "cache/cache.h"

// Tipos de mensajes del bus
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
