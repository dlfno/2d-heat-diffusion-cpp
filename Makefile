# Makefile para 2D Heat Equation
# Ajusta el compilador 
CXX = g++-15
MPICXX = mpic++

# Flags de compilación: -O3 (optimización máxima) es vital en HPC
CXXFLAGS = -O3 -Wall -std=c++17
OMPFLAGS = -fopenmp

# Nombres de ejecutables
TARGET_SERIAL = heat_serial
TARGET_OMP = heat_omp
TARGET_MPI = heat_mpi

all: serial omp mpi

serial: src/heat_serial.cpp
	$(CXX) $(CXXFLAGS) -o $(TARGET_SERIAL) src/heat_serial.cpp

omp: src/heat_omp.cpp
	$(CXX) $(CXXFLAGS) $(OMPFLAGS) -o $(TARGET_OMP) src/heat_omp.cpp

mpi: src/heat_mpi.cpp
	$(MPICXX) $(CXXFLAGS) -o $(TARGET_MPI) src/heat_mpi.cpp

clean:
	rm -f $(TARGET_SERIAL) $(TARGET_OMP) $(TARGET_MPI) output/*.csv output/*.png
