#include "ClusterMerger.h"
#include "Logger.h"
#include "Options.h"

#include <algorithm>

size_t ClusterMerger::mergeClusters(std::unordered_map<std::string, PhraseCluster>& clusters, size_t maxDiff,
                                    size_t endLength) {
    Logger::log("ClusterMerger", LogLevel::Info, "Starting cluster merging...");

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
    std::sort(sortedKeys.begin(), sortedKeys.end());

    // Iterate over sorted keys and merge similar clusters
    for (size_t i = 1; i < sortedKeys.size(); ++i) {
        const std::string& currentKey = sortedKeys[i];
        const std::string& previousKey = sortedKeys[i - 1];

        // Check if clusters still exist (may have been merged already)
        if (!clusters.contains(currentKey) || !clusters.contains(previousKey)) {
            continue;
        }

        if (areKeysSimilar(previousKey, currentKey) || areKeysSimilar(previousKey, currentKey, 2, 4, true)) {
            // Move all phrases from the current cluster to the previous cluster
            auto& previousCluster = clusters[previousKey];
            auto& currentCluster = clusters[currentKey];

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

bool ClusterMerger::areKeysSimilar(const std::string& key1, const std::string& key2, size_t maxDiff, size_t endLength,
                                   bool checkFirstOnly) {
    // Split keys into words
    std::istringstream stream1(key1);
    std::istringstream stream2(key2);

    std::vector<std::string> words1, words2;
    std::string word;

    while (stream1 >> word) {
        words1.push_back(word);
    }
    while (stream2 >> word) {
        words2.push_back(word);
    }

    // Must have same number of words
    if (words1.size() != words2.size()) {
        return false;
    }

    size_t diffCount = 0;
    bool firstWordChecked = false;

    // Compare the beginnings and endings of each word
    for (size_t i = 0; i < words1.size(); ++i) {
        const std::string& w1 = words1[i];
        const std::string& w2 = words2[i];

        // Short words must match exactly
        if ((w1.length() <= 8 || w2.length() <= 8) && w1 != w2) {
            return false;
        }

        // Extract stems (everything except last endLength chars)
        std::string stem1 = (w1.length() > endLength) ? w1.substr(0, w1.length() - endLength) : "";
        std::string stem2 = (w2.length() > endLength) ? w2.substr(0, w2.length() - endLength) : "";

        // Stems must be similar length
        if (std::abs(static_cast<int>(stem1.length()) - static_cast<int>(stem2.length())) >=
            static_cast<int>(maxDiff)) {
            return false;
        }

        // Equalize stem lengths for comparison
        if (stem1.length() != stem2.length()) {
            if (stem1.length() > stem2.length()) {
                stem1 = stem1.substr(0, stem2.length());
            } else {
                stem2 = stem2.substr(0, stem1.length());
            }
        }

        // Skip if either stem is empty
        if (stem1.empty() || stem2.empty()) {
            continue;
        }

        // Stems must match
        if (stem1 != stem2) {
            return false;
        }

        // Extract and compare endings
        std::string end1 = (w1.length() >= endLength) ? w1.substr(w1.length() - endLength) : w1;
        std::string end2 = (w2.length() >= endLength) ? w2.substr(w2.length() - endLength) : w2;

        // Count ending differences
        if (end1 != end2) {
            ++diffCount;

            if (checkFirstOnly) {
                firstWordChecked = true;
                if (diffCount > maxDiff) {
                    return false;
                }
            } else {
                if (diffCount > maxDiff) {
                    return false;
                }
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
    targetCluster.phrases.insert(targetCluster.phrases.end(), sourceCluster.phrases.begin(),
                                 sourceCluster.phrases.end());

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