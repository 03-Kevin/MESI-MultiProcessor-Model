#include "include/config.h"
#include "pe/pe.h"
#include "bus/bus.h"
#include "memory/memory.h"
#include "cache/cache.h"
#include <stdio.h>
#include <pthread.h>
#include <time.h>

// Función para imprimir las métricas de las cachés
void print_cache_metrics(Cache *caches)
{
    printf("\n[MÉTRICAS DE CACHÉS]\n");
    for (int i = 0; i < NUM_PES; i++)
    {
        printf("\n[MÉTRICAS - PE%d]\n", i);
        printf("Lecturas fallidas (read misses): %lu\n", caches[i].read_misses);
        printf("Escrituras fallidas (write misses): %lu\n", caches[i].write_misses);
        printf("Writebacks realizados: %lu\n", caches[i].writebacks);
        printf("Invalidaciones recibidas: %lu\n", caches[i].invalidations_received);
        printf("Transiciones de estado MESI:\n");
        printf("  - Modified (M): %lu\n", caches[i].transitions[M]);
        printf("  - Exclusive (E): %lu\n", caches[i].transitions[E]);
        printf("  - Shared (S): %lu\n", caches[i].transitions[S]);
        printf("  - Invalid (I): %lu\n", caches[i].transitions[I]);
    }
}

// Función para cargar vectores en memoria con alineación
void load_vectors(double *memory, double *vector_a, double *vector_b, size_t vector_size)
{
    // Cargar vector A
    mem_load_data(memory, VECTOR_A, 0, vector_a, vector_size, ALIGNMENT);

    // Cargar vector B
    mem_load_data(memory, VECTOR_B, 0, vector_b, vector_size, ALIGNMENT);

    printf("[MAIN] Vectores A y B cargados en memoria con alineación.\n");
}

int main()
{
    // Crear un nombre único para el archivo de salida
    char filename[64];
    time_t now = time(NULL);
    strftime(filename, sizeof(filename), "%Y%m%d_%H%M%S_output.txt", localtime(&now));

    // Redirigir stdout y stderr al archivo
    FILE *output_file = freopen(filename, "w", stdout);
    if (!output_file)
    {
        perror("Error al abrir el archivo de salida");
        return 1;
    }
    freopen(filename, "a", stderr);

    mem_init();
    printf("[MAIN] Memoria inicializada.\n");

    // Inicializar SUMS a 0.0 para cada PE
    for (int i = 0; i < NUM_PES; i++)
    {
        mem_write(SUMS, i, 0.0); // Escribe 0.0 en la dirección correspondiente a SUMS[i]
        printf("[MAIN] SUMS[%d] inicializado a 0.0\n", i);
    }

    // Crear vectores de entrada
    double vector_a[VECTOR_SIZE];
    double vector_b[VECTOR_SIZE];

    // Llenar los vectores con datos de prueba
    for (int i = 0; i < VECTOR_SIZE; i++)
    {
        vector_a[i] = (double)(i + 1); // Vector A: 1, 2, 3, ...
        vector_b[i] = (double)(i + 2); // Vector B: 2, 3, 4, ...
    }

    // Cargar los vectores en la memoria principal
    load_vectors(main_memory, vector_a, vector_b, VECTOR_SIZE);

    // Verificar los valores inicializados
    for (int i = 0; i < VECTOR_SIZE; i++)
    {
        printf("A[%d] = %f, B[%d] = %f\n", i, mem_read(VECTOR_A, i), i, mem_read(VECTOR_B, i));
    }

    Bus bus;
    Cache caches[NUM_PES];
    PE pes[NUM_PES];
    pthread_t threads[NUM_PES];

    for (int i = 0; i < NUM_PES; i++)
    {
        cache_init(&caches[i]);
        caches[i].bus = &bus;
        printf("[MAIN] Caché %d inicializada.\n", i);
    }

    Cache *cache_ptrs[NUM_PES];
    for (int i = 0; i < NUM_PES; i++)
        cache_ptrs[i] = &caches[i];
    bus_init(&bus, cache_ptrs);
    printf("[MAIN] Bus inicializado.\n");

    for (int i = 0; i < NUM_PES; i++)
    {
        pes[i].id = i;
        pes[i].cache = &caches[i];
        pthread_create(&threads[i], NULL, pe_run, &pes[i]);
        printf("[MAIN] PE%d ejecutándose.\n", i);
    }

    for (int i = 0; i < NUM_PES; i++)
    {
        pthread_join(threads[i], NULL);
        printf("[MAIN] Hilo de PE%d finalizado.\n", i);
    }

    // Imprimir estados finales de las cachés antes del flush
    debug_print_bus_cache_states(&bus); // Se actualizó el nombre de la función

    // Flushing caches antes de leer resultados
    printf("\n[MAIN] Flushing caches antes de leer resultados...\n");
    for (int i = 0; i < NUM_PES; i++)
    {
        cache_flush(&caches[i]);
    }

    // Leer resultados finales usando segmentación
    double resultado_final = 0.0;
    for (int i = 0; i < NUM_PES; i++)
    {
        double parcial = mem_read(SUMS, i);
        printf("[MAIN] parcial[%d] = %f\n", i, parcial);
        resultado_final += parcial;
    }

    printf("[MAIN] Producto punto calculado: %.6f\n", resultado_final);

    printf("VECTOR_A_ADDR: %d\n", VECTOR_A_ADDR);
    printf("VECTOR_B_ADDR: %d\n", VECTOR_B_ADDR);
    printf("SUMS_ADDR: %d\n", SUMS_ADDR);
    printf("DONE_ADDR: %d\n", DONE_ADDR);
    printf("RESULT_ADDR: %d\n", RESULT_ADDR);
    printf("MEM_SIZE: %d\n", MEM_SIZE);

    // Calcular el producto punto esperado
    double resultado_esperado = 0.0;
    for (int i = 0; i < VECTOR_SIZE; i++)
    {
        resultado_esperado += (double)(i + 1) * (double)(i + 2);
    }

    printf("[MAIN] Producto punto esperado: %.6f\n", resultado_esperado);

    if (resultado_final == resultado_esperado)
        printf("[MAIN] ✅ Producto punto correcto.\n");
    else
        printf("[MAIN] ❌ Producto punto incorrecto.\n");

    // Imprimir métricas de las cachés
    print_cache_metrics(caches);

    // Imprimir tráfico del bus
    bus_print_metrics(&bus);

    printf("[MAIN] Fin del programa.\n");

    // Cerrar el archivo de salida
    fclose(output_file);

    return 0;
}