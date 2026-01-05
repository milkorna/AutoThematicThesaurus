#pragma once

#include "PatternPhrasesStorage.h"

#include <string>

/**
 * @brief Merges similar phrase clusters based on morphological variations
 * @details Follows TextCorpusFilter pattern for PatternPhrasesStorage operations.
 * Combines morphological variants of the same phrase (e.g., "смежный вектора" and "смежный вектор")
 */
class ClusterMerger {
  public:
    /**
     * @brief Merges similar clusters in the storage
     * @details Combines clusters that have morphological variations based on key similarity.
     * Uses edit distance and suffix matching to identify candidates for merging.
     *
     * @param storage Reference to the phrase storage for processing
     * @param maxDiff Maximum number of differing characters allowed (default: 2)
     * @param endLength Length of suffix to compare (default: 4)
     */
    static void mergeClusters(PatternPhrasesStorage& storage, size_t maxDiff = 2, size_t endLength = 4);

  private:
    /**
     * @brief Checks if two cluster keys are similar
     * @details Compares keys using edit distance and suffix matching to identify morphological variants.
     *
     * @param key1 First cluster key
     * @param key2 Second cluster key
     * @param maxDiff Maximum number of differing characters allowed (default: 2)
     * @param endLength Length of suffix to compare (default: 4)
     * @param checkFirstOnly If true, compares only the first word in the keys (default: false)
     * @return true if keys are similar enough to merge, false otherwise
     */
    [[nodiscard]] static bool areKeysSimilar(const std::string& key1, const std::string& key2, size_t maxDiff = 2,
                                             size_t endLength = 4, bool checkFirstOnly = false);

    /**
     * @brief Logs merge operation statistics
     * @details Outputs information about cluster count before and after merging
     *
     * @param originalCount Number of clusters before merging
     * @param mergedCount Number of clusters after merging
     */
    static void logMergeStatistics(size_t originalCount, size_t mergedCount);
};