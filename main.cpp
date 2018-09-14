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
#include "image-downloader.hpp"

std::vector<ImageDownloader> imageDownloaders;
int m_numThreads = std::thread::hardware_concurrency() - 1;
std::unique_ptr<SyncFileBuffer[]> syncFileBuffers(new SyncFileBuffer[m_numThreads]);

//#include "image-downloader.hpp"
bool isValidInput(int argc, char *argv[]) {
    if (((argc ^ 1) & 1) == 0) return true; // must be an even number.
    std::cerr << "You must include pairs of image links and output\nfile paths for those links as arguments.\n";
    std::cerr << R"(Example: https://imgur/someurl /your_file_path/filename.jpg)" << std::endl;
    return false;
}

void spawnThreads(int numImages, char *argv[]) {
    int avg = numImages / m_numThreads;
    int mod = numImages % m_numThreads;

    // make one image downloader per thread and distribute images equitably to downloaders
    for (int i = 0; i < m_numThreads && i < numImages; ++i) {
        int idx = (i * avg) + std::min(i, mod);
        int numberToProcess = ((i + 1) * avg) + std::min(i + 1, mod) - idx;
        auto imgd = ImageDownloader(&argv[idx], numberToProcess);
        imageDownloaders.push_back(imgd);
    }

    //bootstrap threads. Don't care about joining them.
    for (int i = 0; i < imageDownloaders.size(); ++i) {
        std::thread t(&ImageDownloader::startDownload, &imageDownloaders[i], &syncFileBuffers[i]);
    }
}

void writeFiles(int numImages) {
    auto file_map = std::map<std::string, std::ofstream>();
    int filesProcessed = 0;
    while (filesProcessed < numImages) {
        for(int i = 0; i < m_numThreads; ++i) {
            auto sfb = &syncFileBuffers[i];
            if (sfb->lockIfBufferReady()) {
                if (file_map.find(sfb->currentFilepath) == file_map.end()) {
                    file_map[sfb->currentFilepath] = std::ofstream(sfb->currentFilepath,  std::ofstream::binary);
                }
                if (sfb->bytesAvailable <= 0) {
                    file_map[sfb->currentFilepath].close();
                    ++filesProcessed;
                    file_map.erase(sfb->currentFilepath);
                } else {
                    file_map[sfb->currentFilepath].write(sfb->buffer, sfb->bytesAvailable);
                }
            }
        }
    }
}

int main(int argc, char *argv[]) {
    if (!isValidInput(argc, argv)) return -1;

    spawnThreads((argc - 1) / 2, &argv[1]);
    writeFiles((argc - 1) / 2);
    return 0;
}
