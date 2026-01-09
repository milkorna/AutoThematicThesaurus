#include "CorpusVocabulary.h"
#include "Logger.h"
#include "StringUtils.h"

using json = nlohmann::ordered_json;

void CorpusVocabulary::save(const std::string& filename) {
    Logger::log("TextCorpusLoader", LogLevel::Info, "Serializing corpus to file: " + filename);

    try {
        std::ofstream file(filename);
        if (!file.is_open()) {
            throw std::runtime_error("Failed to open file for writing: " + filename);
        }

        json j = serialize();
        file << j.dump(4);
        file.close();

        Logger::log("TextCorpusLoader", LogLevel::Info, "Corpus saved successfully");
    } catch (const std::exception& e) {
        Logger::log("TextCorpusLoader", LogLevel::Error, "Save failed: " + std::string(e.what()));
        throw;
    }
}

void CorpusVocabulary::load(const std::string& filename) {
    Logger::log("TextCorpusLoader", LogLevel::Info, "Loading corpus global statistics from: " + filename);

    try {
        std::ifstream file(filename);
        if (!file.is_open()) {
            throw std::runtime_error("Failed to open file: " + filename);
        }

        json j;
        file >> j;
        file.close();

        deserialize(j);
        Logger::log("TextCorpusLoader", LogLevel::Info,
                    "Corpus loaded successfully: " + std::to_string(wordCount) + " words, " +
                        std::to_string(documentCount) + " documents");
    } catch (const std::exception& e) {
        Logger::log("TextCorpusLoader", LogLevel::Error, "Load failed: " + std::string(e.what()));
        throw;
    }
}

const std::unordered_map<std::string, size_t>& CorpusVocabulary::getWordFrequencies() const {
    return wordFrequency;
}

const std::unordered_map<std::string, size_t>& CorpusVocabulary::getDocumentFrequencies() const {
    return documentFrequency;
}

size_t CorpusVocabulary::getWordFrequency(const std::string& lemma) const {
    if (wordFrequency.contains(lemma)) {
        return wordFrequency.at(lemma);
    }
    return 0;
}

size_t CorpusVocabulary::getDocumentFrequency(const std::string& lemma) const {
    if (documentFrequency.contains(lemma)) {
        return documentFrequency.at(lemma);
    }
    return 0;
}

size_t CorpusVocabulary::getDocumentCount() const {
    return documentCount;
}

size_t CorpusVocabulary::getWordCount() const {
    return wordCount;
}

void CorpusVocabulary::updateWordFrequency(const std::string& lemma) {
    wordFrequency[lemma]++;
    wordCount++;
}

void CorpusVocabulary::updateDocumentFrequency(const std::string& lemma) {
    documentFrequency[lemma]++;
}

void CorpusVocabulary::incrementDocumentCount() {
    documentCount++;
}

void CorpusVocabulary::filter() {
    Logger::log("TextCorpusFilter", LogLevel::Info, "Filtering vocabulary...");

    size_t originalWordCount = wordFrequency.size();
    size_t originalDocFreqCount = documentFrequency.size();
    size_t removedWordCount = 0;

    auto wordIt = wordFrequency.begin();
    while (wordIt != wordFrequency.end()) {
        if (StringUtils::shouldBeFiltered(wordIt->first)) {
            // Уменьшаем глобальный счетчик слов
            wordCount -= wordIt->second;
            removedWordCount += wordIt->second;

            // Удаляем из documentFrequency
            documentFrequency.erase(wordIt->first);

            // Удаляем из wordFrequency
            wordIt = wordFrequency.erase(wordIt);
        } else {
            ++wordIt;
        }
    }

    Logger::log("TextCorpusFilter", LogLevel::Info,
                "Filtered: " + std::to_string(originalWordCount) + " → " + std::to_string(wordFrequency.size()) +
                    " unique words, " + std::to_string(removedWordCount) + " tokens removed");

    Logger::log("TextCorpusFilter", LogLevel::Debug,
                "Document frequencies updated: " + std::to_string(originalDocFreqCount) + " → " +
                    std::to_string(documentFrequency.size()));
}

void CorpusVocabulary::clear() {
    wordFrequency.clear();
    documentFrequency.clear();
    wordCount = 0;
    documentCount = 0;
}

json CorpusVocabulary::serialize() {
    json j;

    // Метаданные
    j["corpus_metadata"]["total_documents"] = documentCount;
    j["corpus_metadata"]["total_words"] = wordCount;

    // Глобальная статистика
    std::map<std::string, size_t> sortedWordFreq(wordFrequency.begin(), wordFrequency.end());
    j["global_statistics"]["word_frequency"] = sortedWordFreq;
    std::map<std::string, size_t> sortedDocFreq(documentFrequency.begin(), documentFrequency.end());
    j["global_statistics"]["document_frequency"] = sortedDocFreq;

    return j;
}

void CorpusVocabulary::deserialize(const json& j) {
    try {
        // Очистить существующую статистику
        clear();

        // Загрузить метаданные
        documentCount = j.at("corpus_metadata").at("total_documents").get<size_t>();
        wordCount = j.at("corpus_metadata").at("total_words").get<size_t>();

        // Загрузить word_frequency
        if (j.at("global_statistics").contains("word_frequency")) {
            for (const auto& [lemma, count] : j.at("global_statistics").at("word_frequency").items()) {
                wordFrequency[lemma] = count.get<size_t>();
            }
        }

        // Загрузить document_frequency
        if (j.at("global_statistics").contains("document_frequency")) {
            for (const auto& [lemma, count] : j.at("global_statistics").at("document_frequency").items()) {
                documentFrequency[lemma] = count.get<size_t>();
            }
        }

        Logger::log("TextCorpusLoader", LogLevel::Debug,
                    "Loaded: " + std::to_string(wordFrequency.size()) + " words, " +
                        std::to_string(documentFrequency.size()) + " document frequencies");

    } catch (const json::exception& e) {
        throw std::runtime_error("JSON parsing error: " + std::string(e.what()));
    } catch (const std::exception& e) {
        throw std::runtime_error("Deserialization error: " + std::string(e.what()));
    }
}
