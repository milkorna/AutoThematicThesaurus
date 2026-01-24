#include "ClusterAggregator.h"
#include "Logger.h"

#include <algorithm>
#include <fstream>
#include <ranges>

using json = nlohmann::ordered_json;
namespace fs = std::filesystem;

PhraseClusters ClusterAggregator::loadFromResultsDirectory(const fs::path& resultsDir) {
    Logger::log("ClusterAggregator", LogLevel::Info, "Starting cluster aggregation from: " + resultsDir.string());

    PhraseClusters clusters;

    if (!fs::exists(resultsDir) || !fs::is_directory(resultsDir)) {
        throw std::runtime_error("Results directory does not exist or is not a directory: " + resultsDir.string());
    }

    auto resultFiles = getResultFilesFromDirectory(resultsDir);
    Logger::log("ClusterAggregator", LogLevel::Info, "Found " + std::to_string(resultFiles.size()) + " result files");

    if (resultFiles.empty()) {
        Logger::log("ClusterAggregator", LogLevel::Warning, "No result files found in directory");
        return clusters;
    }

    size_t totalPhrases = 0;
    for (const auto& filePath : resultFiles) {
        try {
            auto phraseCountBefore = countPhrases(clusters);
            loadResultFile(filePath, clusters);
            auto phraseCountAfter = countPhrases(clusters);
            auto phrasesAdded = phraseCountAfter - phraseCountBefore;
            totalPhrases += phrasesAdded;

            Logger::log("ClusterAggregator", LogLevel::Debug,
                        "Loaded " + std::to_string(phrasesAdded) + " phrases from " + filePath.filename().string());
        } catch (const std::exception& e) {
            Logger::log("ClusterAggregator", LogLevel::Warning,
                        "Error loading file " + filePath.string() + ": " + std::string(e.what()));
        }
    }

    Logger::log("ClusterAggregator", LogLevel::Info,
                "Aggregation completed: " + std::to_string(clusters.size()) + " clusters with " +
                    std::to_string(totalPhrases) + " total phrases");

    return clusters;
}

PhraseClusters ClusterAggregator::loadFromJsonFile(const fs::path& filePath) {
    Logger::log("ClusterAggregator", LogLevel::Info, "Loading clusters from JSON file: " + filePath.string());

    PhraseClusters clusters;

    if (!fs::exists(filePath)) {
        throw std::runtime_error("Clusters file not found: " + filePath.string());
    }

    std::ifstream file(filePath);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open clusters file: " + filePath.string());
    }

    json j;
    try {
        file >> j;
    } catch (const json::exception& e) {
        throw std::runtime_error("Invalid JSON in clusters file " + filePath.string() + ": " + std::string(e.what()));
    }
    file.close();

    if (!j.is_object()) {
        throw std::runtime_error("Expected JSON object in clusters file, got " + std::string(j.type_name()));
    }

    Logger::log("ClusterAggregator", LogLevel::Debug, "Found " + std::to_string(j.size()) + " clusters in file");

    size_t successCount = 0;
    for (auto& [key, clusterJson] : j.items()) {
        try {
            clusters[key] = deserializeClusterFromJson(key, clusterJson);
            successCount++;

            Logger::log("ClusterAggregator", LogLevel::Debug, "Loaded cluster: " + key);
        } catch (const std::exception& e) {
            Logger::log("ClusterAggregator", LogLevel::Warning,
                        "Failed to load cluster '" + key + "': " + std::string(e.what()));
        }
    }

    Logger::log("ClusterAggregator", LogLevel::Info, "Loaded " + std::to_string(successCount) + " clusters from file");

    return clusters;
}

void ClusterAggregator::saveClusters(const PhraseClusters& clusters, const fs::path& outputPath) {
    Logger::log("ClusterAggregator", LogLevel::Info, "Saving clusters to: " + outputPath.string());

    json outputJson = json::object();

    // Sort clusters by key
    std::vector<std::string> sortedKeys;
    for (const auto& [key, _] : clusters) {
        sortedKeys.push_back(key);
    }
    std::ranges::sort(sortedKeys);

    // Process clusters in sorted order
    for (const auto& key : sortedKeys) {
        const auto& cluster = clusters.at(key);
        json clusterJson = json::object();

        // Basic cluster metadata (note: key is implicit in JSON object key)
        clusterJson["modelName"] = cluster.modelName;
        clusterJson["phraseSize"] = cluster.phraseSize;
        clusterJson["frequency"] = cluster.frequency;
        clusterJson["topicRelevance"] = cluster.topicRelevance;
        clusterJson["centralityScore"] = cluster.centralityScore;
        clusterJson["tagMatch"] = cluster.tagMatch;
        clusterJson["is_term"] = cluster.is_term;

        // Lemmas with their metrics
        json lemmasJson = json::array();
        for (const auto& lemmaMetrics : cluster.lemmas) {
            json lemmaJson = json::object();
            lemmaJson["text"] = lemmaMetrics.text;
            lemmaJson["tf"] = lemmaMetrics.tf;
            lemmaJson["idf"] = lemmaMetrics.idf;
            lemmaJson["tfidf"] = lemmaMetrics.tfidf;
            lemmasJson.push_back(lemmaJson);
        }
        clusterJson["lemmas"] = lemmasJson;

        // Phrases (array of phrase occurrences)
        json phrasesJson = json::array();
        for (const auto& phrase : cluster.phrases) {
            json phraseJson = json::object();
            phraseJson["textForm"] = phrase->textForm;
            phraseJson["docId"] = phrase->pos.docId;
            phraseJson["sentNum"] = phrase->pos.sentNum;
            phraseJson["start_token_ind"] = phrase->pos.start;
            phraseJson["end_token_ind"] = phrase->pos.end;
            phraseJson["charStart"] = phrase->pos.charStart;
            phraseJson["charEnd"] = phrase->pos.charEnd;
            phrasesJson.push_back(phraseJson);
        }
        clusterJson["phrases"] = phrasesJson;

        outputJson[key] = clusterJson;
    }

    // Write to file
    std::ofstream outFile(outputPath);
    if (!outFile.is_open()) {
        throw std::runtime_error("Cannot open output file: " + outputPath.string());
    }

    outFile << outputJson.dump(4) << "\n";
    outFile.close();

    Logger::log("ClusterAggregator", LogLevel::Info,
                "Saved " + std::to_string(clusters.size()) + " clusters to " + outputPath.filename().string());
}

