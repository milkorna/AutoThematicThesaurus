#include "PhraseAggregator.h"

#include "Logger.h"

#include <algorithm>
#include <fstream>
#include <sstream>

std::unordered_map<std::string, PhraseCluster> PhraseAggregator::aggregatePhrases(const fs::path& resultsDir) {
    Logger::log("PhraseAggregator", LogLevel::Info, "Starting phrase aggregation from: " + resultsDir.string());

    std::unordered_map<std::string, PhraseCluster> clusters;

    if (!fs::exists(resultsDir) || !fs::is_directory(resultsDir)) {
        throw std::runtime_error("Results directory does not exist or is not a directory: " + resultsDir.string());
    }

    // Get all result files
    auto resultFiles = getResultFilesFromDirectory(resultsDir);
    Logger::log("PhraseAggregator", LogLevel::Info, "Found " + std::to_string(resultFiles.size()) + " result files");

    if (resultFiles.empty()) {
        Logger::log("PhraseAggregator", LogLevel::Warning, "No result files found in directory");
        return clusters;
    }

    // Load each result file
    size_t totalPhrases = 0;
    for (const auto& filePath : resultFiles) {
        try {
            size_t phrasesBefore = 0;
            for (const auto& cluster : clusters) {
                phrasesBefore += cluster.second.phrases.size();
            }

            loadResultFile(filePath, clusters);

            size_t phrasesAfter = 0;
            for (const auto& cluster : clusters) {
                phrasesAfter += cluster.second.phrases.size();
            }

            size_t phrasesAdded = phrasesAfter - phrasesBefore;
            totalPhrases += phrasesAdded;

            Logger::log("PhraseAggregator", LogLevel::Debug,
                        "Loaded " + std::to_string(phrasesAdded) + " phrases from " + filePath.filename().string());
        } catch (const std::exception& e) {
            Logger::log("PhraseAggregator", LogLevel::Warning,
                        "Error loading file " + filePath.string() + ": " + std::string(e.what()));
            // Continue loading other files
        }
    }

    Logger::log("PhraseAggregator", LogLevel::Info,
                "Aggregation completed: " + std::to_string(clusters.size()) + " clusters with " +
                    std::to_string(totalPhrases) + " total phrases");

    return clusters;
}

