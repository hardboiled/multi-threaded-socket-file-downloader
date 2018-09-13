#ifndef IMAGE_BUFFER_OBJECT
#define IMAGE_BUFFER_OBJECT
#include <string>
#include <mutex>

class ImageBuffer {
private:
    int bytes_available_;
    std::string filepath_;
    char* buffer;
    std::mutex buffer_mutex;
public:
    ImageBuffer() : bytes_available_(0) {};
    void waitUntilBufferConsumed();
    void waitUntilBufferWritten();
    void setFileDownloaded();
    void setBufferConsumed();
    void setBufferWritten(std::string filepath, int bytes_available);
};

#endif
