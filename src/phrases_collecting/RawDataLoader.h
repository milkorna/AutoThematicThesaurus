#pragma once

#include "Document.h"

#include <filesystem>
#include <nlohmann/json.hpp>
#include <vector>

namespace fs = std::filesystem;

/**
 * @class RawDataLoader
 * @brief Handles loading and parsing of raw document data from JSON files
 * @details Responsible for:
 *   - Reading JSON files in RuTermEval_processed format
 *   - Parsing document records with validation
 *   - Extracting doc_id, title, and text fields
 *   - Logging load statistics and errors
 */
class RawDataLoader {
  public:
    /**
     * @brief Loads documents from a JSON file
     * @param jsonFile Path to the JSON file to load
     * @return Vector of Document objects successfully loaded and validated
     */
    static std::vector<Document> loadFromJson(const fs::path& jsonFile);

  private:
    /**
     * @brief Validates that the file path is valid and the file exists
     * @param filePath Path to validate
     * @return true if path is valid and file exists, false otherwise
     */
    static bool validateFilePath(const fs::path& filePath);

    /**
     * @brief Extracts and parses the documents array from JSON data
     * @param jsonStr Parsed JSON object as string
     * @param sourceFilePath Path to the source JSON file
     * @return Vector of successfully loaded and validated DocumentRecord objects
     *
     * @details Validates:
     *   - "documents" field exists and is an array
     *   - Each document has required fields
     *   - Each document passes validation criteria
     */
    static std::vector<Document> parseDocumentsArray(const std::string& jsonStr, const fs::path& sourceFilePath);

    /**
     * @brief Validates a single document record
     * @param doc Document to validate
     * @param index Index of document in the source array (for logging)
     * @return true if document is valid, false otherwise
     *
     * @details Validates:
     *   - doc_id is not empty
     *   - At least one of title or text is non-empty
     */
    static bool validateDocument(const Document& doc, size_t index);

    static std::string getCurrentTimestamp();
};