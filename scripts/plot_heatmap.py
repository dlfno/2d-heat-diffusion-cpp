import matplotlib.pyplot as plt
import numpy as np
import sys

def plot_heatmap(csv_path, output_path):
    print(f"Leyendo datos de {csv_path}...")
    try:
        # Cargar datos (puede tardar un poco si el archivo es gigante)
        data = np.loadtxt(csv_path, delimiter=',')

        plt.figure(figsize=(10, 8))
        # 'inferno' o 'hot' son buenos colormaps para calor
        plt.imshow(data, cmap='inferno', origin='upper') 
        plt.colorbar(label='Temperatura')
        plt.title('Difusión de Calor 2D')
        plt.xlabel('X')
        plt.ylabel('Y')

        print(f"Guardando imagen en {output_path}...")
        plt.savefig(output_path)
        plt.close()
        print("¡Listo!")

    except Exception as e:
        print(f"Error: {e}")

if __name__ == "__main__":
    # Rutas por defecto
    csv_file = 'output/serial_result.csv'
    img_file = 'output/heatmap_serial.png'

    plot_heatmap(csv_file, img_file)
