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
std::vector<SyncFileBuffer> syncFileBuffers;

//#include "image-downloader.hpp"
bool isValidInput(int argc, char *argv[]) {
    if (((argc ^ 1) & 1) == 0) return true; // must be an even number.
    std::cerr << "You must include pairs of image links and output\nfile paths for those links as arguments.\n";
    std::cerr << R"(Example: https://imgur/someurl /your_file_path/filename.jpg)" << std::endl;
    return false;
}

void spawnThreads(int numImages, char *argv[]) {
    int numThreads = std::thread::hardware_concurrency() - 1;
    int avg = numImages / numThreads;
    int mod = numImages % numThreads;
    for (int i = 0; i < numThreads && i < numImages; ++i) {
        int idx = (i * avg) + std::min(i, mod);
        int numberToProcess = ((i + 1) * avg) + std::min(i + 1, mod) - idx;
        syncFileBuffers.push_back(SyncFileBuffer());
        imageDownloaders.push_back(ImageDownloader(&argv[idx], numberToProcess, &syncFileBuffers.back()));
    }
}

void writeFiles() {
    auto file_map = std::map<std::string, std::ofstream>();
    for(auto &sfb : syncFileBuffers) {
        if (sfb.lockIfBufferReady()) {
            if (file_map.find(*sfb.currentFilepath) == file_map.end()) {
                file_map[*sfb.currentFilepath] = std::ofstream(*sfb.currentFilepath,  std::ofstream::out | std::ofstream::binary);
            }
            if (sfb.bytesAvailable <= 0) {
                file_map[*sfb.currentFilepath].close();
                file_map.erase(*sfb.currentFilepath);
            } else {
                file_map[*sfb.currentFilepath].write(sfb.currentBuffer, sfb.bytesAvailable);
            }
        }
    }
}
int main(int argc, char *argv[]) {
    if (!isValidInput(argc, argv)) return -1;
    std::string route;
    std::string hostname;

    std::for_each_n(&argv[1], argc - 1, [&route, &hostname](auto c_str)-> void { getHostnameAndRouteFromUrl(c_str, route, hostname); });

    spawnThreads((argc - 1) / 2, argv);
    writeFiles();

    return 0;
}
