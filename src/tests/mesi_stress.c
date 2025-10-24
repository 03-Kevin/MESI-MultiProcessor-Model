// tests/mesi_stress.c
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

#include "include/config.h"
#include "memory/memory.h"
#include "cache/cache.h"
#include "bus/bus.h"

#define ITERS 5000       // iteraciones por hilo (ajusta para más/menos stress)
#define SLEEP_US_MAX 200 // microsleep máximo entre operaciones

typedef struct {
    int pe_id;
    Cache* cache;
} ThreadArg;

static Bus bus;
static Cache caches[NUM_PES];

void *stress_thread(void *arg) {
    ThreadArg *t = (ThreadArg*)arg;
    int id = t->pe_id;
    Cache *cache = t->cache;
    unsigned int seed = (unsigned int)(time(NULL) ^ (id<<8));

    // elegimos direcciones cercanas para forzar conflictos (3 líneas)
    int base_addr = VECTOR_A_ADDR;
    int addr_count = DOUBLES_PER_LINE * 3;
    if (addr_count <= 0) addr_count = 8;

    for (int it = 0; it < ITERS; ++it) {
        int idx = rand_r(&seed) % addr_count;
        int addr = base_addr + idx;

        int op = rand_r(&seed) % 100;
        if (op < 60) {
            double v = cache_read(cache, addr, id);
            (void)v;
        } else {
            double val = (double)(id * 1000000 + it);
            cache_write(cache, addr, val, id);
        }

        usleep(rand_r(&seed) % SLEEP_US_MAX);
    }
    return NULL;
}

int main(int argc, char **argv) {
    // === Identificador de salida ===
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char timestamp[32];
    strftime(timestamp, sizeof(timestamp), "%Y%m%d-%H%M%S", tm_info);

    char filename[128];
    if (argc > 1) {
        snprintf(filename, sizeof(filename), "ms_%s.txt", argv[1]);  // ./mesi_stress run01
    } else {
        snprintf(filename, sizeof(filename), "ms_%s.txt", timestamp); // ./mesi_stress
    }

    FILE *log = fopen(filename, "w");
    if (!log) {
        perror("fopen");
        exit(EXIT_FAILURE);
    }

    // Redirigir stdout y stderr al archivo
    dup2(fileno(log), STDOUT_FILENO);
    dup2(fileno(log), STDERR_FILENO);
    setvbuf(stdout, NULL, _IONBF, 0);

    printf("[STRESS] Logging en %s\n", filename);
    printf("[STRESS] Inicializando memoria, caches y bus...\n");

    mem_init();

    // Initialize some data in VECTOR_A/B
    for (int i = 0; i < VECTOR_SIZE; ++i) {
        mem_write(VECTOR_A, i, (double)(i + 1));
        mem_write(VECTOR_B, i, (double)(i + 1000));
    }

    // Inicializar caches y asignar bus
    for (int i = 0; i < NUM_PES; ++i) {
        cache_init(&caches[i]);
        caches[i].bus = &bus;
    }
    Cache *cache_ptrs[NUM_PES];
    for (int i = 0; i < NUM_PES; ++i) cache_ptrs[i] = &caches[i];

    bus_init(&bus, cache_ptrs);

    pthread_t threads[NUM_PES];
    ThreadArg args[NUM_PES];
    for (int i = 0; i < NUM_PES; ++i) {
        args[i].pe_id = i;
        args[i].cache = &caches[i];
        if (pthread_create(&threads[i], NULL, stress_thread, &args[i]) != 0) {
            perror("pthread_create");
            exit(EXIT_FAILURE);
        }
    }

    for (int i = 0; i < NUM_PES; ++i) pthread_join(threads[i], NULL);

    printf("[STRESS] Hilos finalizados. Flushing caches...\n");
    for (int i = 0; i < NUM_PES; ++i) cache_flush(&caches[i]);

    printf("\n[STRESS] Métricas por cache:\n");
    for (int i = 0; i < NUM_PES; ++i) cache_print_metrics(&caches[i], i);

    printf("\n[STRESS] Tráfico del bus:\n");
    bus_print_metrics(&bus);

    printf("\n[STRESS] Estados finales de caches (debug):\n");
    debug_print_bus_cache_states(&bus);

    bus_destroy(&bus);

    printf("[STRESS] Fin.\n");

    fclose(log);
    return 0;
}

