#pragma once

#include "PatternPhrasesStorage.h"
#include "ClusterDeserializer.h"
#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include <filesystem>

namespace fs = std::filesystem;
using json = nlohmann::json;

/**
 * @brief Loads cluster and phrase data from files into PatternPhrasesStorage.
 * @details Provides two main loading scenarios:
 *          1. Load pre-computed clusters from JSON file (all metrics preserved)
 *          2. Load raw phrase results from results directory (initial clustering)
 *
 * Uses ClusterDeserializer for JSON parsing and validation.
 */
class PhrasesStorageLoader {
public:
    /**
     * @brief Load complete cluster storage from JSON file.
     * @details Reads JSON file containing clusters with all computed metrics
     *          (TF, IDF, centrality scores, etc.) and loads them into storage.
     *
     * @param storage Reference to PatternPhrasesStorage to populate
     * @param filename Path to JSON file with cluster data
     *
     * @throws std::runtime_error If file cannot be opened or JSON is invalid
     *
     * @see saveClusters() for output format specification
     */
    void loadStorageFromFile(PatternPhrasesStorage& storage, 
                            const std::string& filename);

    /**
     * @brief Load phrase collection results from results directory.
     * @details Reads all JSON result files from resDir, extracts phrase objects,
     *          and aggregates them into clusters. This is the initial loading
     *          step after phrase collection phase.
     *
     * @param storage Reference to PatternPhrasesStorage to populate
     *
     * @note Results directory path comes from Options::getOptions().resDir
     * @note Automatically filters invalid phrase keys (with underscores or digits)
     *
     * @throws std::runtime_error If directory cannot be accessed
     *
     * @see GetResFiles() for file discovery
     */
    void loadPhraseStorageFromResultsDir(PatternPhrasesStorage& storage);

private:
    /**
     * @brief Deserializer instance for parsing JSON objects.
     */
    ClusterDeserializer m_deserializer;

    /**
     * @brief List all result files in results directory.
     * @details Finds all files matching pattern: res_*_text.json
     *
     * @param resultsDir Path to results directory
     * @return Vector of paths to JSON result files
     *
     * @throws std::runtime_error If directory access fails
     */
    std::vector<fs::path> getResultFilesFromDirectory(const fs::path& resultsDir) const;

    /**
     * @brief Load single result JSON file and aggregate phrases.
     * @details Reads JSON array from file, parses each phrase object,
     *          and adds to storage (creating or updating clusters).
     *
     * @param filePath Path to result JSON file
     * @param storage Reference to storage to add phrases
     *
     * @note Silently skips invalid phrase entries
     * @note Handles both new cluster creation and existing cluster updates
     */
    void loadResultFile(const fs::path& filePath, PatternPhrasesStorage& storage);

    /**
     * @brief Create new cluster from single word complex.
     * @details Helper method to initialize WordComplexCluster from
     *          a single phrase when cluster doesn't exist yet.
     *
     * @param key Cluster key (normalized phrase)
     * @param wordComplex The phrase to initialize cluster with
     * @return New WordComplexCluster ready for storage
     */
    WordComplexCluster createClusterFromPhrase(const std::string& key,
                                              const WordComplexPtr& wordComplex) const;
};
