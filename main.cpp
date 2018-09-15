#include <algorithm>
#include <cmath>
#include <curl/curl.h>
#include <curl/easy.h>
#include <iostream>
#include <fstream>
#include <map>
#include <regex>
#include <stdio.h>
#include <thread>
#include <vector>
#include "sync-file-buffer.hpp"
#include "file-downloader.hpp"

int m_numThreads = std::thread::hardware_concurrency() - 1;
std::unique_ptr<SyncFileBuffer[]> m_syncFileBuffers(new SyncFileBuffer[m_numThreads]);
std::thread** m_threads = new std::thread* [m_numThreads]();

//#include "image-downloader.hpp"
bool isValidInput(int argc, char *argv[]) {
    if (((argc ^ 1) & 1) == 0) return true; // must be an even number.
    std::cerr << "You must include pairs of image links and output\nfile paths for those links as arguments.\n";
    std::cerr << R"(Example: https://imgur/someurl /your_file_path/filename.jpg)" << std::endl;
    return false;
}

void spawnThreads(int numArguments, char *urlFileArguments[]) {
    int avg = numArguments / m_numThreads;
    int mod = numArguments % m_numThreads;

    // make start threads and distribute images equitably to each thread
    for (int i = 0; i < m_numThreads && i < numArguments; ++i) {
        int idx = (i * avg) + std::min(i, mod);
        int numberToProcess = ((i + 1) * avg) + std::min(i + 1, mod) - idx;
        m_threads[i] = new std::thread(
            file_downloader::startDownloadingFiles, &urlFileArguments[idx], numberToProcess, &m_syncFileBuffers[i]
        );
    }
}

void writeFiles(int numImages) {
    auto file_map = std::map<std::string, std::ofstream>();
    auto byte_me = std::map<std::string, int>();
    int filesProcessed = 0;
    while (filesProcessed < numImages) {
        for(int i = 0; i < m_numThreads; ++i) {
            auto sfb = &m_syncFileBuffers[i];
            if (sfb->lockIfBufferReady()) {
                if (file_map.find(sfb->currentFilepath) == file_map.end()) {
                    file_map[sfb->currentFilepath] = std::ofstream(sfb->currentFilepath, std::ofstream::trunc);
                    byte_me[sfb->currentFilepath] = 0;
                }
                if (sfb->bytesAvailable <= 0) {
                    file_map[sfb->currentFilepath].close();
                    ++filesProcessed;
                    std::cout << sfb->currentFilepath << " finished " << byte_me[sfb->currentFilepath] << std::endl;
                    file_map.erase(sfb->currentFilepath);
                } else {
                    byte_me[sfb->currentFilepath] += sfb->bytesAvailable;
                    file_map[sfb->currentFilepath].write(sfb->buffer, sfb->bytesAvailable);
                }
                sfb->setBufferConsumed();
            }
        }
    }
}

void endThreads() {
    for (int i = 0; i < m_numThreads; ++i) {
        if (m_threads[i]) {
            m_threads[i]->join();
            delete m_threads[i];
        }
    }
    delete[] m_threads;
}

int main(int argc, char *argv[]) {
    if (!isValidInput(argc, argv)) return -1;

    spawnThreads(argc - 1, &argv[1]); //skip argv[0], because it's the executable name
    try {
        writeFiles((argc - 1) / 2);
    } catch (const char* msg) {
        std::cout << msg << std::endl;
    }
    endThreads();
    return 0;
}
