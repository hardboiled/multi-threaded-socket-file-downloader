#include <algorithm>
#include <mutex>
#include <netinet/in.h>
#include <netdb.h>
#include <regex>
#include <sstream>
#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <thread>
#include "image-downloader.hpp"

#define TLS_PORT 443
#define USER_AGENT "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_13_6) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/68.0.3440.106 Safari/537.36"

ImageDownloader::ImageDownloader(char **argv, int n) {
    for (int i = 0; i < n - 1; ++i) {
        this->m_urlMap[std::string(argv[i])] = std::string(argv[i + 1]);
    }
}

void ImageDownloader::createRequest(int socketFd, std::string &hostname, std::string &route) {
    std::stringstream ss;

    ss << "GET " << route << "\n"
        << "Host: " << hostname << "\r\n"
        << "Accept: image/*\r\n"
        << "User-Agent: " << USER_AGENT
        << "Connection: close"
        << "\r\n\r\n";
    std::string request = ss.str();

    if (send(socketFd, request.c_str(), request.length(), 0) != (int)request.length()) {
        throw "Error sending request.";
    }
}

void ImageDownloader::startDownload(char *buffer, int bufSize) {
    int socketFd, portNo = TLS_PORT;
    struct sockaddr_in servAddr;
    struct hostent *server;
    char * localBuffer = new char[bufSize];

    try {
        std::for_each(this->m_urlMap.begin(), this->m_urlMap.end(), [&](auto url, auto filename) -> void {
            string hostname;
            string route;
            if (this->getHostnameAndRouteFromUrl(url, hostname, route)) {
                throw "Url " + url + " is malformed!";
            }
            socketFd = socket(AF_INET, SOCK_STREAM, 0);
            if (sockfd < 0) throw "ERROR opening socket";
            server = gethostbyname(hostname.c_str());
            this->createRequest(socketFd, hostname, route);

            int prefixLen = -1;
            while ( read(sock, localBuffer, bufSize) > 0 ) {
                if ((prefixLen = this->foundBodySeparator(localBuffer, bufSize) >= 0) break;
            }
            if (prefixLen < bufSize - 1) {
                int remainingSize = bufSize - prefixLen;
                std::lock_guard<std::mutex> lock(buffer);
                memcpy(buffer, &localBuffer[prefixLen], remainingSize));
            }

            while (read(sock, localBuffer, bufSize) > 0) {
                std::lock_guard<std::mutex> lock(buffer);
                memcpy(buffer, localBuffer, bufSize);
            }
            close(socketFd);
        });
    } catch(std::exception& e) {
        close(socketFd);
    }

    delete[] localBuffer;
}

int ImageDownloader::foundBodySeparator(const char *localBuffer, int bufSize, int &separatorPos) {
    std::regex rx(R"((.+\r\n\r\n))");
    std::cmatch cm;
    std::string s = std::string(localBuffer, bufSize + 1); // add space for null character
    std::regex_match(s.c_str(), cm, rx);
    if (cm.size() <= 1) return -1;
    return cm[0].length;
}

bool ImageDownloader::getHostnameAndRouteFromUrl(const std::string &url, std::string &hostname, std::string &route) {
    std::regex rx(R"(https?:\/\/([^\/]+)\/(.+))");
    std::cmatch cm;
    std::regex_match(url.c_str(), cm, rx);
    if (cm.size() <= 2) return false;
    hostname = cm[1];
    route = cm[2];
    return true;
}
