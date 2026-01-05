#pragma once

#include <filesystem>
#include <nlohmann/json.hpp>
#include <string>

using json = nlohmann::json;

/**
 * @brief Processing context for document phrase collection and JSON output
 * @details Manages document metadata, output file, and accumulated JSON results
 * from phrase extraction operations. Automatically loads existing data on construction
 * and saves accumulated results to file on destruction.
 */
struct Process {
    /// @brief Document identifier
    std::string docId;

    /// @brief Output file path for JSON results
    std::filesystem::path outputFile;

    /// @brief JSON array accumulating phrase extraction results
    nlohmann::json jsonData;

    /// @brief Current sentence number (for sentence-level tracking)
    size_t sentNum;

    /**
     * @brief Constructs Process context for document
     * @details Loads existing JSON file if present, otherwise creates new array.
     * Handles corrupted files by creating empty array with error logging.
     *
     * @param docId Unique document identifier
     * @param outputFile Path to output JSON file
     * @param sentNum Initial sentence number (default: 0)
     */
    explicit Process(const std::string& docId, const std::filesystem::path& outputFile, size_t sentNum = 0);

    /**
     * @brief Destructor: writes accumulated JSON to file
     * @details Saves jsonData array to file with 4-space indentation.
     * Logs success or failure of write operation.
     */
    ~Process();

    /**
     * @brief Appends JSON object to results array
     * @details Adds object to jsonData array for later persistence
     *
     * @param newObj JSON object to append
     */
    void addJsonObject(const nlohmann::json& newObj);
};