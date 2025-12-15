#include "TextCorpusLoader.h"

void TextCorpusLoader::save(const TextCorpus& corpus, const std::string& filename) {
    Logger::log("TextCorpusLoader", LogLevel::Info, "Serializing corpus to file: " + filename);

    try {
        std::ofstream file(filename);
        if (!file.is_open()) {
            throw std::runtime_error("Failed to open file for writing: " + filename);
        }

        json j = serialize(corpus);
        file << j.dump(4);
        file.close();

        Logger::log("TextCorpusLoader", LogLevel::Info, "Corpus serialized successfully");
    } catch (const std::exception& e) {
        Logger::log("TextCorpusLoader", LogLevel::Error, "Serialization failed: " + std::string(e.what()));
        throw;
    }
}

void TextCorpusLoader::load(TextCorpus& corpus, const std::string& filename) {
    Logger::log("TextCorpusLoader", LogLevel::Info, "Loading corpus from file: " + filename);

    try {
        std::ifstream file(filename);
        if (!file.is_open()) {
            throw std::runtime_error("Failed to open file: " + filename);
        }

        json j;
        file >> j;
        file.close();

        deserialize(corpus, j);
        Logger::log("TextCorpusLoader", LogLevel::Info, "Corpus loaded successfully");
    } catch (const std::exception& e) {
        Logger::log("TextCorpusLoader", LogLevel::Error, "Deserialization failed: " + std::string(e.what()));
        throw;
    }
}

json TextCorpusLoader::serialize(const TextCorpus& corpus) {
    json j;

    // Serialize metadata
    j["0_totalDocuments"] = corpus.getDocumentCount();
    j["1_totalTexts"] = corpus.getTextCount();
    j["2_totalWords"] = corpus.getWordCount();
    j["3_documentFrequency"] = corpus.getDocumentFrequencies();
    j["4_wordFrequency"] = corpus.getWordFrequencies();

    // Serialize documents and texts
    json documentsJson = json::array();
    for (const auto& [filename, textList] : corpus.getTexts()) {
        json docJson;
        docJson["filename"] = filename;
        docJson["texts"] = textList;
        documentsJson.push_back(docJson);
    }
    j["5_documents"] = documentsJson;

    return j;
}

void TextCorpusLoader::deserialize(TextCorpus& corpus, const json& j) {
    try {
        corpus.clearAllData();

        // Load metadata
        corpus.wordCount = j.at("2_totalWords").get<int>();
        corpus.documentCount = j.at("0_totalDocuments").get<int>();
        corpus.textCount = j.at("1_totalTexts").get<int>();

        // Load frequencies filtering (pure deserialization)
        readDocumentFrequencies(corpus, j);
        readWordFrequencies(corpus, j);

        // Load texts WITHOUT filtering
        readTexts(corpus, j);
    } catch (const json::exception& e) {
        throw std::runtime_error("JSON parsing error: " + std::string(e.what()));
    }
}

void TextCorpusLoader::readWordFrequencies(TextCorpus& corpus, const json& j) {
    auto& wordFreq = corpus.getWordFrequenciesForModification();
    for (const auto& item : j.at("4_wordFrequency").items()) {
        wordFreq[item.key()] = item.value();
    }
}

void TextCorpusLoader::readDocumentFrequencies(TextCorpus& corpus, const json& j) {
    auto& docFreq = corpus.getDocumentFrequenciesForModification();
    for (const auto& item : j.at("3_documentFrequency").items()) {
        docFreq[item.key()] = item.value();
    }
}

void TextCorpusLoader::readTexts(TextCorpus& corpus, const json& j) {
    auto& texts = corpus.getTextsForModification();
    for (const auto& docJson : j.at("5_documents")) {
        std::string filename = docJson.at("filename").get<std::string>();
        std::vector<std::string> docTexts = docJson.at("texts").get<std::vector<std::string>>();
        if (!docTexts.empty()) {
            texts[filename] = docTexts;
        }
    }
}