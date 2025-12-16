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
 * @brief Collects all text files to process from the configured input directory.
 *
 * @return Vector of file paths matching pattern "art*_text.txt".
 */
inline std::vector<std::filesystem::path> getFilesToProcess() {
    std::vector<std::filesystem::path> filesToProcess;

    try {
        auto& options = Options::getOptions();
        std::filesystem::path inputDir = options.textsDir;

        if (!std::filesystem::exists(inputDir) || !std::filesystem::is_directory(inputDir)) {
            throw std::runtime_error("Input directory does not exist or is not a directory: " + inputDir.string());
        }

        filesToProcess.reserve(options.textToProcessCount);

        for (const auto& entry : std::filesystem::directory_iterator(inputDir)) {
            if (entry.is_regular_file()) {
                std::string filename = entry.path().filename().string();
                if (filename.starts_with("art") && filename.ends_with("_text.txt")) {
                    filesToProcess.push_back(entry.path());
                }
            }
        }

        Logger::log("GetFilesToProcess", LogLevel::Info,
                    "Successfully collected " + std::to_string(filesToProcess.size()) + " files for processing.");

    } catch (const std::filesystem::filesystem_error& ex) {
        Logger::log("GetFilesToProcess", LogLevel::Error, "Filesystem error: " + std::string(ex.what()));
    } catch (const std::exception& ex) {
        Logger::log("GetFilesToProcess", LogLevel::Error, "Exception: " + std::string(ex.what()));
    } catch (...) {
        Logger::log("GetFilesToProcess", LogLevel::Error, "Unknown error while collecting files.");
    }

    return filesToProcess;
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

    filesToProcess.reserve(options.textToProcessCount);

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
