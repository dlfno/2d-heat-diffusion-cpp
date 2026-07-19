#!/usr/bin/env python3
"""
Genera un gráfico de speedup (y eficiencia paralela) a partir de los tiempos
medidos por scripts/run_benchmark.sh.

Entrada : output/benchmark.csv  con columnas  impl,workers,time_s
Salida  : output/speedup.png

Speedup(p)    = T_serial / T_paralelo(p)
Eficiencia(p) = Speedup(p) / p
"""
import csv
import os
import sys
from collections import defaultdict

import matplotlib

matplotlib.use("Agg")  # backend sin ventana (headless)
import matplotlib.pyplot as plt

CSV_PATH = sys.argv[1] if len(sys.argv) > 1 else "output/benchmark.csv"
OUT_PATH = sys.argv[2] if len(sys.argv) > 2 else "output/speedup.png"

# Paleta Okabe-Ito: segura para daltonismo. Gris para la referencia ideal.
COLORS = {"openmp": "#0072B2", "mpi": "#D55E00"}
LABELS = {"openmp": "OpenMP (memoria compartida)", "mpi": "MPI (memoria distribuida)"}
IDEAL = "#8C8C8C"


def load(csv_path):
    """Devuelve (t_serial, {impl: [(workers, time_s), ...]})."""
    series = defaultdict(list)
    t_serial = None
    with open(csv_path, newline="") as f:
        for row in csv.DictReader(f):
            impl = row["impl"].strip().lower()
            workers = int(row["workers"])
            t = float(row["time_s"])
            if impl == "serial":
                t_serial = t
            else:
                series[impl].append((workers, t))
    if t_serial is None:
        sys.exit("Error: no se encontró una fila 'serial' en el CSV.")
    for impl in series:
        series[impl].sort()
    return t_serial, series


def main():
    if not os.path.exists(CSV_PATH):
        sys.exit(f"No existe {CSV_PATH}. Ejecuta primero: make bench")

    t_serial, series = load(CSV_PATH)
    max_workers = max(w for pts in series.values() for w, _ in pts)

    fig, (ax_s, ax_e) = plt.subplots(1, 2, figsize=(13, 5.2))
    fig.suptitle(
        "Escalabilidad fuerte — Difusión de Calor 2D  (grid 2000×2000, 4000 iter)",
        fontsize=14, fontweight="bold",
    )

    # --- Panel 1: Speedup ---
    ax_s.plot([1, max_workers], [1, max_workers], "--", color=IDEAL,
              lw=1.5, label="Ideal (lineal)", zorder=1)
    # OpenMP etiqueta arriba, MPI abajo, para que no se solapen cuando coinciden.
    dy = {"openmp": 10, "mpi": -16}
    for impl in ("openmp", "mpi"):
        if impl not in series:
            continue
        xs = [w for w, _ in series[impl]]
        sp = [t_serial / t for _, t in series[impl]]
        ax_s.plot(xs, sp, "-o", color=COLORS[impl], lw=2, ms=7,
                  label=LABELS[impl], zorder=3)
        for x, y in zip(xs, sp):
            ax_s.annotate(f"{y:.2f}×", (x, y), textcoords="offset points",
                          xytext=(0, dy[impl]), ha="center", fontsize=9,
                          color=COLORS[impl], fontweight="bold")

    ax_s.set_title("Speedup", fontsize=12)
    ax_s.set_xlabel("Núcleos / procesos")
    ax_s.set_ylabel("Speedup  (T$_1$ / T$_p$)")
    ax_s.set_xticks(range(1, max_workers + 1))
    ax_s.grid(True, alpha=0.3)
    ax_s.legend(frameon=False, loc="upper left")

    # --- Panel 2: Eficiencia paralela ---
    ax_e.axhline(1.0, ls="--", color=IDEAL, lw=1.5, label="Ideal (100%)", zorder=1)
    for impl in ("openmp", "mpi"):
        if impl not in series:
            continue
        xs = [w for w, _ in series[impl]]
        eff = [(t_serial / t) / w for w, t in series[impl]]
        ax_e.plot(xs, eff, "-o", color=COLORS[impl], lw=2, ms=7,
                  label=LABELS[impl], zorder=3)

    ax_e.set_title("Eficiencia paralela", fontsize=12)
    ax_e.set_xlabel("Núcleos / procesos")
    ax_e.set_ylabel("Eficiencia  (Speedup / p)")
    ax_e.set_xticks(range(1, max_workers + 1))
    ax_e.yaxis.set_major_formatter(lambda v, _: f"{v*100:.0f}%")
    ax_e.grid(True, alpha=0.3)
    ax_e.legend(frameon=False, loc="lower left")

    fig.tight_layout(rect=(0, 0, 1, 0.95))
    fig.savefig(OUT_PATH, dpi=150)
    print(f"Gráfico guardado en {OUT_PATH}")


if __name__ == "__main__":
    main()
