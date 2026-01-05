#pragma once

#include "Logger.h"
#include "Options.h"

#include <cctype>
#include <cstddef>
#include <filesystem>
#include <string>
#include <vector>

namespace util::path {

/**
 * @brief Extracts the first consecutive digit block from a file path.
 *
 * @param p The file path to search.
 * @return The extracted number or 0 if no digits found.
 */
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

/**
 * @brief Collects all result files from the configured results directory.
 *
 * @return Vector of file paths matching pattern "res*_text.json".
 */
inline std::vector<std::filesystem::path> getResultFiles() {
    auto& options = Options::getOptions();
    std::filesystem::path inputDir = options.resDir;
    std::vector<std::filesystem::path> filesToProcess;

    filesToProcess.reserve(options.totalDocuments);

    for (const auto& entry : std::filesystem::directory_iterator(inputDir)) {
        if (entry.is_regular_file()) {
            std::string filename = entry.path().filename().string();
            if (filename.starts_with("res") && filename.ends_with("_text.json")) {
                filesToProcess.push_back(entry.path());
            }
        }
    }

    return filesToProcess;
}

} // namespace util::path