std::vector<fs::path> ClusterAggregator::getResultFilesFromDirectory(const fs::path& resultsDir) {
    if (!fs::exists(resultsDir)) {
        throw std::runtime_error("Results directory does not exist: " + resultsDir.string());
    }

    if (!fs::is_directory(resultsDir)) {
        throw std::runtime_error("Path is not a directory: " + resultsDir.string());
    }

    std::vector<fs::path> resultFiles;

    try {
        for (const auto& entry : fs::directory_iterator(resultsDir)) {
            if (entry.is_regular_file()) {
                const std::string& filename = entry.path().filename().string();
                if (filename.ends_with("_res.json")) {
                    resultFiles.push_back(entry.path());
                }
            }
        }
    } catch (const fs::filesystem_error& e) {
        throw std::runtime_error("Failed to iterate results directory: " + std::string(e.what()));
    }

    std::ranges::sort(resultFiles);
    return resultFiles;
}

void ClusterAggregator::loadResultFile(const fs::path& filePath, PhraseClusters& clusters) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open file: " + filePath.string());
    }

    json j;
    try {
        file >> j;
    } catch (const json::exception& e) {
        throw std::runtime_error("Invalid JSON in file " + filePath.string() + ": " + std::string(e.what()));
    }

    if (!j.is_array()) {
        Logger::log("ClusterAggregator", LogLevel::Warning, "Expected JSON array in result file: " + filePath.string());
        return;
    }

    Logger::log("ClusterAggregator", LogLevel::Debug,
                "Processing " + std::to_string(j.size()) + " phrases from " + filePath.filename().string());

    for (const auto& obj : j) {
        try {
            if (!obj.contains("key") || !obj["key"].is_string()) {
                Logger::log("ClusterAggregator", LogLevel::Debug, "Skipped phrase: missing 'key' field");
                continue;
            }

            std::string key = obj["key"].get<std::string>();
            auto phrase = deserializePhraseFromJson(obj);
            if (!phrase) {
                continue;
            }

            // Extract lemmas
            std::vector<std::string> lemmas;
            if (obj.contains("lemmas") && obj["lemmas"].is_array()) {
                for (const auto& lemma : obj["lemmas"]) {
                    if (lemma.is_string()) {
                        lemmas.push_back(lemma.get<std::string>());
                    }
                }
            }

            if (lemmas.empty()) {
                Logger::log("ClusterAggregator", LogLevel::Debug, "Skipped phrase: empty lemmas for key " + key);
                continue;
            }

            // Add to cluster or create new one
            if (auto it = clusters.find(key); it != clusters.end()) {
                it->second.phrases.push_back(phrase);
            } else {
                clusters[key] = createClusterFromPhrase(key, phrase, lemmas);
            }

        } catch (const std::exception& e) {
            Logger::log("ClusterAggregator", LogLevel::Debug, "Skipped phrase due to error: " + std::string(e.what()));
        }
    }
}

