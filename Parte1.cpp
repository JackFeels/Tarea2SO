#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <fstream>
#include <cstdlib>
#include <chrono>
#include <atomic>
#include <sstream>

// Clase para la cola circular dinámica
class ColaCircular {
private:
    std::vector<int> cola;                // Cola circular dinámica
    std::mutex mtx;                       // Mutex para proteger la cola
    std::condition_variable cv_productor; // Variable de condición para productores
    std::condition_variable cv_consumidor; // Variable de condición para consumidores
    size_t tam_max;                       // Tamaño máximo de la cola
    size_t inicio, fin;                   // Índices de inicio y fin
    size_t elementos;                     // Número de elementos en la cola
    std::ofstream log;                    // Archivo log

    void registrarCambioTamano(const std::string &mensaje) {
        std::lock_guard<std::mutex> lock(mtx);
        log << mensaje << std::endl;
    }

    void duplicarTamano() {
        tam_max *= 2;
        cola.resize(tam_max);
        log << "La cola se duplicó a tamaño " << tam_max << "." << std::endl;
    }

    void reducirTamano() {
        tam_max /= 2;
        cola.resize(tam_max);
        log << "La cola se redujo a tamaño " << tam_max << "." << std::endl;
    }

public:
    explicit ColaCircular(size_t tam, const std::string &log_file) 
        : tam_max(tam), inicio(0), fin(0), elementos(0) {
        cola.resize(tam_max);
        log.open(log_file, std::ios::out);
        if (!log.is_open()) {
            throw std::runtime_error("No se pudo abrir el archivo de log.");
        }
    }

    ~ColaCircular() {
        log.close();
    }

    void agregar(int item) {
        std::unique_lock<std::mutex> lock(mtx);
        cv_productor.wait(lock, [this]() { return elementos < tam_max; });

        cola[fin] = item;
        fin = (fin + 1) % tam_max;
        ++elementos;

        if (elementos == tam_max) {
            duplicarTamano();
        }

        log << "Productor agregó: " << item << ". Elementos en cola: " << elementos << std::endl;
        cv_consumidor.notify_one(); // Notifica a un consumidor que hay un item disponible
    }

    bool extraer(int &item, int tiempo_espera) {
        std::unique_lock<std::mutex> lock(mtx);
        if (!cv_consumidor.wait_for(lock, std::chrono::seconds(tiempo_espera), [this]() { return elementos > 0; })) {
            return false; // Tiempo de espera agotado
        }

        item = cola[inicio];
        inicio = (inicio + 1) % tam_max;
        --elementos;

        if (tam_max > 2 && elementos <= tam_max / 4) {
            reducirTamano();
        }

        log << "Consumidor extrajo: " << item << ". Elementos en cola: " << elementos << std::endl;
        cv_productor.notify_one(); // Notifica a un productor que hay espacio disponible
        return true;
    }
};

void productor(ColaCircular &cola, int id) {
    for (int i = 0; i < 10; ++i) {
        int item = id * 100 + i; // Generar un item único
        cola.agregar(item);
        std::this_thread::sleep_for(std::chrono::milliseconds(rand() % 100));
    }
}

void consumidor(ColaCircular &cola, int id, int tiempo_espera, std::atomic<bool> &terminado) {
    while (!terminado) {
        int item;
        if (!cola.extraer(item, tiempo_espera)) {
            // Tiempo agotado
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(rand() % 150));
    }
}

int main(int argc, char *argv[]) {
    if (argc != 9) {
        std::cerr << "Uso: " << argv[0] << " -p <productores> -c <consumidores> -s <tam_inicial> -t <tiempo_espera>" << std::endl;
        return 1;
    }

    int num_productores = 0, num_consumidores = 0, tam_inicial = 0, tiempo_espera = 0;
    for (int i = 1; i < argc; i += 2) {
        std::string flag = argv[i];
        if (flag == "-p") num_productores = std::stoi(argv[i + 1]);
        else if (flag == "-c") num_consumidores = std::stoi(argv[i + 1]);
        else if (flag == "-s") tam_inicial = std::stoi(argv[i + 1]);
        else if (flag == "-t") tiempo_espera = std::stoi(argv[i + 1]);
    }

    std::string log_file = "simulacion.log";
    ColaCircular cola(tam_inicial, log_file);
    std::atomic<bool> terminado(false);

    // Crear hebras productoras y consumidoras
    std::vector<std::thread> productores;
    std::vector<std::thread> consumidores;

    for (int i = 0; i < num_productores; ++i) {
        productores.emplace_back(productor, std::ref(cola), i + 1);
    }

    for (int i = 0; i < num_consumidores; ++i) {
        consumidores.emplace_back(consumidor, std::ref(cola), i + 1, tiempo_espera, std::ref(terminado));
    }

    // Esperar a que terminen todos los productores
    for (auto &prod : productores) {
        prod.join();
    }

    // Notificar a los consumidores que no habrá más elementos
    terminado = true;
    for (auto &cons : consumidores) {
        cons.join();
    }

    std::cout << "Simulación completada. Revisa el archivo " << log_file << " para más detalles." << std::endl;
    return 0;
}
