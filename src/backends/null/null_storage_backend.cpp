// Null storage backend — returns the current working directory when no
// real storage paths are available. Used as a placeholder.

#include "backends/storage_backend.h"

#include <string>

#ifdef _WIN32
#include <direct.h>
#define getcwd _getcwd
#else
#include <unistd.h>
#endif

namespace PlayOS {
namespace Detail {
namespace {

class NullStorageBackend : public IStorageBackend {
public:
    const char* SavePath() override {
        if (m_savePath.empty()) {
            char cwd[1024];
            m_savePath = std::string(getcwd(cwd, sizeof(cwd)) ? cwd : ".") + "/saves";
        }
        return m_savePath.c_str();
    }

    const char* ConfigPath() override {
        if (m_configPath.empty()) {
            char cwd[1024];
            m_configPath = std::string(getcwd(cwd, sizeof(cwd)) ? cwd : ".") + "/config";
        }
        return m_configPath.c_str();
    }

private:
    std::string m_savePath;
    std::string m_configPath;
};

} // namespace

IStorageBackend* CreateStorageBackend() {
    return new NullStorageBackend();
}

} // namespace Detail
} // namespace PlayOS
