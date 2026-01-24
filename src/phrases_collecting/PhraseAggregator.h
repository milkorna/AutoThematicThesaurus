#pragma once

#include "Phrase.h"
#include "PhraseCluster.h"

#include <filesystem>
#include <nlohmann/json.hpp>
#include <unordered_map>
#include <vector>

namespace fs = std::filesystem;
using json = nlohmann::ordered_json;

/**
 * @brief Aggregates phrase results from multiple JSON files into unified storage
 * @details Single responsibility: collect all phrases from results directory,
 * deduplicate by creating clusters with identical keys, and save to single JSON file.
 * Does NOT compute any metrics (TF, IDF, LSA, topic relevance, etc.).
 */
class PhraseAggregator {
  public:
    /**
     * @brief Loads all phrase results from results directory and aggregates into clusters
     * @details Scans results directory for JSON files, reads phrase objects,
     * groups them by lemma-based key (deduplication).
     *
     * @param resultsDir Path to directory containing phrase result files (*.json)
     * @return Map of cluster key to PhraseCluster containing deduplicated phrases
     *
     * @throws std::runtime_error If directory cannot be accessed or JSON is invalid
     */
    [[nodiscard]] static std::unordered_map<std::string, PhraseCluster> aggregatePhrases(const fs::path& resultsDir);

    /**
     * @brief Saves aggregated clusters to single JSON file
     * @details Serializes clusters to JSON object (key -> cluster data).
     * Format compatible with PhrasesStorageLoader::loadStorageFromFile().
     *
     * @param clusters Map of cluster key to PhraseCluster
     * @param outputPath Path where to save the aggregated JSON file
     *
     * @throws std::runtime_error If file cannot be written
     */
    static void saveClusters(const std::unordered_map<std::string, PhraseCluster>& clusters,
                             const fs::path& outputPath);

  private:
    /**
     * @brief Lists all phrase result files in directory
     * @details Finds all JSON files matching pattern: *_res.json
     *
     * @param resultsDir Path to results directory
     * @return Vector of paths to JSON result files (sorted for deterministic order)
     *
     * @throws std::runtime_error If directory access fails
     */
    [[nodiscard]] static std::vector<fs::path> getResultFilesFromDirectory(const fs::path& resultsDir);

    /**
     * @brief Loads single result JSON file and extracts phrases
     * @details Reads JSON array from file, deserializes phrase objects,
     * and adds them to clusters map (creating new clusters as needed).
     *
     * @param filePath Path to result JSON file
     * @param clusters Map to add phrases to (modified in-place)
     *
     * @note Silently skips invalid phrase entries
     */
    static void loadResultFile(const fs::path& filePath, std::unordered_map<std::string, PhraseCluster>& clusters);

    /**
     * @brief Deserializes phrase from JSON object
     * @details Extracts required fields: key, textForm, modelName, docId, sentNum,
     * start_token_ind, end_token_ind, span, lemmas.
     *
     * @param obj JSON object representing phrase result
     * @return Shared pointer to deserialized Phrase, or nullptr if invalid
     *
     * @note Automatically filters invalid keys (containing underscores or digits)
     */
    [[nodiscard]] static PhrasePtr deserializePhraseFromJson(const json& obj);

    /**
     * @brief Creates cluster key from lemmas
     * @details Joins lemmas with space separator.
     *
     * @param lemmas Vector of lemma strings
     * @return Space-separated key (e.g., "географический название")
     */
    [[nodiscard]] static std::string createKeyFromLemmas(const std::vector<std::string>& lemmas);

    /**
     * @brief Creates new cluster from phrase
     * @details Initializes PhraseCluster with phrase data (lemmas with metrics, model, phrase itself).
     * Leaves all metric values at default (0.0).
     *
     * @param key Cluster key (space-separated lemmas)
     * @param phrase The phrase to initialize cluster with
     * @param lemmas Vector of lemma texts
     * @return New PhraseCluster ready for storage
     */
    [[nodiscard]] static PhraseCluster createClusterFromPhrase(const std::string& key, const PhrasePtr& phrase,
                                                               const std::vector<std::string>& lemmas);
};