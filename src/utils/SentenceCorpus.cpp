#include "SentenceCorpus.h"
#include "Logger.h"

#include <string>

using json = nlohmann::ordered_json;

// Adds a sentence to the corpus.
void SentenceCorpus::addSentence(const std::string& docId, const size_t sentNum, const std::string& data,
                                 const std::string& normalizedData) {
    TokenizedSentence sentence = {docId, sentNum, data, normalizedData};
    sentenceMap[docId][sentNum] = sentence; // Insert the sentence into the map
    sentencesCount++;
}

const std::optional<TokenizedSentence> SentenceCorpus::getSentence(const std::string& docId,
                                                                   const size_t sentNum) const {
    if (sentenceMap.contains(docId) && sentenceMap.at(docId).contains(sentNum)) {
        return sentenceMap.at(docId).at(sentNum);
    }
    return std::nullopt;
}

// Serializes the corpus data to JSON format.
json SentenceCorpus::serialize() const {
    json j;
    j["totalSentences"] = sentencesCount;
    j["sentences"] = json::array();

    // Собрать все предложения в вектор для сортировки
    std::vector<const TokenizedSentence*> sortedSentences;
    sortedSentences.reserve(sentencesCount);

    for (const auto& [docId, sentencesMap] : sentenceMap) {
        for (const auto& [sentNum, sentence] : sentencesMap) {
            sortedSentences.push_back(&sentence);
        }
    }

    // Отсортировать: сначала по docId, потом по sentNum
    std::sort(sortedSentences.begin(), sortedSentences.end(),
              [](const TokenizedSentence* a, const TokenizedSentence* b) {
                  // Сначала сравниваем docId (строки сравниваются лексикографически)
                  if (a->docId != b->docId) {
                      return a->docId < b->docId;
                  }
                  // Если docId одинаковый, сортируем по sentNum
                  return a->sentNum < b->sentNum;
              });

    // Вывести в отсортированном порядке
    j["sentences"] = json::array();
    for (const auto* sentence : sortedSentences) {
        j["sentences"].push_back({{"docId", sentence->docId},
                                  {"sentNum", sentence->sentNum},
                                  {"original", sentence->originalStr},
                                  {"normalized", sentence->normalizedStr}});
    }

    return j;
}

// Deserializes the corpus data from JSON format.
void SentenceCorpus::deserialize(const json& j) {
    sentenceMap.clear();
    sentencesCount = j.at("totalSentences").get<int>();

    for (const auto& item : j.at("sentences")) {
        TokenizedSentence sentence;
        sentence.docId = item.at("docId").get<std::string>();
        sentence.sentNum = item.at("sentNum").get<size_t>();
        sentence.originalStr = item.at("original").get<std::string>();
        sentence.normalizedStr = item.at("normalized").get<std::string>();
        sentenceMap[sentence.docId][sentence.sentNum] = sentence;
    }
}

// Saves the serialized corpus data to a file.
void SentenceCorpus::save(const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open file " + filename + " for saving.");
    }

    json j = serialize();
    file << j.dump(4); // Dump JSON with indentation of 4 spaces for readability
    file.close();
}

// Loads the corpus data from a file, deserializes it, and updates the corpus.
void SentenceCorpus::load(const std::string& filename) {
    Logger::log("SentenceCorpus", LogLevel::Info, "Loading sentences from file: " + filename);
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open file " + filename + " for loading.");
    }

    json j;
    file >> j;
    file.close();
    deserialize(j);

    Logger::log("SentenceCorpus", LogLevel::Info,
                "Sentences loaded successfully. Total sentences: " + std::to_string(sentencesCount));
}

void SentenceCorpus::clear() {
    sentenceMap.clear();
    sentencesCount = 0;
}