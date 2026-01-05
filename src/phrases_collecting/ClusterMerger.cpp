#include "ClusterMerger.h"

#include <algorithm>

void ClusterMerger::mergeClusters(PatternPhrasesStorage& storage, size_t maxDiff, size_t endLength) {
    Logger::log("ClusterMerger", LogLevel::Info, "Starting cluster merging...");

    auto& clusters = storage.clusters;

    size_t originalCount = clusters.size();

    if (originalCount == 0) {
        Logger::log("ClusterMerger", LogLevel::Warning, "No clusters to merge");
        return;
    }

    // Get all keys from the map and sort them
    std::vector<std::string> sortedKeys;
    sortedKeys.reserve(clusters.size());

    for (const auto& pair : clusters) {
        sortedKeys.push_back(pair.first);
    }

    std::sort(sortedKeys.begin(), sortedKeys.end());

    // Iterate over sorted keys and merge similar clusters
    for (size_t i = 1; i < sortedKeys.size(); ++i) {
        std::string& currentKey = sortedKeys[i];
        std::string& previousKey = sortedKeys[i - 1];

        if (AreKeysSimilar(previousKey, currentKey) || AreKeysSimilar(previousKey, currentKey, 2, 4, true)) {
            // Move all wordComplexes from the current cluster to the previous cluster
            auto& previousCluster = clusters[previousKey];
            auto& currentCluster = clusters[currentKey];

            for (int j = 0; j < previousCluster.tfidf.size(); j++) {
                if (currentCluster.tf[j] > previousCluster.tf[j])
                    previousCluster.tf[j] = currentCluster.tf[j];
                if (currentCluster.idf[j] > previousCluster.idf[j])
                    previousCluster.idf[j] = currentCluster.idf[j];
                if (currentCluster.tfidf[j] > previousCluster.tfidf[j])
                    previousCluster.tfidf[j] = currentCluster.tfidf[j];
            }

            previousCluster.tagMatch = currentCluster.tagMatch || previousCluster.tagMatch;

            previousCluster.wordComplexes.insert(previousCluster.wordComplexes.end(),
                                                 currentCluster.wordComplexes.begin(),
                                                 currentCluster.wordComplexes.end());

            // Remove the current cluster after the move
            clusters.erase(currentKey);
        }
    }

    size_t mergedCount = clusters.size();

    logMergeStatistics(originalCount, mergedCount);

    Logger::log("ClusterMerger", LogLevel::Info, "Cluster merging completed successfully.");
}

bool ClusterMerger::AreKeysSimilar(const std::string& key1, const std::string& key2, size_t maxDiff, size_t endLength,
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

    // If the number of words is different, the keys are not considered similar
    if (words1.size() != words2.size()) {
        return false;
    }

    size_t diffCount = 0;
    bool firstWordChecked = false;

    // Compare the beginnings and endings of each word
    for (size_t i = 0; i < words1.size(); ++i) {
        if ((words1[i].length() <= 8 || words2[i].length() <= 8) && words1[i] != words2[i]) {
            return false;
        }

        // Extract the initial parts of the words (everything except the last few characters)
        std::string start1 =
            (words1[i].length() > endLength) ? words1[i].substr(0, words1[i].length() - endLength) : "";
        std::string start2 =
            (words2[i].length() > endLength) ? words2[i].substr(0, words2[i].length() - endLength) : "";

        // Check if the difference in length between the initial parts exceeds maxDiff
        if (std::abs(static_cast<long>(start1.length()) - static_cast<long>(start2.length())) >= maxDiff) {
            return false;
        }

        // Adjust the initial parts to be the same length
        if (start1.length() != start2.length()) {
            if (start1.length() > start2.length()) {
                start1 = start1.substr(0, start2.length());
            } else {
                start2 = start2.substr(0, start1.length());
            }
        }

        // Skip comparison if either start is empty
        if (start1.empty() || start2.empty()) {
            continue;
        }

        // Compare the initial parts of the words
        if (start1 != start2) {
            // If initial parts do not match, keys are not similar
            return false;
        }

        // Extract the endings of the words
        std::string end1 =
            (words1[i].length() >= endLength) ? words1[i].substr(words1[i].length() - endLength) : words1[i];
        std::string end2 =
            (words2[i].length() >= endLength) ? words2[i].substr(words2[i].length() - endLength) : words2[i];

        // Compare the endings of the words
        if (end1 != end2) {
            ++diffCount;

            // If CheckFirstOnly is true, only check the first word for differences
            if (checkFirstOnly) {
                // If the first word has been checked and the difference is not zero, subsequent words must match
                // exactly
                firstWordChecked = true;
                if (diffCount > maxDiff) {
                    return false;
                }
            } else {
                // If not checking the first word only, ensure differences don't exceed maxDiff
                if (diffCount > maxDiff) {
                    return false;
                }
            }
        }

        // If checkFirstOnly is true and the first word has been checked with differences, ensure remaining words match
        if (checkFirstOnly && firstWordChecked && i > 0) {
            if (words1[i] != words2[i]) {
                return false;
            }
        }
    }

    // If the number of differences in the endings does not exceed maxDiff, the keys are considered similar
    return diffCount <= maxDiff;
}

void ClusterMerger::logMergeStatistics(size_t originalCount, size_t mergedCount) {
    size_t mergedClusters = originalCount - mergedCount;
    double reductionPercent = (mergedClusters / static_cast<double>(originalCount)) * 100.0;

    Logger::log("ClusterMerger", LogLevel::Info,
                "Clusters merged: " + std::to_string(mergedClusters) + " (" +
                    std::to_string(static_cast<int>(reductionPercent)) + "%)");
    Logger::log("ClusterMerger", LogLevel::Info,
                "Result: " + std::to_string(originalCount) + " → " + std::to_string(mergedCount));
}
