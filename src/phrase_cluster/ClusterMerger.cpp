#include "ClusterMerger.h"
#include "Logger.h"
#include "Options.h"

#include <algorithm>
#include <ranges>
#include <sstream>

size_t ClusterMerger::mergeClusters(PhraseClusters& clusters, size_t maxDiff, size_t endLength) {
    Logger::log("ClusterMerger", LogLevel::Info, "Starting cluster merging...");

    if (clusters.empty()) {
        Logger::log("ClusterMerger", LogLevel::Warning, "No clusters to merge");
        return 0;
    }

    size_t originalCount = clusters.size();
    size_t totalMerged = 0;

    // Get all keys and sort them
    std::vector<std::string> sortedKeys;
    sortedKeys.reserve(clusters.size());
    for (const auto& [key, _] : clusters) {
        sortedKeys.push_back(key);
    }
    std::ranges::sort(sortedKeys);

    // Iterate over sorted keys and merge similar clusters
    for (size_t i = 1; i < sortedKeys.size(); ++i) {
        const std::string& currentKey = sortedKeys[i];
        const std::string& previousKey = sortedKeys[i - 1];

        // Check if clusters still exist (may have been merged already)
        if (!clusters.contains(currentKey) || !clusters.contains(previousKey)) {
            continue;
        }

        // Check similarity - delegate both comparisons to areKeysSimilar with different params
        bool isSimilar = areKeysSimilar(previousKey, currentKey, maxDiff, endLength, false) ||
                         areKeysSimilar(previousKey, currentKey, maxDiff, endLength, true);

        if (isSimilar) {
            // Merge current into previous
            mergeClusterData(clusters.at(previousKey), clusters.at(currentKey));

            // Remove current cluster
            clusters.erase(currentKey);
            totalMerged++;
        }
    }

    size_t finalCount = clusters.size();
    logMergeStatistics(originalCount, finalCount, totalMerged);

    Logger::log("ClusterMerger", LogLevel::Info, "Cluster merging completed successfully.");

    return totalMerged;
}

/**
 * @brief Helper to split string by whitespace
 * @param str Input string
 * @return Vector of words
 */
static std::vector<std::string> splitWords(const std::string& str) {
    std::istringstream stream(str);
    std::vector<std::string> words;
    std::string word;

    while (stream >> word) {
        words.push_back(word);
    }

    return words;
}

bool ClusterMerger::areKeysSimilar(const std::string& key1, const std::string& key2, size_t maxDiff, size_t endLength,
                                   bool checkFirstOnly) {
    // Split keys into words
    auto words1 = splitWords(key1);
    auto words2 = splitWords(key2);

    // Must have same number of words
    if (words1.size() != words2.size()) {
        return false;
    }

    size_t diffCount = 0;
    bool firstWordChecked = false;

    // Compare the beginnings and endings of each word
    for (size_t i = 0; i < words1.size(); ++i) {
        const auto& w1 = words1[i];
        const auto& w2 = words2[i];

        // Short words must match exactly
        if ((w1.length() <= 8 || w2.length() <= 8) && w1 != w2) {
            return false;
        }

        // Extract stems (everything except last endLength chars)
        auto getStem = [endLength](const std::string& word) -> std::string {
            return (word.length() > endLength) ? word.substr(0, word.length() - endLength) : "";
        };

        std::string stem1 = getStem(w1);
        std::string stem2 = getStem(w2);

        // Stems must be similar length
        if (std::abs(static_cast<int>(stem1.length()) - static_cast<int>(stem2.length())) >=
            static_cast<int>(maxDiff)) {
            return false;
        }

        // Equalize stem lengths for comparison
        size_t minStemLength = std::min(stem1.length(), stem2.length());
        if (minStemLength > 0) {
            // Compare only the common prefix
            if (stem1.substr(0, minStemLength) != stem2.substr(0, minStemLength)) {
                return false;
            }
        }

        // Extract and compare endings
        auto getEnd = [endLength](const std::string& word) -> std::string {
            return (word.length() >= endLength) ? word.substr(word.length() - endLength) : word;
        };

        std::string end1 = getEnd(w1);
        std::string end2 = getEnd(w2);

        // Count ending differences
        if (end1 != end2) {
            ++diffCount;

            if (diffCount > maxDiff) {
                return false;
            }

            if (checkFirstOnly) {
                firstWordChecked = true;
            }
        }

        // If checkFirstOnly and first word checked, remaining words must match exactly
        if (checkFirstOnly && firstWordChecked && i > 0) {
            if (w1 != w2) {
                return false;
            }
        }
    }

    return diffCount <= maxDiff;
}

void ClusterMerger::mergeClusterData(PhraseCluster& targetCluster, const PhraseCluster& sourceCluster) {
    // Simply add all phrases from source to target
    targetCluster.phrases.insert(targetCluster.phrases.end(), std::make_move_iterator(sourceCluster.phrases.begin()),
                                 std::make_move_iterator(sourceCluster.phrases.end()));

    // Note: All metrics (TF, IDF, TF-IDF, topicRelevance, centrality) are LEFT UNCHANGED
    // They will be recalculated by MetricsCalculator if needed
}

void ClusterMerger::logMergeStatistics(size_t originalCount, size_t mergedCount, size_t totalMerged) {
    double reductionPercent = originalCount > 0 ? (totalMerged / static_cast<double>(originalCount)) * 100.0 : 0.0;

    Logger::log("ClusterMerger", LogLevel::Info,
                "Clusters merged: " + std::to_string(totalMerged) + " (" +
                    std::to_string(static_cast<int>(reductionPercent)) + "%)");

    Logger::log("ClusterMerger", LogLevel::Info,
                "Result: " + std::to_string(originalCount) + " → " + std::to_string(mergedCount));
}