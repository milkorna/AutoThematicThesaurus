#pragma once

#include "Phrase.h"
#include "PhraseCluster.h"

#include <nlohmann/json.hpp>
#include <string>

/**
 * @brief Deserializes JSON objects into PhraseCluster objects.
 * @details Reconstructs complete cluster data from JSON format with validation
 *          and error handling. Supports two deserialization scenarios:
 *          1. Full clusters with all metrics (from saveClusters output)
 *          2. Raw phrase results from sentence processing
 */
class ClusterDeserializer {
  public:
    /**
     * @brief Deserialize a single cluster from JSON object.
     * @details Reconstructs PhraseCluster with all fields:
     *          lemmas, TF/IDF values, semantic relations, word complexes.
     *
     * @param obj JSON object with structure matching ClusterSerializer output
     * @param key The cluster key (normalized phrase form)
     * @return Fully reconstructed PhraseCluster
     *
     * @throws std::runtime_error If JSON structure is invalid or required fields missing
     */
    [[nodiscard]] PhraseCluster deserializeCluster(const nlohmann::ordered_json& obj, const std::string& key);

    /**
     * @brief Deserialize phrase result object into phrase.
     * @details Used when loading raw results from phrase collection pipeline.
     *          Validates key format and filters invalid entries.
     *
     * @param obj JSON object with phrase result structure
     * @return PhrasePtr if valid, nullptr if should be skipped
     *
     * @note Returns nullptr for entries with underscores in key or digits in key
     * @see LoadPhraseStorageFromResultsDir
     */
    [[nodiscard]] PhrasePtr deserializePhraseResult(const nlohmann::ordered_json& obj);

  private:
    /**
     * @brief Deserialize lemmas array from JSON.
     * @param lemmas_json JSON array with lemma objects
     * @param cluster Reference to cluster to populate
     *
     * @throws std::runtime_error If lemma structure is invalid
     */
    void deserializeLemmas(const nlohmann::ordered_json& lemmas_json, PhraseCluster& cluster);

    /**
     * @brief Deserialize word complexes (phrases) array from JSON.
     * @param phrases_json JSON array with phrase objects
     * @param cluster Reference to cluster to populate
     *
     * @throws std::runtime_error If phrase structure is invalid
     */
    void deserializePhrases(const nlohmann::ordered_json& phrases_json, PhraseCluster& cluster);

    /**
     * @brief Extract lemma string from numbered format.
     * @details Converts "0_lemma_name" to "lemma_name"
     *
     * @param numberedLemma Lemma string with number prefix
     * @return Pure lemma string without prefix
     */
    // [[nodiscard]] std::string extractLemmaString(const std::string& numberedLemma) const;

    /**
     * @brief Validate that phrase key should be included.
     * @details Filters out keys with underscores or containing digits
     *
     * @param key The phrase key to validate
     * @return true if key is valid, false if should be skipped
     */
    bool isValidPhraseKey(const std::string& key) const;
};
