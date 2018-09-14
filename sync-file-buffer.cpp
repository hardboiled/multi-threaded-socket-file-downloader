#include "sync-file-buffer.hpp"
#include <utility>

void SyncFileBuffer::setBufferConsumed() {
    if (this->consumerLock_.owns_lock()) {
        this->bufferConsumed_ = true;
        this->consumerLock_.unlock();
        this->cond_.notify_one();
    }
}

void SyncFileBuffer::waitForConsumption() {
    this->cond_.wait(this->producerLock_, [this]()-> bool { return !this->bufferConsumed_; });
}

void SyncFileBuffer::setBufferReady(std::string filepath, int bytesAvailable) {
    if (this->producerLock_.owns_lock()) {
        this->bytesAvailable = bytesAvailable;
        this->currentFilepath = filepath;
        this->bufferConsumed_ = false;
        this->producerLock_.unlock();
    }
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
