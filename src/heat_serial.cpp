#include <iostream>
#include <vector>
#include <fstream>
#include <iomanip>
#include <chrono> // Para medir tiempo

// Parámetros de la simulación
const int N = 2000;          // Tamaño de la grilla (1000x1000)
// const int MAX_ITER = 20000;  // Para la imágen muestra
const int MAX_ITER = 4000;   // Pasos de tiempo (se usó para las mediciones)
const double ALPHA = 0.1;    // Coeficiente de difusión ficticio (para estabilidad)

// Función para guardar resultados en CSV
void save_to_csv(const std::vector<double>& grid, const std::string& filename) {
    std::ofstream file(filename);
    for (int i = 0; i < N; ++i) {
        for (int j = 0; j < N; ++j) {
            file << grid[i * N + j];
            if (j < N - 1) file << ",";
        }
        file << "\n";
    }
    std::cout << "Resultados guardados en " << filename << std::endl;
}

int main() {
    // 1. Inicialización de memoria (Flattened arrays)
    std::vector<double> u(N * N, 0.0);      // Matriz actual (Tiempo n)
    std::vector<double> u_next(N * N, 0.0); // Matriz siguiente (Tiempo n+1)

    // // 2. Condiciones Iniciales: Círculo Caliente en el Centro
    // int center_x = N / 2;
    // int center_y = N / 2;
    // int radius = N / 8; // El radio será 1/8 del tamaño de la placa

    // std::cout << "Inicializando con un punto caliente central..." << std::endl;

    // for (int i = 0; i < N; ++i) {
    //     for (int j = 0; j < N; ++j) {
    //         // Ecuación del círculo: (x-x0)^2 + (y-y0)^2 < r^2
    //         // Usamos 'long long' para evitar desbordamiento en N grandes
    //         long long dist_sq = (long long)(i - center_x)*(i - center_x) + 
    //                             (long long)(j - center_y)*(j - center_y);
            
    //         if (dist_sq < (long long)radius * radius) {
    //             u[i * N + j] = 100.0;
    //             u_next[i * N + j] = 100.0;
    //         } else {
    //             // Asegurar que el resto es 0
    //             u[i * N + j] = 0.0;
    //             u_next[i * N + j] = 0.0;
    //         }
    //     }
    // }


    
    // 2. Condiciones de Frontera Iniciales (Para la imágen muestra)
    // Ejemplo: Pared izquierda (j=0) caliente a 100.0 grados
    for (int i = 0; i < N; ++i) {
        u[i * N + 0] = 100.0;
        u_next[i * N + 0] = 100.0; // Mantener la condición en el buffer siguiente también
    }

    std::cout << "Iniciando simulacion serial..." << std::endl;
    std::cout << "Grid: " << N << "x" << N << " | Iteraciones: " << MAX_ITER << std::endl;

    auto start_time = std::chrono::high_resolution_clock::now();

    // 3. Bucle Principal de Tiempo
    for (int t = 0; t < MAX_ITER; ++t) {
        
        // Bucle Espacial (evitamos los bordes para no salirnos de la memoria)
        for (int i = 1; i < N - 1; ++i) {
            for (int j = 1; j < N - 1; ++j) {
                int idx = i * N + j;
                
                // Stencil de 5 puntos (Arriba, Abajo, Izq, Der, Centro)
                double u_up    = u[(i - 1) * N + j];
                double u_down  = u[(i + 1) * N + j];
                double u_left  = u[i * N + (j - 1)];
                double u_right = u[i * N + (j + 1)];
                double u_center = u[idx];

                // Ecuación de calor discretizada (simplificada con ALPHA)
                // T_new = T_old + alpha * (vecinos - 4*centro)
                u_next[idx] = u_center + ALPHA * (u_up + u_down + u_left + u_right - 4 * u_center);
            }
        }

        // Pointer Swapping (Simulado con vectores)
        // En C++ moderno, swap es O(1) porque solo intercambia punteros internos
        std::swap(u, u_next);
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end_time - start_time;

    std::cout << "Simulacion terminada en: " << elapsed.count() << " segundos." << std::endl;

    // 4. Guardar resultados
    save_to_csv(u, "output/serial_result.csv");

    return 0;
}
