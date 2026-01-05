#pragma once

#include "TextCorpus.h"

#include <nlohmann/json.hpp>

/**
 * @brief Serializes and deserializes TextCorpus to/from JSON format
 * @details Handles persistence of corpus data including word frequencies, document frequencies,
 * and text collections. Deserialization performs no filtering (raw data loading).
 */
class TextCorpusLoader {
  public:
    /**
     * @brief Saves corpus data to JSON file
     * @details Serializes all corpus components (word frequencies, document frequencies, texts)
     * to JSON format with indentation for readability.
     *
     * @param corpus TextCorpus to save
     * @param filename Output file path
     *
     * @throws std::runtime_error If file cannot be opened or written
     */
    static void save(const TextCorpus& corpus, const std::string& filename);

    /**
     * @brief Loads corpus from JSON file without filtering
     * @details Deserializes all corpus components from JSON.
     * No stop word or text filtering applied - raw data only.
     *
     * @param corpus Reference to TextCorpus to populate
     * @param filename Path to JSON file
     *
     * @throws std::runtime_error If file cannot be opened or JSON is invalid
     */
    static void load(TextCorpus& corpus, const std::string& filename);

  private:
    /**
     * @brief Converts TextCorpus to JSON representation
     * @details Serializes document count, text count, word count, frequencies, and text data.
     * Uses numeric prefixes (0-5) to control JSON key ordering for readability.
     *
     * @param corpus TextCorpus to serialize
     * @return JSON object with corpus data
     */
    [[nodiscard]] static nlohmann::json serialize(const TextCorpus& corpus);

    /**
     * @brief Populates TextCorpus from JSON representation
     * @details Deserializes all corpus components without any filtering.
     * Clears existing corpus data before loading.
     *
     * @param corpus Reference to TextCorpus to populate
     * @param j JSON object containing corpus data
     *
     * @throws std::runtime_error If JSON structure is invalid or missing required fields
     */
    static void deserialize(TextCorpus& corpus, const nlohmann::json& j);

    /**
     * @brief Loads word frequency map from JSON
     * @details Reads word-to-frequency mappings from JSON object
     *
     * @param corpus TextCorpus to update
     * @param j JSON object containing word frequency data
     */
    static void readWordFrequencies(TextCorpus& corpus, const nlohmann::json& j);

    /**
     * @brief Loads document frequency map from JSON
     * @details Reads word-to-document-count mappings from JSON object
     *
     * @param corpus TextCorpus to update
     * @param j JSON object containing document frequency data
     */
    static void readDocumentFrequencies(TextCorpus& corpus, const nlohmann::json& j);

    /**
     * @brief Loads text collections from JSON
     * @details Reads documents and their associated text lists from JSON
     *
     * @param corpus TextCorpus to update
     * @param j JSON object containing text data
     */
    static void readTexts(TextCorpus& corpus, const nlohmann::json& j);
};