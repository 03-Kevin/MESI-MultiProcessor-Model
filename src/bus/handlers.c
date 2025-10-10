// src/bus/handlers.c
#include "handlers.h"
#include "../memory/memory.h"
#include "../cache/cache.h"
#include <stdio.h>

// ================
// REGISTRO DE HANDLERS
// ================
void bus_register_handlers(Bus* bus) {
    bus->handlers[BUS_RD]   = handle_busrd;
    bus->handlers[BUS_RDX]  = handle_busrdx;
    bus->handlers[BUS_UPGR] = handle_busupgr;
    bus->handlers[BUS_WB]   = handle_buswb;
}

// ================
// HANDLER: BusRd
// ================
void handle_busrd(Bus* bus, int addr, int src_pe) {
    printf("[BUS] Ejecutando handler BusRd (PE%d, addr=%d)\n", src_pe, addr);
    int found = 0;
    for (int i = 0; i < NUM_PES; i++) {
        if (i == src_pe) continue;
        CacheLine* line = cache_get_line(bus->caches[i], addr);
        if (line) {
            found = 1;
            if (line->state == M) {
                mem_write(addr, line->data[0]);
                line->state = S;
            } else if (line->state == E) {
                line->state = S;
            }
        }
    }
    if (!found) {
        printf("[BUS] Línea no encontrada → leyendo de memoria\n");
        mem_read(addr);
    }
}

// ================
// HANDLER: BusRdX
// ================
void handle_busrdx(Bus* bus, int addr, int src_pe) {
    printf("[BUS] Ejecutando handler BusRdX (PE%d, addr=%d)\n", src_pe, addr);
    for (int i = 0; i < NUM_PES; i++) {
        if (i == src_pe) continue;
        CacheLine* line = cache_get_line(bus->caches[i], addr);
        if (line) {
            if (line->state == M)
                mem_write(addr, line->data[0]);
            line->state = I;
        }
    }
}

// ================
// HANDLER: BusUpgr
// ================
void handle_busupgr(Bus* bus, int addr, int src_pe) {
    printf("[BUS] Ejecutando handler BusUpgr (PE%d, addr=%d)\n", src_pe, addr);
    for (int i = 0; i < NUM_PES; i++) {
        if (i == src_pe) continue;
        CacheLine* line = cache_get_line(bus->caches[i], addr);
        if (line) line->state = I;
    }
}

// ================
// HANDLER: BusWB
// ================
void handle_buswb(Bus* bus, int addr, int src_pe) {
    printf("[BUS] Ejecutando handler BusWB (PE%d, addr=%d)\n", src_pe, addr);
    CacheLine* line = cache_get_line(bus->caches[src_pe], addr);
    if (line && line->state == M) {
        mem_write(addr, line->data[0]);
        line->state = S;
    }
}
