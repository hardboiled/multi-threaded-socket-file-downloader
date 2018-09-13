#ifndef IMAGE_DOWNLOADER_HPP
#define IMAGE_DOWNLOADER_HPP
#include <string>
#include <map>
#include "sync-file-buffer.hpp"

#define IMAGE_FILE_BUFFER_SIZE 4096

class ImageDownloader {
    private:
        char buffer_[IMAGE_FILE_BUFFER_SIZE];
        SyncFileBuffer* syncFileBuffer_;
        std::map<std::string, std::string> m_urlMap_;
        bool getHostnameAndRouteFromUrl(const std::string &url, std::string &hostname, std::string &route);
        void createRequest(int socketFd, std::string &hostname, std::string &route);
        int findBodySeparator(int bufSize);
    public:
        ImageDownloader(char **argv, int n, SyncFileBuffer* syncFileBuffer);
        void startDownload();
};
#endif
