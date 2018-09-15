#ifndef IMAGE_BUFFER_OBJECT
#define IMAGE_BUFFER_OBJECT
#include <string>
#include <mutex>

#define SYNC_FILE_BUFFER_SIZE 16384

class SyncFileBuffer {
private:
    std::mutex bufferMutex_;
    bool bufferConsumed_ = false;
    std::unique_lock<std::mutex> producerLock_;
    std::unique_lock<std::mutex> consumerLock_;

public:
    int bytesAvailable;
    char buffer[SYNC_FILE_BUFFER_SIZE];
    std::string currentFilepath;

    SyncFileBuffer();
    bool lockIfBufferReady();
    void waitForConsumption();
    void setBufferConsumed();
    void setBufferReady(std::string filepath, int bytesAvailable);
};

#endif
