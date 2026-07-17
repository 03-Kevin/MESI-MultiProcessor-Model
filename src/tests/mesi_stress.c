// tests/mesi_stress.c
// Versión del stress test que vuelca stdout/stderr a un fichero:
//   ./mesi_stress run01   -> ms_run01.txt
//   ./mesi_stress          -> ms_YYYYMMDD-HHMMSS.txt
//
// Compilar / ejecutar como antes (se recomienda usar la regla `make mesi_stress`).
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <errno.h>

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
    struct tm tm_storage;
    struct tm *tm_info = localtime_r(&now, &tm_storage);
    if (!tm_info) {
        perror("localtime_r");
        return EXIT_FAILURE;
    }

    char timestamp[32];
    if (strftime(timestamp, sizeof(timestamp), "%Y%m%d-%H%M%S", tm_info) == 0) {
        strncpy(timestamp, "unknown", sizeof(timestamp));
        timestamp[sizeof(timestamp)-1] = '\0';
    }

    char filename[128];
    if (argc > 1) {
        snprintf(filename, sizeof(filename), "ms_%s.txt", argv[1]);  // ./mesi_stress run01
    } else {
        snprintf(filename, sizeof(filename), "ms_%s.txt", timestamp); // ./mesi_stress
    }

    FILE *log = fopen(filename, "w");
    if (!log) {
        fprintf(stderr, "fopen(%s) failed: %s\n", filename, strerror(errno));
        exit(EXIT_FAILURE);
    }

    /* Redirigir stdout y stderr al archivo */
    if (dup2(fileno(log), STDOUT_FILENO) < 0) {
        perror("dup2 stdout");
        fclose(log);
        exit(EXIT_FAILURE);
    }
    if (dup2(fileno(log), STDERR_FILENO) < 0) {
        perror("dup2 stderr");
        fclose(log);
        exit(EXIT_FAILURE);
    }

    /* Opcional: desactivar bufferado para ver logs inmediatamente */
    setvbuf(stdout, NULL, _IONBF, 0);

    printf("[STRESS] Logging en %s\n", filename);
    printf("[STRESS] Inicializando memoria, caches y bus...\n");

    mem_init();

    // Inicializar algunos datos en VECTOR_A/B
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
            bus_destroy(&bus);
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

    /* Recomendado: detener dispatcher limpiamente */
    bus_destroy(&bus);

    printf("[STRESS] Fin.\n");

    /* Cerrar FILE*; stdout/stderr siguen apuntando al descriptor duplicado. */
    fclose(log);

    return 0;
}
