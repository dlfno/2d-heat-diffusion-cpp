#include <mpi.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>

// Mismos parámetros pesados para comparar con OpenMP
const int N = 2000;
const int MAX_ITER = 4000;
const double ALPHA = 0.1;

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank); // ¿Quién soy? (0, 1, 2...)
    MPI_Comm_size(MPI_COMM_WORLD, &size); // ¿Cuántos somos?

    // 1. Descomposición de Dominio
    // Dividimos las filas N entre el número de procesos
    int rows_per_rank = N / size;
    
    // Manejo de residuo (si N no es divisible por size, el último se lleva el resto)
    int local_rows = (rank == size - 1) ? (N - rank * (N / size)) : rows_per_rank;

    // 2. Memoria Local
    // Necesitamos local_rows + 2 filas fantasma (una arriba, una abajo)
    // El tamaño total del vector local es: (local_rows + 2) * N
    std::vector<double> u_local((local_rows + 2) * N, 0.0);
    std::vector<double> u_next((local_rows + 2) * N, 0.0);

    // 3. Condiciones de Frontera (Globales)
    // Solo necesitamos establecer la pared izquierda caliente.
    // Como cada rank tiene todo el ancho (N columnas), todos lo hacen.
    for (int i = 1; i <= local_rows; ++i) {
        u_local[i * N + 0] = 100.0;
        u_next[i * N + 0] = 100.0;
    }

    // Cronómetro (solo el maestro cuenta)
    double start_time;
    if (rank == 0) {
        std::cout << "Iniciando simulacion MPI con " << size << " procesos..." << std::endl;
        start_time = MPI_Wtime();
    }

    // 4. Bucle Principal
    for (int t = 0; t < MAX_ITER; ++t) {
        
        // --- COMUNICACIÓN DE GHOST ROWS (Halo Exchange) ---
        // Envío mi fila 1 al vecino de ARRIBA, recibo en mi fila fantasma inferior (local_rows + 1)
        // Envío mi fila local_rows al vecino de ABAJO, recibo en mi fila fantasma superior (0)
        
        MPI_Request reqs[4];
        int n_reqs = 0;

        // Enviar hacia ARRIBA (hacia rank-1) y recibir de él
        if (rank > 0) {
            // Envío mi primera fila real (índice 1)
            MPI_Isend(&u_local[1 * N], N, MPI_DOUBLE, rank - 1, 0, MPI_COMM_WORLD, &reqs[n_reqs++]);
            // Recibo en mi fila fantasma superior (índice 0)
            MPI_Irecv(&u_local[0], N, MPI_DOUBLE, rank - 1, 1, MPI_COMM_WORLD, &reqs[n_reqs++]);
        }

        // Enviar hacia ABAJO (hacia rank+1) y recibir de él
        if (rank < size - 1) {
            // Envío mi última fila real (índice local_rows)
            MPI_Isend(&u_local[local_rows * N], N, MPI_DOUBLE, rank + 1, 1, MPI_COMM_WORLD, &reqs[n_reqs++]);
            // Recibo en mi fila fantasma inferior (índice local_rows + 1)
            MPI_Irecv(&u_local[(local_rows + 1) * N], N, MPI_DOUBLE, rank + 1, 0, MPI_COMM_WORLD, &reqs[n_reqs++]);
        }

        // Esperar a que toda la comunicación termine antes de calcular
        MPI_Waitall(n_reqs, reqs, MPI_STATUSES_IGNORE);


        // --- CÁLCULO (Stencil) ---
        // Iteramos solo sobre las filas REALES (de 1 a local_rows)
        for (int i = 1; i <= local_rows; ++i) {
            // Conversión de coordenadas:
            // Mi fila local 'i' corresponde a la fila global 'rank * rows_per_rank + (i-1)'
            // Esto sirve para saber si estoy en el borde superior o inferior global del plato.
            int global_row = rank * (N/size) + (i - 1); 

            // Si soy la primerísima fila global o la ultimísima, no calculo (fronteras fijas 0 grados)
            if (global_row == 0 || global_row == N - 1) continue;

            for (int j = 1; j < N - 1; ++j) {
                int idx = i * N + j;
                
                double u_up    = u_local[(i - 1) * N + j]; // Accede a ghost row si i=1
                double u_down  = u_local[(i + 1) * N + j]; // Accede a ghost row si i=local_rows
                double u_left  = u_local[i * N + (j - 1)];
                double u_right = u_local[i * N + (j + 1)];
                double u_center = u_local[idx];

                u_next[idx] = u_center + ALPHA * (u_up + u_down + u_left + u_right - 4 * u_center);
            }
        }
        
        // Swap local
        std::swap(u_local, u_next);
        
        // Re-imponer condiciones de frontera izquierda (en caso de que la difusión la borre)
        for (int i = 1; i <= local_rows; ++i) {
            u_local[i * N + 0] = 100.0;
        }
    }

    // 5. Recolección de Datos (Gather)
    // El rank 0 debe recibir los pedazos de todos y armar la matriz final.
    // NOTA: Para matrices gigantes esto no es eficiente, pero para N=2000 está bien.
    
    std::vector<double> u_final;
    if (rank == 0) {
        u_final.resize(N * N);
    }

    // Primero copiamos mis propios datos a un buffer limpio (sin ghost rows)
    std::vector<double> my_clean_data(local_rows * N);
    for(int i=0; i < local_rows; ++i) {
         for(int j=0; j<N; ++j) {
             my_clean_data[i*N + j] = u_local[(i+1)*N + j];
         }
    }

    // MPI_Gather asume que todos envían la misma cantidad.
    // Simplificación: Asumimos que N es divisible por size para el Gather final.
    // Si no, la lógica se complica mucho (MPI_Gatherv). 
    // Para este ejercicio, usa N=2000 y np=4 (divisible).
    
    MPI_Gather(my_clean_data.data(), local_rows * N, MPI_DOUBLE,
               u_final.data(), local_rows * N, MPI_DOUBLE,
               0, MPI_COMM_WORLD);

    if (rank == 0) {
        double end_time = MPI_Wtime();
        std::cout << "Simulacion MPI terminada en: " << end_time - start_time << " segundos." << std::endl;
        
        // Guardar CSV (reutilizamos la logica, puedes copiar la funcion save_to_csv aqui si quieres)
        // O simplemente imprimir el tiempo, ya validamos la física.
    }

    MPI_Finalize();
    return 0;
}