PhrasePtr ClusterAggregator::deserializePhraseFromJson(const json& obj) {
    try {
        if (!obj.contains("textForm") || !obj["textForm"].is_string()) {
            return nullptr;
        }
        if (!obj.contains("docId") || !obj["docId"].is_string()) {
            return nullptr;
        }
        if (!obj.contains("modelName") || !obj["modelName"].is_string()) {
            return nullptr;
        }
        if (!obj.contains("lemmas") || !obj["lemmas"].is_array()) {
            return nullptr;
        }

        auto phrase = std::make_shared<Phrase>();

        phrase->pos.docId = obj["docId"].get<std::string>();
        phrase->pos.sentNum = obj.value("sentNum", 0);
        phrase->pos.start = obj.value("start_token_ind", 0);
        phrase->pos.end = obj.value("end_token_ind", 0);

        if (obj.contains("span") && obj["span"].is_array() && obj["span"].size() >= 2) {
            phrase->pos.charStart = obj["span"][0].get<size_t>();
            phrase->pos.charEnd = obj["span"][1].get<size_t>();
        } else {
            phrase->pos.charStart = 0;
            phrase->pos.charEnd = 0;
        }

        phrase->textForm = obj["textForm"].get<std::string>();
        phrase->modelName = obj["modelName"].get<std::string>();

        for (const auto& lemmaObj : obj["lemmas"]) {
            if (lemmaObj.is_string()) {
                phrase->lemmas.push_back(lemmaObj.get<std::string>());
            }
        }

        return phrase;

    } catch (const std::exception& e) {
        Logger::log("ClusterAggregator", LogLevel::Debug, "Error deserializing phrase: " + std::string(e.what()));
        return nullptr;
    }
}

PhraseCluster ClusterAggregator::deserializeClusterFromJson(const std::string& key, const json& clusterJson) {
    PhraseCluster cluster;
    cluster.key = key;

    if (!clusterJson.contains("modelName") || !clusterJson["modelName"].is_string()) {
        throw std::runtime_error("Missing or invalid 'modelName' field");
    }
    if (!clusterJson.contains("lemmas") || !clusterJson["lemmas"].is_array()) {
        throw std::runtime_error("Missing or invalid 'lemmas' array");
    }
    if (!clusterJson.contains("phrases") || !clusterJson["phrases"].is_array()) {
        throw std::runtime_error("Missing or invalid 'phrases' array");
    }

    cluster.modelName = clusterJson["modelName"].get<std::string>();
    cluster.phraseSize = clusterJson.value("phraseSize", 0);
    cluster.frequency = clusterJson.value("frequency", 0.0);
    cluster.topicRelevance = clusterJson.value("topicRelevance", 0.0);
    cluster.centralityScore = clusterJson.value("centralityScore", 0.0);
    cluster.tagMatch = clusterJson.value("tagMatch", false);
    cluster.is_term = clusterJson.value("is_term", false);

    cluster.lemmas.reserve(clusterJson["lemmas"].size());
    for (const auto& lemmaJson : clusterJson["lemmas"]) {
        try {
            cluster.lemmas.push_back(deserializeLemmaFromJson(lemmaJson));
        } catch (const std::exception& e) {
            Logger::log("ClusterAggregator", LogLevel::Debug,
                        "Skipped invalid lemma in cluster '" + key + "': " + std::string(e.what()));
        }
    }

    if (cluster.lemmas.empty()) {
        throw std::runtime_error("No valid lemmas found");
    }

    cluster.phrases.reserve(clusterJson["phrases"].size());
    for (const auto& phraseJson : clusterJson["phrases"]) {
        try {
            auto phrase = deserializePhraseFromJson(phraseJson);
            if (phrase) {
                cluster.phrases.push_back(phrase);
            }
        } catch (const std::exception& e) {
            Logger::log("ClusterAggregator", LogLevel::Debug,
                        "Skipped invalid phrase in cluster '" + key + "': " + std::string(e.what()));
        }
    }

    if (cluster.phrases.empty()) {
        throw std::runtime_error("No valid phrases found");
    }

    return cluster;
}

LemmaMetrics ClusterAggregator::deserializeLemmaFromJson(const json& lemmaJson) {
    if (!lemmaJson.contains("text") || !lemmaJson["text"].is_string()) {
        throw std::runtime_error("Missing or invalid 'text' field in lemma");
    }

    return LemmaMetrics{.text = lemmaJson["text"].get<std::string>(),
                        .tf = lemmaJson.value("tf", 0.0),
                        .idf = lemmaJson.value("idf", 0.0),
                        .tfidf = lemmaJson.value("tfidf", 0.0)};
}

PhraseCluster ClusterAggregator::createClusterFromPhrase(const std::string& key, const PhrasePtr& phrase,
                                                         const std::vector<std::string>& lemmas) {
    PhraseCluster cluster;

    // Basic properties
    cluster.key = key;
    cluster.modelName = phrase->modelName;
    cluster.phraseSize = lemmas.size();

    // Initialize lemmas with metrics
    for (const auto& lemma : lemmas) {
        cluster.lemmas.push_back(LemmaMetrics{.text = lemma, .tf = 0.0, .idf = 0.0, .tfidf = 0.0});
    }

    // Default metrics (will be computed by separate tools)
    cluster.frequency = 0.0;
    cluster.topicRelevance = 0.0;
    cluster.centralityScore = 0.0;
    cluster.tagMatch = false;
    cluster.is_term = false;

    // Initialize semantic relation maps
    for (const auto& lemma : lemmas) {
        cluster.hypernyms[lemma] = {};
        cluster.hyponyms[lemma] = {};
    }

    // Add the phrase
    cluster.phrases.push_back(phrase);

    return cluster;
}

inline size_t ClusterAggregator::countPhrases(const PhraseClusters& clusters) {
    size_t total = 0;
    for (const auto& [_, cluster] : clusters) {
        total += cluster.phrases.size();
    }
    return total;
}