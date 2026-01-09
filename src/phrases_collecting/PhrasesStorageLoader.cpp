#include "PhrasesStorageLoader.h"
#include "CorpusVocabulary.h"
#include "Logger.h"
#include "Options.h"

#include <algorithm>
#include <fstream>

using json = nlohmann::json;

// Define static member
ClusterDeserializer PhrasesStorageLoader::deserializer;

void PhrasesStorageLoader::loadStorageFromFile(PatternPhrasesStorage& storage, const std::string& filename) {
    Logger::log("PhrasesStorageLoader", LogLevel::Info, "Loading storage from file: " + filename);

    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open file: " + filename);
    }

    try {
        json j;
        file >> j;
        file.close();

        if (!j.is_object()) {
            throw std::runtime_error("Expected JSON object at root level");
        }

        // Deserialize each cluster
        for (auto it = j.begin(); it != j.end(); ++it) {
            const std::string& key = it.key();
            const json& obj = it.value();

            WordComplexCluster cluster = deserializer.deserializeCluster(obj, key);
            storage.addCluster(key, cluster);
        }

        Logger::log("PhrasesStorageLoader", LogLevel::Info,
                    "Successfully loaded " + std::to_string(j.size()) + " clusters");

    } catch (const json::exception& e) {
        throw std::runtime_error("JSON parsing error: " + std::string(e.what()));
    }
}

void PhrasesStorageLoader::loadPhraseStorageFromResultsDir(PatternPhrasesStorage& storage) {
    Logger::log("PhrasesStorageLoader", LogLevel::Info, "Loading phrase storage from results directory...");

    auto& options = Options::getOptions();
    fs::path resultsDir = options.resDir;

    // Create directory if it doesn't exist
    try {
        fs::create_directories(resultsDir);
    } catch (const fs::filesystem_error& e) {
        throw std::runtime_error("Failed to create results directory: " + std::string(e.what()));
    }

    // Reserve space for clusters
    auto& corpus = CorpusVocabulary::GetCorpus();
    //  storage.reserveClusters(corpus.getTextCount());

    // Load all result files
    auto resultFiles = getResultFilesFromDirectory(resultsDir);

    Logger::log("PhrasesStorageLoader", LogLevel::Info,
                "Found " + std::to_string(resultFiles.size()) + " result files");

    for (const auto& filePath : resultFiles) {
        try {
            loadResultFile(filePath, storage);
        } catch (const std::exception& e) {
            Logger::log("PhrasesStorageLoader", LogLevel::Warning,
                        "Error loading file " + filePath.string() + ": " + std::string(e.what()));
            // Continue loading other files instead of failing completely
        }
    }

    Logger::log("PhrasesStorageLoader", LogLevel::Info, "Phrase storage loading completed");
}

std::vector<fs::path> PhrasesStorageLoader::getResultFilesFromDirectory(const fs::path& resultsDir) {

    std::vector<fs::path> resultFiles;

    if (!fs::exists(resultsDir) || !fs::is_directory(resultsDir)) {
        throw std::runtime_error("Results directory does not exist or is not a directory: " + resultsDir.string());
    }

    try {
        for (const auto& entry : fs::directory_iterator(resultsDir)) {
            if (entry.is_regular_file()) {
                const std::string& filename = entry.path().filename().string();

                // Match pattern: res_*_text.json
                if (filename.find("res_") == 0 && filename.find("_text.json") != std::string::npos) {
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

void PhrasesStorageLoader::loadResultFile(const fs::path& filePath, PatternPhrasesStorage& storage) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        Logger::log("PhrasesStorageLoader", LogLevel::Warning, "Cannot open result file: " + filePath.string());
        return;
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
        Logger::log("PhrasesStorageLoader", LogLevel::Warning,
                    "Expected JSON array in result file: " + filePath.string());
        return;
    }

    Logger::log("PhrasesStorageLoader", LogLevel::Debug,
                "Processing " + std::to_string(j.size()) + " phrases from " + filePath.filename().string());

    // Process each phrase result
    for (const auto& obj : j) {
        try {
            // Deserialize phrase result
            WordComplexPtr wordComplex = deserializer.deserializePhraseResult(obj);

            if (!wordComplex) {
                // Skip invalid phrase (deserializePhraseResult already logged)
                continue;
            }

            // Get or create cluster
            std::string key;
            for (const auto& lemma : wordComplex->lemmas) {
                if (!key.empty())
                    key += " ";
                key += lemma;
            }

            auto existingCluster = storage.findCluster(key);

            if (existingCluster != nullptr) {
                // Add to existing cluster if not duplicate
                auto found = std::find(existingCluster->wordComplexes.begin(), existingCluster->wordComplexes.end(),
                                       wordComplex);
                if (found == existingCluster->wordComplexes.end()) {
                    existingCluster->wordComplexes.push_back(wordComplex);
                }
            } else {
                // Create new cluster
                WordComplexCluster newCluster = createClusterFromPhrase(key, wordComplex);
                storage.addCluster(key, newCluster);
            }

        } catch (const std::exception& e) {
            Logger::log("PhrasesStorageLoader", LogLevel::Debug,
                        "Skipped phrase result due to error: " + std::string(e.what()));
            // Continue processing other phrases
        }
    }
}

WordComplexCluster PhrasesStorageLoader::createClusterFromPhrase(const std::string& key,
                                                                 const WordComplexPtr& wordComplex) {

    WordComplexCluster newCluster;

    // Basic properties
    newCluster.key = key;
    newCluster.modelName = wordComplex->modelName;
    newCluster.phraseSize = wordComplex->lemmas.size();
    newCluster.frequency = 1.0;
    newCluster.topicRelevance = 0.0;
    newCluster.centralityScore = 0.0;
    newCluster.tagMatch = false;

    // Initialize lemma-related structures
    for (const auto& lemma : wordComplex->lemmas) {
        newCluster.lemmas.push_back(lemma);
        newCluster.tf.push_back(0.0);
        newCluster.idf.push_back(0.0);
        newCluster.tfidf.push_back(0.0);
        newCluster.wordVectors.push_back(std::make_shared<WordEmbedding>(lemma));
        newCluster.hypernyms[lemma] = {};
        newCluster.hyponyms[lemma] = {};
    }

    // Add the phrase
    newCluster.wordComplexes.push_back(wordComplex);

    return newCluster;
}
