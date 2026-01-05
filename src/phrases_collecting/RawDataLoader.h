#pragma once

#include "DocumentRecord.h"

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
     * @return Vector of DocumentRecord objects successfully loaded and validated
     *
     * @details Expected JSON format:
     * {
     *   "metadata": { ... },
     *   "documents": [
     *     {
     *       "doc_id": "string",
     *       "title": "string",
     *       "text": "string",
     *       ...
     *     },
     *     ...
     *   ]
     * }
     *
     * @note Documents are validated for:
     *   - Non-empty doc_id
     *   - At least one of title or text must be non-empty
     */
    static std::vector<DocumentRecord> LoadFromJson(const fs::path& jsonFile);

  private:
    /**
     * @brief Validates that the file path is valid and the file exists
     * @param filePath Path to validate
     * @return true if path is valid and file exists, false otherwise
     */
    static bool ValidateFilePath(const fs::path& filePath);

    /**
     * @brief Extracts and parses the documents array from JSON data
     * @param data Parsed JSON object as string
     * @return Vector of successfully loaded and validated DocumentRecord objects
     *
     * @details Validates:
     *   - "documents" field exists and is an array
     *   - Each document has required fields
     *   - Each document passes validation criteria
     */
    static std::vector<DocumentRecord> ParseDocumentsArray(const std::string& jsonStr);

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
    static bool ValidateDocument(const DocumentRecord& doc, size_t index);
};