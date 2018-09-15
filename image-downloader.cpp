#include <algorithm>
#include <iostream>
#include <regex>
#include <sstream>
#include <string>

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#include "image-downloader.hpp"

#define PORT "80"
#define USER_AGENT "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_13_6) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/68.0.3440.106 Safari/537.36"

void ImageDownloader::getSocketInfo(std::string &hostname, int &socketFd, struct addrinfo* &servinfo) {
    int status = -1;
    struct addrinfo hints;

    memset(&hints, 0, sizeof hints); // make sure the struct is empty
    hints.ai_family = AF_UNSPEC;     // don't care IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM; // TCP stream sockets
    hints.ai_flags = AI_PASSIVE;     // fill in my IP for me

    if ((status = getaddrinfo(hostname.c_str(), PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        exit(1);
    }
    socketFd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
    if (socketFd < 0) throw "ERROR opening socket";

    status = connect(socketFd, servinfo->ai_addr, servinfo->ai_addrlen);
    if (status < 0) throw "Got Error while connecting " +  std::to_string(errno);
}
ImageDownloader::ImageDownloader(char **argv, int n) {
    for (int i = 0; i < n - 1; i+=2) {
        this->m_urlMap_[std::string(argv[i])] = std::string(argv[i + 1]);
    }
}

void ImageDownloader::createRequest(int socketFd, std::string &hostname, std::string &route) {
    std::stringstream ss;

    ss << "GET " << route << " HTTP/1.1\r\n"
        << "Host: " << hostname << "\r\n"
        << "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,image/apng,*/*;q=0.8\r\n"
        << "Accept-Language: en-US,en;q=0.9\r\n"
        << "Cache-Control: no-cache\r\n"
        << "User-Agent: " << USER_AGENT << "\r\n"
        << "Connection: close\r\n"
        << "Pragma: no-cache\r\n"
        << "\r\n";
    std::string request = ss.str();

    if (send(socketFd, request.c_str(), request.length(), 0) != (int)request.length()) {
        throw "Error sending request.";
    }
}

void ImageDownloader::startDownload(SyncFileBuffer* sfb) {
    int socketFd;
    struct addrinfo *servinfo;
    std::string filepath;

    auto cleanup = [&socketFd, &filepath, &servinfo, &sfb]() {
        close(socketFd);
        freeaddrinfo(servinfo);
        sfb->setBufferReady(filepath, 0);
    };

    try {
        for (auto it = this->m_urlMap_.begin(); it != this->m_urlMap_.end(); ++it) {
            std::string url =  it->first;
            filepath = it->second;
            std::string hostname;
            std::string route;

            if (!this->getHostnameAndRouteFromUrl(url, hostname, route)) {
                throw "Url " + url + " is malformed!";
            }

            this->getSocketInfo(hostname, socketFd, servinfo);

            this->createRequest(socketFd, hostname, route);

            int prefixLen = -1;

            int bytesRead = 0;
            sfb->waitForConsumption();
            while ((bytesRead = recv(socketFd, sfb->buffer, SYNC_FILE_BUFFER_SIZE, 0)) > 0) {
                if ((prefixLen = this->findBodySeparator(sfb->buffer, SYNC_FILE_BUFFER_SIZE)) >= 0) break;
            }

            if (prefixLen < bytesRead) {
                int remainingSize = bytesRead - prefixLen;
                memcpy(sfb->buffer, &sfb->buffer[prefixLen], remainingSize);
                sfb->setBufferReady(filepath, remainingSize);
                sfb->waitForConsumption();
            }

            if (bytesRead <= 0) {
                throw "initial read for " + filepath + " failed";
            }

            while ((bytesRead = recv(socketFd, sfb->buffer, SYNC_FILE_BUFFER_SIZE, 0)) > 0) {
                sfb->setBufferReady(filepath, bytesRead);
                sfb->waitForConsumption();
            }

            cleanup();
        }
    } catch(const char* msg) {
        std::cout << "exception " << msg << std::endl;
        cleanup();
        exit(1);
    }
}

int ImageDownloader::findBodySeparator(const char* const buffer,int bufSize) {
    int offset = 0;
    std::string target = "\r\n\r\n";
    int idx = 0;
    while (idx < bufSize && offset < target.length()) {
        if (target.at(offset) == buffer[idx]) ++offset;
        else offset = 0;
        ++idx;
    }
    return (offset == target.length() && idx < bufSize) ? idx : -1;
}

bool ImageDownloader::getHostnameAndRouteFromUrl(const std::string &url, std::string &hostname, std::string &route) {
    std::regex rx(R"(https?:\/\/([^\/]+)(\/.+))");
    std::cmatch cm;
    std::regex_match(url.c_str(), cm, rx);
    if (cm.size() <= 2) return false;
    hostname = std::string(cm[1]);
    route = std::string(cm[2]);
    return true;
}