void PhraseAggregator::saveClusters(const std::unordered_map<std::string, PhraseCluster>& clusters,
                                    const fs::path& outputPath) {
    Logger::log("PhraseAggregator", LogLevel::Info, "Saving clusters to: " + outputPath.string());

    json outputJson = json::object();

    // Sort clusters by key for consistent output
    std::vector<std::string> sortedKeys;
    sortedKeys.reserve(clusters.size());
    for (const auto& [key, _] : clusters) {
        sortedKeys.push_back(key);
    }
    std::sort(sortedKeys.begin(), sortedKeys.end());

    // Process clusters in sorted order
    for (const auto& key : sortedKeys) {
        const auto& cluster = clusters.at(key);
        json clusterJson = json::object();

        // Basic cluster metadata
        clusterJson["key"] = key;
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

    Logger::log("PhraseAggregator", LogLevel::Info,
                "Saved " + std::to_string(clusters.size()) + " clusters to " + outputPath.filename().string());
}

std::vector<fs::path> PhraseAggregator::getResultFilesFromDirectory(const fs::path& resultsDir) {
    std::vector<fs::path> resultFiles;

    if (!fs::exists(resultsDir)) {
        throw std::runtime_error("Results directory does not exist: " + resultsDir.string());
    }

    if (!fs::is_directory(resultsDir)) {
        throw std::runtime_error("Path is not a directory: " + resultsDir.string());
    }

    try {
        for (const auto& entry : fs::directory_iterator(resultsDir)) {
            if (entry.is_regular_file()) {
                const std::string& filename = entry.path().filename().string();
                // Look for files matching pattern: *_res.json
                if (filename.ends_with("_res.json")) {
                    resultFiles.push_back(entry.path());
                }
            }
        }
    } catch (const fs::filesystem_error& e) {
        throw std::runtime_error("Failed to iterate results directory: " + std::string(e.what()));
    }

    // Sort for deterministic order
    std::sort(resultFiles.begin(), resultFiles.end());

    return resultFiles;
}

void PhraseAggregator::loadResultFile(const fs::path& filePath,
                                      std::unordered_map<std::string, PhraseCluster>& clusters) {
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
    file.close();

    // Expect array of phrase objects
    if (!j.is_array()) {
        Logger::log("PhraseAggregator", LogLevel::Warning, "Expected JSON array in result file: " + filePath.string());
        return;
    }

    Logger::log("PhraseAggregator", LogLevel::Debug,
                "Processing " + std::to_string(j.size()) + " phrases from " + filePath.filename().string());

    // Process each phrase result
    for (const auto& obj : j) {
        try {
            // Get key directly from JSON (no need to reconstruct from lemmas)
            if (!obj.contains("key") || !obj["key"].is_string()) {
                Logger::log("PhraseAggregator", LogLevel::Debug, "Skipped phrase: missing 'key' field");
                continue;
            }

            std::string key = obj["key"].get<std::string>();

            // Deserialize phrase
            PhrasePtr phrase = deserializePhraseFromJson(obj);
            if (!phrase) {
                continue; // Skip invalid phrase
            }

            // Extract lemmas for cluster structure
            std::vector<std::string> lemmas;
            if (obj.contains("lemmas") && obj["lemmas"].is_array()) {
                for (const auto& lemma : obj["lemmas"]) {
                    if (lemma.is_string()) {
                        lemmas.push_back(lemma.get<std::string>());
                    }
                }
            }

            if (lemmas.empty()) {
                Logger::log("PhraseAggregator", LogLevel::Debug, "Skipped phrase: empty lemmas for key " + key);
                continue;
            }

            // Add to cluster or create new one
            auto it = clusters.find(key);
            if (it != clusters.end()) {
                // Add to existing cluster
                it->second.phrases.push_back(phrase);
            } else {
                // Create new cluster
                PhraseCluster newCluster = createClusterFromPhrase(key, phrase, lemmas);
                clusters[key] = newCluster;
            }

        } catch (const std::exception& e) {
            Logger::log("PhraseAggregator", LogLevel::Debug, "Skipped phrase due to error: " + std::string(e.what()));
            // Continue processing other phrases
        }
    }
}

PhrasePtr PhraseAggregator::deserializePhraseFromJson(const json& obj) {
    try {
        // Required fields
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

        // Position data
        phrase->pos.docId = obj["docId"].get<std::string>();
        phrase->pos.sentNum = obj.value("sentNum", 0);
        phrase->pos.start = obj.value("start_token_ind", 0);
        phrase->pos.end = obj.value("end_token_ind", 0);
        phrase->pos.charStart =
            obj.contains("span") && obj["span"].is_array() && obj["span"].size() > 0 ? obj["span"][0].get<size_t>() : 0;
        phrase->pos.charEnd =
            obj.contains("span") && obj["span"].is_array() && obj["span"].size() > 1 ? obj["span"][1].get<size_t>() : 0;

        // Text and model
        phrase->textForm = obj["textForm"].get<std::string>();
        phrase->modelName = obj["modelName"].get<std::string>();

        // Lemmas
        for (const auto& lemmaObj : obj["lemmas"]) {
            if (lemmaObj.is_string()) {
                phrase->lemmas.push_back(lemmaObj.get<std::string>());
            }
        }

        return phrase;

    } catch (const std::exception& e) {
        Logger::log("PhraseAggregator", LogLevel::Debug, "Error deserializing phrase: " + std::string(e.what()));
        return nullptr;
    }
}

PhraseCluster PhraseAggregator::createClusterFromPhrase(const std::string& key, const PhrasePtr& phrase,
                                                        const std::vector<std::string>& lemmas) {
    PhraseCluster cluster;

    // Basic properties
    cluster.key = key;
    cluster.modelName = phrase->modelName;
    cluster.phraseSize = lemmas.size();

    // Initialize lemmas with metrics
    for (const auto& lemma : lemmas) {
        LemmaMetrics lm;
        lm.text = lemma;
        lm.tf = 0.0;
        lm.idf = 0.0;
        lm.tfidf = 0.0;
        cluster.lemmas.push_back(lm);
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
