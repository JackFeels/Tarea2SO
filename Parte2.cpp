#include <iostream>
#include <fstream>
#include <vector>
#include <unordered_map>
#include <list>
#include <queue>
#include <string>
#include <algorithm>
#include <set>
#include <climits> 

class PageReplacementAlgorithm {
public:
    virtual int simulate(const std::vector<int>& references, int numFrames) = 0;
};

class Optimo : public PageReplacementAlgorithm {
public:
    int simulate(const std::vector<int>& references, int numFrames) override {
        std::set<int> frames;
        std::unordered_map<int, int> nextUse;
        int faults = 0;

        for (size_t i = 0; i < references.size(); ++i) {
            int page = references[i];
            if (frames.find(page) == frames.end()) {
                if (frames.size() >= numFrames) {
                    int pageToRemove = -1, maxDistance = -1;
                    for (int f : frames) {
                        auto it = std::find(references.begin() + i + 1, references.end(), f);
                        int distance = (it == references.end()) ? INT_MAX : it - references.begin();
                        if (distance > maxDistance) {
                            maxDistance = distance;
                            pageToRemove = f;
                        }
                    }
                    frames.erase(pageToRemove);
                }
                frames.insert(page);
                faults++;
            }
        }
        return faults;
    }
};

class FIFO : public PageReplacementAlgorithm {
public:
    int simulate(const std::vector<int>& references, int numFrames) override {
        std::queue<int> frames;
        std::set<int> frameSet;
        int faults = 0;

        for (int page : references) {
            if (frameSet.find(page) == frameSet.end()) {
                if (frames.size() >= numFrames) {
                    int toRemove = frames.front();
                    frames.pop();
                    frameSet.erase(toRemove);
                }
                frames.push(page);
                frameSet.insert(page);
                faults++;
            }
        }
        return faults;
    }
};

class LRU : public PageReplacementAlgorithm {
public:
    int simulate(const std::vector<int>& references, int numFrames) override {
        std::list<int> frames;
        std::set<int> frameSet;
        int faults = 0;

        for (int page : references) {
            if (frameSet.find(page) == frameSet.end()) {
                if (frames.size() >= numFrames) {
                    int toRemove = frames.back();
                    frames.pop_back();
                    frameSet.erase(toRemove);
                }
                faults++;
            } else {
                frames.remove(page);
            }
            frames.push_front(page);
            frameSet.insert(page);
        }
        return faults;
    }
};

class Clock : public PageReplacementAlgorithm {
public:
    int simulate(const std::vector<int>& references, int numFrames) override {
        std::vector<int> frames(numFrames, -1);
        std::vector<bool> useBit(numFrames, false);
        int pointer = 0, faults = 0;

        for (int page : references) {
            auto it = std::find(frames.begin(), frames.end(), page);
            if (it == frames.end()) {
                while (useBit[pointer]) {
                    useBit[pointer] = false;
                    pointer = (pointer + 1) % numFrames;
                }
                frames[pointer] = page;
                useBit[pointer] = true;
                pointer = (pointer + 1) % numFrames;
                faults++;
            } else {
                useBit[it - frames.begin()] = true;
            }
        }
        return faults;
    }
};

void printUsage() {
    std::cout << "Usar: ./mvirtual -m <numFrames> -a <algorithm> -f <file>\n";
}

std::vector<int> loadReferences(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("No se pudo abrir el archivo.");
    }
    std::vector<int> references;
    int page;
    while (file >> page) {
        references.push_back(page);
    }
    return references;
}

int main(int argc, char* argv[]) {
    if (argc != 7) {
        printUsage();
        return 1;
    }

    int numFrames = 0;
    std::string algorithm;
    std::string file;

    for (int i = 1; i < argc; i += 2) {
        std::string arg = argv[i];
        if (arg == "-m") {
            numFrames = std::stoi(argv[i + 1]);
        } else if (arg == "-a") {
            algorithm = argv[i + 1];
        } else if (arg == "-f") {
            file = argv[i + 1];
        }
    }

    std::vector<int> references;
    try {
        references = loadReferences(file);
    } catch (const std::exception& e) {
        std::cerr << e.what() << "\n";
        return 1;
    }

    PageReplacementAlgorithm* algo = nullptr;
    if (algorithm == "Optimo") {
        algo = new Optimo();
    } else if (algorithm == "FIFO") {
        algo = new FIFO();
    } else if (algorithm == "LRU") {
        algo = new LRU();
    } else if (algorithm == "Clock") {
        algo = new Clock();
    } else {
        std::cerr << "Algoritmo Invalido, intentar con Optimo, FIFO, LRU o Clock.\n";
        return 1;
    }

    int faults = algo->simulate(references, numFrames);
    std::cout << "Fallos de página: " << faults << "\n";

    delete algo;
    return 0;
}

