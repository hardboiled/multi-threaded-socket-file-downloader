#ifndef IMAGE_BUFFER_OBJECT
#define IMAGE_BUFFER_OBJECT
#include <string>
#include <mutex>

class SyncFileBuffer {
private:
    std::mutex *bufferMutex_;
    std::condition_variable *cond_;
    bool bufferConsumed_ = false;
    std::unique_lock<std::mutex>* producerLock_;
    std::unique_lock<std::mutex>* consumerLock_;

public:
    int bytesAvailable;
    std::string* currentFilepath;
    const char* currentBuffer;

    SyncFileBuffer();
    ~SyncFileBuffer();
    bool lockIfBufferReady();
    void waitForConsumption();
    void setBufferConsumed();
    void setBufferReady(std::string* filepath, const char *buffer, int bytesAvailable);
};

#endif
