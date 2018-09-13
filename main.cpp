#include <algorithm>
#include <curl/curl.h>
#include <curl/easy.h>
#include <iostream>
#include <regex>
#include <stdio.h>
#include <thread>

//#include "image-downloader.hpp"
bool isValidInput(int argc, char *argv[]) {
    if (((argc ^ 1) & 1) == 0) return true; // must be an even number.
    std::cerr << "You must include pairs of image links and output\nfile paths for those links as arguments.\n";
    std::cerr << R"(Example: https://imgur/someurl /your_file_path/filename.jpg)" << std::endl;
    return false;
}

int main(int argc, char *argv[]) {
    std::cout << std::thread::hardware_concurrency() << " threads\n";
    if (!isValidInput(argc, argv)) return -1;
    std::string route;
    std::string hostname;

    std::for_each_n(&argv[1], argc - 1, [&route, &hostname](auto c_str)-> void { getHostnameAndRouteFromUrl(c_str, route, hostname); });

    vector



    return 0;
}
