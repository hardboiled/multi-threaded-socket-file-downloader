#include <algorithm>
#include <mutex>
#include <regex>
#include <sstream>
#include <string>
#include <thread>

#include <netdb.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#include "image-downloader.hpp"

#define TLS_PORT 443
#define USER_AGENT "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_13_6) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/68.0.3440.106 Safari/537.36"

ImageDownloader::ImageDownloader(char **argv, int n, SyncFileBuffer* syncFileBuffer) : syncFileBuffer_(syncFileBuffer) {
    for (int i = 0; i < n - 1; ++i) {
        this->m_urlMap_[std::string(argv[i])] = std::string(argv[i + 1]);
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

void ImageDownloader::startDownload() {
    int socketFd, portNo = TLS_PORT;
    struct sockaddr_in servAddr;
    struct hostent *server;

    try {
        for (auto it = this->m_urlMap_.begin(); it != this->m_urlMap_.end(); ++it) {
            std::string url =  it->first;
            std::string filepath = it->second;
            std::string hostname;
            std::string route;
            if (this->getHostnameAndRouteFromUrl(url, hostname, route)) {
                throw "Url " + url + " is malformed!";
            }
            socketFd = socket(AF_INET, SOCK_STREAM, 0);
            if (socketFd < 0) throw "ERROR opening socket";
            server = gethostbyname(hostname.c_str());
            this->createRequest(socketFd, hostname, route);

            int prefixLen = -1;
            this->syncFileBuffer_->waitForConsumption();
            while ( read(socketFd, this->buffer_, IMAGE_FILE_BUFFER_SIZE) > 0 ) {
                if ((prefixLen = this->findBodySeparator(IMAGE_FILE_BUFFER_SIZE)) >= 0) break;
            }
            if (prefixLen < IMAGE_FILE_BUFFER_SIZE - 1) {
                int remainingSize = IMAGE_FILE_BUFFER_SIZE - prefixLen;
                memcpy(this->buffer_, &this->buffer_[prefixLen], remainingSize);
            }

            int bytesRead = 0;
            while ((bytesRead = read(socketFd, this->buffer_, IMAGE_FILE_BUFFER_SIZE)) > 0) {
                this->syncFileBuffer_->setBufferReady(&filepath, this->buffer_, bytesRead);
                this->syncFileBuffer_->waitForConsumption();
            }
            this->syncFileBuffer_->setBufferReady(&filepath, this->buffer_, -1);
            close(socketFd);
        }
    } catch(std::exception& e) {
        close(socketFd);
    }
}

int ImageDownloader::findBodySeparator(int bufSize) {
    std::regex rx(R"((.+\r\n\r\n))");
    std::cmatch cm;
    std::string s = std::string(this->buffer_, bufSize + 1); // add space for null character
    std::regex_match(s.c_str(), cm, rx);
    if (cm.size() <= 1) return -1;
    return cm[0].length();
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
