#include <chrono>
#include <thread>
#include <utility>

#include "sync-file-buffer.hpp"

SyncFileBuffer::SyncFileBuffer() : bytesAvailable(0), bufferConsumed_(true), currentFilepath("") {
    this->consumerLock_ = std::unique_lock<std::mutex>(this->bufferMutex_, std::defer_lock);
    this->producerLock_ = std::unique_lock<std::mutex>(this->bufferMutex_, std::defer_lock);
}

void SyncFileBuffer::setBufferConsumed() {
    this->bufferConsumed_ = true;
    this->consumerLock_.unlock();
    this->cond_.notify_one();
}

void SyncFileBuffer::waitForConsumption() {
    this->producerLock_.lock();
    this->cond_.wait(this->producerLock_, [this]() -> bool { return this->bufferConsumed_; });
}

void SyncFileBuffer::setBufferReady(std::string filepath, int bytesAvailable) {
    this->bytesAvailable = bytesAvailable;
    this->currentFilepath = filepath;
    this->bufferConsumed_ = false;
    this->producerLock_.unlock();
}

bool SyncFileBuffer::lockIfBufferReady() {
    if (this->bufferConsumed_ || !this->consumerLock_.try_lock()) return false;
    return true;
}
