// Linux storage backend — uses XDG Base Directory paths for save and
// config directories. Linux-only; no engine dependency.

#include "backends/storage_backend.h"

#include <cstdlib>
#include <string>

#include <sys/stat.h>
#include <sys/types.h>

namespace PlayOS {
namespace Detail {
namespace {

std::string BaseDir() {
    const char* xdg = getenv("XDG_DATA_HOME");
    if (xdg && *xdg) return std::string(xdg) + "/PlayOS";
    const char* home = getenv("HOME");
    if (home && *home) return std::string(home) + "/.local/share/PlayOS";
    return "./PlayOS";
}

void EnsureDir(const std::string& path) {
    mkdir(path.c_str(), 0755);
}

class LinuxStorageBackend : public IStorageBackend {
public:
    const char* SavePath() override {
        if (m_savePath.empty()) {
            m_savePath = BaseDir() + "/saves";
            EnsureDir(BaseDir());
            EnsureDir(m_savePath);
        }
        return m_savePath.c_str();
    }

    const char* ConfigPath() override {
        if (m_configPath.empty()) {
            m_configPath = BaseDir() + "/config";
            EnsureDir(BaseDir());
            EnsureDir(m_configPath);
        }
        return m_configPath.c_str();
    }

private:
    std::string m_savePath;
    std::string m_configPath;
};

} // namespace

IStorageBackend* CreateStorageBackend() {
    return new LinuxStorageBackend();
}

} // namespace Detail
} // namespace PlayOS
