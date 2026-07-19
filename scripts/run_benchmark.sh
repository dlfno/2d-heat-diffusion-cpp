#!/usr/bin/env bash
#
# Benchmark de escalabilidad fuerte (strong scaling) para las tres
# implementaciones. Escribe los resultados en output/benchmark.csv.
#
# Metodología: cada configuración se ejecuta REPS veces, INTERCALANDO las
# corridas (una pasada completa por repetición) y se conserva el MÍNIMO tiempo.
# El mínimo es la medida más limpia: representa la corrida con menos
# interferencia del sistema. Intercalar reduce el sesgo por thermal throttling
# en máquinas de enfriamiento pasivo (p. ej. MacBook Air M1).
#
# Uso:
#   ./scripts/run_benchmark.sh
#
# Variables de entorno (opcionales):
#   HEAT_N     tamaño de la grilla NxN        (default 2000)
#   HEAT_ITER  número de iteraciones          (default 4000)
#   OMP_LIST   lista de hilos para OpenMP     (default "1 2 4 8")
#   MPI_LIST   lista de procesos para MPI     (default "1 2 4 8")
#   REPS       repeticiones por configuración (default 3)
#
set -euo pipefail

cd "$(dirname "$0")/.."
mkdir -p output

HEAT_N="${HEAT_N:-2000}"
HEAT_ITER="${HEAT_ITER:-4000}"
OMP_LIST="${OMP_LIST:-1 2 4 8}"
MPI_LIST="${MPI_LIST:-1 2 4 8}"
REPS="${REPS:-3}"
export HEAT_N HEAT_ITER

CSV="output/benchmark.csv"
RAW="$(mktemp)"
trap 'rm -f "$RAW"' EXIT

# Extrae los segundos de una línea "... en: 23.29 segundos."
extract_time() {
    grep -oE 'en: [0-9]+(\.[0-9]+)?' | tail -1 | grep -oE '[0-9]+(\.[0-9]+)?'
}

# record <impl> <workers> <comando...>
record() {
    local impl="$1" workers="$2"; shift 2
    local t
    t=$("$@" | extract_time)
    echo "$impl $workers $t" >> "$RAW"
    printf '    %-8s %-3s  %ss\n' "$impl" "$workers" "$t"
}

# min_of <impl> <workers> : mínimo tiempo registrado para esa configuración
min_of() {
    awk -v i="$1" -v w="$2" \
        '$1==i && $2==w { if (m=="" || $3<m) m=$3 } END { print m }' "$RAW"
}

echo "==> Benchmark  N=${HEAT_N}  ITER=${HEAT_ITER}  REPS=${REPS}"

for r in $(seq 1 "$REPS"); do
    echo "--> repetición $r/$REPS"
    record serial 1 ./heat_serial
    for n in $OMP_LIST; do
        record openmp "$n" env OMP_NUM_THREADS="$n" ./heat_omp
    done
    for n in $MPI_LIST; do
        record mpi "$n" mpirun --oversubscribe -np "$n" ./heat_mpi
    done
done

# Volcar el mejor (mínimo) tiempo de cada configuración, en orden estable.
{
    echo "impl,workers,time_s"
    echo "serial,1,$(min_of serial 1)"
    for n in $OMP_LIST; do echo "openmp,$n,$(min_of openmp "$n")"; done
    for n in $MPI_LIST; do echo "mpi,$n,$(min_of mpi "$n")"; done
} > "$CSV"

echo "==> Mejores tiempos escritos en $CSV"
