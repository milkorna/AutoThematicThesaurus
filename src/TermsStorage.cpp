#include <TermsStorage.h>
#include <mutex>
#include <thread>

std::mutex mtx;

// Checks if the key of phrase1 is a prefix of phrase2's key
bool IsPrefix(const std::string& phrase1Key, const std::string& phrase2Key)
{
    return phrase2Key.find(phrase1Key) == 0; // Checks if phrase1Key is at the beginning of phrase2Key
}

// Checks exclusion conditions based on TF-IDF and frequency
bool ShouldExcludeBasedOnTfidfAndFrequency(const WordComplexCluster& phrase1, const WordComplexCluster& phrase2,
                                           const WordComplexCluster& phrase3)
{
    // Check TF-IDF conditions for phrase1 and phrase2
    bool tfidfCondition =
        !phrase1.tfidf.empty() && !phrase3.tfidf.empty() && phrase1.tfidf[0] < 0.0005 && phrase3.tfidf.back() > 0.0005;

    // Check frequency conditions for phrase1 and phrase2 compared to phrase3
    bool frequencyCondition =
        phrase1.frequency < phrase3.frequency / 10.0 && phrase2.frequency < phrase3.frequency / 10.0;

    // Return true if both conditions are met
    return tfidfCondition && frequencyCondition;
}

void WriteClustersToFile(const std::unordered_set<std::string>& clustersToInclude, const std::string& filename)
{
    // Convert the unordered set to a vector for sorting
    std::vector<std::string> sortedClusters(clustersToInclude.begin(), clustersToInclude.end());

    // Sort the clusters alphabetically
    std::sort(sortedClusters.begin(), sortedClusters.end());

    // Open the file for writing
    std::ofstream outFile(filename);
    if (!outFile.is_open()) {
        std::cerr << "Error: Unable to open file " << filename << " for writing." << std::endl;
        return;
    }

    // Write each cluster to the file
    for (const auto& cluster : sortedClusters) {
        outFile << cluster << std::endl;
    }

    // Close the file
    outFile.close();
}

void TermsStorage::CollectTerms(PatternPhrasesStorage& phrases)
{
    std::set<std::string> sortedKeys;
    std::unordered_set<std::string> clustersToInclude;
    const auto& clusters = phrases.GetClusters();

    // Insert keys into sorted set
    for (const auto& pair : clusters) {
        sortedKeys.insert(pair.first);
        clustersToInclude.insert(pair.first);
    }

    auto it = sortedKeys.begin();
    while (it != sortedKeys.end()) {
        auto nextIt = std::next(it);

        // Check if next exists
        if (nextIt == sortedKeys.end()) {
            break;
        }

        const auto& key1 = *it;
        const auto& key2 = *nextIt;

        const auto& phrase1 = clusters.at(key1);
        const auto& phrase2 = clusters.at(key2);

        // Check models and prefix relationship
        if (((phrase1.modelName == "Прил + С" && phrase2.modelName == "(Прил + С) + Срд") ||
             (phrase1.modelName == "Прич + С" && phrase2.modelName == "(Прич + С) + Срд")) &&
            IsPrefix(key1, key2)) {
            // Find the part of key2 after the first space
            std::string trimmedKey = key2.substr(key2.find(' ') + 1); // Get everything after the first space

            // Check if the third key exists
            auto thirdIt = clusters.find(trimmedKey);
            if (thirdIt != clusters.end() && thirdIt->second.modelName == "С + Срд") {
                const auto& phrase3 = thirdIt->second;

                // Check exclusion conditions based on TF-IDF and frequency
                if (phrase1.centralityScore < 0.15 &&
                    ShouldExcludeBasedOnTfidfAndFrequency(phrase1, phrase2, phrase3)) {
                    // Log deletion with tf-idf and frequency
                    std::cout << "tf-idf: " << std::to_string(phrase1.tfidf[0]) << " " << key1 << " "
                              << ", freq: " << phrase1.frequency << " and " << key2 << ", freq: " << phrase2.frequency
                              << " because of: " << trimmedKey << ", tf-idf: " << std::to_string(phrase3.tfidf[0])
                              << " " << std::to_string(phrase3.tfidf[1]) << ", freq: " << phrase3.frequency
                              << std::endl;
                    clustersToInclude.erase(key1);
                    clustersToInclude.erase(key2);
                }
            }
        }

        // Move to the next element
        ++it;
    }

    // Write sorted clusters to a file
    WriteClustersToFile(clustersToInclude, "terms.txt");
}