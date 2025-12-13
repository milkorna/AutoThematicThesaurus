#include "TextCorpusDeserializer.h"

void TextCorpusDeserializer::deserialize(TextCorpus& corpus, const std::string& filename) {
    Logger::log("TextCorpusDeserializer", LogLevel::Info, "Loading corpus from file: " + filename);

    try {
        std::ifstream file(filename);
        if (!file.is_open()) {
            throw std::runtime_error("Failed to open file: " + filename);
        }

        json j;
        file >> j;
        file.close();

        loadFromJson(corpus, j);
        Logger::log("TextCorpusDeserializer", LogLevel::Info, "Corpus loaded successfully");
    } catch (const std::exception& e) {
        Logger::log("TextCorpusDeserializer", LogLevel::Error, "Deserialization failed: " + std::string(e.what()));
        throw;
    }
}

void TextCorpusDeserializer::loadFromJson(TextCorpus& corpus, const json& j) {
    try {
        corpus.clearAllData();

        // Load metadata
        corpus.wordCount = j.at("2_totalWords").get<int>();
        corpus.documentCount = j.at("0_totalDocuments").get<int>();
        corpus.textCount = j.at("1_totalTexts").get<int>();

        // Load frequencies filtering (pure deserialization)
        loadDocumentFrequencies(corpus, j);
        loadWordFrequencies(corpus, j);

        // Load texts WITHOUT filtering
        loadTexts(corpus, j);
    } catch (const json::exception& e) {
        throw std::runtime_error("JSON parsing error: " + std::string(e.what()));
    }
}

void TextCorpusDeserializer::loadWordFrequencies(TextCorpus& corpus, const json& j) {
    auto& wordFreq = corpus.getWordFrequenciesForModification();
    for (const auto& item : j.at("4_wordFrequency").items()) {
        wordFreq[item.key()] = item.value();
    }
}

void TextCorpusDeserializer::loadDocumentFrequencies(TextCorpus& corpus, const json& j) {
    auto& docFreq = corpus.getDocumentFrequenciesForModification();
    for (const auto& item : j.at("3_documentFrequency").items()) {
        docFreq[item.key()] = item.value();
    }
}

void TextCorpusDeserializer::loadTexts(TextCorpus& corpus, const json& j) {
    auto& texts = corpus.getTextsForModification();
    for (const auto& docJson : j.at("5_documents")) {
        std::string filename = docJson.at("filename").get<std::string>();
        std::vector<std::string> docTexts = docJson.at("texts").get<std::vector<std::string>>();
        if (!docTexts.empty()) {
            texts[filename] = docTexts;
        }
    }
}