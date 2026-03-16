#pragma once

#include <cstdlib>
#include <expected>
#include <filesystem>
#include <fstream>
#include <limits>
#include <new>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

#include <ankerl/unordered_dense.h>
#include <incstd/core/hashing.hpp>

#if ! defined(_WIN32)
#include <cerrno>
#endif

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


inline std::expected<fs::path, std::error_code> get_curExeDir() {
    try {
#if defined(_WIN32)
        wchar_t buffer[MAX_PATH];
        const DWORD len = GetModuleFileNameW(nullptr, buffer, MAX_PATH);
        if (len == 0) {
            const DWORD winErr = GetLastError();
            return std::unexpected(
                std::error_code(static_cast<int>(winErr != 0 ? winErr : ERROR_GEN_FAILURE), std::system_category()));
        }

        std::error_code ec;
        const fs::path  exeDir = fs::path(buffer).parent_path();
        const fs::path  canonicalExeDir = fs::canonical(exeDir, ec);
        if (ec) { return std::unexpected(ec); }
        return canonicalExeDir;
#elif defined(__APPLE__)
        char     buffer[1024];
        uint32_t size = sizeof(buffer);
        if (_NSGetExecutablePath(buffer, &size) != 0) {
            return std::unexpected(std::make_error_code(std::errc::file_name_too_long));
        }

        std::error_code ec;
        const fs::path  exeDir = fs::path(buffer).parent_path();
        const fs::path  canonicalExeDir = fs::canonical(exeDir, ec);
        if (ec) { return std::unexpected(ec); }
        return canonicalExeDir;
#else
        char    buffer[1024];
        ssize_t count = readlink("/proc/self/exe", buffer, sizeof(buffer));
        if (count == -1) { return std::unexpected(std::error_code(errno, std::generic_category())); }

        std::error_code ec;
        const fs::path  exeDir = fs::path(std::string(buffer, static_cast<std::size_t>(count))).parent_path();
        const fs::path  canonicalExeDir = fs::canonical(exeDir, ec);
        if (ec) { return std::unexpected(ec); }
        return canonicalExeDir;
#endif
    }
    catch (const fs::filesystem_error &e) {
        return std::unexpected(e.code());
    }
    catch (const std::bad_alloc &) {
        return std::unexpected(std::make_error_code(std::errc::not_enough_memory));
    }
    catch (...) {
        return std::unexpected(std::make_error_code(std::errc::io_error));
    }
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
using namespace std::literals;

namespace detail {

inline std::expected<fs::path, std::error_code> no_location() {
    return std::unexpected(std::make_error_code(std::errc::no_such_file_or_directory));
}

inline std::optional<fs::path> env_path(const std::string &name) {
    if (const char *raw = std::getenv(name.c_str()); raw != nullptr && *raw != '\0') { return fs::path(raw); }
    return std::nullopt;
}
inline std::expected<fs::path, std::error_code> fallback_from_cwd(std::string_view category, bool allowFallback) {
    if (! allowFallback) { return no_location(); }

    std::error_code ec;
    const fs::path  cwd = fs::current_path(ec);
    if (ec) { return std::unexpected(ec); }

    fs::path out = cwd;
    if (! category.empty()) { out /= std::string(category); }
    return out;
}

inline std::expected<fs::path, std::error_code> with_app_name(std::expected<fs::path, std::error_code> base,
                                                              std::string_view                         appName) {
    if (! base) { return std::unexpected(base.error()); }
    if (appName.empty()) { return std::unexpected(std::make_error_code(std::errc::invalid_argument)); }
    return *base / std::string(appName);
}

inline std::expected<fs::path, std::error_code> with_app_name_windows_suffix(
    std::expected<fs::path, std::error_code> base, std::string_view appName, std::string_view windowsSuffix) {
    if (! base) { return std::unexpected(base.error()); }
    if (appName.empty()) { return std::unexpected(std::make_error_code(std::errc::invalid_argument)); }

#if defined(_WIN32)
    fs::path out  = *base;
    out          /= std::string(appName);
    if (! windowsSuffix.empty()) { out /= std::string(windowsSuffix); }
    return out;
#endif
    return *base / std::string(appName);
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

inline std::expected<fs::path, std::error_code> roaming_user_dir(bool allowFallback = true) {
#if defined(_WIN32)
    if (const auto base = detail::known_folder(FOLDERID_RoamingAppData); base) { return *base; }
    if (allowFallback) {
        if (const auto appData = detail::env_path("APPDATA"); appData) { return *appData; }
    }
#endif

    return detail::fallback_from_cwd("roaming", allowFallback);
}

inline std::expected<fs::path, std::error_code> roaming_user_dir(std::string_view appName, bool allowFallback = true) {
    return detail::with_app_name(roaming_user_dir(allowFallback), appName);
}

inline std::expected<fs::path, std::error_code> local_user_dir(bool allowFallback = true) {
#if defined(_WIN32)
    // if (const auto base = detail::known_folder(FOLDERID_LocalAppData); base) { return *base; }
    if (allowFallback) {
        if (const auto appData = detail::env_path("LOCALAPPDATA"); appData) { return *appData; }
    }
#endif

    return detail::fallback_from_cwd("local", allowFallback);
}

inline std::expected<fs::path, std::error_code> local_user_dir(std::string_view appName, bool allowFallback = true) {
    return detail::with_app_name(local_user_dir(allowFallback), appName);
}

inline std::expected<fs::path, std::error_code> config_dir(bool allowFallback = true) {
#if defined(_WIN32)
    return roaming_user_dir(allowFallback);
#elif defined(__APPLE__)
    if (const auto xdg = detail::env_path("XDG_CONFIG_HOME"); xdg) { return *xdg; }
    if (const auto home = detail::env_path("HOME"); home) { return *home / "Library" / "Application Support"; }
    return detail::fallback_from_cwd("config", allowFallback);
#else
    if (const auto xdg = detail::env_path("XDG_CONFIG_HOME"); xdg) { return *xdg; }
    if (const auto home = detail::env_path("HOME"); home) { return *home / ".config"; }
    return detail::fallback_from_cwd("config", allowFallback);
#endif
}

inline std::expected<fs::path, std::error_code> config_dir(std::string_view appName, bool allowFallback = true) {
    return detail::with_app_name(config_dir(allowFallback), appName);
}

inline std::expected<fs::path, std::error_code> data_dir(bool allowFallback = true) {
#if defined(_WIN32)
    return local_user_dir(allowFallback);
#elif defined(__APPLE__)
    if (const auto xdg = detail::env_path("XDG_DATA_HOME"); xdg) { return *xdg; }
    if (const auto home = detail::env_path("HOME"); home) { return *home / "Library" / "Application Support"; }
    return detail::fallback_from_cwd("data", allowFallback);
#else
    if (const auto xdg = detail::env_path("XDG_DATA_HOME"); xdg) { return *xdg; }
    if (const auto home = detail::env_path("HOME"); home) { return *home / ".local" / "share"; }
    return detail::fallback_from_cwd("data", allowFallback);
#endif
}

inline std::expected<fs::path, std::error_code> data_dir(std::string_view appName, bool allowFallback = true) {
    return detail::with_app_name(data_dir(allowFallback), appName);
}

inline std::expected<fs::path, std::error_code> state_dir(bool allowFallback = true) {
#if defined(_WIN32)
    return local_user_dir(allowFallback);
#elif defined(__APPLE__)
    if (const auto xdg = detail::env_path("XDG_STATE_HOME"); xdg) { return *xdg; }
    if (const auto home = detail::env_path("HOME"); home) {
        return *home / "Library" / "Application Support" / "State";
    }
    return detail::fallback_from_cwd("state", allowFallback);
#else
    if (const auto xdg = detail::env_path("XDG_STATE_HOME"); xdg) { return *xdg; }
    if (const auto home = detail::env_path("HOME"); home) { return *home / ".local" / "state"; }
    return detail::fallback_from_cwd("state", allowFallback);
#endif
}

inline std::expected<fs::path, std::error_code> state_dir(std::string_view appName, bool allowFallback = true) {
    return detail::with_app_name_windows_suffix(state_dir(allowFallback), appName, "State"sv);
}

inline std::expected<fs::path, std::error_code> cache_dir(bool allowFallback = true) {
#if defined(_WIN32)
    return local_user_dir(allowFallback);
#elif defined(__APPLE__)
    if (const auto xdg = detail::env_path("XDG_CACHE_HOME"); xdg) { return *xdg; }
    if (const auto home = detail::env_path("HOME"); home) { return *home / "Library" / "Caches"; }
    return detail::fallback_from_cwd("cache", allowFallback);
#else
    if (const auto xdg = detail::env_path("XDG_CACHE_HOME"); xdg) { return *xdg; }
    if (const auto home = detail::env_path("HOME"); home) { return *home / ".cache"; }
    return detail::fallback_from_cwd("cache", allowFallback);
#endif
}

inline std::expected<fs::path, std::error_code> cache_dir(std::string_view appName, bool allowFallback = true) {
    return detail::with_app_name_windows_suffix(cache_dir(allowFallback), appName, "Cache"sv);
}

inline std::expected<fs::path, std::error_code> logs_dir(bool allowFallback = true) {
#if defined(_WIN32)
    return local_user_dir(allowFallback);
#elif defined(__APPLE__)
    if (const auto home = detail::env_path("HOME"); home) { return *home / "Library" / "Logs"; }
    return detail::fallback_from_cwd("logs", allowFallback);
#else
    auto base = state_dir(allowFallback);
    if (base) { return *base / "logs"; }
    return std::unexpected(base.error());
#endif
}

inline std::expected<fs::path, std::error_code> logs_dir(std::string_view appName, bool allowFallback = true) {
    return detail::with_app_name_windows_suffix(logs_dir(allowFallback), appName, "Logs"sv);
}

inline std::expected<fs::path, std::error_code> runtime_dir(bool allowFallback = true) {
#if defined(_WIN32)
    return local_user_dir(allowFallback);
#elif defined(__linux__)
    if (const auto xdg = detail::env_path("XDG_RUNTIME_DIR"); xdg) { return *xdg; }
    if (! allowFallback) { return detail::no_location(); }
#endif

    std::error_code ec;
    const fs::path  tmpRoot = fs::temp_directory_path(ec);
    if (! ec) { return tmpRoot / "runtime"; }

    return detail::fallback_from_cwd("runtime", allowFallback);
}

inline std::expected<fs::path, std::error_code> runtime_dir(std::string_view appName, bool allowFallback = true) {
    return detail::with_app_name_windows_suffix(runtime_dir(allowFallback), appName, "Runtime"sv);
}

inline std::expected<fs::path, std::error_code> temp_dir(bool allowFallback = true) {
    std::error_code ec;
    const fs::path  tmpRoot = fs::temp_directory_path(ec);
    if (! ec) { return tmpRoot; }

    return detail::fallback_from_cwd("temp", allowFallback);
}

inline std::expected<fs::path, std::error_code> temp_dir(std::string_view appName, bool allowFallback = true) {
    return detail::with_app_name(temp_dir(allowFallback), appName);
}

inline std::expected<fs::path, std::error_code> machine_data_dir(bool allowFallback = true) {
#if defined(_WIN32)
    if (const auto base = detail::known_folder(FOLDERID_ProgramData); base) { return *base; }
    if (allowFallback) {
        if (const auto envProgramData = detail::env_path("PROGRAMDATA"); envProgramData) { return *envProgramData; }
    }
#elif defined(__APPLE__)
    return fs::path("/Library") / "Application Support";
#elif defined(__linux__)
    return fs::path("/var") / "lib";
#endif

    return detail::fallback_from_cwd("program-data", allowFallback);
}

inline std::expected<fs::path, std::error_code> machine_data_dir(std::string_view appName, bool allowFallback = true) {
    return detail::with_app_name(machine_data_dir(allowFallback), appName);
}
} // namespace locations


// 3 Locations get tested:
// 1) Next to executable running this code
// 2) appName_CONFIG_DIR env variable directory
// 3) Default directory expected for a particular system (Linux, MacOS, Windows)
inline std::expected<fs::path, std::error_code> find_configFile(const std::string &appName, const std::string &file) {
    auto exeDir = get_curExeDir();
    if (! exeDir) { return std::unexpected(exeDir.error()); }

    std::error_code ec;
    fs::path         pthToTry = *exeDir / file;
    if (fs::exists(pthToTry, ec)) { return pthToTry; }
    if (ec) { return std::unexpected(ec); }

    else if (const char *env = std::getenv((appName + "_CONFIG_DIR").c_str())) {
        pthToTry = fs::path(env) / file;
        ec.clear();
        if (fs::exists(pthToTry, ec)) { return pthToTry; }
        if (ec) { return std::unexpected(ec); }
    }

    else {
#if defined(_WIN32)
        // 2. Windows AppData
        if (char *appData = std::getenv("APPDATA")) {
            pthToTry = fs::path(appData) / appName / file;
            ec.clear();
            if (fs::exists(pthToTry, ec)) { return pthToTry; }
            if (ec) { return std::unexpected(ec); }
        }
#elif defined(__APPLE__)
        // 2. macOS Application Support
        if (const char *home = std::getenv("HOME")) {
            pthToTry = fs::path(home) / "Library" / "Application Support" / appName / file;
            ec.clear();
            if (fs::exists(pthToTry, ec)) { return pthToTry; }
            if (ec) { return std::unexpected(ec); }
        }
#else
        // 2. Linux/Unix: XDG or ~/.config
        if (const char *xdg = std::getenv("XDG_CONFIG_HOME")) {
            pthToTry = fs::path(xdg) / appName / file;
            ec.clear();
            if (fs::exists(pthToTry, ec)) { return pthToTry; }
            if (ec) { return std::unexpected(ec); }
        }
        if (const char *home = std::getenv("HOME")) {
            pthToTry = fs::path(home) / ".config" / appName / file;
            ec.clear();
            if (fs::exists(pthToTry, ec)) { return pthToTry; }
            if (ec) { return std::unexpected(ec); }
        }
#endif
    }
    return std::unexpected(std::make_error_code(std::errc::no_such_file_or_directory));
}

} // namespace incom::standard::filesys