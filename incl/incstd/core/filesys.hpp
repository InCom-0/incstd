#pragma once

#include <cctype>
#include <cstdlib>
#include <expected>
#include <filesystem>
#include <fstream>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

#include <ankerl/unordered_dense.h>
#include <incstd/core/hashing.hpp>

#if defined(__MINGW64__)
#include <shlobj.h>
#include <unistd.h>
#include <windows.h>

#elif defined(_WIN64)
#include <io.h>
#include <shlobj.h>
#include <windows.h>

#elif defined(__linux__) || (defined(__APPLE__) && defined(__MACH__))
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#endif

#if defined(__APPLE__)
#include <mach-o/dyld.h>
#endif


namespace incom::standard::filesys {
namespace fs = std::filesystem;

// TODO: This is somewhat weird, probably not at all smart to do it this way
// TODO: At any rate change the interface so that it is using std::expected
inline std::optional<std::string_view> get_file_textual(std::string_view const &sv) {
    static ankerl::unordered_dense::map<std::string, std::string, hashing::XXH3Hasher> storageMP;
    if (auto ele = storageMP.find(std::string(sv)); ele != storageMP.end()) { return std::string_view(ele->second); }
    else {
        std::ifstream ifs;
        ifs.open(fs::path(sv));
        if (ifs.is_open()) {
            return storageMP
                .insert({std::string(sv), std::string(std::istreambuf_iterator(ifs), std::istreambuf_iterator<char>())})
                .first->second;
        }
        else { return std::nullopt; }
    };
}

// TODO: At any rate change the interface so that it is using std::expected
inline std::optional<std::vector<std::byte>> get_file_bytes(std::string_view const &sv) {
    const auto    path = fs::path(sv);
    std::ifstream ifs(path, std::ios::binary | std::ios::ate);
    if (! ifs.is_open()) { return std::nullopt; }

    const auto endPos = ifs.tellg();
    if (endPos < 0) { return std::nullopt; }

    const auto sizeAsUmax = static_cast<std::uintmax_t>(endPos);
    if (sizeAsUmax > std::numeric_limits<std::size_t>::max()) { return std::nullopt; }
    if (sizeAsUmax > static_cast<std::uintmax_t>(std::numeric_limits<std::streamsize>::max())) { return std::nullopt; }

    const auto             size = static_cast<std::size_t>(sizeAsUmax);
    std::vector<std::byte> bytes(size);

    ifs.seekg(0, std::ios::beg);
    if (! ifs) { return std::nullopt; }

    if (size != 0U) {
        ifs.read(reinterpret_cast<char *>(bytes.data()), static_cast<std::streamsize>(size));
        if (static_cast<std::size_t>(ifs.gcount()) != size) { return std::nullopt; }
    }

    return bytes;
}


inline fs::path get_curExeDir() {
#if defined(_WIN32)
    wchar_t buffer[MAX_PATH];
    GetModuleFileNameW(NULL, buffer, MAX_PATH);
    return fs::path(buffer).parent_path();
#elif defined(__APPLE__)
    char     buffer[1024];
    uint32_t size = sizeof(buffer);
    if (_NSGetExecutablePath(buffer, &size) == 0) { return fs::path(buffer).parent_path(); }
    else { throw std::runtime_error("Executable path too long"); }
#else
    char    buffer[1024];
    ssize_t count = readlink("/proc/self/exe", buffer, sizeof(buffer));
    if (count == -1) { throw std::runtime_error("Unable to resolve /proc/self/exe"); }
    return fs::path(std::string(buffer, count)).parent_path();
#endif
}

struct AccessInfo {
    bool readable;
    bool writable;
};
inline std::expected<AccessInfo, std::filesystem::file_type> check_access(const std::filesystem::path &p) {
    namespace fs = std::filesystem;
    std::error_code ec;

    // Resolve symlink (follows by default)
    auto ft = fs::status(p, ec).type();
    if (ec || ft == fs::file_type::not_found) { return std::unexpected(fs::file_type::not_found); }

    // Only allow normal files and directories
    if (ft != fs::file_type::regular && ft != fs::file_type::directory) { return std::unexpected(ft); }

    AccessInfo res{false, false};

    // ---- Regular file ----
    if (ft == fs::file_type::regular) {
        res.readable = std::ifstream(p).good();
        res.writable = std::ofstream(p, std::ios::app).good();
    }

    // ---- Directory ----
    else if (ft == fs::file_type::directory) {
        // Check directory readability by attempting to iterate
        fs::directory_iterator it(p, ec);
        res.readable = ! ec;

        // Check directory writability by trying to create a temp file
        auto          testfile = p / ".fs_test.tmp";
        std::ofstream ofs(testfile);
        if (ofs.good()) {
            res.writable = true;
            ofs.close();
            fs::remove(testfile, ec);
        }
    }

    return res;
}

namespace locations {
namespace detail {

inline std::expected<fs::path, std::error_code> invalid_app_name() {
    return std::unexpected(std::make_error_code(std::errc::invalid_argument));
}

inline std::expected<fs::path, std::error_code> no_location() {
    return std::unexpected(std::make_error_code(std::errc::no_such_file_or_directory));
}

inline std::optional<fs::path> env_path(const std::string &name) {
    if (const char *raw = std::getenv(name.c_str()); raw != nullptr && *raw != '\0') { return fs::path(raw); }
    return std::nullopt;
}

inline std::string app_env_key(std::string_view appName, std::string_view suffix) {
    std::string key;
    key.reserve(appName.size() + suffix.size() + 1U);
    for (const unsigned char ch : appName) {
        if (std::isalnum(ch) != 0) { key.push_back(static_cast<char>(std::toupper(ch))); }
        else { key.push_back('_'); }
    }
    key.push_back('_');
    key.append(suffix);
    return key;
}

inline fs::path with_vendor_and_app(fs::path base, std::string_view vendor, std::string_view appName) {
    if (! vendor.empty()) { base /= std::string(vendor); }
    return base / std::string(appName);
}

inline std::expected<fs::path, std::error_code> fallback_from_cwd(std::string_view appName, std::string_view category,
                                                                  bool allowFallback) {
    if (! allowFallback) { return no_location(); }

    std::error_code ec;
    const fs::path  cwd = fs::current_path(ec);
    if (ec) { return std::unexpected(ec); }

    fs::path out = cwd / ("." + std::string(appName));
    if (! category.empty()) { out /= std::string(category); }
    return out;
}

#if defined(_WIN32)
inline std::optional<fs::path> known_folder(const KNOWNFOLDERID &id) {
    PWSTR raw = nullptr;
    if (SHGetKnownFolderPath(id, KF_FLAG_DEFAULT, nullptr, &raw) != S_OK || raw == nullptr) { return std::nullopt; }

    fs::path out(raw);
    CoTaskMemFree(raw);
    return out;
}
#endif

} // namespace detail

inline std::expected<fs::path, std::error_code> roaming_user_dir(std::string_view appName, std::string_view vendor = {},
                                                                 bool allowFallback = true) {
    if (appName.empty()) { return detail::invalid_app_name(); }

    if (const auto overrideP = detail::env_path(detail::app_env_key(appName, "ROAMING_DIR")); overrideP) {
        return detail::with_vendor_and_app(overrideP.value(), vendor, appName);
    }

#if defined(_WIN32)
    if (const auto base = detail::known_folder(FOLDERID_RoamingAppData); base) {
        return detail::with_vendor_and_app(base.value(), vendor, appName);
    }
    if (allowFallback) {
        if (const auto appData = detail::env_path("APPDATA"); appData) {
            return detail::with_vendor_and_app(appData.value(), vendor, appName);
        }
    }
#else
    (void)vendor;
#endif

    return detail::fallback_from_cwd(appName, "roaming", allowFallback);
}

inline std::expected<fs::path, std::error_code> local_user_dir(std::string_view appName, std::string_view vendor = {},
                                                               bool allowFallback = true) {
    if (appName.empty()) { return detail::invalid_app_name(); }

    if (const auto overrideP = detail::env_path(detail::app_env_key(appName, "LOCAL_DIR")); overrideP) {
        return detail::with_vendor_and_app(*overrideP, vendor, appName);
    }

#if defined(_WIN32)
    if (const auto base = detail::known_folder(FOLDERID_LocalAppData); base) {
        return detail::with_vendor_and_app(*base, vendor, appName);
    }
    if (allowFallback) {
        if (const auto appData = detail::env_path("LOCALAPPDATA"); appData) {
            return detail::with_vendor_and_app(*appData, vendor, appName);
        }
    }
#else
    (void)vendor;
#endif

    return detail::fallback_from_cwd(appName, "local", allowFallback);
}

inline std::expected<fs::path, std::error_code> config_dir(std::string_view appName, std::string_view vendor = {},
                                                           bool allowFallback = true) {
    if (appName.empty()) { return detail::invalid_app_name(); }

    if (const auto overrideP = detail::env_path(detail::app_env_key(appName, "CONFIG_DIR")); overrideP) {
        return detail::with_vendor_and_app(*overrideP, vendor, appName);
    }

#if defined(_WIN32)
    return roaming_user_dir(appName, vendor, allowFallback);
#elif defined(__APPLE__)
    if (const auto xdg = detail::env_path("XDG_CONFIG_HOME"); xdg) { return *xdg / std::string(appName); }
    if (const auto home = detail::env_path("HOME"); home) {
        return detail::with_vendor_and_app(*home / "Library" / "Application Support", vendor, appName);
    }
    return detail::fallback_from_cwd(appName, "config", allowFallback);
#else
    if (const auto xdg = detail::env_path("XDG_CONFIG_HOME"); xdg) { return *xdg / std::string(appName); }
    if (const auto home = detail::env_path("HOME"); home) { return *home / ".config" / std::string(appName); }
    return detail::fallback_from_cwd(appName, "config", allowFallback);
#endif
}

inline std::expected<fs::path, std::error_code> data_dir(std::string_view appName, std::string_view vendor = {},
                                                         bool allowFallback = true) {
    if (appName.empty()) { return detail::invalid_app_name(); }

    if (const auto overrideP = detail::env_path(detail::app_env_key(appName, "DATA_DIR")); overrideP) {
        return detail::with_vendor_and_app(*overrideP, vendor, appName);
    }

#if defined(_WIN32)
    return local_user_dir(appName, vendor, allowFallback);
#elif defined(__APPLE__)
    if (const auto xdg = detail::env_path("XDG_DATA_HOME"); xdg) { return *xdg / std::string(appName); }
    if (const auto home = detail::env_path("HOME"); home) {
        return detail::with_vendor_and_app(*home / "Library" / "Application Support", vendor, appName);
    }
    return detail::fallback_from_cwd(appName, "data", allowFallback);
#else
    if (const auto xdg = detail::env_path("XDG_DATA_HOME"); xdg) { return *xdg / std::string(appName); }
    if (const auto home = detail::env_path("HOME"); home) { return *home / ".local" / "share" / std::string(appName); }
    return detail::fallback_from_cwd(appName, "data", allowFallback);
#endif
}

inline std::expected<fs::path, std::error_code> state_dir(std::string_view appName, std::string_view vendor = {},
                                                          bool allowFallback = true) {
    if (appName.empty()) { return detail::invalid_app_name(); }

    if (const auto overrideP = detail::env_path(detail::app_env_key(appName, "STATE_DIR")); overrideP) {
        return detail::with_vendor_and_app(*overrideP, vendor, appName);
    }

#if defined(_WIN32)
    auto base = local_user_dir(appName, vendor, allowFallback);
    if (base) { return *base / "State"; }
    return std::unexpected(base.error());
#elif defined(__APPLE__)
    if (const auto xdg = detail::env_path("XDG_STATE_HOME"); xdg) { return *xdg / std::string(appName); }
    if (const auto home = detail::env_path("HOME"); home) {
        return detail::with_vendor_and_app(*home / "Library" / "Application Support", vendor, appName) / "State";
    }
    return detail::fallback_from_cwd(appName, "state", allowFallback);
#else
    if (const auto xdg = detail::env_path("XDG_STATE_HOME"); xdg) { return *xdg / std::string(appName); }
    if (const auto home = detail::env_path("HOME"); home) { return *home / ".local" / "state" / std::string(appName); }
    return detail::fallback_from_cwd(appName, "state", allowFallback);
#endif
}

inline std::expected<fs::path, std::error_code> cache_dir(std::string_view appName, std::string_view vendor = {},
                                                          bool allowFallback = true) {
    if (appName.empty()) { return detail::invalid_app_name(); }

    if (const auto overrideP = detail::env_path(detail::app_env_key(appName, "CACHE_DIR")); overrideP) {
        return detail::with_vendor_and_app(*overrideP, vendor, appName);
    }

#if defined(_WIN32)
    auto base = local_user_dir(appName, vendor, allowFallback);
    if (base) { return *base / "Cache"; }
    return std::unexpected(base.error());
#elif defined(__APPLE__)
    if (const auto xdg = detail::env_path("XDG_CACHE_HOME"); xdg) { return *xdg / std::string(appName); }
    if (const auto home = detail::env_path("HOME"); home) {
        return detail::with_vendor_and_app(*home / "Library" / "Caches", vendor, appName);
    }
    return detail::fallback_from_cwd(appName, "cache", allowFallback);
#else
    if (const auto xdg = detail::env_path("XDG_CACHE_HOME"); xdg) { return *xdg / std::string(appName); }
    if (const auto home = detail::env_path("HOME"); home) { return *home / ".cache" / std::string(appName); }
    return detail::fallback_from_cwd(appName, "cache", allowFallback);
#endif
}

inline std::expected<fs::path, std::error_code> logs_dir(std::string_view appName, std::string_view vendor = {},
                                                         bool allowFallback = true) {
    if (appName.empty()) { return detail::invalid_app_name(); }

    if (const auto overrideP = detail::env_path(detail::app_env_key(appName, "LOG_DIR")); overrideP) {
        return detail::with_vendor_and_app(*overrideP, vendor, appName);
    }

#if defined(_WIN32)
    auto base = local_user_dir(appName, vendor, allowFallback);
    if (base) { return *base / "Logs"; }
    return std::unexpected(base.error());
#elif defined(__APPLE__)
    if (const auto home = detail::env_path("HOME"); home) {
        return detail::with_vendor_and_app(*home / "Library" / "Logs", vendor, appName);
    }
    return detail::fallback_from_cwd(appName, "logs", allowFallback);
#else
    auto base = state_dir(appName, vendor, allowFallback);
    if (base) { return *base / "logs"; }
    return std::unexpected(base.error());
#endif
}

inline std::expected<fs::path, std::error_code> runtime_dir(std::string_view appName, std::string_view vendor = {},
                                                            bool allowFallback = true) {
    if (appName.empty()) { return detail::invalid_app_name(); }

    if (const auto overrideP = detail::env_path(detail::app_env_key(appName, "RUNTIME_DIR")); overrideP) {
        return detail::with_vendor_and_app(*overrideP, vendor, appName);
    }

#if defined(_WIN32)
    auto base = local_user_dir(appName, vendor, allowFallback);
    if (base) { return *base / "Runtime"; }
    return std::unexpected(base.error());
#elif defined(__linux__)
    if (const auto xdg = detail::env_path("XDG_RUNTIME_DIR"); xdg) { return *xdg / std::string(appName); }
    if (! allowFallback) { return detail::no_location(); }
#endif

    std::error_code ec;
    const fs::path  tmpRoot = fs::temp_directory_path(ec);
    if (! ec) {
        fs::path out = detail::with_vendor_and_app(tmpRoot, vendor, appName);
        return out / "runtime";
    }

    return detail::fallback_from_cwd(appName, "runtime", allowFallback);
}

inline std::expected<fs::path, std::error_code> temp_dir(std::string_view appName, std::string_view vendor = {},
                                                         bool allowFallback = true) {
    if (appName.empty()) { return detail::invalid_app_name(); }

    if (const auto overrideP = detail::env_path(detail::app_env_key(appName, "TEMP_DIR")); overrideP) {
        return detail::with_vendor_and_app(*overrideP, vendor, appName);
    }

    std::error_code ec;
    const fs::path  tmpRoot = fs::temp_directory_path(ec);
    if (! ec) { return detail::with_vendor_and_app(tmpRoot, vendor, appName); }

    return detail::fallback_from_cwd(appName, "temp", allowFallback);
}

inline std::expected<fs::path, std::error_code> machine_data_dir(std::string_view appName, std::string_view vendor = {},
                                                                 bool allowFallback = true) {
    if (appName.empty()) { return detail::invalid_app_name(); }

    if (const auto overrideP = detail::env_path(detail::app_env_key(appName, "PROGRAM_DATA_DIR")); overrideP) {
        return detail::with_vendor_and_app(*overrideP, vendor, appName);
    }

#if defined(_WIN32)
    if (const auto base = detail::known_folder(FOLDERID_ProgramData); base) {
        return detail::with_vendor_and_app(*base, vendor, appName);
    }
    if (allowFallback) {
        if (const auto envProgramData = detail::env_path("PROGRAMDATA"); envProgramData) {
            return detail::with_vendor_and_app(*envProgramData, vendor, appName);
        }
    }
#elif defined(__APPLE__)
    return detail::with_vendor_and_app(fs::path("/Library") / "Application Support", vendor, appName);
#elif defined(__linux__)
    return detail::with_vendor_and_app(fs::path("/var") / "lib", vendor, appName);
#else
    (void)vendor;
#endif

    return detail::fallback_from_cwd(appName, "program-data", allowFallback);
}
} // namespace locations


// 3 Locations get tested:
// 1) Next to executable running this code
// 2) appName_CONFIG_DIR env variable directory
// 3) Default directory expected for a particular system (Linux, MacOS, Windows)
inline std::expected<fs::path, std::error_code> find_configFile(const std::string &appName, const std::string &file) {
    fs::path pthToTry = get_curExeDir() / file;
    if (fs::exists(pthToTry)) { return pthToTry; }
    else if (const char *env = std::getenv((appName + "_CONFIG_DIR").c_str())) {
        pthToTry = fs::path(env) / file;
        if (fs::exists(pthToTry)) { return pthToTry; }
    }

    else {
#if defined(_WIN32)
        // 2. Windows AppData
        if (char *appData = std::getenv("APPDATA")) {
            pthToTry = fs::path(appData) / appName / file;
            if (fs::exists(pthToTry)) { return pthToTry; }
        }
#elif defined(__APPLE__)
        // 2. macOS Application Support
        if (const char *home = std::getenv("HOME")) {
            pthToTry = fs::path(home) / "Library" / "Application Support" / appName / file;
            if (fs::exists(pthToTry)) { return pthToTry; }
        }
#else
        // 2. Linux/Unix: XDG or ~/.config
        if (const char *xdg = std::getenv("XDG_CONFIG_HOME")) {
            pthToTry = fs::path(xdg) / appName / file;
            if (fs::exists(pthToTry)) { return pthToTry; }
        }
        if (const char *home = std::getenv("HOME")) {
            pthToTry = fs::path(home) / ".config" / appName / file;
            if (fs::exists(pthToTry)) { return pthToTry; }
        }
#endif
    }
    return std::unexpected(std::make_error_code(std::errc::no_such_file_or_directory));
}

} // namespace incom::standard::filesys