#ifndef FILE_DOWNLOADER_HPP
#define FILE_DOWNLOADER_HPP
#include "sync-file-buffer.hpp"

namespace file_downloader {
    void startDownloadingFiles(char** urlFileArguments, int n, SyncFileBuffer* sfb);
}
#endif
