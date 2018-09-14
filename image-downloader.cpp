#include <algorithm>
#include <fstream>
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

#include "image-downloader-2.hpp"

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

    // std::cout << request;
    if (send(socketFd, request.c_str(), request.length(), 0) != (int)request.length()) {
        throw "Error sending request.";
    }
}

void ImageDownloader::startDownload(SyncFileBuffer* sfb) {
    int socketFd;
    std::ofstream ofs;
    struct addrinfo *servinfo;
    auto cleanup = [&socketFd, &ofs, &servinfo]() {
        ofs.close();
        close(socketFd);
        freeaddrinfo(servinfo);
    };

    try {
        for (auto it = this->m_urlMap_.begin(); it != this->m_urlMap_.end(); ++it) {
            std::string url =  it->first;
            std::string filepath = it->second;
            std::string hostname;
            std::string route;

            // std::cout << "before ofstream url " << url << "\n";
            // std::cout << "before ofstream " << filepath << "\n";
            // std::cout << "after ofstream" << "\n";
            if (!this->getHostnameAndRouteFromUrl(url, hostname, route)) {
                throw "Url " + url + " is malformed!";
            }

            // std::cout << "before getsocketInfo" << "\n";
            this->getSocketInfo(hostname, socketFd, servinfo);

            // std::cout << "before createRequest" << "\n";
            this->createRequest(socketFd, hostname, route);

            int prefixLen = -1;

            // std::cout << "before first recv" << "\n";
            int bytesRead = 0;
            while ((bytesRead = recv(socketFd, this->buffer_, IMAGE_FILE_BUFFER_SIZE, 0)) > 0) {
                if ((prefixLen = this->findBodySeparator(IMAGE_FILE_BUFFER_SIZE)) >= 0) break;
            }

            // std::cout.write(this->buffer_, prefixLen);
            // std::cout << "last bytes read " << bytesRead << "\n";
            // std::cout << "before first ofs write" << "\n";
            if (prefixLen < bytesRead) {
                // std::cout.write(this->buffer_, bytesRead);
                int remainingSize = bytesRead - prefixLen;
                memcpy(this->buffer_, &this->buffer_[prefixLen], remainingSize);
                ofs.write(this->buffer_, remainingSize);
            }


            // // std::cout << "before second recv" << "\n";
            while ((bytesRead = recv(socketFd, this->buffer_, IMAGE_FILE_BUFFER_SIZE, 0)) > 0) {
                ofs.write(this->buffer_, bytesRead);
            }

            // std::cout << "final bytes read " << bytesRead << "\n";
            // std::cout << "before cleanup" << "\n";
            cleanup();
        }
    } catch(const char* msg) {
        // std::cout << "exception " << msg << "\n";
        cleanup();
    }
}

int ImageDownloader::findBodySeparator(int bufSize) {
    int offset = 0;
    std::string target = "\r\n\r\n";
    int idx = 0;
    while (idx < bufSize && offset < target.length()) {
        if (target.at(offset) == this->buffer_[idx]) ++offset;
        else offset = 0;
        ++idx;
    }
    // std::cout << "offset is " << offset << "\n";
    // std::cout << "ids is " << idx << "\n";
    return (offset == target.length() && idx < bufSize) ? idx : -1;
}

bool ImageDownloader::getHostnameAndRouteFromUrl(const std::string &url, std::string &hostname, std::string &route) {
    std::regex rx(R"(https?:\/\/([^\/]+)(\/.+))");
    std::cmatch cm;
    std::regex_match(url.c_str(), cm, rx);
    // std::cout << "after match" << "\n";
    if (cm.size() <= 2) return false;
    // std::cout << cm[1] << "\n";
    // std::cout << cm[2] << "\n";
    hostname = std::string(cm[1]);
    route = std::string(cm[2]);
    // std::cout << hostname << route << " blah \n";
    return true;
}
