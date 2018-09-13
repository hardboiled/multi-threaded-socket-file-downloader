#ifndef IMAGE_DOWNLOADER_HPP
#define IMAGE_DOWNLOADER_HPP
#include <string>
#include <map>

#define IMAGE_FILE_BUFFER_SIZE 4096

class ImageDownloader {
    private:
        char buffer[IMAGE_FILE_BUFFER_SIZE];
        std::map<std::string, std::string> m_urlMap;
        int m_socketFd;
        bool getHostnameAndRouteFromUrl(const std::string &url, std::string &hostname, std::string &route);
        void createRequest(int socketFd, std::string &hostname, std::string &route);
        int ImageDownloader::foundBodySeparator(const char *localBuffer, int bufSize, int &separatorPos);
    public:
        ImageDownloader(char **argv, int n);
        void startDownload(char *buffer, int size);
};
#endif
