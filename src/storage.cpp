#include "playos/storage.h"

#include "backends/storage_backend.h"

#include <memory>

namespace PlayOS {
namespace {

std::unique_ptr<Detail::IStorageBackend> g_backend;

Detail::IStorageBackend* backend() {
    if (!g_backend) {
        g_backend.reset(Detail::CreateStorageBackend());
    }
    return g_backend.get();
}

} // namespace

namespace Storage {

const char* SavePath() {
    return backend()->SavePath();
}

const char* ConfigPath() {
    return backend()->ConfigPath();
}

} // namespace Storage
} // namespace PlayOS
