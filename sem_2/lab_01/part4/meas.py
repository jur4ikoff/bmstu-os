import matplotlib.pyplot as plt
import os

def append_values(filename):
    durations = []
    with open(filename, 'r') as f:
        for line in f:
            line = line.strip()
            if line:
                try:
                    val = float(line) / 1000
                    durations.append(val)
                except ValueError:
                    continue

    return durations
    

def plot_measures(filename_mp, filename_epl):
    if not os.path.exists(filename_mp) or not os.path.exists(filename_epl):
        print(f"Файл не найден!")
        return

    durations_mp = append_values(filename_mp)
    durations_epl = append_values(durations_epl)


    if not durations_mp:
        print("Файл пуст или содержит некорректные данные.")
        return

    plt.figure(figsize=(10, 6))
    plt.plot(durations_mp, marker='o', linestyle='-', markersize=2, color='blue', alpha=0.6, label='mp')
    plt.plot(durations_epl, marker='o', linestyle='-', markersize=2, color='red', alpha=0.6, label='multi')
    
    plt.title('Сравнение на задача производство потребление (многопоточный мультиплекс)')
    plt.xlabel('Номер запроса')
    plt.ylabel('Время (микросекунды)')
    plt.grid(True, which='both', linestyle='--', alpha=0.5)
    
    plt.savefig('performance_graph.png')
    print("График сохранен как performance_graph.png")
    plt.show()

if __name__ == "__main__":
    plot_measures("measures.txt")