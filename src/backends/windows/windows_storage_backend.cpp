// Windows storage backend — uses %LOCALAPPDATA%/PlayOS for save and
// config directories. Windows-only; no engine dependency.

#include "backends/storage_backend.h"

#include <cstdlib>
#include <string>

#include <shlobj.h>

namespace PlayOS {
namespace Detail {
namespace {

std::string BaseDir() {
    wchar_t path[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_LOCAL_APPDATA, nullptr, 0,
                                   path))) {
        char narrow[MAX_PATH];
        WideCharToMultiByte(CP_UTF8, 0, path, -1, narrow, MAX_PATH, nullptr,
                            nullptr);
        return std::string(narrow) + "\\PlayOS";
    }
    return std::string(getenv("USERPROFILE") ? getenv("USERPROFILE") : ".") +
           "\\PlayOS";
}

void EnsureDir(const std::string& path) {
    CreateDirectoryA(path.c_str(), nullptr);
}

class WindowsStorageBackend : public IStorageBackend {
public:
    const char* SavePath() override {
        if (m_savePath.empty()) {
            m_savePath = BaseDir() + "\\saves";
            EnsureDir(BaseDir());
            EnsureDir(m_savePath);
        }
        return m_savePath.c_str();
    }

    const char* ConfigPath() override {
        if (m_configPath.empty()) {
            m_configPath = BaseDir() + "\\config";
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
    return new WindowsStorageBackend();
}

} // namespace Detail
} // namespace PlayOS
