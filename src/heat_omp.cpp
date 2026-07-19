#include <iostream>
#include <vector>
#include <fstream>
#include <chrono>
#include <cstdlib> // Para getenv / atoi
#include <omp.h>   // <--- NECESARIO PARA OPENMP

static int env_int(const char* name, int def) {
    const char* v = std::getenv(name);
    return (v && *v) ? std::atoi(v) : def;
}

// Parámetros (Mismos que serial para comparación justa)
const int N = env_int("HEAT_N", 2000);
const int MAX_ITER = env_int("HEAT_ITER", 4000);
const double ALPHA = 0.1;

int main() {
    // Aplanar matriz (1D array para simular 2D)
    std::vector<double> u(N * N, 0.0);
    std::vector<double> u_next(N * N, 0.0);

    // Condición de frontera inicial
    for (int i = 0; i < N; ++i) {
        u[i * N + 0] = 100.0;
        u_next[i * N + 0] = 100.0;
    }

    std::cout << "Iniciando simulacion OpenMP..." << std::endl;
    std::cout << "Hilos Maximos Disponibles: " << omp_get_max_threads() << std::endl;
    
    auto start_time = std::chrono::high_resolution_clock::now();

    for (int t = 0; t < MAX_ITER; ++t) {
        
        // --- PARALELIZACIÓN ---
        // #pragma omp parallel for: reparte las filas (bucle i) entre los hilos.
        // schedule(static): bloques fijos y contiguos de filas -> buena localidad
        //   de caché y balanceo perfecto para esta carga uniforme.
        //
        // NOTA: se paraleliza SOLO el bucle externo (i), a propósito.
        // Añadir collapse(2) fusiona i y j en un único índice: el compilador debe
        // recuperar i,j con división/módulo por celda y, peor aún, PIERDE la
        // vectorización SIMD del bucle interno j. Medido, collapse(2) hacía la
        // versión de 1 hilo ~2x más lenta que el serial. Sin collapse el bucle j
        // se auto-vectoriza y cada hilo procesa filas completas de forma secuencial.
        #pragma omp parallel for schedule(static)
        for (int i = 1; i < N - 1; ++i) {
            for (int j = 1; j < N - 1; ++j) {
                int idx = i * N + j;
                
                double u_up    = u[(i - 1) * N + j];
                double u_down  = u[(i + 1) * N + j];
                double u_left  = u[i * N + (j - 1)];
                double u_right = u[i * N + (j + 1)];
                double u_center = u[idx];

                u_next[idx] = u_center + ALPHA * (u_up + u_down + u_left + u_right - 4 * u_center);
            }
        }

        // Intercambio de punteros (No se puede paralelizar, es muy rápido)
        std::swap(u, u_next);
        
        // Re-aplicar condiciones de frontera si es necesario (en este caso simple, no cambia mucho)
        // Pero en simulaciones complejas, a veces hay que asegurar los bordes aquí.
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end_time - start_time;

    std::cout << "Simulacion OpenMP terminada en: " << elapsed.count() << " segundos." << std::endl;

    // No guardamos CSV en el test de velocidad para no perder tiempo en I/O
    // save_to_csv(u, "output/omp_result.csv"); 

    return 0;
}
