#pragma once

#include <filesystem>
#include <fstream>
#include <optional>

#include <incstd/hashing.hpp>
#include <ankerl/unordered_dense.h>

namespace incom::standard::filesys {
inline std::optional<std::string_view> get_file_textual(std::string_view const &sv) {
    static ankerl::unordered_dense::map<std::string, std::string, hashing::XXH3Hasher> storageMP;
    if (auto ele = storageMP.find(std::string(sv)); ele != storageMP.end()) { return std::string_view(ele->second); }
    else {
        std::ifstream ifs;
        ifs.open(std::filesystem::path(sv));
        if (ifs.is_open()) {
            return storageMP
                .insert({std::string(sv), std::string(std::istreambuf_iterator(ifs), std::istreambuf_iterator<char>())})
                .first->second;
        }
        else { return std::nullopt; }
    };
}
} // namespace incom::standard::filesys