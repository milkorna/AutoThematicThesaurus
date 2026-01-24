#pragma once

#include "PatternPhrasesStorage.h"

#include <string>

/**
 * @brief Merges similar phrase clusters based on morphological variations
 * @details Clean responsibility: identify clusters with similar keys (morphological variants)
 * and merge them. Does NOT touch any metrics - all TF, IDF, LSA scores remain untouched.
 *
 * Example: "смежный вектора" + "смежный вектор" → merged into single cluster
 */
class ClusterMerger {
  public:
    /**
     * @brief Merges similar clusters in the storage
     * @details Identifies clusters with morphological variations and combines them.
     * Uses suffix/prefix matching to detect variants (e.g., different case endings).
     * All metrics (TF, IDF, centrality, etc.) are preserved from source clusters.
     *
     * @param clusters Reference to cluster map (modified in-place)
     * @param maxDiff Maximum number of differing characters allowed (default: 2)
     * @param endLength Length of suffix to compare (default: 4)
     *
     * @return Number of clusters merged
     */
    static size_t mergeClusters(std::unordered_map<std::string, PhraseCluster>& clusters, size_t maxDiff = 2,
                                size_t endLength = 4);

  private:
    /**
     * @brief Checks if two cluster keys are morphologically similar
     * @details Compares keys using:
     * 1. Same word count
     * 2. Prefix matching (stems match)
     * 3. Suffix matching (endings differ by <= maxDiff chars)
     *
     * @param key1 First cluster key
     * @param key2 Second cluster key
     * @param maxDiff Maximum differing character count
     * @param endLength Suffix length to compare
     * @param checkFirstOnly If true, only first word checked for differences
     * @return true if keys are morphologically similar
     */
    [[nodiscard]] static bool areKeysSimilar(const std::string& key1, const std::string& key2, size_t maxDiff = 2,
                                             size_t endLength = 4, bool checkFirstOnly = false);

    /**
     * @brief Merges source cluster into target cluster
     * @details Combines all phrases from source into target. Keeps metrics from target.
     * Source cluster should be deleted after merge.
     *
     * @param targetCluster Target cluster to merge into (modified)
     * @param sourceCluster Source cluster to merge from
     */
    static void mergeClusterData(PhraseCluster& targetCluster, const PhraseCluster& sourceCluster);

    /**
     * @brief Logs merge statistics
     * @param originalCount Clusters before merge
     * @param mergedCount Clusters after merge
     * @param totalMerged Total clusters that were merged away
     */
    static void logMergeStatistics(size_t originalCount, size_t mergedCount, size_t totalMerged);
};