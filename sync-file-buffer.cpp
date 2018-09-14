#include "sync-file-buffer.hpp"
#include <utility>

SyncFileBuffer::SyncFileBuffer() : bytesAvailable(0), bufferConsumed_(true) {
    this->bufferMutex_ = new std::mutex();
    this->cond_ = new std::condition_variable();
    this->producerLock_ = new std::unique_lock<std::mutex>(*this->bufferMutex_);
    this->consumerLock_ = new std::unique_lock<std::mutex>(*this->bufferMutex_);
}

SyncFileBuffer::SyncFileBuffer(SyncFileBuffer&& that)
    : bufferMutex_(nullptr), cond_(nullptr), producerLock_(nullptr), consumerLock_(nullptr) {
    std::mutex* mp = that.bufferMutex_;
    std::lock_guard<std::mutex> locker(*mp);
    std::swap(this->bufferMutex_, that.bufferMutex_);
    std::swap(this->cond_, that.cond_);
    std::swap(this->producerLock_, that.producerLock_);
    std::swap(this->consumerLock_, that.consumerLock_);
    memcpy(this->currentBuffer, that.currentBuffer, SYNC_FILE_BUFFER_SIZE);
}

SyncFileBuffer& SyncFileBuffer::operator=(SyncFileBuffer&& that) {
    if (this != &that) {
        SyncFileBuffer(that);
    }
    return *this;
}

SyncFileBuffer::~SyncFileBuffer() {
    delete this->producerLock_;
    delete this->consumerLock_;
    delete this->bufferMutex_;
    delete this->cond_;
}

void SyncFileBuffer::setBufferConsumed() {
    this->bufferConsumed_ = true;
    this->consumerLock_->unlock();
    this->cond_->notify_one();
}

void SyncFileBuffer::waitForConsumption() {
    this->cond_->wait(*this->producerLock_, [this]()-> bool { return !this->bufferConsumed_; });
}

void SyncFileBuffer::setBufferReady(std::string* filepath, int bytesAvailable) {
    this->bytesAvailable = bytesAvailable;
    this->currentFilepath = filepath;
    this->bufferConsumed_ = false;
    this->producerLock_->unlock();
}

bool SyncFileBuffer::lockIfBufferReady() {
    this->consumerLock_->try_lock();
    if (this->consumerLock_->owns_lock()) {
        if (this->bufferConsumed_ == false) {
            return true;
        }
        this->consumerLock_->unlock();
        this->cond_->notify_one();
    }
    return false;
}
