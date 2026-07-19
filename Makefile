# Makefile para 2D Heat Equation
# Ajusta el compilador
CXX = g++-15
MPICXX = mpic++
# Usa el venv local (uwu/) si existe; si no, python3 del sistema.
# Puedes forzarlo:  make plot PYTHON=/ruta/a/python
PYTHON ?= $(shell if [ -x uwu/bin/python ]; then echo uwu/bin/python; else echo python3; fi)

# Flags de compilación: -O3 (optimización máxima) es vital en HPC
CXXFLAGS = -O3 -Wall -std=c++17
OMPFLAGS = -fopenmp

# Nombres de ejecutables
TARGET_SERIAL = heat_serial
TARGET_OMP = heat_omp
TARGET_MPI = heat_mpi

.PHONY: all serial omp mpi bench plot clean

all: serial omp mpi

serial: src/heat_serial.cpp
	$(CXX) $(CXXFLAGS) -o $(TARGET_SERIAL) src/heat_serial.cpp

omp: src/heat_omp.cpp
	$(CXX) $(CXXFLAGS) $(OMPFLAGS) -o $(TARGET_OMP) src/heat_omp.cpp

mpi: src/heat_mpi.cpp
	$(MPICXX) $(CXXFLAGS) -o $(TARGET_MPI) src/heat_mpi.cpp

# Ejecuta el benchmark completo (serial + OpenMP + MPI) y escribe output/benchmark.csv
bench: all
	@mkdir -p output
	./scripts/run_benchmark.sh

# Genera el gráfico de speedup a partir de output/benchmark.csv
plot:
	@mkdir -p output
	$(PYTHON) scripts/plot_speedup.py

clean:
	rm -f $(TARGET_SERIAL) $(TARGET_OMP) $(TARGET_MPI) output/*.csv output/*.png
