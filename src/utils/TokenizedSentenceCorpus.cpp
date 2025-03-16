
#include "Logger.h"
#include <TokenizedSentenceCorpus.h>

// Adds a sentence to the corpus.
void TokenizedSentenceCorpus::AddSentence(const size_t docNum, const size_t sentNum, const std::string& data,
                                          const std::string& normalizedData)
{
    TokenizedSentence sentence = {docNum, sentNum, data, normalizedData};
    sentenceMap[docNum][sentNum] = sentence; // Insert the sentence into the map
    totalSentences++;
}

// Retrieves a sentence by document and sentence number.
const TokenizedSentence* TokenizedSentenceCorpus::GetSentence(size_t docNum, size_t sentNum) const
{
    auto docIt = sentenceMap.find(docNum);
    if (docIt != sentenceMap.end()) {
        auto sentIt = docIt->second.find(sentNum);
        if (sentIt != docIt->second.end()) {
            return &sentIt->second; // Return a pointer to the found sentence
        }
    }
    return nullptr; // Return nullptr if the sentence is not found
}

// Serializes the corpus data to JSON format.
json TokenizedSentenceCorpus::Serialize() const
{
    json j;
    j["totalSentences"] = totalSentences;
    j["sentences"] = json::array();

    for (const auto& [docNum, sentencesMap] : sentenceMap) {
        for (const auto& [sentNum, sentence] : sentencesMap) {
            j["sentences"].push_back({{"docNum", sentence.docNum},
                                      {"sentNum", sentence.sentNum},
                                      {"originalStr", sentence.originalStr},
                                      {"normalizedStr", sentence.normalizedStr}});
        }
    }

    return j;
}

// Deserializes the corpus data from JSON format.
void TokenizedSentenceCorpus::Deserialize(const json& j)
{
    totalSentences = j.at("totalSentences").get<int>();
    sentenceMap.clear(); // Clear existing data before loading new ones

    for (const auto& item : j.at("sentences")) {
        if (item.at("normalizedStr").get<std::string>().size() < 50)
            continue;
        TokenizedSentence sentence;
        sentence.docNum = item.at("docNum").get<size_t>();
        sentence.sentNum = item.at("sentNum").get<size_t>();
        sentence.originalStr = item.at("originalStr").get<std::string>();
        sentence.normalizedStr = item.at("normalizedStr").get<std::string>();
        sentenceMap[sentence.docNum][sentence.sentNum] = sentence;
    }
}

// Saves the serialized corpus data to a file.
void TokenizedSentenceCorpus::SaveToFile(const std::string& filename)
{
    std::ofstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open file " + filename + " for saving.");
    }

    json j = Serialize();
    file << j.dump(4); // Dump JSON with indentation of 4 spaces for readability
    file.close();
}

// Loads the corpus data from a file, deserializes it, and updates the corpus.
void TokenizedSentenceCorpus::LoadFromFile(const std::string& filename)
{
    Logger::log("TokenizedSentenceCorpus", LogLevel::Info, "Loading tokenized sentences from file: " + filename);
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open file " + filename + " for loading.");
    }

    json j;
    file >> j;
    file.close();
    Deserialize(j);

    Logger::log("TokenizedSentenceCorpus", LogLevel::Info,
                "Sentences loaded successfully. Total sentences: " + totalSentences);
}