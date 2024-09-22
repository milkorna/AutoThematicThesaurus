#ifndef TOKENIZED_SENTENCE_CORPUS_H
#define TOKENIZED_SENTENCE_CORPUS_H

#include <boost/algorithm/string.hpp>
#include <fstream>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

#include <boost/algorithm/string.hpp>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

struct TokenizedSentence {
    size_t docNum;
    size_t sentNum;
    std::string originalStr;
    std::string normalizedStr;
};

class TokenizedSentenceCorpus {
public:
    // Default constructor for the TokenizedSentenceCorpus class.
    TokenizedSentenceCorpus() = default;

    // Singleton instance getter for the TokenizedSentenceCorpus class.
    static TokenizedSentenceCorpus& GetCorpus()
    {
        static TokenizedSentenceCorpus corpus;
        return corpus;
    }

    // Adds a sentence to the corpus.
    void AddSentence(const size_t docNum, const size_t sentNum, const std::string& data,
                     const std::string& normalizedData);

    // Serializes the corpus data to JSON format for storage or transmission.
    json Serialize() const;

    // Returns the total number of words (lemmas) in the corpus.
    void Deserialize(const json& j);

    // Saves the serialized corpus data to a file.
    void SaveToFile(const std::string& filename);

    // Loads the corpus data from a file, deserializes it, and returns the restored TextCorpus object.
    void LoadFromFile(const std::string& filename);

    // Retrieves a sentence by document and sentence number.
    const TokenizedSentence* GetSentence(size_t docNum, size_t sentNum) const;

    std::unordered_map<size_t, std::unordered_map<size_t, TokenizedSentence>>
        sentenceMap; // Map for fast sentence retrieval
    int totalSentences = 0;
};

#endif // TEXT_CORPUS_H
