// Internal storage backend interface. Each platform implements one backend;
// the public PlayOS::Storage API (storage.cpp) delegates to it.
#pragma once

namespace PlayOS {
namespace Detail {

class IStorageBackend {
public:
    virtual ~IStorageBackend() = default;

    // Returns the absolute path for persistent save data.
    // The backend ensures the directory exists.
    virtual const char* SavePath() = 0;

    // Returns the absolute path for configuration data.
    // The backend ensures the directory exists.
    virtual const char* ConfigPath() = 0;
};

// Creates the platform-selected storage backend.
IStorageBackend* CreateStorageBackend();

} // namespace Detail
} // namespace PlayOS
