#ifndef IMAGE_DOWNLOADER_HPP
#define IMAGE_DOWNLOADER_HPP
#include <string>
#include <map>
#include "sync-file-buffer.hpp"

class ImageDownloader {
    private:
        std::map<std::string, std::string> m_urlMap_;
        bool getHostnameAndRouteFromUrl(const std::string &url, std::string &hostname, std::string &route);
        void getSocketInfo(std::string &hostname, int &socketFd, struct addrinfo* &servinfo);
        void createRequest(int socketFd, std::string &hostname, std::string &route);
        int findBodySeparator(const char * const buffer, int bufSize);
    public:
        ImageDownloader(char **argv, int n);
        void startDownload(SyncFileBuffer* sfb);
};
#endif
