#ifndef SYNC_FILE_BUFFER_HPP
#define SYNC_FILE_BUFFER_HPP
#include <string>
#include <mutex>

#define SYNC_FILE_BUFFER_SIZE 65536

class SyncFileBuffer {
private:
    std::mutex bufferMutex_;
    bool bufferConsumed_ = false;
    std::unique_lock<std::mutex> producerLock_;
    std::unique_lock<std::mutex> consumerLock_;
    std::condition_variable cond_;

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
