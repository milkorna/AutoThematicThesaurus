#pragma once

#include <cctype>
#include <cstddef>
#include <filesystem>
#include <string>

namespace util::path {

// первый подряд идущий числовой блок из строки, иначе 0
inline std::size_t extractNumberFromPath(const std::string& s) {
    std::size_t res = 0;
    bool seen = false;
    for (unsigned char ch : s) {
        if (std::isdigit(ch)) {
            seen = true;
            res = res * 10 + (ch - '0');
        } else if (seen)
            break;
    }
    return res;
}

// извлечь первый подряд идущий числовой блок из строки пути; иначе 0
inline std::size_t extractNumberFromPath(const std::filesystem::path& p) {
    const std::string s = p.string();
    std::size_t res = 0;
    bool seen = false;
    for (unsigned char ch : s) {
        if (std::isdigit(ch)) {
            seen = true;
            res = res * 10 + (ch - '0');
        } else if (seen)
            break;
    }
    return res;
}

} // namespace util::path
