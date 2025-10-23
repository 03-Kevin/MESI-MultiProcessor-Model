#ifndef CACHE_H
#define CACHE_H

#include "../include/config.h"
#include "../mesi/mesi.h"
#include <pthread.h>

// Forward declaration para evitar dependencia circular
struct Bus;
typedef struct Bus Bus;

// Definición de una línea de caché
typedef struct {
    unsigned long tag;
    MESI_State state;
    double data[DOUBLES_PER_LINE]; // Palabras dentro de la línea
    int valid;
    int dirty;
    int age; // Nuevo campo para rastrear el uso reciente (LRU)
} CacheLine;

// Definición de un set de caché (para set-associative)
typedef struct {
    CacheLine lines[WAYS];
} CacheSet;

// Definición principal de la caché
typedef struct Cache {
    Bus* bus;             // Puntero al bus para coherencia MESI
    CacheSet sets[SETS];  // Conjunto de sets de la caché
    pthread_mutex_t lock; // Mutex para operaciones concurrentes

    // Métricas de simulación
    unsigned long read_misses;
    unsigned long write_misses;
    unsigned long invalidations_received;
    unsigned long transitions[4]; // Conteo de transiciones a M/E/S/I
    unsigned long writebacks;
    unsigned long total_reads;    // Total de lecturas realizadas
    unsigned long total_writes;   // Total de escrituras realizadas
    unsigned long read_hits;      // Total de lecturas que fueron hits
    unsigned long write_hits;     // Total de escrituras que fueron hits
} Cache;

// Prototipos de funciones
void cache_init(Cache* cache);
double cache_read(Cache* cache, int addr, int pe_id);
void cache_write(Cache* cache, int addr, double value, int pe_id);
CacheLine* cache_get_line(Cache* cache, int addr);
void cache_flush(Cache *cache);

void cache_print_metrics(Cache *cache, int pe_id);

#endif // CACHE_H