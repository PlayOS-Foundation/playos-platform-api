// Platform storage backend — uses OS-standard application data directories.
// Windows: %LOCALAPPDATA%/PlayOS
// Linux:   $XDG_DATA_HOME/PlayOS  (or ~/.local/share/PlayOS)
// macOS:   ~/Library/Application Support/PlayOS

#include "backends/storage_backend.h"

#include <cstdlib>
#include <string>

#ifdef _WIN32
#include <shlobj.h>
#else
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#endif

namespace PlayOS {
namespace Detail {
namespace {

std::string BaseDir() {
#ifdef _WIN32
    wchar_t path[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_LOCAL_APPDATA, nullptr, 0, path))) {
        // Convert wide to narrow (ASCII-safe subset only for PoC).
        char narrow[MAX_PATH];
        WideCharToMultiByte(CP_UTF8, 0, path, -1, narrow, MAX_PATH, nullptr, nullptr);
        return std::string(narrow) + "\\PlayOS";
    }
    return std::string(getenv("USERPROFILE") ? getenv("USERPROFILE") : ".") + "\\PlayOS";
#else
    const char* xdg = getenv("XDG_DATA_HOME");
    if (xdg && *xdg) return std::string(xdg) + "/PlayOS";
    const char* home = getenv("HOME");
    if (home && *home) return std::string(home) + "/.local/share/PlayOS";
    return "./PlayOS";
#endif
}

void EnsureDir(const std::string& path) {
#ifdef _WIN32
    CreateDirectoryA(path.c_str(), nullptr);
#else
    mkdir(path.c_str(), 0755);
#endif
}

class PlatformStorageBackend : public IStorageBackend {
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
    return new PlatformStorageBackend();
}

} // namespace Detail
} // namespace PlayOS
