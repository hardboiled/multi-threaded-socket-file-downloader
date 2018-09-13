#include "sync-file-buffer.hpp"

SyncFileBuffer::SyncFileBuffer()  : bytesAvailable(0), bufferConsumed_(true) {
    this->producerLock_ = std::unique_lock<std::mutex>(this->bufferMutex_);
    this->consumerLock_ = std::unique_lock<std::mutex>(this->bufferMutex_);
}
void SyncFileBuffer::setBufferConsumed() {
    this->bufferConsumed_ = true;
    this->consumerLock_.unlock();
    this->cond_.notify_one();
}

void SyncFileBuffer::waitForConsumption() {
    this->cond_.wait(this->producerLock_, [this]()-> bool { return !this->bufferConsumed_; });
}

void SyncFileBuffer::setBufferReady(std::string* filepath, const char *buffer, int bytesAvailable) {
    this->bytesAvailable = bytesAvailable;
    this->currentFilepath = filepath;
    this->currentBuffer = buffer;
    this->bufferConsumed_ = false;
    this->producerLock_.unlock();
}

bool SyncFileBuffer::lockIfBufferReady() {
    this->consumerLock_.try_lock();
    if (this->consumerLock_.owns_lock()) {
        if (this->bufferConsumed_ == false) {
            return true;
        }
        this->consumerLock_.unlock();
        this->cond_.notify_one();
    }
    return false;
}
