#pragma once

#include "PhraseCluster.h"
#include <filesystem>
#include <string>
#include <unordered_map>

namespace fs = std::filesystem;
using json = nlohmann::ordered_json;

/**
 * @brief Aggregates and manages phrase clusters from various sources
 * @details Handles loading clusters from multiple sources (results directory, JSON file)
 * and saving to multiple formats (JSON, results directory).
 *
 * This class is responsible for all I/O operations related to phrase clusters,
 * providing a symmetric interface for loading and saving operations.
 */
class ClusterAggregator {
  public:
    /**
     * @brief Aggregates phrases from results directory into clusters
     * @details Iterates through all _res.json files in the directory,
     * loads phrases, and groups them by cluster key (space-separated lemmas).
     * Performs deduplication and creates cluster metadata.
     *
     * @param resultsDir Path to directory containing _res.json files
     * @return Map of cluster key to PhraseCluster objects
     * @throws std::runtime_error if directory doesn't exist or is invalid
     */
    [[nodiscard]] static std::unordered_map<std::string, PhraseCluster>
    loadFromResultsDirectory(const fs::path& resultsDir);

    /**
     * @brief Loads pre-aggregated clusters from JSON file
     * @details Parses JSON object where keys are cluster keys and values
     * are complete PhraseCluster objects with all metadata and phrases.
     * Useful for loading cached or previously aggregated results.
     *
     * @param filePath Path to JSON file with aggregated clusters
     * @return Map of cluster key to PhraseCluster objects
     * @throws std::runtime_error if file not found or JSON is invalid
     */
    [[nodiscard]] static std::unordered_map<std::string, PhraseCluster> loadFromJsonFile(const fs::path& filePath);

    /**
     * @brief Saves clusters to JSON file
     * @details Serializes complete cluster data including all metadata,
     * lemmas with metrics, and phrases with positions.
     * Output is sorted by cluster key for consistent output.
     *
     * @param clusters Map of cluster key to PhraseCluster objects
     * @param outputPath Path to output JSON file
     * @throws std::runtime_error if file cannot be opened for writing
     */
    static void saveClusters(const std::unordered_map<std::string, PhraseCluster>& clusters,
                             const fs::path& outputPath);

  private:
    // ===== Deserialization helpers =====

    /**
     * @brief Gets all result files from directory
     * @details Finds all files matching pattern *_res.json and sorts them.
     *
     * @param resultsDir Directory to search
     * @return Sorted vector of file paths
     */
    [[nodiscard]] static std::vector<fs::path> getResultFilesFromDirectory(const fs::path& resultsDir);

    /**
     * @brief Loads and processes single result file
     * @details Reads JSON array of phrases and adds them to clusters map.
     *
     * @param filePath Path to _res.json file
     * @param clusters Map to add phrases to
     */
    static void loadResultFile(const fs::path& filePath, std::unordered_map<std::string, PhraseCluster>& clusters);

    /**
     * @brief Deserializes phrase from JSON object
     * @details Extracts all phrase fields from JSON.
     *
     * @param obj JSON object with phrase data
     * @return Shared pointer to Phrase, nullptr if invalid
     */
    [[nodiscard]] static PhrasePtr deserializePhraseFromJson(const json& obj);

    /**
     * @brief Deserializes single cluster from JSON object
     * @details Reconstructs PhraseCluster with all metadata and phrases.
     *
     * @param key Cluster key (from parent object)
     * @param clusterJson JSON object representing cluster
     * @return PhraseCluster with all data populated
     */
    [[nodiscard]] static PhraseCluster deserializeClusterFromJson(const std::string& key, const json& clusterJson);

    /**
     * @brief Deserializes lemma metrics from JSON
     * @details Extracts text, tf, idf, tfidf from lemma object.
     *
     * @param lemmaJson JSON object with lemma data
     * @return LemmaMetrics with populated fields
     */
    [[nodiscard]] static LemmaMetrics deserializeLemmaFromJson(const json& lemmaJson);

    /**
     * @brief Creates new cluster from phrase
     * @details Initializes PhraseCluster with phrase data.
     * Leaves all metric values at default (0.0).
     *
     * @param key Cluster key
     * @param phrase The phrase to initialize cluster with
     * @param lemmas Vector of lemma texts
     * @return New PhraseCluster ready for storage
     */
    [[nodiscard]] static PhraseCluster createClusterFromPhrase(const std::string& key, const PhrasePtr& phrase,
                                                               const std::vector<std::string>& lemmas);

    /**
     * @brief Counts total number of phrases across all clusters
     * @details Iterates through all clusters and sums up phrase counts.
     * Used to track aggregation progress and verify loading correctness.
     *
     * @param clusters Map of cluster key to PhraseCluster objects
     * @return Total count of phrases in all clusters
     */
    [[nodiscard]] static size_t countPhrases(const std::unordered_map<std::string, PhraseCluster>& clusters);
};