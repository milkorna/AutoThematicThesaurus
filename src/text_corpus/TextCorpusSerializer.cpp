#include "TextCorpusSerializer.h"

void TextCorpusSerializer::serialize(const TextCorpus& corpus, const std::string& filename) {
    Logger::log("TextCorpusSerializer", LogLevel::Info, "Serializing corpus to file: " + filename);

    try {
        std::ofstream file(filename);
        if (!file.is_open()) {
            throw std::runtime_error("Failed to open file for writing: " + filename);
        }

        json j = corpusToJson(corpus);
        file << j.dump(4);
        file.close();

        Logger::log("TextCorpusSerializer", LogLevel::Info, "Corpus serialized successfully");
    } catch (const std::exception& e) {
        Logger::log("TextCorpusSerializer", LogLevel::Error, "Serialization failed: " + std::string(e.what()));
        throw;
    }
}

json TextCorpusSerializer::corpusToJson(const TextCorpus& corpus) {
    json j;

    // Serialize metadata
    j["0_totalDocuments"] = corpus.GetTotalDocuments();
    j["1_totalTexts"] = corpus.GetTotalTexts();
    j["2_totalWords"] = corpus.GetTotalWords();
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